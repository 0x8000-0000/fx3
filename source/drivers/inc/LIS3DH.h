/**
 * @file LIS3DH.h
 * @brief Driver for ST LIS3DH accelerometer
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

#ifndef __LIS3DH_H__
#define __LIS3DH_H__

#include <stdint.h>
#include <status.h>
#include <mems.h>

enum Status LIS3DH_initialize(void);

enum Status LIS3DH_getChipId(uint32_t* expectedId, uint32_t* actualId);

struct LIS3DH_rawData
{
   int16_t x;
   int16_t y;
   int16_t z;
};

enum Status LIS3DH_getSensitivity(uint8_t* sensitivity);

enum Status LIS3DH_getRawCounts(uint8_t* dataStatus, struct LIS3DH_rawData* rawData);

void LIS3DH_computeAcceleration(const struct LIS3DH_rawData* rawData, uint32_t dataSize, uint8_t sensitivity, struct acceleration* accel);

enum Status LIS3DH_enableFIFO(void);

enum Status LIS3DH_disableFIFO(void);

enum Status LIS3DH_readFIFO(struct LIS3DH_rawData* data, uint32_t capacity, uint32_t* valuesCount);

#define LIS3DH_FIFO_SIZE 32

#endif // __LIS3DH_H__

