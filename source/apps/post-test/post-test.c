/**
 * @file post-test.c
 * @brief Test the task message queue
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

#include <buffer.h>
#include <board.h>
#include <task.h>

struct led_toggler
{
   uint32_t ledId;
   uint32_t initialDelay_ms;
   uint32_t sleepBeforeWork_ms;
   uint32_t messageOn_count;
   uint32_t messageOff_count;
};

#define SLICE_MS    500

static const struct led_toggler toggler[] =
{
   {
      .ledId              = 0,
      .initialDelay_ms    = 2,
      .sleepBeforeWork_ms = 25,
      .messageOn_count    = 4,
      .messageOff_count   = 4,
   },
   {
      .ledId              = 1,
      .initialDelay_ms    = SLICE_MS / 4,
      .sleepBeforeWork_ms = 25,
      .messageOn_count    = 5,
      .messageOff_count   = 3,
   },
   {
      .ledId              = 2,
      .initialDelay_ms    = SLICE_MS / 2,
      .sleepBeforeWork_ms = 25,
      .messageOn_count    = 6,
      .messageOff_count   = 6,
   },
   {
      .ledId              = 3,
      .initialDelay_ms    = 3 * SLICE_MS / 4,
      .sleepBeforeWork_ms = 25,
      .messageOn_count    = 7,
      .messageOff_count   = 5,
   },
};

static void toggleLed(const void* arg)
{
   struct led_toggler* tog = (struct led_toggler*) arg;

   fx3_suspendTask(tog->initialDelay_ms);

   while (true)
   {
      fx3_suspendTask(tog->sleepBeforeWork_ms);

      for (uint32_t ii = 0; ii < tog->messageOn_count; ii ++)
      {
         struct buffer* buf = (struct buffer*) fx3_waitForMessage();
         buf_free(buf);
      }

      bsp_turnOnLED(tog->ledId);

      fx3_suspendTask(tog->sleepBeforeWork_ms);

      for (uint32_t ii = 0; ii < tog->messageOff_count; ii ++)
      {
         struct buffer* buf = (struct buffer*) fx3_waitForMessage();
         buf_free(buf);
      }

      bsp_turnOffLED(tog->ledId);
   }
}

#define TOGGLER_TASK_STACK_SIZE 256

static uint8_t togglerLedStack[TOGGLER_TASK_STACK_SIZE * 4] __attribute__ ((aligned (16)));

static const struct task_config ledTogglerTaskConfig =
{
   .name            = "Blinker",
   .handler         = toggleLed,
   .argument        = toggler,
   .priority        = 5,
   .stackBase       = togglerLedStack,
   .stackSize       = TOGGLER_TASK_STACK_SIZE,
   .timeSlice_ticks = 20,
};

static struct task_control_block togglerTCB[4];

/*
 * messages producers
 */

static struct messager
{
   uint32_t initialDelay_ms;
   uint32_t train_count;
   uint32_t messageInterval_ms;
   uint32_t period_ms;
}  messagers[] =
{
   {
      .initialDelay_ms    = 100,
      .train_count        = 5,
      .messageInterval_ms = 5,
      .period_ms          = 100,
   },
   {
      .initialDelay_ms    = 200,
      .train_count        = 5,
      .messageInterval_ms = 5,
      .period_ms          = 200,
   },
   {
      .initialDelay_ms    = 200,
      .train_count        = 10,
      .messageInterval_ms = 10,
      .period_ms          = 200,
   },
   {
      .initialDelay_ms    = 500,
      .train_count        = 10,
      .messageInterval_ms = 5,
      .period_ms          = 500,
   },
};

static void sendMessages(const void* arg)
{
   struct messager* msg = (struct messager*) arg;

   fx3_suspendTask(msg->initialDelay_ms);

   uint32_t nextValue = 0;

   while (true)
   {
      for (uint32_t ii = 0; ii < msg->train_count; ii ++)
      {
         nextValue += 7;

         struct buffer* buf = buf_alloc(8);

         fx3_sendMessage(&togglerTCB[nextValue % 4], &buf->element);

         fx3_suspendTask(msg->messageInterval_ms);
      }

      fx3_suspendTask(msg->period_ms);
   }
}

#define MESSAGER_TASK_STACK_SIZE 128

static uint8_t messagerStack[MESSAGER_TASK_STACK_SIZE * 4] __attribute__ ((aligned (16)));

static const struct task_config messagerTaskConfig =
{
   .name            = "Message",
   .handler         = sendMessages,
   .argument        = messagers,
   .priority        = 4,
   .stackBase       = messagerStack,
   .stackSize       = MESSAGER_TASK_STACK_SIZE,
   .timeSlice_ticks = 20,
};

static struct task_control_block messagerTCB[4];

int main(void)
{
   bsp_initialize();

   buf_initialize();

   fx3_initialize();

   fx3_createTaskPool(togglerTCB, &ledTogglerTaskConfig, sizeof(struct led_toggler), 4);

   fx3_createTaskPool(messagerTCB, &messagerTaskConfig, sizeof(struct messager), 4);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}
