/**
 * @file task.h
 * @brief Public task declarations
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

#ifndef __FX3_TASK_H__
#define __FX3_TASK_H__

#include <stdint.h>
#include <stdbool.h>

#include <buffer.h>

/** @mainpage FX3 RTOS
 *
 * @section FX3_General General Facilities
 *
 * @section FX3_Synchronization Synchronization
 *
 * @section FX3_Communication Inter-task communication
 *
 * @section FX3_Memory Memory allocation
 *
 */

/** @defgroup FX3 FX3 RTOS
 * FX3 API
 * @{
 */

enum task_state
{
   TS_UNINITIALIZED,
   TS_RUNNING,
   TS_READY,
   TS_RESTING,                   // yielded before its round-robin quanta was exhausted
   TS_EXHAUSTED,                 // used-up its round-robin quanta
   TS_SLEEPING,                  // voluntary sleep
   TS_WAITING_FOR_MUTEX,
   TS_WAITING_FOR_SEMAPHORE,
   TS_WAITING_FOR_EVENT,
   TS_WAITING_FOR_MESSAGE,

   TS_STATE_COUNT,
};

struct task_config
{
   const char*    name;

   union
   {
      void           (* handler)(const void* arg);
      uint32_t       handlerAddress;
   };

   const void*    argument;

   uint32_t       priority;

   const void*    stackBase;
   uint32_t       stackSize;
   uint32_t       timeSlice_ticks;

   bool           usesFloatingPoint;      // TODO: set proper control flags
   uint8_t        padding[3];
};

/** Task Control Block
 *
 * @note All members are private; the contents of this structure shall
 * be opaque to the application.
 */
struct task_control_block
{
   /** @privatesection */

   /// @note must be the first element  (synchronization uses it)
   struct task_control_block*    next;          // used on a waiting list

   /// @note must be the second element (context_switch.S uses it)
   uint32_t*                     stackPointer;

   /// Points to task configuration
   const struct task_config*     config;

   /// Unique identifier for this task
   uint32_t                      id;

   /** The effective priority of this task
    * @note used when on the runnable list
    */
   uint32_t                      effectivePriority;

   /** The absolute tick until this task will sleep
    * @note used when on the sleeping list
    */
   uint32_t                      sleepUntil_ticks;

   /** Current state for this task
    */
   enum task_state               state;

   uint8_t                       padding[3];

   /** What object is this task waiting on
    */
   void*                         waitingOn;

   /// Number of ticks left in this tasks slice, when running round-robin
   uint32_t                      roundRobinSliceLeft_ticks;

   /// Number of total ticks used to execute this task
   uint32_t                      totalRunTime_ticks;
   uint32_t                      startedRunningAt_ticks;

   /// number of times this task has transitioned to running state
   uint32_t                      startedRunning_count;

   struct task_control_block*    nextWithSamePriority;
   uint32_t                      roundRobinCumulative_ticks;

   /// Single-linked list of all tasks, used for periodic verification
   struct task_control_block*    nextTaskInTheGreatLink;

   /// linked list of messages received
   volatile struct buffer*       inbox;

   // linked list of received messages that are about to be processed
   struct buffer*                messageQueue;
};

/** Initialize the FX3 data structures
 */
void fx3_initialize(void);

/** Define a new task
 *
 * @param tcb is the task control block
 * @param config contains the task configuration
 */
void fx3_createTask(struct task_control_block* tcb, const struct task_config* config);

void fx3_createTaskPool(struct task_control_block* tcb, const struct task_config* config, uint32_t argumentSize, uint32_t poolSize);

/** Give control to FX3 to start multitasking
 * This method does not return.
 */
void fx3_startMultitasking(void);

void fx3_yield(void);

/** Put current task to sleep
 *
 * @param timeout_ms is the minimum amount of time to sleep
 */
void fx3_suspendTask(uint32_t timeout_ms);

/** Post a message to a task queue
 *
 * @param tcb identifies the task
 * @param buf contains the message; the receiving task takes ownership of the buffer
 */
void fx3_sendMessage(struct task_control_block* tcb, struct buffer* buf);

/** Wait until there is a message in my task queue
 *
 * @return the buffer containing the message
 */
struct buffer* fx3_waitForMessage(void);

/** @} */

#endif // __FX3_TASK_H__

