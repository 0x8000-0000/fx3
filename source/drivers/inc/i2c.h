/**
 * @file i2c.h
 * @brief I2C driver interface
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

#ifndef __I2C_H__
#define __I2C_H__

#include <stdint.h>

#include <status.h>

struct I2CHandle;

struct I2CConfiguration
{
   uint32_t    speed;
};

void i2c_initialize(struct I2CHandle* handle, const struct I2CConfiguration* config);

void i2c_acquireBus(struct I2CHandle* handle);

void i2c_releaseBus(struct I2CHandle* handle);

enum Status i2c_readRegisters(struct I2CHandle* handle, uint16_t deviceAddress, uint16_t registerAddress, uint8_t* buffer, uint16_t bufferSize, uint16_t* bytesReceived);

enum Status i2c_writeRegisters(struct I2CHandle* handle, uint16_t deviceAddress, uint16_t registerAddress, const uint8_t* buffer, uint16_t bufferSize, uint16_t* bytesSent);


#endif // __I2C_H__

