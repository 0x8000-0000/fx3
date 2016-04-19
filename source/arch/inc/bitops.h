/**
 * @file bitops.h
 * @brief Bitmap allocator
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

#ifndef __BITOPS_H__
#define __BITOPS_H__

#include <stdint.h>

/** Initializes a bitmap
 *
 * @param bitMap is the bitmap
 * @param bitCount is the initial number of free bits
 */
void bit_initialize(volatile uint32_t* bitMap, uint32_t bitCount);

/** Allocates a bit if available
 *
 * @param bitMap is the bitmap
 * @return 32 if no bits are available, or the allocated bit
 */
uint32_t bit_alloc(volatile uint32_t* bitMap);

/** Frees a bit
 *
 * @param bitMap is the bitmap
 * @param bitPos is the bit index
 */
void bit_free(volatile uint32_t* bitMap, uint32_t bitPos);

#endif // __BITOPS_H__

