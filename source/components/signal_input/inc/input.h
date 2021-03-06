/**
 * @file input.h
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

#ifndef __INPUT_H__
#define __INPUT_H__

#ifdef __cplusplus
#include <cstdint>
extern "C"
{
#else
#include <stdint.h>
#endif

#include <list_utils.h>

struct input_event
{
   union
   {
      struct input_event*    next;          // used on a waiting list
      struct list_element    element;
   };

   unsigned int  inputId  : 8;

   unsigned int  isHigh   : 1;

   // private
   unsigned int  debounceIntervalExpired: 1;

   int           position: 22;
};

void inp_initialize(void);

/** @addtogroup debouncing Input Debouncing
 * @{
 */

void inp_monitorSwitch(uint8_t switchId, uint32_t inputPin);

void inp_onSwitchStateChange(struct input_event* event);

void inp_recycleEvent(struct input_event* event);

/** @} */

/** @addtogroup encoders Encoder Handling
 * @{
 */

void inp_monitorEncoder(uint8_t encoderId, uint32_t inputPinA, uint32_t inputPinB, uint32_t delta);

void inp_onEncoderUp(struct input_event* event);

void inp_onEncoderDown(struct input_event* event);

int32_t inp_getEncoderPosition(uint8_t encoderId);

void inp_resetEncoderPosition(uint8_t encoderId);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // __INPUT_H__

