/**
 * @file test_output_pin.c
 * @brief Test output pin setting functionality
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

static const struct USARTConfiguration usartConfig =
{
   .baudRate    = 115200,
   .flowControl = USART_FLOW_CONTROL_NONE,
   .bits        = 8,
   .parity      = USART_PARITY_NONE,
   .stopBits    = 1,
};

static const uint8_t APP_BANNER[] = "Test Output Pin application\r\n";

static const uint8_t TURN_OFF_LED[] = "Turn Blue LED Off\r\n";

static const uint8_t TURN_ON_LED[] = "Turn Blue LED On\r\n";

#define BLUE_LED_PIN  PIN('D', 15)

static void runTest(const void* arg)
{
   struct USARTHandle* usart = (struct USARTHandle*) arg;

   uint32_t bytesWritten = 0;
   enum Status status = usart_write(usart, APP_BANNER, sizeof(APP_BANNER) - 1, &bytesWritten);
   assert(STATUS_OK == status);
   assert((sizeof(APP_BANNER) - 1) == bytesWritten);

   while (true)
   {
      {
         status = usart_write(usart, TURN_ON_LED, sizeof(TURN_ON_LED) - 1, &bytesWritten);
         assert(STATUS_OK == status);
         assert((sizeof(TURN_ON_LED) - 1) == bytesWritten);

         bsp_setOutputPin(BLUE_LED_PIN, true);
         fx3_suspendTask(5000);
      }

      {
         status = usart_write(usart, TURN_OFF_LED, sizeof(TURN_OFF_LED) - 1, &bytesWritten);
         assert(STATUS_OK == status);
         assert((sizeof(TURN_OFF_LED) - 1) == bytesWritten);

         bsp_setOutputPin(BLUE_LED_PIN, false);
         fx3_suspendTask(5000);
      }
   }
}

static uint8_t testStack[2048] __attribute__ ((aligned (16)));

extern struct USARTHandle CONSOLE_USART;

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

   fx3_initialize();

   fx3_createTask(&testTCB, &testConfig);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}

