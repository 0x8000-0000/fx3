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

#ifndef MAX_DEBOUNCE_INPUT_COUNT
#define MAX_DEBOUNCE_INPUT_COUNT 16
#endif

#ifndef MAX_QUADRATURE_ENCODER_COUNT
#define MAX_QUADRATURE_ENCODER_COUNT 4
#endif

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

enum special_input_id
{
   ID_TIMEOUT,
   ID_SWITCH_INTERRUPT,
};

_Static_assert(8 == sizeof(struct input_event), "struct size");

struct input_pin_config
{
   uint32_t pinAddress;
   uint8_t  switchId;
   uint8_t  integrator;          // could combine into a single byte
   uint8_t  outputValue;
   uint8_t  lastOutputValue;
};

_Static_assert(8 == sizeof(struct input_pin_config), "struct size");

struct quadrature_encoder_input
{
   uint32_t pinAddress_A;
   uint32_t pinAddress_B;
   uint32_t delta;

   unsigned int encoderId   : 8;

   unsigned int integrator_A: 4;
   unsigned int integrator_B: 4;

   unsigned int currentValue: 2;
   unsigned int lastValue   : 2;

   unsigned int reserved    : 12;

   int32_t position;
   int32_t lastNotifiedAtPosition;
};

_Static_assert(24 == sizeof(struct quadrature_encoder_input), "struct size");

static struct debounce_input
{
   uint32_t count;
   struct input_pin_config pin[MAX_DEBOUNCE_INPUT_COUNT];
}  debounceInput;

static struct quadrature_encoder
{
   uint32_t count;
   struct quadrature_encoder_input encoder[MAX_QUADRATURE_ENCODER_COUNT];
}  quadratureEncoder;

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

   for (uint32_t ii = 0; ii < debounceInput.count; ii ++)
   {
      struct input_pin_config* ip = &debounceInput.pin[ii];

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
         event->inputId            = ip->switchId;
         event->isHigh             = ip->outputValue;
         inp_onSwitchStateChange(event);

         stateChange = true;
      }
   }

   return stateChange;
}

static const int8_t INCREMENT[4][4] =
{
   // 00  01  10  11
   {   0, -1, +1,  9 },    // 00
   {  +1,  0,  9, -1 },    // 01
   {  -1,  9,  0, +1 },    // 10
   {   9, +1, -1,  0 },    // 11
};

static bool pollEncoderSignals(void)
{
   bool stateChange = false;

   for (uint32_t ii = 0; ii < quadratureEncoder.count; ii ++)
   {
      struct quadrature_encoder_input* qei = &quadratureEncoder.encoder[ii];

      qei->lastValue = qei->currentValue;

      /*
       * pin A
       */
      if (bsp_getInputState(qei->pinAddress_A))
      {
         if (DEBOUNCE_INTEGRATOR_MAX > qei->integrator_A)
         {
            qei->integrator_A ++;
         }
      }
      else
      {
         if (qei->integrator_A)
         {
            qei->integrator_A --;
         }
      }

      if (0 == qei->integrator_A)
      {
         qei->currentValue &= 1;       // reset 2nd bit
      }
      else
      {
         if (DEBOUNCE_INTEGRATOR_MAX == qei->integrator_A)
         {
            qei->currentValue |= 2;    // set 2nd bit
         }
      }

      /*
       * pin B
       */
      if (bsp_getInputState(qei->pinAddress_B))
      {
         if (DEBOUNCE_INTEGRATOR_MAX > qei->integrator_B)
         {
            qei->integrator_B ++;
         }
      }
      else
      {
         if (qei->integrator_B)
         {
            qei->integrator_B --;
         }
      }

      if (0 == qei->integrator_B)
      {
         qei->currentValue &= 2;       // reset 1st bit
      }
      else
      {
         if (DEBOUNCE_INTEGRATOR_MAX == qei->integrator_B)
         {
            qei->currentValue |= 1;     // set 1st bit
         }
      }

      if (qei->currentValue != qei->lastValue)
      {
         stateChange = true;

         int8_t increment = INCREMENT[qei->lastValue][qei->currentValue];
         assert(increment);

         if (9 != increment)
         {
            qei->position += increment;
         }
         else
         {
            assert(false);
         }

         int32_t delta = qei->position - qei->lastNotifiedAtPosition;
         if ((0 < delta) && (qei->delta <= delta))
         {
            struct input_event* event = allocateEvent();
            event->inputId            = qei->encoderId;
            event->isHigh             = true;
            event->position           = qei->position;
            inp_onEncoderUp(event);
            qei->lastNotifiedAtPosition = qei->position;
         }

         if ((0 > delta) && (qei->delta <= -delta))
         {
            struct input_event* event = allocateEvent();
            event->inputId            = qei->encoderId;
            event->isHigh             = false;
            event->position           = qei->position;
            inp_onEncoderDown(event);
            qei->lastNotifiedAtPosition = qei->position;
         }
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

      if (ID_TIMEOUT == event->inputId)
      {
         debouncePeriods --;

         if (pollInputSignals())
         {
            debouncePeriods = DEBOUNCE_SAMPLE_COUNT;
         }

         if (pollEncoderSignals())
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
   memset(&debounceInput, 0, sizeof(debounceInput));
   memset(&quadratureEncoder, 0, sizeof(quadratureEncoder));

   bit_initialize(&eventBitmap, MAX_EVENT_COUNT);

   fx3_createTask(&inputDebouncerTCB, &inputDebouncerConfig);

   bsp_enableInputStateNotifications();
}

void inp_monitorSwitch(uint8_t switchId, uint32_t inputPin)
{
   if (debounceInput.count < MAX_DEBOUNCE_INPUT_COUNT)
   {
      struct input_pin_config* ip = &debounceInput.pin[debounceInput.count];
      debounceInput.count ++;

      ip->pinAddress      = inputPin;
      ip->switchId        = switchId;
      ip->integrator      = 0;
      ip->outputValue     = bsp_getInputState(inputPin);
      ip->lastOutputValue = ip->outputValue;
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
   event->inputId            = ID_SWITCH_INTERRUPT;
   fx3_sendMessage(&inputDebouncerTCB, &event->element);
}

bool bsp_onDebounceIntervalTimeout(void)
{
   // send message to task
   struct input_event* event = allocateEvent();
   event->inputId            = ID_TIMEOUT;
   fx3_sendMessage(&inputDebouncerTCB, &event->element);

   return true;
}

void inp_monitorEncoder(uint8_t encoderId, uint32_t inputPinA, uint32_t inputPinB, uint32_t delta)
{
   if (quadratureEncoder.count < MAX_QUADRATURE_ENCODER_COUNT)
   {
      unsigned int inputA = bsp_getInputState(inputPinA);
      unsigned int inputB = bsp_getInputState(inputPinB);

      struct quadrature_encoder_input* qei = &quadratureEncoder.encoder[quadratureEncoder.count];
      quadratureEncoder.count ++;

      qei->pinAddress_A           = inputPinA;
      qei->pinAddress_B           = inputPinB;
      qei->delta                  = delta;
      qei->encoderId              = encoderId;

      qei->integrator_A           = 0;
      qei->integrator_B           = 0;

      qei->currentValue           = (inputA << 1) | inputB;
      qei->lastValue              = qei->currentValue;

      qei->position               = 0;
      qei->lastNotifiedAtPosition = 0;
   }
   else
   {
      assert(false);
   }
}

int32_t inp_getEncoderPosition(uint8_t encoderId)
{
   int32_t result = 0;
   bool found = false;

   for (uint32_t ii = 0; ii < quadratureEncoder.count; ii ++)
   {
      struct quadrature_encoder_input* qei = &quadratureEncoder.encoder[ii];

      if (qei->encoderId == encoderId)
      {
         result = qei->position;
         found = true;
         break;
      }
   }

   assert(found);

   return result;
}

void inp_resetEncoderPosition(uint8_t encoderId)
{
   bool found = false;

   for (uint32_t ii = 0; ii < quadratureEncoder.count; ii ++)
   {
      struct quadrature_encoder_input* qei = &quadratureEncoder.encoder[ii];

      if (qei->encoderId == encoderId)
      {
         qei->position               = 0;
         qei->lastNotifiedAtPosition = 0;
         found = true;
         break;
      }
   }

   assert(found);
}

void __attribute__((weak)) inp_onSwitchStateChange(struct input_event* event)
{
   inp_recycleEvent(event);
}

void __attribute__((weak)) inp_onEncoderUp(struct input_event* event)
{
   inp_recycleEvent(event);
}

void __attribute__((weak)) inp_onEncoderDown(struct input_event* event)
{
   inp_recycleEvent(event);
}

