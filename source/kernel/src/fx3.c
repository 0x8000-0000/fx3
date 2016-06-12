/**
 * @file fx3.c
 * @brief Portable Kernel
 * @author Florin Iucha <florin@signbit.net>
 * @copyright Apache License, Version 2.0
 */

/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of FX3 RTOS for ARM Cortex-M4
 */

#include <assert.h>
#include <string.h>
#include <stddef.h>

#include <priority_queue.h>
#include <bitops.h>

#include <board.h>

#include <task.h>
#include <synchronization.h>

#include <fx3_config.h>

#include <task_priv.h>

#ifdef FX3_RTT_TRACE
#include <SEGGER_SYSVIEW.h>
#endif

/*
 * FX3 command infrastructure
 */

enum command_type
{
   FX3_INVALID_COMMAND,

   FX3_BLOCK_TASK,
   FX3_READY_TASK,

   FX3_SIGNAL_SEMAPHORE,

   FX3_TIMER_REQUEST_WAKEUP,

   /// Sent by timer handler to FX3
   FX3_TIMER_EVENT_WAKEUP,
   FX3_TIMER_EVENT_EPOCH_ROLLOVER,
};

#ifndef FX3_COMMAND_QUEUE_SIZE
#define FX3_COMMAND_QUEUE_SIZE 16
#endif

struct fx3_command
{
   /// used for intrusive data structures
   union
   {
      struct fx3_command*     next;
      struct list_element     element;
   };

   enum command_type          type;

   struct task_control_block* task;

   void*                      object;
};

static struct fx3_message_center
{
   /// pool of available commands
   struct fx3_command         pool[FX3_COMMAND_QUEUE_SIZE];

   /// bitmap
   volatile uint32_t          available;

   volatile struct list_element*       inbox;

}  fx3MessageCenter;

static inline struct fx3_command* allocateFX3Command(void)
{
   uint32_t idx = bit_alloc(&fx3MessageCenter.available);
   assert(FX3_COMMAND_QUEUE_SIZE > idx);

   struct fx3_command* cmd = &fx3MessageCenter.pool[idx];

   // cleaned on free, because alloc tends to be more time-sensitive
   assert(FX3_INVALID_COMMAND == cmd->type);
   assert(NULL == cmd->task);
   assert(NULL == cmd->object);
   assert(NULL == cmd->next);

   return cmd;
}

static inline void postFX3Command(struct fx3_command* cmd)
{
   lst_pushElement(&fx3MessageCenter.inbox, &cmd->element);
   bsp_scheduleContextSwitch();
}

static inline void freeFX3Command(struct fx3_command* cmd)
{
   assert(cmd > fx3MessageCenter.pool);
   ptrdiff_t idx = cmd - fx3MessageCenter.pool;
   assert(FX3_COMMAND_QUEUE_SIZE > idx);
   assert(0 == (fx3MessageCenter.available & (1 << idx)));

   memset(cmd, 0, sizeof(*cmd));

   bit_free(&fx3MessageCenter.available, idx);
}


/*
 * A task can be:
 *    * running
 *    * on the runnable queue
 *    * on the sleeping queue
 *    * waiting on a semaphore, mutex, event...
 */
static bool fx3IsInitialized = false;

struct task_control_block* runningTask;
struct task_control_block* nextRunningTask;

// +1 for the heap header and +1 for the idle task
static uint32_t* runnableTasksMemPool[FX3_MAX_TASK_COUNT + 2];
static struct priority_queue runnableTasks;

static struct task_control_block* allValidTaskControlBlocks[FX3_MAX_TASK_COUNT];

static struct fx3_timer
{
   struct task_control_block* firstSleepingTaskToAwake;

   // +1 for the heap header and +1 for the idle task
   uint32_t* sleepingTasksMemPool_0[FX3_MAX_TASK_COUNT + 2];
   struct priority_queue sleepingTasks_0;
   // +1 for the heap header and +1 for the idle task
   uint32_t* sleepingTasksMemPool_1[FX3_MAX_TASK_COUNT + 2];
   struct priority_queue sleepingTasks_1;

   struct priority_queue* sleepingTasks;
   struct priority_queue* sleepingTasksNextEpoch;

   volatile uint32_t lastWokenUpAt;

}  fx3Timer;

/*
 *
 */
static uint8_t idleTaskStack[128] __attribute__ ((aligned (16)));

static volatile uint32_t sleepCycles;

static void idleTaskHandler(const void* arg __attribute__((unused)))
{
   while (true)
   {
      sleepCycles ++;
      bsp_sleep();
   }
}

static const struct task_config idleTaskConfig =
{
   .name            = "Idle",
   .handler         = idleTaskHandler,
   .argument        = NULL,
   .priority        = 0xffff,
   .stackBase       = idleTaskStack,
   .stackSize       = sizeof(idleTaskStack),
   .timeSlice_ticks = 0,
};

struct task_control_block idleTask;

static inline uint32_t computeEffectivePriority(enum task_state state, const struct task_config* config)
{
   /*
    * Shift priority value to allow ready / resting / exhausted "sub-priorities"
    *
    * This allows efficient implementation of round-robin scheduling. All tasks
    * with the same nominal priority, are adjacent in the priority list, and
    * separate from tasks with other nominal priorities.
    *
    * Tasks that have 'exhausted' their round-robin slice have still higher
    * priorities than runnable tasks with lower priorities. Just, when
    * selecting between two tasks with the same nominal priority, the
    * not-exhausted one is clearly more ready to run.
    */
   return config->priority * 16 + state;
}

/* Mark task ready
 */
static void markTaskReady(struct task_control_block* tcb)
{
   assert(tcb->config->timeSlice_ticks >= tcb->roundRobinSliceLeft_ticks);
   if (tcb->config->timeSlice_ticks && (0 == tcb->roundRobinSliceLeft_ticks))
   {
      tcb->state = TS_EXHAUSTED;
   }
   else
   {
      tcb->state = TS_READY;
   }

   tcb->sleepUntil_ticks  = 0;
   tcb->effectivePriority = computeEffectivePriority(tcb->state, tcb->config);
   prq_push(&runnableTasks, &tcb->effectivePriority);

#ifdef FX3_RTT_TRACE
   if (&idleTask != tcb)
   {
      SEGGER_SYSVIEW_OnTaskStartReady((uint32_t) tcb);
   }
#endif
}

static void scheduleReadyTask(struct task_control_block* tcb)
{
   struct fx3_command* cmd = allocateFX3Command();

   cmd->type = FX3_READY_TASK;
   cmd->task = tcb;

   postFX3Command(cmd);
}

static uint32_t tasksCreated_count;

static void verifyTaskControlBlocks(bool expectTaskInRunningState)
{
#ifdef FX3_RTT_TRACE
   //SEGGER_SYSVIEW_RecordVoid(32);
#endif

   bool foundTaskInRunningState = false;
   uint32_t tasksVisited = 0;

   /*
    * check task links
    */

   struct task_control_block* tcb = &idleTask;
   do
   {
      assert(tcb);
      assert(tcb->config);
      assert(tcb->config->priority);

      tasksVisited ++;

      if (TS_RUNNING == tcb->state)
      {
         assert(! foundTaskInRunningState);
         foundTaskInRunningState = true;
      }

      assert(tcb->nextWithSamePriority);

      if (tcb->nextWithSamePriority == tcb)
      {
         assert(0 == tcb->config->timeSlice_ticks);
      }
      else
      {
         assert(tcb->config->timeSlice_ticks);
      }

      struct task_control_block* peerPriorityTask = tcb;
      do
      {
         assert(tcb->config->priority == peerPriorityTask->config->priority);

         peerPriorityTask = peerPriorityTask->nextWithSamePriority;
      }
      while (peerPriorityTask != tcb);

      tcb = tcb->nextTaskInTheGreatLink;

      /*
       * verify tcb points to a valid task
       */
      uint32_t tt = 0;
      while (tt < tasksCreated_count)
      {
         if (allValidTaskControlBlocks[tt] == tcb)
         {
            break;
         }
         else
         {
            tt ++;
         }
      }
      assert(tt < tasksCreated_count);
   }
   while (&idleTask != tcb);

   if (expectTaskInRunningState)
   {
      assert(foundTaskInRunningState);
   }

   assert(tasksVisited == tasksCreated_count);

   /*
    * check running queue
    * note; the queue is a heap, and indices start at 1
    */
   for (uint32_t ii = 1; ii <= runnableTasks.size; ii ++)
   {
      uint32_t* priority = runnableTasks.memPool[ii];
      struct task_control_block* readyTask = (struct task_control_block*) (((uint8_t*) priority) - (offsetof(struct task_control_block, effectivePriority)));

      assert((TS_READY == readyTask->state) || (TS_EXHAUSTED == readyTask->state));
   }

   /*
    * check sleeping queue
    * note; the queue is a heap, and indices start at 1
    */
   for (uint32_t ii = 0; ii < fx3Timer.sleepingTasks->size; ii ++)
   {
      uint32_t* sleepUntil = fx3Timer.sleepingTasks->memPool[ii + 1];
      struct task_control_block* sleepingTask = (struct task_control_block*) (((uint8_t*) sleepUntil) - (offsetof(struct task_control_block, sleepUntil_ticks)));;

      assert(TS_SLEEPING == sleepingTask->state);
   }

   for (uint32_t ii = 0; ii < fx3Timer.sleepingTasksNextEpoch->size; ii ++)
   {
      uint32_t* sleepUntil = fx3Timer.sleepingTasksNextEpoch->memPool[ii + 1];
      struct task_control_block* sleepingTask = (struct task_control_block*) (((uint8_t*) sleepUntil) - (offsetof(struct task_control_block, sleepUntil_ticks)));;

      assert(TS_SLEEPING == sleepingTask->state);
   }

#ifdef FX3_RTT_TRACE
   // supported from SysView 2.36 onwards
   //SEGGER_SYSVIEW_RecordEndCall(32);
#endif
}

void fx3_initialize(void)
{
   idleTask.nextTaskInTheGreatLink = 0;

   tasksCreated_count = 0;

#ifdef FX3_RTT_TRACE
   SEGGER_SYSVIEW_Conf();
#endif

   prq_initialize(&runnableTasks, runnableTasksMemPool, FX3_MAX_TASK_COUNT + 2);
   memset(allValidTaskControlBlocks, 0, sizeof(allValidTaskControlBlocks));

   prq_initialize(&fx3Timer.sleepingTasks_0, fx3Timer.sleepingTasksMemPool_0, FX3_MAX_TASK_COUNT + 1);
   prq_initialize(&fx3Timer.sleepingTasks_1, fx3Timer.sleepingTasksMemPool_1, FX3_MAX_TASK_COUNT + 1);
   fx3Timer.sleepingTasks          = &fx3Timer.sleepingTasks_0;
   fx3Timer.sleepingTasksNextEpoch = &fx3Timer.sleepingTasks_1;
   fx3Timer.firstSleepingTaskToAwake = NULL;

   memset(&fx3MessageCenter, 0, sizeof(fx3MessageCenter));
   bit_initialize(&fx3MessageCenter.available, FX3_COMMAND_QUEUE_SIZE);

   sleepCycles = 0;

   fx3IsInitialized = true;

   fx3_createTask(&idleTask, &idleTaskConfig);

#ifdef FX3_RTT_TRACE
   SEGGER_SYSVIEW_Conf();
#endif
}

void createTaskImpl(struct task_control_block* tcb, const struct task_config* config, uint32_t* stackPointer, const void* argument)
{
   allValidTaskControlBlocks[tasksCreated_count] = tcb;

   tasksCreated_count ++;

   tcb->id = tasksCreated_count;

   tcb->config = config;
   tcb->roundRobinSliceLeft_ticks = config->timeSlice_ticks;

#ifdef FX3_RTT_TRACE
   if (&idleTask != tcb)
   {
      SEGGER_SYSVIEW_TASKINFO taskInfo;
      memset(&taskInfo, 0, sizeof(taskInfo));

      taskInfo.TaskID    = (uint32_t) tcb,
      taskInfo.sName     = config->name,
      taskInfo.Prio      = config->priority,
      taskInfo.StackBase = (uint32_t) stackPointer,
      taskInfo.StackSize = config->stackSize,

      SEGGER_SYSVIEW_OnTaskCreate((uint32_t) tcb);
      SEGGER_SYSVIEW_SendTaskInfo(&taskInfo);
   }
#endif

   // park the task for now
   tcb->effectivePriority = config->priority;
   prq_push(fx3Timer.sleepingTasks, &tcb->effectivePriority);

   // set up stack
   stackPointer[0]  = 0xFFFFFFFDUL;                   // initial EXC_RETURN
   stackPointer[1]  = 0x2;                            // initial CONTROL : privileged, PSP, no FP
   stackPointer[2]  = 0x0404;       // R4
   stackPointer[3]  = 0x0505;       // R5
   stackPointer[4]  = 0x0606;       // R6
   stackPointer[5]  = 0x0708;       // R7
   stackPointer[6]  = 0x0808;       // R8
   stackPointer[7]  = 0x0909;       // R9
   stackPointer[8]  = 0x0A0A;       // R10
   stackPointer[9]  = 0x0B0B;       // R11
   stackPointer[10] = (uint32_t) argument;       // R0
   stackPointer[11] = 0x0101;
   stackPointer[12] = 0x0B0B;
   stackPointer[12] = 0x0202;
   stackPointer[14] = 0x0C0C;
   stackPointer[15] = 0x0000;

   stackPointer[16] = config->handlerAddress;         // initial Program Counter
   stackPointer[17] = 0x01000000;                     // initial xPSR

   tcb->stackPointer = stackPointer;
}

void fx3_createTask(struct task_control_block* tcb, const struct task_config* config)
{
   assert(fx3IsInitialized);

   memset(tcb, 0, sizeof(*tcb));
   memset((void*) config->stackBase, 0, config->stackSize);

   uint32_t* stackPointer = (uint32_t*) (((uint8_t*) config->stackBase + config->stackSize) - 18 * 4);
   createTaskImpl(tcb, config, stackPointer, config->argument);
}

void fx3_createTaskPool(struct task_control_block* tcb, const struct task_config* config, uint32_t argumentSize, uint32_t poolSize)
{
   memset(tcb, 0, sizeof(*tcb) * poolSize);
   memset((void*) config->stackBase, 0, config->stackSize * poolSize);

   for (uint32_t ii = 0; ii < poolSize; ii ++)
   {
      uint32_t thisStackBase = (uint32_t) (config->stackBase) + ii * config->stackSize;
      uint32_t* stackPointer = (uint32_t*) (thisStackBase + config->stackSize - 18 * 4);
      createTaskImpl(&tcb[ii], config, stackPointer, ((const uint8_t*) config->argument) + ii * argumentSize);
   }
}

static void setupTasksLinks(void)
{
   uint32_t* taskPrio = prq_pop(fx3Timer.sleepingTasks);
   assert(taskPrio);    // we should have a task

   uint32_t lastPrio = *taskPrio;

   struct task_control_block* currentTask = (struct task_control_block*) (((uint8_t*) taskPrio) - (offsetof(struct task_control_block, effectivePriority)));
   assert(&idleTask != currentTask);

   struct task_control_block* firstTaskAtCurrentPrio             = currentTask;
   uint32_t                   cumulativeTicksAtCurrentPrio_ticks = currentTask->config->timeSlice_ticks;

   idleTask.nextTaskInTheGreatLink = currentTask;
   idleTask.nextWithSamePriority   = &idleTask;
   markTaskReady(currentTask);

   while (! prq_isEmpty(fx3Timer.sleepingTasks))
   {
      taskPrio = prq_pop(fx3Timer.sleepingTasks);
      struct task_control_block* nextTask = (struct task_control_block*) (((uint8_t*) taskPrio) - (offsetof(struct task_control_block, effectivePriority)));

      if (*taskPrio == lastPrio)
      {
         // tasks with shared priorities must have a time slice defined
         assert(currentTask->config->timeSlice_ticks);

         currentTask->nextWithSamePriority   = nextTask;
         cumulativeTicksAtCurrentPrio_ticks += nextTask->config->timeSlice_ticks;
      }
      else
      {
         lastPrio = *taskPrio;

         // distribute the cumulativeTicksAtCurrentPrio_ticks to all tasks at same prio
         for (struct task_control_block* tcb = firstTaskAtCurrentPrio; tcb; tcb = tcb->nextWithSamePriority)
         {
            tcb->roundRobinCumulative_ticks = cumulativeTicksAtCurrentPrio_ticks;
         }

         // close the link
         currentTask->nextWithSamePriority  = firstTaskAtCurrentPrio;

         if (currentTask == firstTaskAtCurrentPrio)
         {
            // tasks alone on a priority must not have a time slice defined
            assert(0 == currentTask->config->timeSlice_ticks);
         }

         firstTaskAtCurrentPrio             = nextTask;
         cumulativeTicksAtCurrentPrio_ticks = nextTask->config->timeSlice_ticks;
      }

      currentTask->nextTaskInTheGreatLink = nextTask;
      currentTask = nextTask;
      markTaskReady(currentTask);
   }

   memset(fx3Timer.sleepingTasksMemPool_0, 0, sizeof(fx3Timer.sleepingTasksMemPool_0));
   memset(fx3Timer.sleepingTasksMemPool_1, 0, sizeof(fx3Timer.sleepingTasksMemPool_1));
   prq_initialize(&fx3Timer.sleepingTasks_0, fx3Timer.sleepingTasksMemPool_0, FX3_MAX_TASK_COUNT + 1);
   prq_initialize(&fx3Timer.sleepingTasks_1, fx3Timer.sleepingTasksMemPool_1, FX3_MAX_TASK_COUNT + 1);

   /*
    * the last task is the idleTask, as it has the lowest priority
    */
   assert(&idleTask == currentTask);
}

void fx3_startMultitasking(void)
{
   setupTasksLinks();

   uint32_t* firstTaskPrio = prq_pop(&runnableTasks);

   runningTask = (struct task_control_block*) (((uint8_t*) firstTaskPrio) - (offsetof(struct task_control_block, effectivePriority)));
   runningTask->state = TS_RUNNING;
   runningTask->startedRunningAt_ticks = bsp_getTimestamp_ticks();

   uint32_t runningTaskPSP = (uint32_t) (runningTask->stackPointer + 16);

   bsp_startMainClock();

   verifyTaskControlBlocks(true);

   fx3_startMultitaskingImpl(runningTaskPSP, runningTask->config->handler, runningTask->config->argument);
}

static volatile uint32_t lastContextSwitchAt;

static void selectNextRunningTask(void)
{
   assert(TS_RUNNING != runningTask->state);

#ifdef FX3_RTT_TRACE
   if (&idleTask != runningTask)
   {
      SEGGER_SYSVIEW_OnTaskStopReady((uint32_t) runningTask, runningTask->state);
   }
#endif

   lastContextSwitchAt = bsp_getTimestamp_ticks();

   uint32_t runTime = bsp_computeInterval_ticks(runningTask->startedRunningAt_ticks, lastContextSwitchAt);

   runningTask->totalRunTime_ticks += runTime;
   runningTask->startedRunningAt_ticks = 0;

   verifyTaskControlBlocks(false);

   uint32_t* nextRunningTaskPriority = prq_pop(&runnableTasks);
   assert(nextRunningTaskPriority);    // idle task, if nothing else

   nextRunningTask = (struct task_control_block*) (((uint8_t*) nextRunningTaskPriority) - (offsetof(struct task_control_block, effectivePriority)));

   if (TS_EXHAUSTED == nextRunningTask->state)
   {
      /*
       * All tasks with the same priority have exhausted their round-robin ticks:
       * replenish their ticks, and change their state to runnable and their effective
       * priorities.
       * Note that this does not affect their standing in the runnable queue, because
       * their effective priority is still higher than the next runnable tasks.
       */
      nextRunningTask->state                     = TS_READY;
      nextRunningTask->roundRobinSliceLeft_ticks = nextRunningTask->config->timeSlice_ticks;
      nextRunningTask->effectivePriority         = computeEffectivePriority(runningTask->state, runningTask->config);

      for (struct task_control_block* tcb = nextRunningTask->nextWithSamePriority; tcb != nextRunningTask; tcb = tcb->nextWithSamePriority)
      {
         assert(tcb->config->priority == nextRunningTask->config->priority);

         if (TS_EXHAUSTED == tcb->state)
         {
            tcb->state             = TS_READY;
            tcb->effectivePriority = computeEffectivePriority(runningTask->state, runningTask->config);
         }
         else
         {
            assert((TS_SLEEPING == tcb->state) || (TS_WAITING_FOR_MESSAGE == tcb->state));
         }
         tcb->roundRobinSliceLeft_ticks = nextRunningTask->config->timeSlice_ticks;
      }
   }

   if (nextRunningTask->config->timeSlice_ticks)
   {
      assert(nextRunningTask->roundRobinSliceLeft_ticks);

      uint32_t roundRobinDeadline = 0;
      nextRunningTask->roundRobinSliceLeft_ticks --;        // charge task for 'making runnable'
      bsp_computeWakeUp_ticks(nextRunningTask->roundRobinSliceLeft_ticks, &roundRobinDeadline);
      bsp_requestRoundRobinSliceTimeout_ticks(roundRobinDeadline);
   }

   nextRunningTask->state = TS_RUNNING;
   nextRunningTask->startedRunning_count ++;
   nextRunningTask->startedRunningAt_ticks = lastContextSwitchAt;

#ifdef FX3_RTT_TRACE
   if (&idleTask != nextRunningTask)
   {
      SEGGER_SYSVIEW_OnTaskStartExec((uint32_t) nextRunningTask);
   }
   else
   {
      SEGGER_SYSVIEW_OnIdle();
   }
#endif
}


/** Transition this task from running to asleep
 *
 */
bool handleSleepRequest(struct fx3_command* cmd)
{
   assert(FX3_TIMER_REQUEST_WAKEUP == cmd->type);

   struct task_control_block* sleepyTask = cmd->task;
   uint32_t timeout_ms = (uint32_t) cmd->object;
   freeFX3Command(cmd);

   bsp_disableSystemTimer();

   bsp_cancelRoundRobinSliceTimeout();

   if (sleepyTask->config->timeSlice_ticks)
   {
      uint32_t runTime = bsp_computeInterval_ticks(sleepyTask->startedRunningAt_ticks, bsp_getTimestamp_ticks());
      assert(sleepyTask->roundRobinSliceLeft_ticks >= runTime);
      sleepyTask->roundRobinSliceLeft_ticks -= runTime;
   }

   if (timeout_ms)
   {
      const uint32_t sleepDuration_ticks = bsp_getTicksForMS(timeout_ms);

      if (sleepyTask->config->timeSlice_ticks)
      {
         /*
          * This task will sleep more than a full period of round-robin
          * scheduling at this priority so replenish its full slice.
          */

         if (sleepDuration_ticks >= sleepyTask->roundRobinCumulative_ticks)
         {
            sleepyTask->roundRobinSliceLeft_ticks = sleepyTask->config->timeSlice_ticks;
         }
      }

      //sleepyTask->state             = TS_SLEEPING;
      sleepyTask->effectivePriority = 0xffff;

      bool nextEpoch = bsp_computeWakeUp_ticks(sleepDuration_ticks, &sleepyTask->sleepUntil_ticks);
      if (nextEpoch)
      {
         prq_push(fx3Timer.sleepingTasksNextEpoch, &sleepyTask->sleepUntil_ticks);
      }
      else
      {
         /*
          * the sleep will complete this epoch
          */

         if (0 == fx3Timer.firstSleepingTaskToAwake)
         {
            /*
             * there is no other sleeping task
             */

            assert(prq_isEmpty(fx3Timer.sleepingTasks));

            fx3Timer.firstSleepingTaskToAwake = sleepyTask;
            bsp_wakeUpAt_ticks(sleepyTask->sleepUntil_ticks);
         }
         else
         {
            assert(TS_SLEEPING == fx3Timer.firstSleepingTaskToAwake->state);
            if (fx3Timer.firstSleepingTaskToAwake->sleepUntil_ticks > sleepyTask->sleepUntil_ticks)
            {
               /*
                * this new task is sleeping less than the previous task with the shortest sleep
                */

               prq_push(fx3Timer.sleepingTasks, &fx3Timer.firstSleepingTaskToAwake->sleepUntil_ticks);

               fx3Timer.firstSleepingTaskToAwake = sleepyTask;
               bsp_wakeUpAt_ticks(sleepyTask->sleepUntil_ticks);
            }
            else
            {
               prq_push(fx3Timer.sleepingTasks, &sleepyTask->sleepUntil_ticks);
            }
         }
      }
   }
   else
   {
      // just yield

      runningTask->state             = TS_READY;
      runningTask->effectivePriority = computeEffectivePriority(runningTask->state, runningTask->config);
      prq_push(&runnableTasks, &runningTask->effectivePriority);
   }

   bsp_enableSystemTimer();

   return true;
}

void fx3_suspendTask(uint32_t timeout_ms)
{
   runningTask->state = TS_SLEEPING;

   struct fx3_command* cmd = allocateFX3Command();

   cmd->type   = FX3_TIMER_REQUEST_WAKEUP;
   cmd->task   = runningTask;
   cmd->object = (void*) timeout_ms;

   postFX3Command(cmd);
}

void task_block(enum task_state newState)
{
   assert(TS_WAITING_FOR_MUTEX <= newState);
   assert(TS_STATE_COUNT > newState);

   bsp_cancelRoundRobinSliceTimeout();

   runningTask->state = newState;
   bsp_scheduleContextSwitch();
}

static bool handleWakeUpAlarm(struct fx3_command* cmd)
{
   assert(FX3_TIMER_EVENT_WAKEUP == cmd->type);
   struct task_control_block* sleepingTaskToAwake = cmd->task;
   freeFX3Command(cmd);

   bool runningTaskDethroned = false;

   markTaskReady(sleepingTaskToAwake);
   if (sleepingTaskToAwake->effectivePriority < runningTask->effectivePriority)
   {
      runningTaskDethroned = true;
   }
   /*
    * done with the task that was waiting to be woken up
    */

   // who's next?
   while ((NULL == fx3Timer.firstSleepingTaskToAwake) && (! prq_isEmpty(fx3Timer.sleepingTasks)))
   {
      uint32_t* nextWakeupDeadline = prq_pop(fx3Timer.sleepingTasks);
      assert(nextWakeupDeadline);

      fx3Timer.firstSleepingTaskToAwake = (struct task_control_block*) (((uint8_t*) nextWakeupDeadline) - (offsetof(struct task_control_block, sleepUntil_ticks)));
      assert(TS_SLEEPING == fx3Timer.firstSleepingTaskToAwake->state);
      if (*nextWakeupDeadline <= bsp_getTimestamp_ticks())
      {
         markTaskReady(fx3Timer.firstSleepingTaskToAwake);
         if (fx3Timer.firstSleepingTaskToAwake->effectivePriority < runningTask->effectivePriority)
         {
            runningTaskDethroned = true;
         }
         fx3Timer.firstSleepingTaskToAwake = NULL;
      }
   }

   if (fx3Timer.firstSleepingTaskToAwake)
   {
      bsp_wakeUpAt_ticks(fx3Timer.firstSleepingTaskToAwake->sleepUntil_ticks);
   }

   if (runningTaskDethroned)
   {
      markTaskReady(runningTask);
   }

   return runningTaskDethroned;
}

bool bsp_onWokenUp(void)
{
   fx3Timer.lastWokenUpAt = bsp_getTimestamp_ticks();

   assert(fx3Timer.firstSleepingTaskToAwake);
   assert(fx3Timer.firstSleepingTaskToAwake->sleepUntil_ticks <= fx3Timer.lastWokenUpAt);
   assert(TS_SLEEPING == fx3Timer.firstSleepingTaskToAwake->state);

   struct fx3_command* cmd = allocateFX3Command();

   cmd->type = FX3_TIMER_EVENT_WAKEUP;
   cmd->task = fx3Timer.firstSleepingTaskToAwake;

   fx3Timer.firstSleepingTaskToAwake = NULL;

   postFX3Command(cmd);

   return true;
}

static bool handleEpochRollover(struct fx3_command* cmd)
{
   assert(FX3_TIMER_EVENT_EPOCH_ROLLOVER == cmd->type);
   freeFX3Command(cmd);

   assert(prq_isEmpty(fx3Timer.sleepingTasks));

   // swap queues
   {
      struct priority_queue* temp = fx3Timer.sleepingTasks;
      fx3Timer.sleepingTasks = fx3Timer.sleepingTasksNextEpoch;
      fx3Timer.sleepingTasksNextEpoch = temp;
   }

   bool runningTaskDethroned = false;

   while ((NULL == fx3Timer.firstSleepingTaskToAwake) && (! prq_isEmpty(fx3Timer.sleepingTasks)))
   {
      uint32_t* nextWakeupDeadline = prq_pop(fx3Timer.sleepingTasks);
      assert(nextWakeupDeadline);

      fx3Timer.firstSleepingTaskToAwake = (struct task_control_block*) (((uint8_t*) nextWakeupDeadline) - (offsetof(struct task_control_block, sleepUntil_ticks)));
      assert(TS_SLEEPING == fx3Timer.firstSleepingTaskToAwake->state);
      if (0 == fx3Timer.firstSleepingTaskToAwake->sleepUntil_ticks)
      {
         markTaskReady(fx3Timer.firstSleepingTaskToAwake);
         if (fx3Timer.firstSleepingTaskToAwake->effectivePriority < runningTask->effectivePriority)
         {
            runningTaskDethroned = true;
         }
         fx3Timer.firstSleepingTaskToAwake = NULL;
      }
   }

   if (fx3Timer.firstSleepingTaskToAwake)
   {
      bsp_wakeUpAt_ticks(fx3Timer.firstSleepingTaskToAwake->sleepUntil_ticks);
   }

   if (runningTaskDethroned)
   {
      markTaskReady(runningTask);
   }

   return runningTaskDethroned;
}

bool bsp_onEpochRollover(void)
{
   assert(NULL == fx3Timer.firstSleepingTaskToAwake);

   struct fx3_command* cmd = allocateFX3Command();

   cmd->type = FX3_TIMER_EVENT_EPOCH_ROLLOVER;

   postFX3Command(cmd);

   return true;
}

bool bsp_onRoundRobinSliceTimeout(void)
{
   assert(TS_RUNNING == runningTask->state);
   assert(runningTask->config->timeSlice_ticks);

   runningTask->roundRobinSliceLeft_ticks = 0;

   scheduleReadyTask(runningTask);

   bsp_scheduleContextSwitch();

   return true;
}

struct task_control_block* fx3_getRunningTask(void)
{
   return runningTask;
}

void fx3_sendMessage(struct task_control_block* tcb, struct buffer* buf)
{
   /*
    * There is an efficient lock-free stack push algorithm for
    * multiple producers. This leaves us with a list of messages
    * in the reverse order of insertion.
    *
    * The lock-free stack pop is quite involved for the multiple
    * consumer case, but it is trivial for the single consumer case:
    * just swap the 'full' incoming queue with the 'empty' todo list.
    *
    * Then, we just have to reverse the order of the todo list which
    * can be done efficiently in linear time.
    */

   // lock-free push buf into the stack pointed to by tcb->inbox
   lst_pushElement(&tcb->inbox, &buf->element);

   // make tcb runnable if blocked on its queue
   if (TS_WAITING_FOR_MESSAGE == tcb->state)
   {
      scheduleReadyTask(tcb);
   }
}

struct buffer* fx3_waitForMessage(void)
{
   struct task_control_block* thisTask = runningTask;

   /*
    * There is no need to protect messageQueue, this is the only function
    * that operates on it.
    */

   while (! thisTask->messageQueue)
   {
      // lock-free fetch the inbox variable and simultaneously reset it
      struct buffer* todo = (struct buffer*) lst_fetchAll(&thisTask->inbox);

      if (todo)
      {
         /*
          * Elements in the todo list are in the reverse order (most
          * recent element is first). We need to process messages
          * in FIFO order.
          *
          * So, treat both todo and message queue as stacks again,
          * unstacking from todo and stacking into message queue.
          *
          * This will reverse the order, and the first message in the
          * message queue is the oldest.
          */
         while (todo)
         {
            struct buffer* next    = todo->next;
            todo->next             = thisTask->messageQueue;
            thisTask->messageQueue = todo;
            todo                   = next;
         }
      }
      else
      {
         task_block(TS_WAITING_FOR_MESSAGE);
      }
   }

   struct buffer* buf     = thisTask->messageQueue;
   thisTask->messageQueue = buf->next;
   buf->next              = NULL;

   return buf;
}

static inline int __attribute__((pure)) compareTaskPriorities(const struct list_element* left, const struct list_element* right) 
{
   const struct task_control_block* leftTask  = (const struct task_control_block*) left;
   const struct task_control_block* rightTask = (const struct task_control_block*) right;

   if (leftTask->effectivePriority < rightTask->effectivePriority)
   {
      return -1;
   }
   else
   {
      return 1;
   }
}

static bool handleSemaphoreSignal(struct fx3_command* cmd)
{
   assert(FX3_SIGNAL_SEMAPHORE == cmd->type);
   struct semaphore* sem = cmd->object;
   freeFX3Command(cmd);

   bool runningTaskDethroned = false;

   // fetch late arrivals
   struct list_element* todo = lst_fetchAll(&sem->antechamber);

   assert(lst_isSortedAscending(&sem->waitList->element, compareTaskPriorities));

   /*
    * merge tasks from antechamber into wait list
    */
   if ((NULL == sem->waitList) && (todo) && (NULL == todo->next))
   {
      // shortcut; target empty, source has only one element
      sem->waitList = (struct task_control_block*) todo;
   }
   else
   {
      lst_mergeListIntoSortedList((struct list_element**) &sem->waitList, todo, compareTaskPriorities);
   }

   // select the highest priority waiting task and mark ready
   struct task_control_block* highestPriorityWaitingTask = sem->waitList;
   if (highestPriorityWaitingTask)
   {
      sem->waitList                    = highestPriorityWaitingTask->next;
      highestPriorityWaitingTask->next = NULL;

      markTaskReady(highestPriorityWaitingTask);
      runningTaskDethroned = (highestPriorityWaitingTask->effectivePriority < runningTask->effectivePriority);
   }

   if (runningTaskDethroned)
   {
      markTaskReady(runningTask);
   }

   return runningTaskDethroned;
}

bool fx3_processPendingCommands(void)
{
   bool contextSwitchNeeded = (TS_RUNNING != runningTask->state);

   verifyTaskControlBlocks(false);

   while (true)
   {
      // lock-free fetch the inbox variable and simultaneously reset it
      struct fx3_command* todo = (struct fx3_command*) lst_fetchAll(&fx3MessageCenter.inbox);

      if (! todo)
      {
         break;
      }
      else
      {
         struct fx3_command* messageQueue = NULL;

         /*
          * Elements in the todo list are in the reverse order (most
          * recent element is first). We need to process messages
          * in FIFO order.
          *
          * So, treat both todo and message queue as stacks again,
          * unstacking from todo and stacking into message queue.
          *
          * This will reverse the order, and the first message in the
          * message queue is the oldest.
          */
         while (todo)
         {
            struct fx3_command* next = todo->next;
            todo->next               = messageQueue;
            messageQueue             = todo;
            todo                     = next;
         }

         while (messageQueue)
         {
            struct fx3_command* cmd = (struct fx3_command*) messageQueue;
            messageQueue = messageQueue->next;

            switch (cmd->type)
            {
               case FX3_READY_TASK:
                  markTaskReady(cmd->task);
                  freeFX3Command(cmd);
                  break;

               case FX3_TIMER_REQUEST_WAKEUP:
                  handleSleepRequest(cmd);
                  contextSwitchNeeded = true;
                  break;

               case FX3_TIMER_EVENT_WAKEUP:
                  contextSwitchNeeded |= handleWakeUpAlarm(cmd);
                  break;

               case FX3_TIMER_EVENT_EPOCH_ROLLOVER:
                  contextSwitchNeeded |= handleEpochRollover(cmd);
                  break;

               case FX3_SIGNAL_SEMAPHORE:
                  contextSwitchNeeded |= handleSemaphoreSignal(cmd);
                  break;

               default:
                  assert(false);
                  break;
            }
         }
      }
   }

   if (contextSwitchNeeded)
   {
      selectNextRunningTask();
   }

   return contextSwitchNeeded;
}

void fx3impl_wakeupTasksWaitingOnSemaphore(struct semaphore* sem)
{
   struct fx3_command* cmd = allocateFX3Command();

   cmd->type   = FX3_SIGNAL_SEMAPHORE;
   cmd->object = sem;

   postFX3Command(cmd);
}

void fx3impl_enqueueTaskOnSemaphore(struct semaphore* sem)
{
   runningTask->waitingOn = sem;
   runningTask->state     = TS_WAITING_FOR_SEMAPHORE;

   lst_pushElement(&sem->antechamber, &runningTask->element);

   bsp_scheduleContextSwitch();
}
