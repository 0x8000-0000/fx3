/**
 * @file BMP085.h
 * @brief Driver for Bosch Sensortech BMP085 Pressure Sensor
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

#ifndef __BMP085_H__
#define __BMP085_H__

#include <status.h>

void BMP085_initialize(void);

enum Status BMP085_getTemperature(float* temperature_C);

enum Status BMP085_getPressure(int32_t* pressure_Pa);

#endif // __BMP085_H__

