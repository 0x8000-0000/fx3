/**
 * @file chargen.c
 * @brief Test UART input and output
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

static const char WELCOME_MESSAGE[] = "\r\n\nHello, please type something\r\n";


#define INPUT_BUFFER_SIZE  64

static uint8_t inputBuffer[INPUT_BUFFER_SIZE];

static uint32_t bytesAvailable;
static uint32_t bytesRead;
static uint32_t bytesWritten;

static void emitBytes(const void* arg)
{
   struct USARTHandle* usart = (struct USARTHandle*) arg;

   usart_write(usart, (uint8_t*) WELCOME_MESSAGE, (uint32_t) (sizeof(WELCOME_MESSAGE) - 1), &bytesWritten);
   assert((sizeof(WELCOME_MESSAGE) - 1) == bytesWritten);

   while (true)
   {
      bytesAvailable = 0;
      bytesRead      = 0;
      bytesWritten   = 0;

      enum Status status = usart_waitForReadable(usart, &bytesAvailable);
      if (STATUS_OK == status)
      {
         status = usart_read(usart, inputBuffer, sizeof(inputBuffer), &bytesRead);
         if (STATUS_OK == status)
         {
            usart_write(usart, inputBuffer, bytesRead, &bytesWritten);
            assert(bytesRead == bytesWritten);
         }
      }
   }
}

static uint8_t echoStack[256] __attribute__ ((aligned (16)));

extern struct USARTHandle usart1;
extern struct USARTHandle usart2;

static const struct task_config echoTaskConfig =
{
   .name            = "Emit Bytes",
   .handler         = emitBytes,
   .argument        = &CONSOLE_USART,
   .priority        = 4,
   .stackBase       = echoStack,
   .stackSize       = sizeof(echoStack),
   .timeSlice_ticks = 0,
};

static struct task_control_block echoTCB;

int main(void)
{
   bsp_initialize();
   usart_initialize(&CONSOLE_USART, &usartConfig);

   fx3_initialize();

   fx3_createTask(&echoTCB,  &echoTaskConfig);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}

