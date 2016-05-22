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
#include <string.h>

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

static SPI_HandleTypeDef hspi3;

static void initializeSPI(void)
{
   __SPI3_CLK_ENABLE();

   __GPIOA_CLK_ENABLE();

   static const GPIO_InitTypeDef GPIO_InitStructNSS =
   {
      .Pin       = GPIO_PIN_4,
      .Mode      = GPIO_MODE_AF_PP,
      .Pull      = GPIO_NOPULL,
      .Speed     = GPIO_SPEED_HIGH,
      .Alternate = GPIO_AF6_SPI3,
   };
   HAL_GPIO_Init(GPIOA, (GPIO_InitTypeDef*) &GPIO_InitStructNSS);

   __GPIOC_CLK_ENABLE();

   static const GPIO_InitTypeDef GPIO_InitStruct =
   {
      .Pin       = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12,
      .Mode      = GPIO_MODE_AF_PP,
      .Pull      = GPIO_NOPULL,
      .Speed     = GPIO_SPEED_HIGH,
      .Alternate = GPIO_AF6_SPI3,
   };
   HAL_GPIO_Init(GPIOC, (GPIO_InitTypeDef*) &GPIO_InitStruct);

   hspi3.Instance               = SPI3;
   hspi3.Init.Mode              = SPI_MODE_MASTER;
   hspi3.Init.Direction         = SPI_DIRECTION_2LINES;
   hspi3.Init.DataSize          = SPI_DATASIZE_8BIT;
   hspi3.Init.CLKPolarity       = SPI_POLARITY_LOW;
   hspi3.Init.CLKPhase          = SPI_PHASE_1EDGE;
   hspi3.Init.NSS               = SPI_NSS_HARD_OUTPUT;
   hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
   hspi3.Init.FirstBit          = SPI_FIRSTBIT_MSB;
   hspi3.Init.TIMode            = SPI_TIMODE_DISABLED;
   hspi3.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
   hspi3.Init.CRCPolynomial     = 10;
   HAL_SPI_Init(&hspi3);
}

uint8_t transmitBufferSupport[512];
uint8_t receiveBufferSupport[512];

struct USARTHandle usart1;
struct USARTHandle usart2;

static void initializeUART(void)
{

   __GPIOA_CLK_ENABLE();

   // UART1
   {
      memset(&usart1, 0, sizeof(usart1));
      usart1.receiveBuffer.size = 256;
      usart1.receiveBuffer.data = receiveBufferSupport;
      usart1.transmitBuffer.size = 256;
      usart1.transmitBuffer.data = transmitBufferSupport;

      static const GPIO_InitTypeDef GPIO_InitStruct =
      {
         .Pin       = GPIO_PIN_9 | GPIO_PIN_10,
         .Mode      = GPIO_MODE_AF_PP,
         .Pull      = GPIO_PULLUP,
         .Speed     = GPIO_SPEED_HIGH,
         .Alternate = GPIO_AF7_USART1,
      };
      HAL_GPIO_Init(GPIOA, (GPIO_InitTypeDef*) &GPIO_InitStruct);

      __HAL_RCC_USART1_CLK_ENABLE();
      __HAL_RCC_DMA2_CLK_ENABLE();

      usart1.huart.Instance       = USART1;
      usart1.uartIRQ              = USART1_IRQn;

      usart1.transmitDMA.Instance = DMA2_Stream7;
      usart1.transmitDMAIRQ       = DMA2_Stream7_IRQn;
      usart1.transmitDMAChannel   = DMA_CHANNEL_4;

      usart1.receiveDMA.Instance  = DMA2_Stream2;
      usart1.receiveDMAIRQ        = DMA2_Stream2_IRQn;
      usart1.receiveDMAChannel    = DMA_CHANNEL_4;
   }

   // UART2
   {
      memset(&usart2, 0, sizeof(usart2));
      usart2.receiveBuffer.size = 256;
      usart2.receiveBuffer.data = receiveBufferSupport + 256;
      usart2.transmitBuffer.size = 256;
      usart2.transmitBuffer.data = transmitBufferSupport + 256;

      static const GPIO_InitTypeDef GPIO_InitStruct =
      {
         .Pin       = GPIO_PIN_2 | GPIO_PIN_3,
         .Mode      = GPIO_MODE_AF_PP,
         .Pull      = GPIO_NOPULL,
         .Speed     = GPIO_SPEED_LOW,
         .Alternate = GPIO_AF7_USART2,
      };
      HAL_GPIO_Init(GPIOA, (GPIO_InitTypeDef*) &GPIO_InitStruct);

      __HAL_RCC_USART2_CLK_ENABLE();
      __HAL_RCC_DMA1_CLK_ENABLE();

      usart2.huart.Instance       = USART2;
      usart2.uartIRQ              = USART2_IRQn;

      usart2.transmitDMA.Instance = DMA1_Stream6;
      usart2.transmitDMAIRQ       = DMA1_Stream7_IRQn;
      usart2.transmitDMAChannel   = DMA_CHANNEL_4;

      usart2.receiveDMA.Instance  = DMA1_Stream5;
      usart2.receiveDMAIRQ        = DMA1_Stream5_IRQn;
      usart2.receiveDMAChannel    = DMA_CHANNEL_4;
   }
}

void bsp_initialize(void)
{
   chp_initialize();

   NVIC_SetPriority(PendSV_IRQn, 0xFF); // Set PendSV to lowest possible priority

   initializeMainClock();

   initializeLEDs();

   initializeSPI();

   initializeUART();
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

