/**
 * @file task_priv.h
 * @brief Private task declarations
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

#ifndef __FX3_TASK_PRIV_H__
#define __FX3_TASK_PRIV_H__

#include <task.h>
#include <buffer.h>

void fx3_startMultitaskingImpl(uint32_t taskPSP, void (* handler)(const void* arg), const void* arg);

void task_sleep_ticks(uint32_t timeout_ticks);

void task_block(enum task_state newState);

void fx3_readyTask(struct task_control_block* tcb);

struct task_control_block* fx3_getRunningTask(void);

/** Push 'buf' into the message stack pointed to by head.
 *
 */
void fx3_enqueueMessage(struct buffer** head, struct buffer* buf);

/** Reset *head and return old value.
 *
 */
struct buffer* fx3_flushInbox(struct buffer** head);

#endif // __FX3_TASK_PRIV_H__

