/**
 * @file test_EN25F80.c
 * @brief Test EN25F80 driver
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
#include <task.h>

#include <usart.h>
#include <spi.h>

#include <EN25F80.h>

static const struct USARTConfiguration usartConfig =
{
   .baudRate    = 115200,
   .flowControl = USART_FLOW_CONTROL_NONE,
   .bits        = 8,
   .parity      = USART_PARITY_NONE,
   .stopBits    = 1,
};

static const struct SPIConfiguration spiConfig =
{
   .speed = 400 * 1000,        // 400 kHz
};

static const uint8_t APP_BANNER[] = "Test EN25F80 application\r\n";

static volatile enum Status status;
static volatile uint32_t chipId;
static volatile uint8_t electronicSignature[4];

static void runTest(const void* arg)
{
   struct USARTHandle* usart = (struct USARTHandle*) arg;

   uint32_t bytesWritten = 0;
   status = usart_write(usart, APP_BANNER, sizeof(APP_BANNER) - 1, &bytesWritten);
   assert(STATUS_OK == status);
   assert((sizeof(APP_BANNER) - 1) == bytesWritten);

   status = EN25F80_initialize();

   while (true)
   {
      status = EN25F80_getChipId((uint32_t*) &chipId);
      
      if (STATUS_OK == status)
      {
         bsp_toggleLED(LED_ID_GREEN);
      }
      else
      {
         bsp_toggleLED(LED_ID_RED);
      }

      fx3_suspendTask(1000);
   }
}

static uint8_t testStack[384] __attribute__ ((aligned (16)));

extern struct USARTHandle CONSOLE_USART;

extern struct SPIBus EN25F80_BUS;

static const struct task_config testConfig =
{
   .name            = "Test Handler",
   .handler         = runTest,
   .argument        = &CONSOLE_USART,
   .priority        = 4,
   .stackBase       = testStack,
   .stackSize       = sizeof(testStack),
   .timeSlice_ticks = 0,
};

static struct task_control_block testTCB;

int main(void)
{
   bsp_initialize();
   usart_initialize(&CONSOLE_USART, &usartConfig);
   spi_initialize(&EN25F80_BUS, &spiConfig);

   fx3_initialize();

   fx3_createTask(&testTCB, &testConfig);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}

