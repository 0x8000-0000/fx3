/**
 * @file blinky.c
 * @brief "Dynamic" blinky application
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

#include <bitops.h>

struct led_toggler
{
   uint32_t initialDelay_ms;
   uint32_t period_ms;
};

static volatile uint32_t availableLEDs;

#define SLICE_MS    500

static const struct led_toggler greenToggler =
{
   .initialDelay_ms = 0,
   .period_ms       = SLICE_MS,
};

static const struct led_toggler orangeToggler =
{
   .initialDelay_ms = SLICE_MS / 4,
   .period_ms       = SLICE_MS,
};

static const struct led_toggler redToggler =
{
   .initialDelay_ms = SLICE_MS / 2,
   .period_ms       = SLICE_MS,
};

static const struct led_toggler blueToggler =
{
   .initialDelay_ms = 3 * SLICE_MS / 4,
   .period_ms       = SLICE_MS,
};

static void toggleLed(const void* arg)
{
   struct led_toggler* tog = (struct led_toggler*) arg;

   task_sleep_ms(tog->initialDelay_ms);

   while (true)
   {
      uint32_t ledId = bit_alloc(&availableLEDs);

      bsp_turnOnLED(ledId);

      task_sleep_ms(tog->period_ms);

      bsp_turnOffLED(ledId);

      bit_free(&availableLEDs, ledId);

      task_sleep_ms(tog->period_ms);
   }
}

static uint8_t blinkGreenLedStack[256] __attribute__ ((aligned (16)));
static uint8_t blinkRedLedStack[256] __attribute__ ((aligned (16)));
static uint8_t blinkOrangeLedStack[256] __attribute__ ((aligned (16)));
static uint8_t blinkBlueLedStack[256] __attribute__ ((aligned (16)));

static const struct task_config redLedTogglerTaskConfig =
{
   .name            = "Red",
   .handler         = toggleLed,
   .argument        = &redToggler,
   .priority        = 3,
   .stackBase       = blinkRedLedStack,
   .stackSize       = sizeof(blinkRedLedStack),
   .timeSlice_ticks = 0,
};

static const struct task_config greenLedTogglerTaskConfig =
{
   .name            = "Green",
   .handler         = toggleLed,
   .argument        = &greenToggler,
   .priority        = 4,
   .stackBase       = blinkGreenLedStack,
   .stackSize       = sizeof(blinkGreenLedStack),
   .timeSlice_ticks = 0,
};

static const struct task_config orangeLedTogglerTaskConfig =
{
   .name            = "Orange",
   .handler         = toggleLed,
   .argument        = &orangeToggler,
   .priority        = 5,
   .stackBase       = blinkOrangeLedStack,
   .stackSize       = sizeof(blinkOrangeLedStack),
   .timeSlice_ticks = 20,
};

static const struct task_config blueLedTogglerTaskConfig =
{
   .name            = "Blue",
   .handler         = toggleLed,
   .argument        = &blueToggler,
   .priority        = 5,
   .stackBase       = blinkBlueLedStack,
   .stackSize       = sizeof(blinkBlueLedStack),
   .timeSlice_ticks = 20,
};

static struct task_control_block greenTogglerTCB;
static struct task_control_block redTogglerTCB;
static struct task_control_block orangeTogglerTCB;
static struct task_control_block blueTogglerTCB;

int main(void)
{
   bsp_initialize();

   fx3_initialize();

   bit_initialize(&availableLEDs, LED_COUNT);

   fx3_createTask(&greenTogglerTCB,  &greenLedTogglerTaskConfig);
   fx3_createTask(&orangeTogglerTCB, &orangeLedTogglerTaskConfig);
   if (LED_COUNT > 2)
   {
      fx3_createTask(&redTogglerTCB,    &redLedTogglerTaskConfig);
      fx3_createTask(&blueTogglerTCB,   &blueLedTogglerTaskConfig);
   }

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}
