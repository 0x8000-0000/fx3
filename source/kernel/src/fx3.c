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

#include <task.h>
#include <priority_queue.h>

#include <board.h>

#include <fx3_config.h>

#include <task_priv.h>

#ifdef FX3_RTT_TRACE
#include <SEGGER_SYSVIEW.h>
#endif

/*
 * A task can be:
 *    * running
 *    * on the runnable queue
 *    * on the sleeping queue
 *    * waiting on a semaphore, mutex, event...
 */
struct task_control_block* runningTask;
struct task_control_block* nextRunningTask;

struct task_control_block* firstSleepingTaskToAwake;

// +1 for the heap and +1 for the idle task
static uint32_t* runnableTasksMemPool[FX3_MAX_TASK_COUNT + 2];
static struct priority_queue runnableTasks;

// +1 for the heap and +1 for the idle task
static uint32_t* sleepingTasksMemPool_0[FX3_MAX_TASK_COUNT + 2];
static struct priority_queue sleepingTasks_0;
// +1 for the heap and +1 for the idle task
static uint32_t* sleepingTasksMemPool_1[FX3_MAX_TASK_COUNT + 2];
static struct priority_queue sleepingTasks_1;

static struct priority_queue* sleepingTasks;
static struct priority_queue* sleepingTasksNextEpoch;

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

void fx3_readyTask(struct task_control_block* tcb)
{
   assert(tcb->config->timeSlice_ticks >= tcb->roundRobinSliceLeft_ticks);
   tcb->state = TS_READY;
   if (tcb->config->timeSlice_ticks)
   {
      if (0 == tcb->roundRobinSliceLeft_ticks)
      {
         tcb->state = TS_EXHAUSTED;
      }
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

static uint32_t tasksCreated_count;

static void verifyTaskControlBlocks(bool expectTaskInRunningState)
{
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
      else
      {
         assert(0 == tcb->startedRunningAt_ticks);
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
   }
   while (&idleTask != tcb);

   if (expectTaskInRunningState)
   {
      assert(foundTaskInRunningState);
   }

   assert(tasksVisited == tasksCreated_count);

   /*
    * check running queue
    */
   for (uint32_t ii = 1; ii < runnableTasks.size; ii ++)
   {
      uint32_t* priority = runnableTasks.memPool[ii];
      struct task_control_block* readyTask = (struct task_control_block*) (((uint8_t*) priority) - (offsetof(struct task_control_block, effectivePriority)));

      assert((TS_READY == readyTask->state) || (TS_EXHAUSTED == readyTask->state));
   }

   /*
    * check sleeping queue
    */
   for (uint32_t ii = 1; ii < sleepingTasks->size; ii ++)
   {
      uint32_t* sleepUntil = sleepingTasks->memPool[ii];
      struct task_control_block* sleepingTask = (struct task_control_block*) (((uint8_t*) sleepUntil) - (offsetof(struct task_control_block, sleepUntil_ticks)));;

      assert(TS_SLEEPING == sleepingTask->state);
   }
}

void fx3_initialize(void)
{
   idleTask.nextTaskInTheGreatLink = 0;

   tasksCreated_count = 0;

#ifdef FX3_RTT_TRACE
   SEGGER_SYSVIEW_Conf();
#endif

   prq_initialize(&runnableTasks, runnableTasksMemPool, FX3_MAX_TASK_COUNT + 2);

   prq_initialize(&sleepingTasks_0, sleepingTasksMemPool_0, FX3_MAX_TASK_COUNT + 1);
   prq_initialize(&sleepingTasks_1, sleepingTasksMemPool_1, FX3_MAX_TASK_COUNT + 1);
   sleepingTasks          = &sleepingTasks_0;
   sleepingTasksNextEpoch = &sleepingTasks_1;

   firstSleepingTaskToAwake = NULL;

   sleepCycles = 0;

   fx3_createTask(&idleTask, &idleTaskConfig);

#ifdef FX3_RTT_TRACE
   SEGGER_SYSVIEW_Conf();
#endif
}

void createTaskImpl(struct task_control_block* tcb, const struct task_config* config, uint32_t* stackPointer, const void* argument)
{
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
   prq_push(sleepingTasks, &tcb->effectivePriority);

   // set up stack
   stackPointer[0]  = 0xFFFFFFFDUL;                   // initial EXC_RETURN
   stackPointer[1]  = 0x3;                            // initial CONTROL : unprivileged, PSP, no FP
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
   uint32_t* taskPrio = prq_pop(sleepingTasks);
   assert(taskPrio);    // we should have a task

   uint32_t lastPrio = *taskPrio;

   struct task_control_block* currentTask = (struct task_control_block*) (((uint8_t*) taskPrio) - (offsetof(struct task_control_block, effectivePriority)));
   assert(&idleTask != currentTask);

   struct task_control_block* firstTaskAtCurrentPrio             = currentTask;
   uint32_t                   cumulativeTicksAtCurrentPrio_ticks = currentTask->config->timeSlice_ticks;

   idleTask.nextTaskInTheGreatLink = currentTask;
   idleTask.nextWithSamePriority   = &idleTask;
   fx3_readyTask(currentTask);

   while (! prq_isEmpty(sleepingTasks))
   {
      taskPrio = prq_pop(sleepingTasks);
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
      fx3_readyTask(currentTask);
   }

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

void fx3_selectNextRunningTask(void)
{
   // no need to disable interrupts, the caller did it for us

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
void task_sleep_ms(uint32_t timeout_ms)
{
   __disable_irq();

   bsp_cancelRoundRobinSliceTimeout();

   if (runningTask->config->timeSlice_ticks)
   {
      uint32_t runTime = bsp_computeInterval_ticks(runningTask->startedRunningAt_ticks, bsp_getTimestamp_ticks());
      assert(runningTask->roundRobinSliceLeft_ticks >= runTime);
      runningTask->roundRobinSliceLeft_ticks -= runTime;
   }

   if (timeout_ms)
   {
      const uint32_t sleepDuration_ticks = bsp_getTicksForMS(timeout_ms);

      if (runningTask->config->timeSlice_ticks)
      {
         /*
          * This task will sleep more than a full period of round-robin
          * scheduling at this priority so replenish its full slice.
          */

         if (sleepDuration_ticks >= runningTask->roundRobinCumulative_ticks)
         {
            runningTask->roundRobinSliceLeft_ticks = runningTask->config->timeSlice_ticks;
         }
      }

      runningTask->state             = TS_SLEEPING;
      runningTask->effectivePriority = 0xffff;

      bool nextEpoch = bsp_computeWakeUp_ticks(sleepDuration_ticks, &runningTask->sleepUntil_ticks);
      if (nextEpoch)
      {
         prq_push(sleepingTasksNextEpoch, &runningTask->sleepUntil_ticks);
      }
      else
      {
         /*
          * the sleep will complete this epoch
          */

         if (0 == firstSleepingTaskToAwake)
         {
            /*
             * there is no other sleeping task
             */

            assert(prq_isEmpty(sleepingTasks));

            firstSleepingTaskToAwake = runningTask;
            bsp_wakeUpAt_ticks(runningTask->sleepUntil_ticks);
         }
         else
         {
            assert(TS_SLEEPING == firstSleepingTaskToAwake->state);
            if (firstSleepingTaskToAwake->sleepUntil_ticks > runningTask->sleepUntil_ticks)
            {
               /*
                * this new task is sleeping less than the previous task with the shortest sleep
                */

               prq_push(sleepingTasks, &firstSleepingTaskToAwake->sleepUntil_ticks);

               firstSleepingTaskToAwake = runningTask;
               bsp_wakeUpAt_ticks(runningTask->sleepUntil_ticks);
            }
            else
            {
               prq_push(sleepingTasks, &runningTask->sleepUntil_ticks);
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

   bsp_scheduleContextSwitch();

   __enable_irq();
}

void task_block(enum task_state newState)
{
   assert(TS_WAITING_FOR_MUTEX <= newState);
   assert(TS_STATE_COUNT > newState);

   bsp_cancelRoundRobinSliceTimeout();

   __disable_irq();
   runningTask->state = newState;
   bsp_scheduleContextSwitch();
   __enable_irq();
}

static volatile uint32_t lastWokenUpAt;

bool bsp_onWokenUp(void)
{
   __disable_irq();

   verifyTaskControlBlocks(true);

   assert(TS_RUNNING == runningTask->state);

   lastWokenUpAt = bsp_getTimestamp_ticks();

   assert(firstSleepingTaskToAwake);
   assert(firstSleepingTaskToAwake->sleepUntil_ticks <= lastWokenUpAt);
   assert(TS_SLEEPING == firstSleepingTaskToAwake->state);

   bool runningTaskDethroned = false;

   fx3_readyTask(firstSleepingTaskToAwake);
   if (firstSleepingTaskToAwake->effectivePriority < runningTask->effectivePriority)
   {
      runningTaskDethroned = true;
   }
   firstSleepingTaskToAwake = NULL;
   /*
    * done with the task that was waiting to be woken up
    */

   // who's next?
   while ((NULL == firstSleepingTaskToAwake) && (! prq_isEmpty(sleepingTasks)))
   {
      uint32_t* nextWakeupDeadline = prq_pop(sleepingTasks);
      assert(nextWakeupDeadline);

      firstSleepingTaskToAwake = (struct task_control_block*) (((uint8_t*) nextWakeupDeadline) - (offsetof(struct task_control_block, sleepUntil_ticks)));
      assert(TS_SLEEPING == firstSleepingTaskToAwake->state);
      if (*nextWakeupDeadline <= bsp_getTimestamp_ticks())
      {
         fx3_readyTask(firstSleepingTaskToAwake);
         if (firstSleepingTaskToAwake->effectivePriority < runningTask->effectivePriority)
         {
            runningTaskDethroned = true;
         }
         firstSleepingTaskToAwake = NULL;
      }
   }

   if (firstSleepingTaskToAwake)
   {
      bsp_wakeUpAt_ticks(firstSleepingTaskToAwake->sleepUntil_ticks);
   }
   else
   {
      bsp_cancelWakeUp();
   }

   if (runningTaskDethroned)
   {
      fx3_readyTask(runningTask);
      bsp_scheduleContextSwitchInHandlerMode();
   }

   __enable_irq();

   return runningTaskDethroned;
}

bool bsp_onEpochRollover(void)
{
   __disable_irq();

   assert(NULL == firstSleepingTaskToAwake);
   assert(prq_isEmpty(sleepingTasks));

   // swap queues
   {
      struct priority_queue* temp = sleepingTasks;
      sleepingTasks = sleepingTasksNextEpoch;
      sleepingTasksNextEpoch = temp;
   }

   bool runningTaskDethroned = false;

   while ((NULL == firstSleepingTaskToAwake) && (! prq_isEmpty(sleepingTasks)))
   {
      uint32_t* nextWakeupDeadline = prq_pop(sleepingTasks);
      assert(nextWakeupDeadline);

      firstSleepingTaskToAwake = (struct task_control_block*) (((uint8_t*) nextWakeupDeadline) - (offsetof(struct task_control_block, sleepUntil_ticks)));
      assert(TS_SLEEPING == firstSleepingTaskToAwake->state);
      if (0 == firstSleepingTaskToAwake->sleepUntil_ticks)
      {
         fx3_readyTask(firstSleepingTaskToAwake);
         if (firstSleepingTaskToAwake->effectivePriority < runningTask->effectivePriority)
         {
            runningTaskDethroned = true;
         }
         firstSleepingTaskToAwake = NULL;
      }
   }

   if (firstSleepingTaskToAwake)
   {
      bsp_wakeUpAt_ticks(firstSleepingTaskToAwake->sleepUntil_ticks);
   }

   if (runningTaskDethroned)
   {
      fx3_readyTask(runningTask);
      bsp_scheduleContextSwitchInHandlerMode();
   }

   __enable_irq();

   return runningTaskDethroned;
}

bool bsp_onRoundRobinSliceTimeout(void)
{
   __disable_irq();
   struct task_control_block* thisTask = runningTask;

   assert(TS_RUNNING == thisTask->state);

   thisTask->state                     = TS_EXHAUSTED;
   thisTask->roundRobinSliceLeft_ticks = 0;
   thisTask->effectivePriority         = computeEffectivePriority(thisTask->state, thisTask->config);
   prq_push(&runnableTasks, &thisTask->effectivePriority);

   bsp_scheduleContextSwitchInHandlerMode();

   __enable_irq();

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
   fx3_enqueueMessage((struct buffer**) &tcb->inbox, buf);

   // make tcb runnable if blocked on its queue
   if (TS_WAITING_FOR_MESSAGE == tcb->state)
   {
      __disable_irq();
      fx3_readyTask(tcb);
      __enable_irq();
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
      struct buffer* todo = fx3_flushInbox((struct buffer**) &thisTask->inbox);

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

