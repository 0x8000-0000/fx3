/**
 * @file mpu-6050.c
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

/*
 * Developed based on:
 *    "MPU-6000/MPU-6050 Product Specification"
 *       Document Number: PS-MPU-6000A-00
 *       Revision: 3.4
 *       Release Date: 08/19/2013
 *
 *    "MPU-6000/MPU-6050 Register Map and Descriptions"
 *       Document Number: RM-MPU-6000A-00
 *       Revision: 4.2
 *       Release Date: 08/19/2013
 */

#include <string.h>
#include <math.h>

#include <board.h>

#include <i2c.h>

#include <MPU-6050.h>

extern struct I2CHandle MPU_6050_BUS;

#define MPU_6050_ADDRESS   0x68

enum MPU_6050_REGISTER
{
   SELF_TEST_X          = 0x0D,
   SELF_TEST_Y          = 0x0E,
   SELF_TEST_Z          = 0x0F,
   SELF_TEST_A          = 0x10,

   SMPLRT_DIV           = 0x19,

   CONFIG               = 0x1A,
   GYRO_CONFIG          = 0x1B,
   ACCEL_CONFIG         = 0x1C,

   FIFO_EN              = 0x23,
   I2C_MST_CTRL         = 0x24,
   I2C_SLV0_ADDR        = 0x25,
   I2C_SLV0_REG         = 0x26,
   I2C_SLV0_CTRL        = 0x27,
   /* more slave regs */
   I2C_MST_STATUS       = 0x36,
   INT_PIN_CFG          = 0x37,
   INT_ENABLE           = 0x38,

   INT_STATUS           = 0x3A,
   ACCEL_XOUT_H         = 0x3B,
   ACCEL_XOUT_L         = 0x3C,
   ACCEL_YOUT_H         = 0x3D,
   ACCEL_YOUT_L         = 0x3E,
   ACCEL_ZOUT_H         = 0x3F,
   ACCEL_ZOUT_L         = 0x40,
   TEMP_OUT_H           = 0x41,
   TEMP_OUT_L           = 0x42,
   GYRO_XOUT_H          = 0x43,
   GYRO_XOUT_L          = 0x44,
   GYRO_YOUT_H          = 0x45,
   GYRO_YOUT_L          = 0x46,
   GYRO_ZOUT_H          = 0x47,
   GYRO_ZOUT_L          = 0x48,
   EXT_SENS_DATA_00     = 0x49,
   /* more ext sens data */
   EXT_SENS_DATA_23     = 0x60,

   I2C_SLV0_DO          = 0x63,
   I2C_SLV1_DO          = 0x64,
   I2C_SLV2_DO          = 0x65,
   I2C_SLV3_DO          = 0x66,
   I2C_MST_DEELAY_CTRL  = 0x67,
   SIGNAL_PATH_RESET    = 0x68,

   USER_CTRL            = 0x6A,
   PWR_MGMT_1           = 0x6B,
   PWR_MGMT_2           = 0x6C,

   FIFO_COUNTH          = 0x72,
   FIFO_COUNTL          = 0x73,
   FIFO_R_W             = 0x74,
   WHO_AM_I             = 0x75,
};

struct factory_trim_data
{
   unsigned int xa_test_hi: 3;
   unsigned int xg_test   : 5;
   unsigned int ya_test_hi: 3;
   unsigned int yg_test   : 5;
   unsigned int za_test_hi: 3;
   unsigned int zg_test   : 5;
   unsigned int reserved  : 2;
   unsigned int xa_test_lo: 2;
   unsigned int ya_test_lo: 2;
   unsigned int za_test_lo: 2;
};

struct factory_trim
{
   float accelX;
   float accelY;
   float accelZ;

   float gyroX;
   float gyroY;
   float gyroZ;

   bool valid;
};

static struct factory_trim FT;

static const uint16_t gyroscopeScale[] =
{
   250,
   500,
   1000,
   2000,
};

static uint8_t gyroscopeResolution = 0;

static const uint8_t accelerometerScale[] =
{
   2,
   4,
   8,
   16,
};

static uint8_t accelerometerResolution = 0;

enum Status mpu6050_initialize(void)
{
   memset(&FT, 0, sizeof(FT));

   uint16_t xmit = 0;
   uint8_t cmd = 0;     // disable sleep

   enum Status status = i2c_writeRegisters(&MPU_6050_BUS, MPU_6050_ADDRESS, PWR_MGMT_1, &cmd, 1, &xmit);

   if (STATUS_OK == status)
   {
      struct factory_trim_data ft;
      status = i2c_readRegisters(&MPU_6050_BUS, MPU_6050_ADDRESS, SELF_TEST_X, (uint8_t*) &ft, sizeof(ft), &xmit);

      if (STATUS_OK == status)
      {
         uint32_t xa_test = (((uint32_t) ft.xa_test_hi) << 2) | ft.xa_test_lo;
         uint32_t ya_test = (((uint32_t) ft.ya_test_hi) << 2) | ft.ya_test_lo;
         uint32_t za_test = (((uint32_t) ft.za_test_hi) << 2) | ft.za_test_lo;

         if (xa_test)
         {
            FT.accelX = 4096 * 0.34f * powf(0.92f / 0.34f, (xa_test - 1) / 30.f);
         }

         if (ya_test)
         {
            FT.accelY = 4096 * 0.34f * powf(0.92f / 0.34f, (ya_test - 1) / 30.f);
         }

         if (za_test)
         {
            FT.accelZ = 4096 * 0.34f * powf(0.92f / 0.34f, (za_test - 1) / 30.f);
         }

         if (ft.xg_test)
         {
            FT.gyroX = 25 * 131 * powf(1.046f, ft.xg_test - 1);
         }

         if (ft.yg_test)
         {
            FT.gyroY = -25 * 131 * powf(1.046f, ft.yg_test - 1);
         }

         if (ft.zg_test)
         {
            FT.gyroZ = 25 * 131 * powf(1.046f, ft.zg_test - 1);
         }

         FT.valid = true;
      }
   }

   return status;
}

enum Status mpu6050_getId(uint8_t* mpuId)
{
   uint16_t xmit;
   *mpuId = 0;

   enum Status status = i2c_readRegisters(&MPU_6050_BUS, MPU_6050_ADDRESS, WHO_AM_I, mpuId, 1, &xmit);

   return status;
}

enum Status mpu6050_getAcceleration(struct acceleration* accel)
{
   uint16_t xmit;
   uint8_t rawData[6];

   enum Status status = i2c_readRegisters(&MPU_6050_BUS, MPU_6050_ADDRESS, ACCEL_XOUT_H, rawData, sizeof(rawData), &xmit);
   if (STATUS_OK == status)
   {
      int16_t xv = (int16_t) ((((uint16_t) rawData[0]) << 8) | rawData[1]);
      int16_t yv = (int16_t) ((((uint16_t) rawData[2]) << 8) | rawData[3]);
      int16_t zv = (int16_t) ((((uint16_t) rawData[4]) << 8) | rawData[5]);

      float scale = (float) accelerometerScale[accelerometerResolution];

      accel->x_g = (xv * scale) / 32768.0f;
      accel->y_g = (yv * scale) / 32768.0f;
      accel->z_g = (zv * scale) / 32768.0f;
   }

   return status;
}

enum Status mpu6050_getRotation(struct rotation* gyro)
{
   uint16_t xmit;
   uint8_t rawData[6];

   enum Status status = i2c_readRegisters(&MPU_6050_BUS, MPU_6050_ADDRESS, GYRO_XOUT_H, rawData, sizeof(rawData), &xmit);
   if (STATUS_OK == status)
   {
      int16_t xv = (int16_t) ((((uint16_t) rawData[0]) << 8) | rawData[1]);
      int16_t yv = (int16_t) ((((uint16_t) rawData[2]) << 8) | rawData[3]);
      int16_t zv = (int16_t) ((((uint16_t) rawData[4]) << 8) | rawData[5]);

      float scale = (float) gyroscopeScale[gyroscopeResolution];

      gyro->x_deg = (xv * scale) / 32768.0f;
      gyro->y_deg = (yv * scale) / 32768.0f;
      gyro->z_deg = (zv * scale) / 32768.0f;
   }

   return status;
}

