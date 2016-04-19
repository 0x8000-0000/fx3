/**
 * @file blinky.c
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

#include <stdbool.h>
#include <stdatomic.h>

#include <bitops.h>

/*
 * TODO: test this implementation
 */

void bit_initialize(volatile uint32_t* bitMap, uint32_t bitCount)
{
   *bitMap = (uint32_t) ((1 << bitCount) - 1);
}

uint32_t bit_alloc(volatile uint32_t* bitMap)
{
   uint32_t currentValue = __atomic_load_n(bitMap, __ATOMIC_SEQ_CST);

   uint32_t desiredValue = 0;
   uint32_t availableBit = 32;

   do
   {
      availableBit = (uint32_t) (31 - __builtin_clz(currentValue));

      desiredValue = currentValue & (uint32_t) (~(1 << availableBit));
   }
   while (! __atomic_compare_exchange_n(bitMap, &currentValue, desiredValue, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

   return availableBit;
}

void bit_free(volatile uint32_t* bitMap, uint32_t bitPos)
{
   __atomic_and_fetch(bitMap, (1 << bitPos), __ATOMIC_SEQ_CST);
}

