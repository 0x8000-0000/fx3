/**
 * @file board_stm32f4disco.c
 * @brief Board Support Package implementation for STM32F4-Discovery
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
      .OscillatorType = RCC_OSCILLATORTYPE_HSE,
      .HSEState       = RCC_HSE_ON,
      .PLL =
      {
         .PLLState   = RCC_PLL_ON,
         .PLLSource  = RCC_PLLSOURCE_HSE,
         .PLLM       = 8,
         .PLLN       = 336,
         .PLLP       = RCC_PLLP_DIV2,
         .PLLQ       = 7,
      },
   };
   HAL_RCC_OscConfig((RCC_OscInitTypeDef*) &RCC_OscInitStruct);

   static const RCC_ClkInitTypeDef RCC_ClkInitStruct =
   {
      .ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
      .SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK,
      .AHBCLKDivider  = RCC_SYSCLK_DIV1,
      .APB1CLKDivider = RCC_HCLK_DIV4,
      .APB2CLKDivider = RCC_HCLK_DIV2,
   };
   HAL_RCC_ClockConfig((RCC_ClkInitTypeDef*) &RCC_ClkInitStruct, FLASH_LATENCY_5);

   HAL_RCC_MCOConfig(RCC_MCO2, RCC_MCO2SOURCE_SYSCLK, RCC_MCODIV_4);

   SystemCoreClockUpdate();
}

static void initializeLEDs(void)
{
   __GPIOD_CLK_ENABLE();

   static const GPIO_InitTypeDef GPIO_InitStruct =
   {
      .Pin   = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
      .Mode  = GPIO_MODE_OUTPUT_PP,
      .Pull  = GPIO_NOPULL,
      .Speed = GPIO_SPEED_LOW,
   };
   HAL_GPIO_Init(GPIOD, (GPIO_InitTypeDef*) &GPIO_InitStruct);
}

static void initializeButtons(void)
{
   __GPIOA_CLK_ENABLE();

   static const GPIO_InitTypeDef GPIO_InitStruct =
   {
      .Pin   = GPIO_PIN_0,
      .Mode  = GPIO_MODE_IT_RISING_FALLING,
      .Pull  = GPIO_NOPULL,
      .Speed = GPIO_SPEED_FAST,
   };
   HAL_GPIO_Init(GPIOA, (GPIO_InitTypeDef*) &GPIO_InitStruct);
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
         .Pull      = GPIO_NOPULL,
         .Speed     = GPIO_SPEED_FAST,
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
         .Speed     = GPIO_SPEED_FAST,
         .Alternate = GPIO_AF7_USART2,
      };
      HAL_GPIO_Init(GPIOA, (GPIO_InitTypeDef*) &GPIO_InitStruct);

      __HAL_RCC_USART2_CLK_ENABLE();
      __HAL_RCC_DMA1_CLK_ENABLE();

      usart2.huart.Instance       = USART2;
      usart2.uartIRQ              = USART2_IRQn;

      usart2.transmitDMA.Instance = DMA1_Stream6;
      usart2.transmitDMAIRQ       = DMA1_Stream6_IRQn;
      usart2.transmitDMAChannel   = DMA_CHANNEL_4;

      usart2.receiveDMA.Instance  = DMA1_Stream5;
      usart2.receiveDMAIRQ        = DMA1_Stream5_IRQn;
      usart2.receiveDMAChannel    = DMA_CHANNEL_4;
   }
}

void bsp_delay(uint32_t count)
{
   volatile uint32_t todo = 0;
   while (count)
   {
      todo = count;
      count --;
   }
}

struct I2CHandle i2c1;
struct I2CHandle i2c2;

static void initializeI2C()
{
   // I2C1
   {
      memset(&i2c1, 0, sizeof(i2c1));

      __HAL_RCC_GPIOB_CLK_ENABLE();
      static const GPIO_InitTypeDef GPIO_InitStruct =
      {
         .Pin       = GPIO_PIN_6 | GPIO_PIN_9,
         .Mode      = GPIO_MODE_AF_OD,
         .Pull      = GPIO_PULLUP,
         .Speed     = GPIO_SPEED_FAST,
         .Alternate = GPIO_AF4_I2C1,
      };
      HAL_GPIO_Init(GPIOB, (GPIO_InitTypeDef*) &GPIO_InitStruct);

      __HAL_RCC_I2C1_CLK_ENABLE();

#if 0
      // some code samples show a force reset
      __HAL_RCC_I2C1_FORCE_RESET();
      bsp_delay(128);
      __HAL_RCC_I2C1_RELEASE_RESET();
#endif

      i2c1.hi2c.Instance = I2C1;
      i2c1.evIRQ         = I2C1_EV_IRQn;
      i2c1.erIRQ         = I2C1_ER_IRQn;
   }

   // I2C2
   {
      memset(&i2c2, 0, sizeof(i2c2));

      __HAL_RCC_GPIOB_CLK_ENABLE();
      static const GPIO_InitTypeDef GPIO_InitStruct =
      {
         .Pin       = GPIO_PIN_10 | GPIO_PIN_11,
         .Mode      = GPIO_MODE_AF_OD,
         .Pull      = GPIO_PULLUP,
         .Speed     = GPIO_SPEED_HIGH,
         .Alternate = GPIO_AF4_I2C2,
      };
      HAL_GPIO_Init(GPIOB, (GPIO_InitTypeDef*) &GPIO_InitStruct);

      __HAL_RCC_I2C2_CLK_ENABLE();

#if 0
      __HAL_RCC_I2C2_FORCE_RESET();
      bsp_delay(128);
      __HAL_RCC_I2C2_RELEASE_RESET();
#endif

      i2c2.hi2c.Instance = I2C2;
      i2c2.evIRQ         = I2C2_EV_IRQn;
      i2c2.erIRQ         = I2C2_ER_IRQn;
   }
}

struct SPIBus spiBus1;
struct SPIBus spiBus2;

static void initializeSPI()
{
   // SPI1
   {
      memset(&spiBus1, 0, sizeof(spiBus1));

      __HAL_RCC_GPIOA_CLK_ENABLE();
      static const GPIO_InitTypeDef GPIO_InitStruct =
      {
         //           CLK          MISO         MOSI
         .Pin       = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
         .Mode      = GPIO_MODE_AF_PP,
         .Pull      = GPIO_PULLDOWN,
         .Speed     = GPIO_SPEED_MEDIUM,
         .Alternate = GPIO_AF5_SPI1,
      };
      HAL_GPIO_Init(GPIOA, (GPIO_InitTypeDef*) &GPIO_InitStruct);

      __HAL_RCC_SPI1_CLK_ENABLE();

      spiBus1.halHandle.Instance = SPI1;
   }

   // SPI2
   {
      memset(&spiBus2, 0, sizeof(spiBus2));

      __HAL_RCC_GPIOB_CLK_ENABLE();
      static const GPIO_InitTypeDef GPIO_InitStruct =
      {
         //           CLK           MISO          MOSI
         .Pin       = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
         .Mode      = GPIO_MODE_AF_PP,
         .Pull      = GPIO_PULLDOWN,
         .Speed     = GPIO_SPEED_MEDIUM,
         .Alternate = GPIO_AF5_SPI2,
      };
      HAL_GPIO_Init(GPIOB, (GPIO_InitTypeDef*) &GPIO_InitStruct);

      __HAL_RCC_SPI2_CLK_ENABLE();

      spiBus2.halHandle.Instance = SPI2;
   }
}

static void initializeChipSelects(void)
{
   __GPIOE_CLK_ENABLE();

   {
      static const GPIO_InitTypeDef GPIO_InitStruct =
      {
         .Pin   = GPIO_PIN_3 | GPIO_PIN_15,
         .Mode  = GPIO_MODE_OUTPUT_PP,
         .Pull  = GPIO_NOPULL,
         .Speed = GPIO_SPEED_MEDIUM,
      };
      HAL_GPIO_Init(GPIOE, (GPIO_InitTypeDef*) &GPIO_InitStruct);

      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3 | GPIO_PIN_15, GPIO_PIN_SET);
   }

#if 0
   // INT0 on PE0 and INT2 on PE1 for onboard accelerometer
   {
      static const GPIO_InitTypeDef GPIO_InitStruct =
      {
         .Pin   = GPIO_PIN_0 | GPIO_PIN_1,
         .Mode  = GPIO_MODE_IT_RISING,
         .Pull  = GPIO_NOPULL,
         .Speed = GPIO_SPEED_FAST,
      };
      HAL_GPIO_Init(GPIOE, (GPIO_InitTypeDef*) &GPIO_InitStruct);
   }
#endif
}

void bsp_initialize(void)
{
   chp_initialize();

   NVIC_SetPriority(PendSV_IRQn, 0xFF); // Set PendSV to lowest possible priority

   initializeMainClock();

   initializeLEDs();

   initializeButtons();

   initializeUART();

   initializeI2C();

   initializeSPI();

   initializeChipSelects();
}

void bsp_turnOnLED(uint32_t ledId)
{
   if (LED_COUNT > ledId)
   {
      HAL_GPIO_WritePin(GPIOD, (GPIO_PIN_12 << ledId), GPIO_PIN_SET);
   }
}

void bsp_turnOffLED(uint32_t ledId)
{
   if (LED_COUNT > ledId)
   {
      HAL_GPIO_WritePin(GPIOD, (GPIO_PIN_12 << ledId), GPIO_PIN_RESET);
   }
}

void bsp_toggleLED(uint32_t ledId)
{
   if (LED_COUNT > ledId)
   {
      HAL_GPIO_TogglePin(GPIOD, (GPIO_PIN_12 << ledId));
   }
}

void bsp_startMainClock(void)
{
   /*
    * Core clock runs at 168 MHz
    * Timer clock runs at 84 Mhz
    * Maximum prescaler is 16 bits
    * 42000 will give us two ticks every millisecond
    */
   chp_initializeSystemTimer(42 * 1000 - 1);
}

void bsp_initializeOutputPin(uint32_t outputPin)
{
   bsp_setOutputPin(outputPin, true);
}

void bsp_setOutputPin(uint32_t outputPin, bool high)
{
   GPIO_TypeDef* gpio = (GPIO_TypeDef*) (outputPin & 0xffffff00);
   uint32_t pin = outputPin & 0xff;
   //HAL_GPIO_WritePin(gpio, (GPIO_PIN_0 << pin), high);
   gpio->BSRR = (1 << (pin + 16 * (! high)));
}

bool bsp_getInputState(uint32_t inputPin)
{
   GPIO_TypeDef* gpio = (GPIO_TypeDef*) (inputPin & 0xffffff00);
   uint32_t pin = inputPin & 0xff;
   //HAL_GPIO_ReadPin(gpio, (GPIO_PIN_0 << pin));
   return (0 != (gpio->IDR & (1 << pin)));
}

