/**
 * @file kinetis_chp.c
 * @brief Chip support for Kinetis Cortex-M4 families
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

#include <board.h>
#include <board_local.h>
#include <kinetis_chp.h>

#ifdef FX3_RTT_TRACE
#include <SEGGER_SYSVIEW.h>
#undef CAN_SLEEP_UNDER_DEBUGGER
#endif

uint64_t bsp_getTimestamp64_ticks(void)
{
   return 0;   // TODO
}

static bool runningUnderDebugger;

void chp_initialize(void)
{
   SCB->CCR |= SCB_CCR_STKALIGN_Msk  // Enable double word stack alignment
                                     // (recommended in Cortex-M3 r1p1, default in Cortex-M3 r2px and Cortex-M4)
#ifdef UNALIGNED_SUPPORT_DISABLE
#ifndef FX3_RTT_TRACE
         /* disable for now, because newlib code for Cortex-M4 is built
          * assuming unaligned access is acceptable.
          * | SCB_CCR_UNALIGN_TRP_Msk
          */
#endif
#endif
         ;

   // enable all fault types
   SCB->SHCSR |=
      SCB_SHCSR_USGFAULTENA_Msk |
      SCB_SHCSR_BUSFAULTENA_Msk |
      SCB_SHCSR_MEMFAULTENA_Msk;

   runningUnderDebugger = (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk);

#ifdef CAN_SLEEP_UNDER_DEBUGGER
   if (runningUnderDebugger)
   {
      DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STANDBY | DBGMCU_CR_DBG_STOP;
   }
#endif
}

void chp_initializeSystemTimer(uint16_t prescaler)
{
}

void bsp_sleep(void)
{
#ifdef CAN_SLEEP_UNDER_DEBUGGER
   __WFI();
#else
   if (runningUnderDebugger)
   {
      while (1)
      {
         // wait
         __NOP();
      }
   }
   else
   {
      __WFI();
   }
#endif
}

__attribute__((noreturn)) void bsp_reset(void)
{
   if (runningUnderDebugger)
   {
      __BKPT(0x42);
   }

   NVIC_SystemReset();

   while (1)
   {
      __NOP();
   }
}

