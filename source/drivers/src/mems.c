/**
 * @file mems.c
 * @brief Data structures for MEMS
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
 * Based on information from
 *
 *    ST AN3182
 *    DocID 17289 Rev 1
 *    April 2010
 */

#include <math.h>

#include <mems.h>

void computeTilt(const struct acceleration* accel, struct tilt* tilt)
{
   (void) accel;
   (void) tilt;
}

