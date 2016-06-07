/**
 * @file EN25F80.h
 * @brief Driver for Eon Silicon EN25F80 serial flash memory
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

#ifndef __EN25F80_H__
#define __EN25F80_H__

#include <stdint.h>

#include <status.h>

enum Status EN25F80_initialize(void);

enum Status EN25F80_getChipId(uint32_t* chipId);

enum Status EN25F80_releaseFromDeepSleep(void);

enum Status EN25F80_eraseChip(void);
enum Status EN25F80_eraseSector(uint32_t address);

enum Status EN25F80_enableWrite(void);
enum Status EN25F80_disableWrite(void);

enum Status EN25F80_readByte(uint32_t address, uint8_t* value);
enum Status EN25F80_writeByte(uint32_t address, uint8_t value);

#endif // __EN25F80_H__

