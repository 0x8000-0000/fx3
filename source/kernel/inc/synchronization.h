/**
 * @file synchronization.h
 * @brief Synchronization primitives for FX3
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

#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include <stdint.h>

/** @defgroup FX3_Synchronization Synchronization
 * Synchronization primitives for FX3
 * @{
 */

struct task_control_block;

struct semaphore
{
   uint32_t counter;

   struct task_control_block* antechamber;

   struct task_control_block* waitList;
};

/** Initialize this semaphore
 *
 * @param sem is the semaphore
 * @param count is the initial state of the semaphore
 */
void fx3_initializeSemaphore(struct semaphore* sem, uint32_t count);

/** Wait for this semaphore; block this thread until the
 * semaphore is signaled
 *
 * @param sem is the semaphore
 * @return the semaphore count
 */
uint32_t fx3_waitOnSemaphore(struct semaphore* sem);

/** Signal this semaphore
 *
 * @param sem is the semaphore
 * @return the semaphore count
 */
uint32_t fx3_signalSemaphore(struct semaphore* sem);

/** @} */

#endif // __SYNCHRONIZATION_H__

