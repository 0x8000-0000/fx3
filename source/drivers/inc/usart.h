/**
 * @file usart.h
 * @brief USART driver interface
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

#ifndef __USART_H__
#define __USART_H__

#include <stdint.h>

#include "status.h"
#include "power.h"

struct USARTHandle;

enum USARTFlowControl
{
   USART_FLOW_CONTROL_NONE,
   USART_FLOW_CONTROL_XON_XOFF,
   USART_FLOW_CONTROL_HARDWARE,
};

enum USARTParity
{
   USART_PARITY_NONE,
   USART_PARITY_ODD,
   USART_PARITY_EVEN,
};

struct USARTConfiguration
{
   uint32_t                               baudRate;
   uint8_t /* enum USARTFlowControl */    flowControl;
   uint8_t                                bits;
   uint8_t /* enum USARTParity */         parity;
   uint8_t                                stopBits;
};

enum Status usart_initialize(struct USARTHandle* handle, const struct USARTConfiguration* config);

enum Status usart_uninitialize(struct USARTHandle* handle);

enum Status usart_selectPowerMode(struct USARTHandle* handle, enum PowerMode newMode);


enum Status usart_flushInput(struct USARTHandle* handle);

enum Status usart_waitForReadable(struct USARTHandle* handle, uint32_t* bytesAvailable);

enum Status usart_read(struct USARTHandle* handle, uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesRead);


enum Status usart_flushOutput(struct USARTHandle* handle);

enum Status usart_waitForWritable(struct USARTHandle* handle, uint32_t* bytesAvailable);

enum Status usart_write(struct USARTHandle* handle, const uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesWritten);

enum Status usart_waitForWriteComplete(struct USARTHandle* handle);

#endif // __USART_H__

