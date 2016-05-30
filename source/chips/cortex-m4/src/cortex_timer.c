/**
 * @file cortex_timer.c
 * @brief Main timer implementation based on Cortex SysTick
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

#include <board.h>
#include <board_local.h>

#ifdef FX3_RTT_TRACE
#include <SEGGER_SYSVIEW.h>
#undef CAN_SLEEP_UNDER_DEBUGGER
#endif

uint32_t highClockBits;
uint32_t lowClockBits;

static bool wakeupRequested;
static bool roundRobinRequested;

static uint32_t wakeupAt_ticks;
static uint32_t roundRobinAt_ticks;

void SysTick_Handler(void)
{
#ifdef FX3_RTT_TRACE
   SEGGER_SYSVIEW_RecordEnterISR();
#endif

   bool returnToScheduler = false;

   lowClockBits ++;
   if (0 == lowClockBits)
   {
      highClockBits ++;

      returnToScheduler |= bsp_onEpochRollover();
   }

   if (wakeupRequested)
   {
      if (wakeupAt_ticks == lowClockBits)
      {
         wakeupRequested = false;
         wakeupAt_ticks  = 0;

         returnToScheduler |= bsp_onWokenUp();
      }
   }

   if (roundRobinRequested)
   {
      if (roundRobinAt_ticks == lowClockBits)
      {
         roundRobinRequested = false;
         roundRobinAt_ticks  = 0;

         returnToScheduler |= bsp_onRoundRobinSliceTimeout();
      }
   }

#ifdef FX3_RTT_TRACE
   if (returnToScheduler)
   {
      SEGGER_SYSVIEW_RecordExitISRToScheduler();
   }
   else
   {
      SEGGER_SYSVIEW_RecordExitISR();
   }
#endif
}

extern uint32_t SystemCoreClock;

void bsp_startMainClock(void)
{
   wakeupRequested = false;
   wakeupAt_ticks  = 0;

   roundRobinRequested = false;
   roundRobinAt_ticks  = 0;

   highClockBits = 0;
   lowClockBits = 0;

   SysTick_Config(SystemCoreClock / 1000);         // 1 ms tick
}

void bsp_wakeUpAt_ticks(uint32_t timestamp_ticks)
{
   wakeupAt_ticks  = timestamp_ticks;
   wakeupRequested = true;
}

void bsp_requestRoundRobinSliceTimeout_ticks(uint32_t timestamp_ticks)
{
   roundRobinAt_ticks  = timestamp_ticks;
   roundRobinRequested = true;
}

void bsp_cancelRoundRobinSliceTimeout(void)
{
   roundRobinRequested = false;
   roundRobinAt_ticks = 0;
}


