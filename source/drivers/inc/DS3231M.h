/**
 * @file DS3231M.h
 * @brief Driver for Maxim DS3231M RTC
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

#ifndef __DS3231M_H__
#define __DS3231M_H__

#include <stdint.h>
#include <time.h>

#include <status.h>

void DS3231M_initialize(void);

enum Status DS3231M_getTemperature(float* temperature);

enum Status DS3231M_getTime(struct tm* dsTime);

enum Status DS3231M_setTime(struct tm* dsTime);

#endif //__DS3231M_H__

