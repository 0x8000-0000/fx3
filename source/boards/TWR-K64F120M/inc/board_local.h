/**
 * @file board_local.h
 * @brief Board Support Package interface
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

#ifndef __BOARD_LOCAL_H__
#define __BOARD_LOCAL_H__

#include <stdint.h>
#include <stdbool.h>

#include <MK64F12.h>

static inline uint32_t bsp_getTicksForMS(uint32_t time_ms)
{
   return time_ms;
}

extern uint32_t lowClockBits;

static inline uint32_t bsp_getTimestamp_ticks(void)
{
   return lowClockBits;
}

static inline uint32_t bsp_computeInterval_ticks(uint32_t start_ticks, uint32_t end_ticks)
{
   uint32_t duration_ticks = 0;
   if (end_ticks >= start_ticks)
   {
      duration_ticks = end_ticks - start_ticks;
   }
   else
   {
      duration_ticks = end_ticks + 1 + (0xffffffff - start_ticks);
   }
   return duration_ticks;
}

static inline void bsp_scheduleContextSwitchInHandlerMode(void)
{
   SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Set PendSV to pending
}

static inline bool bsp_computeWakeUp_ticks(uint32_t duration_ticks, uint32_t* wakeupAt_ticks)
{
   *wakeupAt_ticks = (lowClockBits + duration_ticks);    // intentional wrap-around
   return *wakeupAt_ticks < lowClockBits;
}

enum BOARD_LED
{
   LED_ID_GREEN,
   LED_ID_ORANGE,
   LED_ID_RED,
   LED_ID_BLUE,

   LED_COUNT,
};

#endif // __BOARD_LOCAL_H__

