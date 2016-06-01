/**
 * @file blinky.c
 * @brief Example blinky application
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
#include <task.h>

struct led_toggler
{
   uint32_t ledId;
   uint32_t initialDelay_ms;
   uint32_t period_ms;
};

#define SLICE_MS    500

static const struct led_toggler toggler[] =
{
   {
      .ledId           = 0,
      .initialDelay_ms = 0,
      .period_ms       = SLICE_MS,
   },
   {
      .ledId           = 1,
      .initialDelay_ms = SLICE_MS / 4,
      .period_ms       = SLICE_MS,
   },
   {
      .ledId           = 2,
      .initialDelay_ms = SLICE_MS / 2,
      .period_ms       = SLICE_MS,
   },
   {
      .ledId           = 3,
      .initialDelay_ms = 3 * SLICE_MS / 4,
      .period_ms       = SLICE_MS,
   },
};

static void toggleLed(const void* arg)
{
   struct led_toggler* tog = (struct led_toggler*) arg;

   fx3_suspendTask(tog->initialDelay_ms);

   while (true)
   {
      bsp_turnOnLED(tog->ledId);

      fx3_suspendTask(tog->period_ms);

      bsp_turnOffLED(tog->ledId);

      fx3_suspendTask(tog->period_ms);
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

int main(void)
{
   bsp_initialize();

   fx3_initialize();

   fx3_createTaskPool(togglerTCB, &ledTogglerTaskConfig, sizeof(struct led_toggler), 4);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}
