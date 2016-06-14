/**
 * @file stm32f4_driver_input.c
 * @brief GPIO input driver implementation for STM32F4 chips
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
#include <string.h>

#include <board.h>
#include <board_local.h>

#ifdef FX3_RTT_TRACE
#include <SEGGER_SYSVIEW.h>
#endif

void bsp_requestNotificationForInputChange(uint32_t inputPin)
{
}

void bsp_enableInputStateNotifications(void)
{
   HAL_NVIC_EnableIRQ(EXTI0_IRQn);
   HAL_NVIC_EnableIRQ(EXTI1_IRQn);
   HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}

void bsp_disableInputStateNotifications(void)
{
   HAL_NVIC_DisableIRQ(EXTI0_IRQn);
   HAL_NVIC_DisableIRQ(EXTI1_IRQn);
   HAL_NVIC_DisableIRQ(EXTI2_IRQn);
}

void __attribute__((weak)) bsp_onInputStateChanged(uint32_t inputPin, bool status)
{
}

void EXTI0_IRQHandler(void)
{
#ifdef FX3_RTT_TRACE
   SEGGER_SYSVIEW_RecordEnterISR();
#endif

   if ((EXTI->PR & GPIO_PIN_0))
   {
      EXTI->PR = GPIO_PIN_0;

      bool PA0_status = (0 != (GPIOA->IDR & GPIO_PIN_0));
      bsp_onInputStateChanged(PIN('A', 0), PA0_status);
   }

#ifdef FX3_RTT_TRACE
   // we send a message that will wake-up the debouncing task
   SEGGER_SYSVIEW_RecordExitISRToScheduler();
#endif
}

void EXTI1_IRQHandler(void)
{
#ifdef FX3_RTT_TRACE
   SEGGER_SYSVIEW_RecordEnterISR();
#endif

   if ((EXTI->PR & GPIO_PIN_1))
   {
      EXTI->PR = GPIO_PIN_1;

      bool PC1_status = (0 != (GPIOC->IDR & GPIO_PIN_1));
      bsp_onInputStateChanged(PIN('C', 1), PC1_status);
   }

#ifdef FX3_RTT_TRACE
   // we send a message that will wake-up the debouncing task
   SEGGER_SYSVIEW_RecordExitISRToScheduler();
#endif
}

void EXTI2_IRQHandler(void)
{
#ifdef FX3_RTT_TRACE
   SEGGER_SYSVIEW_RecordEnterISR();
#endif

   if ((EXTI->PR & GPIO_PIN_2))
   {
      EXTI->PR = GPIO_PIN_2;

      bool PC2_status = (0 != (GPIOC->IDR & GPIO_PIN_2));
      bsp_onInputStateChanged(PIN('C', 2), PC2_status);
   }

#ifdef FX3_RTT_TRACE
   // we send a message that will wake-up the debouncing task
   SEGGER_SYSVIEW_RecordExitISRToScheduler();
#endif
}
