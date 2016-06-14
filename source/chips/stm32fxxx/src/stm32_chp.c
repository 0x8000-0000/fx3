/**
 * @file stm32_chp.c
 * @brief Chip support for STM32F3 and STM32F4 families
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
#include <stm32_chp.h>

#ifdef FX3_RTT_TRACE
#include <SEGGER_SYSVIEW.h>
#undef CAN_SLEEP_UNDER_DEBUGGER
#endif

static volatile uint32_t clockUpperBits;

uint64_t bsp_getTimestamp64_ticks(void)
{
   uint64_t now = clockUpperBits;
   now <<= 31;
   now |= (TIM2->CNT >> 1);
   return now;
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

   clockUpperBits = 0;

   HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 3);
   HAL_NVIC_SetPriority(EXTI1_IRQn, 2, 3);
   HAL_NVIC_SetPriority(EXTI2_IRQn, 2, 3);
}

void chp_initializeSystemTimer(uint16_t prescaler)
{
   // wake-up timer
   {
      volatile uint32_t dummyRead;
      RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
      dummyRead = RCC->APB1ENR;
      (void) dummyRead;

      HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0);

      /* Set the Auto-reload value */
#ifdef TEST_TIMER_WRAP
      TIM2->ARR = 0xffff;
#else
      TIM2->ARR = 0xffffffff;
#endif

      /* Set the Prescaler value */
      TIM2->PSC = prescaler;

      /* Generate an update event to reload the Prescaler value immediately */
      TIM2->EGR = TIM_EGR_UG;

      // Clear update event
      TIM2->SR = ~TIM_SR_UIF;

      /* Enable the TIM Update interrupt */
      TIM2->DIER |= TIM_DIER_UIE;

      /* Stop the timer when the debugger is stopped */
      DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM2_STOP;

      HAL_NVIC_EnableIRQ(TIM2_IRQn);

      /* Enable the Peripheral */
      TIM2->CR1 |= TIM_CR1_CEN;
   }
}

static volatile uint32_t wakeupRequestedAt;
static volatile uint32_t wakeupRequested;

void bsp_wakeUpAt_ticks(uint32_t timestamp_ticks)
{
   wakeupRequestedAt = bsp_getTimestamp_ticks();
   wakeupRequested   = timestamp_ticks;
   assert(wakeupRequested > wakeupRequestedAt);

   TIM2->CCR1  = timestamp_ticks;
   TIM2->DIER |= TIM_DIER_CC1IE;
}

void bsp_requestRoundRobinSliceTimeout_ticks(uint32_t timestamp_ticks)
{
   TIM2->CCR2  = timestamp_ticks;
   TIM2->DIER |= TIM_DIER_CC2IE;
}

void bsp_cancelRoundRobinSliceTimeout(void)
{
   TIM2->DIER &= ~TIM_DIER_CC2IE;
}

void bsp_requestDebounceTimeout_ticks(uint32_t timestamp_ticks)
{
   TIM2->CCR3  = timestamp_ticks;
   TIM2->DIER |= TIM_DIER_CC3IE;
}

void bsp_cancelDebounceTimeout(void)
{
   TIM2->DIER &= ~TIM_DIER_CC3IE;
}

bool __attribute__((weak)) bsp_onDebounceIntervalTimeout(void)
{
   return false;
}

void bsp_disableSystemTimer(void)
{
   HAL_NVIC_DisableIRQ(TIM2_IRQn);
}

void bsp_enableSystemTimer(void)
{
   HAL_NVIC_EnableIRQ(TIM2_IRQn);
}


/*
 * Used by ST HAL only
 */
void SysTick_Handler(void)
{
   HAL_IncTick();
   HAL_SYSTICK_IRQHandler();
}

static volatile uint32_t lastBlinkedAt;

void TIM2_IRQHandler(void)
{
#ifdef FX3_RTT_TRACE
   SEGGER_SYSVIEW_RecordEnterISR();
#endif

   bool returnToScheduler = false;

   bool handled = false;

   if ((TIM2->SR & TIM_SR_CC1IF))
   {
      // acknowledge interrupt
      TIM2->SR = ~TIM_SR_CC1IF;

      if ((TIM2->DIER & TIM_DIER_CC1IE))
      {
         // one-shot interrupt; will re-arm when needed
         TIM2->DIER &= ~TIM_DIER_CC1IE;
         wakeupRequestedAt = 0;
         wakeupRequested   = 0;

         returnToScheduler |= bsp_onWokenUp();

         handled = true;
      }
   }

   if ((TIM2->SR & TIM_SR_CC2IF))
   {
      // acknowledge interrupt
      TIM2->SR = ~TIM_SR_CC2IF;

      if ((TIM2->DIER & TIM_DIER_CC2IE))
      {
         // one-shot interrupt; will re-arm when needed
         TIM2->DIER &= ~TIM_DIER_CC2IE;

         returnToScheduler |= bsp_onRoundRobinSliceTimeout();

         handled = true;
      }
   }

   if ((TIM2->SR & TIM_SR_CC3IF))
   {
      // acknowledge interrupt
      TIM2->SR = ~TIM_SR_CC3IF;

      if ((TIM2->DIER & TIM_DIER_CC3IE))
      {
         // one-shot interrupt; will re-arm when needed
         TIM2->DIER &= ~TIM_DIER_CC3IE;

         returnToScheduler |= bsp_onDebounceIntervalTimeout();

         handled = true;
      }
   }

   if ((TIM2->SR & TIM_SR_UIF))
   {
      TIM2->SR = ~TIM_SR_UIF;

      if ((TIM2->DIER & TIM_DIER_UIE))
      {
         clockUpperBits ++;

         returnToScheduler |= bsp_onEpochRollover();

         handled = true;
      }
   }

   assert(handled);

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

   while (true)
   {
      __NOP();
   }
}

#ifdef FX3_RTT_TRACE
void bsp_describeInterrupts(void)
{
   SEGGER_SYSVIEW_SendSysDesc("I#44=TIM2");
   SEGGER_SYSVIEW_SendSysDesc("I#22=EXTI0");
   SEGGER_SYSVIEW_SendSysDesc("I#23=EXTI1");
   SEGGER_SYSVIEW_SendSysDesc("I#24=EXTI2");
}
#endif
