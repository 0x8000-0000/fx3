/**
 * @file input_debouncer.c
 * @brief Input component: debouncing and encoders
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

/*
 * Based on 'debounce.c' by by Kenneth A. Kuhn
 *    http://www.kennethkuhn.com/electronics/debounce.c
 */

#include <assert.h>
#include <string.h>

#include <board.h>

#include <bitops.h>
#include <task.h>

#include <input.h>

#ifndef MAX_EVENT_COUNT
#define MAX_EVENT_COUNT 16
#endif

#ifndef DEBOUNCE_INTERVAL_MS
#define DEBOUNCE_INTERVAL_MS 2
#endif

#ifndef DEBOUNCE_SAMPLE_COUNT
#define DEBOUNCE_SAMPLE_COUNT 10
#endif

#ifndef DEBOUNCE_INTEGRATOR_MAX
#define DEBOUNCE_INTEGRATOR_MAX 8
#endif

#ifndef MAX_DEBOUNCE_INPUT_COUNT
#define MAX_DEBOUNCE_INPUT_COUNT 16
#endif

struct input_pin
{
   uint32_t pinAddress;
   uint8_t  integrator;
   uint8_t  outputValue;
   uint8_t  lastOutputValue;
   uint8_t  reserved;
};

static struct input_pin debounceInputPins[MAX_DEBOUNCE_INPUT_COUNT];
static uint32_t debounceInputPins_count;

static volatile uint32_t eventBitmap;
static struct input_event eventPool[MAX_EVENT_COUNT];

static struct input_event* allocateEvent(void)
{
   struct input_event* event = NULL;
   uint32_t eventIndex = bit_alloc(&eventBitmap);
   if (32 > eventIndex)
   {
      event = &eventPool[eventIndex];
   }
   else
   {
      assert(false);
   }

   return event;
}

void inp_recycleEvent(struct input_event* event)
{
   assert(((uintptr_t) eventPool) <= ((uintptr_t) event));
   assert(((uintptr_t) &eventPool[MAX_EVENT_COUNT]) > ((uintptr_t) event));
   uint32_t byteOffset = ((uintptr_t) event) - ((uintptr_t) eventPool);
   assert(0 == byteOffset % sizeof(struct input_event));

   uint32_t bufferIndex = byteOffset / sizeof(struct input_event);
   bit_free(&eventBitmap, (uint32_t) bufferIndex);
}

static bool pollInputSignals(void)
{
   bool stateChange = false;

   for (uint32_t ii = 0; ii < debounceInputPins_count; ii ++)
   {
      struct input_pin* ip = &debounceInputPins[ii];

      ip->lastOutputValue = ip->outputValue;

      if (bsp_getInputState(ip->pinAddress))
      {
         if (DEBOUNCE_INTEGRATOR_MAX > ip->integrator)
         {
            ip->integrator ++;
         }
      }
      else
      {
         if (ip->integrator)
         {
            ip->integrator --;
         }
      }

      if (0 == ip->integrator)
      {
         ip->outputValue = 0;
      }
      else
      {
         if (DEBOUNCE_INTEGRATOR_MAX == ip->integrator)
         {
            ip->outputValue = 1;
         }
      }

      if (ip->outputValue != ip->lastOutputValue)
      {
         struct input_event* event = allocateEvent();

         event->inputPin                = ip->pinAddress;
         event->isHigh                  = ip->outputValue;
         event->debounceIntervalExpired = false;

         inp_onSwitchStateChange(event);

         stateChange = true;
      }
   }

   return stateChange;
}

static uint32_t debouncePeriods;

static void debounceInputs(const void* arg __attribute__((unused)))
{
   debouncePeriods = 0;

   while (true)
   {
      struct input_event* event = (struct input_event*) fx3_waitForMessage();
      bool debounceIntervalExpired = event->debounceIntervalExpired;
      inp_recycleEvent(event);

      if (event->debounceIntervalExpired)
      {
         debouncePeriods --;

         if (pollInputSignals())
         {
            debouncePeriods = DEBOUNCE_SAMPLE_COUNT;
         }
      }
      else
      {
         debouncePeriods = DEBOUNCE_SAMPLE_COUNT;
      }

      if (debouncePeriods)
      {
         uint32_t debounceDeadline = 0;
         bsp_computeWakeUp_ticks(DEBOUNCE_INTERVAL_MS, &debounceDeadline);
         bsp_requestDebounceTimeout_ticks(debounceDeadline);
      }
      else
      {
         bsp_enableInputStateNotifications();
      }
   }
}

static uint8_t inputDebouncerStack[256] __attribute__ ((aligned (16)));

static const struct task_config inputDebouncerConfig =
{
   .name            = "Input Debouncer",
   .handler         = debounceInputs,
   .argument        = NULL,
   .priority        = 8,
   .stackBase       = inputDebouncerStack,
   .stackSize       = sizeof(inputDebouncerStack),
   .timeSlice_ticks = 0,
};

static struct task_control_block inputDebouncerTCB;

void inp_initialize(void)
{
   bit_initialize(&eventBitmap, MAX_EVENT_COUNT);

   fx3_createTask(&inputDebouncerTCB, &inputDebouncerConfig);

   bsp_enableInputStateNotifications();

   memset(debounceInputPins, 0, sizeof(debounceInputPins));
   debounceInputPins_count = 0;
}

void inp_monitorSwitch(uint32_t inputPin)
{
   if (debounceInputPins_count < MAX_DEBOUNCE_INPUT_COUNT)
   {
      debounceInputPins[debounceInputPins_count].pinAddress      = inputPin;
      debounceInputPins[debounceInputPins_count].integrator      = 0;
      debounceInputPins[debounceInputPins_count].outputValue     = 0;
      debounceInputPins[debounceInputPins_count].lastOutputValue = 0;
      debounceInputPins_count ++;
   }
   else
   {
      assert(false);
   }
}

void bsp_onInputStateChanged(uint32_t inputPin, bool newState)
{
   // switch to polling
   bsp_disableInputStateNotifications();

   // send message to task
   struct input_event* event = allocateEvent();

   event->inputPin                = inputPin;
   event->isHigh                  = newState;
   event->debounceIntervalExpired = false;

   fx3_sendMessage(&inputDebouncerTCB, &event->element);
}

bool bsp_onDebounceIntervalTimeout(void)
{
   struct input_event* event = allocateEvent();

   event->inputPin                = 0;
   event->isHigh                  = false;
   event->debounceIntervalExpired = true;

   fx3_sendMessage(&inputDebouncerTCB, &event->element);

   // send message to task
   return true;
}
