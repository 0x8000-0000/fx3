/**
 * @file test_encoder.c
 * @brief Test input driver (encoder)
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

static const uint8_t APP_BANNER[] = "\r\n\r\nTest quadrature encoder\r\n";

static const char MESSAGE[] = "Push button is now";
static const char HIGH_TEXT[] = " down\r\n";
static const char LOW_TEXT[] = " up\r\n";

static uint8_t outBuffer[80];

enum inputs
{
   INVALID,
   PA0_SWTICH,
   LEFT_ENCODER,
   RIGHT_ENCODER,
};

static void testHandler(const void* arg __attribute__((unused)))
{
   uint32_t bytesWritten = 0;
   enum Status status = usart_write(&CONSOLE_USART, APP_BANNER, sizeof(APP_BANNER) - 1, &bytesWritten);
   assert(STATUS_OK == status);
   assert((sizeof(APP_BANNER) - 1) == bytesWritten);

   inp_monitorSwitch(PA0_SWTICH, PUSH_BUTTON0);

   inp_monitorEncoder(RIGHT_ENCODER, RIGHT_ENCODER_A, RIGHT_ENCODER_B, 1);

   while (true)
   {
      struct input_event* evt = (struct input_event*) fx3_waitForMessage();
      uint8_t inputId         = evt->inputId;
      bool isHigh             = evt->isHigh;
      int position            = evt->position;
      inp_recycleEvent(evt);

      if (PA0_SWTICH == inputId)
      {
         position = 0;
         inp_resetEncoderPosition(RIGHT_ENCODER);
         int len = snprintf((char*) outBuffer, sizeof(outBuffer), "\rRight encoder at %d        ", position);
         status = usart_write(&CONSOLE_USART, outBuffer, len, &bytesWritten);
         assert(STATUS_OK == status);
         assert(len == bytesWritten);
      }

      if (RIGHT_ENCODER == inputId)
      {
         int len = snprintf((char*) outBuffer, sizeof(outBuffer), "\rRight encoder at %d ", position);
         status = usart_write(&CONSOLE_USART, outBuffer, len, &bytesWritten);
         assert(STATUS_OK == status);
         assert(len == bytesWritten);
      }
   }
}

static struct task_control_block testTCB;

void inp_onSwitchStateChange(struct input_event* event)
{
   fx3_sendMessage(&testTCB, &event->element);
}

void inp_onEncoderUp(struct input_event* event)
{
   fx3_sendMessage(&testTCB, &event->element);
}

void inp_onEncoderDown(struct input_event* event)
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

