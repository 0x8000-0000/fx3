/**
 * @file board.h
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

static volatile uint32_t clockUpperBits;

static bool runningUnderDebugger;

void chp_initialize(void)
{
   SCB->CCR |= SCB_CCR_STKALIGN_Msk; // Enable double word stack alignment 
   //(recommended in Cortex-M3 r1p1, default in Cortex-M3 r2px and Cortex-M4)

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
}

enum Services
{
   SVC_RESET,
   SVC_SCHEDULE_CONTEXT_SWITCH,
};

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


#if 0
void bsp_wakeUpAfter_ticks(uint32_t duration_ticks)
{
   // TODO: handle roll-over
   wakeupRequestedAt = bsp_getTimestamp_ticks();
   wakeupRequested   = wakeupRequestedAt + duration_ticks;

   TIM2->CCR1 = wakeupRequested;

   TIM2->DIER |= TIM_DIER_CC1IE;
}
#endif

void bsp_cancelWakeUp(void)
{
   wakeupRequestedAt = 0;
   wakeupRequested   = 0;

   TIM2->DIER &= ~TIM_DIER_CC1IE;
}

void SVC_Handler_C(uint32_t* svcArgs)
{
   uint8_t svcNumber = ((uint8_t *) svcArgs[6])[-2]; // Memory[(Stacked PC)-2]
   switch (svcNumber)
   {
      case SVC_RESET:
         NVIC_SystemReset();
         break;

      case SVC_SCHEDULE_CONTEXT_SWITCH:
         SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Set PendSV to pending
         break;

      default:
         while (1)
         {
            // wait
         }
         break;
   }
}

void bsp_startMultitasking(uint32_t taskPSP, void (* handler)(const void* arg), const void* arg)
{
   __set_PSP(taskPSP);        // Set PSP to @R0 of task 0 exception stack frame
   __set_CONTROL(0x3);        // Switch to use Process Stack, unprivileged state
   __ISB();                   // Execute ISB after changing CONTROL (architectural recommendation)

   handler(arg);
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
   bool handled = false;

   if ((TIM2->SR & TIM_SR_CC1IF))
   {
      if ((TIM2->DIER & TIM_DIER_CC1IE))
      {
         // acknowledge interrupt
         TIM2->SR = ~TIM_SR_CC1IF;

         // one-shot interrupt; will re-arm when needed
         TIM2->DIER &= ~TIM_DIER_CC1IE;

         bsp_onWokenUp();

         handled = true;
      }
   }

   if ((TIM2->SR & TIM_SR_CC2IF))
   {
      if ((TIM2->DIER & TIM_DIER_CC2IE))
      {
         // acknowledge interrupt
         TIM2->SR = ~TIM_SR_CC2IF;

         // one-shot interrupt; will re-arm when needed
         TIM2->DIER &= ~TIM_DIER_CC2IE;

         bsp_onRoundRobinSliceTimeout();

         handled = true;
      }
   }

   if ((TIM2->SR & TIM_SR_UIF))
   {
      if ((TIM2->DIER & TIM_DIER_UIE))
      {
         TIM2->SR = ~TIM_SR_UIF;

         clockUpperBits ++;

         bsp_onEpochRollover();

         handled = true;
      }
   }

   assert(handled);
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

#define __SVC(code) __ASM volatile ("svc %0" : : "i" (code) )

void bsp_scheduleContextSwitch(void)
{
   __SVC(SVC_SCHEDULE_CONTEXT_SWITCH);
}

__attribute__((noreturn)) void bsp_reset(void)
{
   if (runningUnderDebugger)
   {
      __BKPT(0x42);
   }

   __SVC(SVC_RESET);

   while (1)
   {
      __NOP();
   }
}

