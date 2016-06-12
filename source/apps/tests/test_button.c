/**
 * @file test_button.c
 * @brief Test input driver (switch/button debouncing)
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
#include <stdio.h>

#include <board.h>
#include <task.h>
#include <usart.h>

#include <input.h>

extern struct USARTHandle CONSOLE_USART;

static const uint8_t APP_BANNER[] = "Test button debouncing\r\n";

static const char MESSAGE[] = "Push button is now";
static const char HIGH_TEXT[] = " down\r\n";
static const char LOW_TEXT[] = " up\r\n";

static uint8_t outBuffer[80];

static void testHandler(const void* arg __attribute__((unused)))
{
   uint32_t bytesWritten = 0;
   enum Status status = usart_write(&CONSOLE_USART, APP_BANNER, sizeof(APP_BANNER) - 1, &bytesWritten);
   assert(STATUS_OK == status);
   assert((sizeof(APP_BANNER) - 1) == bytesWritten);

   inp_monitorSwitch(PUSH_BUTTON0);

   memcpy(outBuffer, MESSAGE, sizeof(MESSAGE) - 1);

   while (true)
   {
      struct input_event* evt = (struct input_event*) fx3_waitForMessage();

      uint32_t inputPin = evt->inputPin;
      bool     isHigh   = evt->isHigh;

      inp_recycleEvent(evt);

      if (PUSH_BUTTON0 == inputPin)
      {
         uint32_t messageLen = 0;
         if (isHigh)
         {
            memcpy(outBuffer + sizeof(MESSAGE) - 1, HIGH_TEXT, sizeof(HIGH_TEXT) - 1);
            messageLen = sizeof(MESSAGE) - 1 + sizeof(HIGH_TEXT) - 1;
         }
         else
         {
            memcpy(outBuffer + sizeof(MESSAGE) - 1, LOW_TEXT, sizeof(LOW_TEXT) - 1);
            messageLen = sizeof(MESSAGE) - 1 + sizeof(LOW_TEXT) - 1;
         }
         status = usart_write(&CONSOLE_USART, outBuffer, messageLen, &bytesWritten);
         assert(STATUS_OK == status);
         assert(messageLen == bytesWritten);
      }
   }
}

static struct task_control_block testTCB;

void inp_onSwitchStateChange(struct input_event* event)
{
   fx3_sendMessage(&testTCB, &event->element);
}

static uint8_t testStack[2048] __attribute__ ((aligned (16)));

static const struct task_config testConfig =
{
   .name            = "Test Handler",
   .handler         = testHandler,
   .argument        = NULL,
   .priority        = 4,
   .stackBase       = testStack,
   .stackSize       = sizeof(testStack),
   .timeSlice_ticks = 0,
};

static const struct USARTConfiguration usartConfig =
{
   .baudRate    = 115200,
   .flowControl = USART_FLOW_CONTROL_NONE,
   .bits        = 8,
   .parity      = USART_PARITY_NONE,
   .stopBits    = 1,
};

extern void utl_startHeartbeat(void);

int main(void)
{
   bsp_initialize();
   usart_initialize(&CONSOLE_USART, &usartConfig);

   fx3_initialize();

   utl_startHeartbeat();
   inp_initialize();

   fx3_createTask(&testTCB, &testConfig);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}

