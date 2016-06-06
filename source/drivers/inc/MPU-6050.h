/**
 * @file mpu-6050.h
 * @brief Driver for InvenSense MPU-6050 sensor
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

#ifndef __MPU_6050_H__
#define __MPU_6050_H__

#include <stdint.h>
#include <status.h>

enum Status mpu6050_initialize(void);

enum Status mpu6050_getId(uint8_t* id);

struct acceleration
{
   float x_g;
   float y_g;
   float z_g;
};

struct rotation
{
   float x_deg;
   float y_deg;
   float z_deg;
};

enum Status mpu6050_getAcceleration(struct acceleration* accel);

enum Status mpu6050_getRotation(struct rotation* gyro);

#endif // __MPU_6050_H__

