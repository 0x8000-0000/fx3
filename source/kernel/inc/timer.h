/**
 * @file timer.h
 * @brief Timer Management
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

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

enum timer_status
{
   TS_UNINITIALIZED,
   TS_ARMED,
   TS_PENDING,
   TS_FIRED,
};

enum timer_type
{
   TT_ONE_SHOT,
   TT_PERIODIC,
};

struct timer_config
{
   void              (* handler)(void* arg);
   void*             argument;
   uint32_t          interval_tick;
   enum timer_type   type;
};

struct timer
{
   struct timer_config* config;

   uint32_t             deadline;

   enum timer_status    status;
};

void tmr_initialize(struct timer* tmr);

void tmr_start(struct timer* tmr);

void tmr_stop(struct timer* tmr);

#endif // __TIMER_H__

