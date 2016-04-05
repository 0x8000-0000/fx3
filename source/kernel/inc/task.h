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

struct task_control_block
{
   struct task_control_block*    next;       // used on a waiting list

   uint32_t*                     stackPointer;

   const struct task_config*     config;

   /// used when on the runnable list
   uint32_t                      effectivePriority;

   /// used when on the sleeping list
   uint32_t                      sleepUntil_ticks;

   enum task_state               state;

   uint8_t                       padding[3];

   void*                         waitingOn;

   /// the ticks in the round-robin slice
   uint32_t                      roundRobinSliceLeft_ticks;

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

/**
 *
 */
void fx3_initialize(void);

void fx3_createTask(struct task_control_block* tcb, const struct task_config* config);

void fx3_createTaskPool(struct task_control_block* tcb, const struct task_config* config, uint32_t argumentSize, uint32_t poolSize);

void fx3_startMultitasking(void);

void fx3_yield(void);

void task_sleep_ms(uint32_t timeout_ms);

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

#endif // __FX3_TASK_H__

