/**
 * @file spi.h
 * @brief SPI driver interface
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

#ifndef __SPI_H__
#define __SPI_H__

#include <stdint.h>
#include <stdbool.h>

#include <status.h>

struct SPIBus;

struct SPIConfiguration
{
   uint32_t speed;
};

void spi_initialize(struct SPIBus* bus, const struct SPIConfiguration* config);

enum Status spi_reserveBus(struct SPIBus* bus, bool polarity, bool phase);

enum Status spi_releaseBus(struct SPIBus* bus);

enum Status spi_read(struct SPIBus* bus, uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesRead);

enum Status spi_write(struct SPIBus* bus, const uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesWritten);

#endif // __SPI_H__

