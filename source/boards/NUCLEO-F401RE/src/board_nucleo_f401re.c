/**
 * @file board_nucleo_f401re.c
 * @brief Board Support Package implementation for NUCLEO-F401RE
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
#include <stdbool.h>

#include <board.h>

#include <stm32f4xx.h>
#include <stm32_chp.h>

extern void SystemCoreClockUpdate(void);

/*
 * This will generate 168 MHz clock using 8MHz HSE
 */
static void initializeMainClock(void)
{
   volatile uint32_t dummyRead;

   // enable power
   RCC->APB1ENR |= RCC_APB1ENR_PWREN;
   dummyRead = RCC->APB1ENR;

   static const RCC_OscInitTypeDef RCC_OscInitStruct =
   {
      .OscillatorType      = RCC_OSCILLATORTYPE_HSI,
      .HSIState            = RCC_HSI_ON,
      .HSICalibrationValue = 0x10,
      .PLL =
      {
         .PLLState   = RCC_PLL_ON,
         .PLLSource  = RCC_PLLSOURCE_HSI,
         .PLLM       = 16,
         .PLLN       = 336,
         .PLLP       = RCC_PLLP_DIV4,
         .PLLQ       = 7,
      },
   };
   HAL_RCC_OscConfig((RCC_OscInitTypeDef*) &RCC_OscInitStruct);

   static const RCC_ClkInitTypeDef RCC_ClkInitStruct =
   {
      .ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
      .SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK,
      .AHBCLKDivider  = RCC_SYSCLK_DIV1,
      .APB1CLKDivider = RCC_HCLK_DIV2,
      .APB2CLKDivider = RCC_HCLK_DIV1,
   };
   HAL_RCC_ClockConfig((RCC_ClkInitTypeDef*) &RCC_ClkInitStruct, FLASH_LATENCY_2);

   HAL_RCC_MCOConfig(RCC_MCO2, RCC_MCO2SOURCE_SYSCLK, RCC_MCODIV_4);

   SystemCoreClockUpdate();
}

static void initializeLEDs(void)
{
   __GPIOA_CLK_ENABLE();

   static const GPIO_InitTypeDef GPIO_InitStruct =
   {
      .Pin   = GPIO_PIN_5,
      .Mode  = GPIO_MODE_OUTPUT_PP,
      .Pull  = GPIO_PULLUP,
      .Speed = GPIO_SPEED_FAST,
   };
   HAL_GPIO_Init(GPIOA, (GPIO_InitTypeDef*) &GPIO_InitStruct);
}

void bsp_initialize(void)
{
   chp_initialize();

   NVIC_SetPriority(PendSV_IRQn, 0xFF); // Set PendSV to lowest possible priority

   initializeMainClock();

   initializeLEDs();
}

void bsp_turnOnLED(uint32_t ledId)
{
   if (LED_COUNT > ledId)
   {
      HAL_GPIO_WritePin(GPIOA, (GPIO_PIN_5 << ledId), GPIO_PIN_RESET);
   }
}

void bsp_turnOffLED(uint32_t ledId)
{
   if (LED_COUNT > ledId)
   {
      HAL_GPIO_WritePin(GPIOA, (GPIO_PIN_5 << ledId), GPIO_PIN_SET);
   }
}

void bsp_startMainClock(void)
{
   /*
    * Core clock runs at 84 MHz
    * Timer clock runs at 84 Mhz
    * Maximum prescaler is 16 bits
    * 42000 will give us two ticks every millisecond
    */
   chp_initializeSystemTimer(42 * 1000 - 1);
}

