/**
 * @file chargen.c
 * @brief Test UART character generation
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

static const char HELLO_WORLD[] = "Hello, World! ";

#ifndef CONSTANT_STRING
static char messageBuffer[128];
#endif

static void emitBytes(const void* arg)
{
   struct USARTHandle* usart = (struct USARTHandle*) arg;

   uint32_t counter = 0;

   while (true)
   {
      uint32_t bytesWritten = 0;

#ifdef CONSTANT_STRING
      usart_write(usart, (uint8_t*) HELLO_WORLD, (uint32_t) (sizeof(HELLO_WORLD) - 1), &bytesWritten);
      assert((sizeof(HELLO_WORLD) - 1) == bytesWritten);
#else
      int length = sizeof(HELLO_WORLD) - 1;
      memcpy(messageBuffer, HELLO_WORLD, length);
      uint32_t val = counter;
      while (val)
      {
         messageBuffer[length] = '0' + (val % 10);
         val /= 10;
         length ++;
      }
      messageBuffer[length] = ' ';
      length ++;
      usart_write(usart, (uint8_t*) messageBuffer, (uint32_t) length, &bytesWritten);
      assert((uint32_t) length == bytesWritten);
#endif

      counter ++;

      fx3_suspendTask(100);
   }
}

static uint8_t byteEmitterStack[256] __attribute__ ((aligned (16)));

extern struct USARTHandle usart1;
extern struct USARTHandle usart2;

static const struct task_config byteEmitterTaskConfig =
{
   .name            = "Emit Bytes",
   .handler         = emitBytes,
   .argument        = &usart1,
   .priority        = 4,
   .stackBase       = byteEmitterStack,
   .stackSize       = sizeof(byteEmitterStack),
   .timeSlice_ticks = 0,
};

static struct task_control_block byteEmitterTCB;

int main(void)
{
   bsp_initialize();
   usart_initialize(&usart1, &usartConfig);
   usart_initialize(&usart2, &usartConfig);

   fx3_initialize();

   fx3_createTask(&byteEmitterTCB,  &byteEmitterTaskConfig);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}

