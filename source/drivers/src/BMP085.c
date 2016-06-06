/**
 * @file BMP085.c
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

/*
 * Developed based on:
 *    "BMP085 Data Sheet"
 *    Rev 1.2  15 October 2009
 */

#include <assert.h>

#include <task.h>

#include <board.h>

#include <i2c.h>

#include <BMP085.h>

extern struct I2CHandle BMP085_BUS;

#define BMP085_ADDRESS 0x77

enum BMP085_register
{
   BP_CONTROL  = 0xF4,
   BP_DATA_MSB = 0xF6,
   BP_EEPROM   = 0xAA,
};

enum BMP085_calib_param
{
   BMP180_CALIB_PARAM_AC1_MSB,
   BMP180_CALIB_PARAM_AC1_LSB,
   BMP180_CALIB_PARAM_AC2_MSB,
   BMP180_CALIB_PARAM_AC2_LSB,
   BMP180_CALIB_PARAM_AC3_MSB,
   BMP180_CALIB_PARAM_AC3_LSB,
   BMP180_CALIB_PARAM_AC4_MSB,
   BMP180_CALIB_PARAM_AC4_LSB,
   BMP180_CALIB_PARAM_AC5_MSB,
   BMP180_CALIB_PARAM_AC5_LSB,
   BMP180_CALIB_PARAM_AC6_MSB,
   BMP180_CALIB_PARAM_AC6_LSB,
   BMP180_CALIB_PARAM_B1_MSB,
   BMP180_CALIB_PARAM_B1_LSB,
   BMP180_CALIB_PARAM_B2_MSB,
   BMP180_CALIB_PARAM_B2_LSB,
   BMP180_CALIB_PARAM_MB_MSB,
   BMP180_CALIB_PARAM_MB_LSB,
   BMP180_CALIB_PARAM_MC_MSB,
   BMP180_CALIB_PARAM_MC_LSB,
   BMP180_CALIB_PARAM_MD_MSB,
   BMP180_CALIB_PARAM_MD_LSB,
};

union
{
   struct
   {
      int16_t AC1;
      int16_t AC2;
      int16_t AC3;
      uint16_t AC4;
      uint16_t AC5;
      uint16_t AC6;
      int16_t B1;
      int16_t B2;
      int16_t MB;
      int16_t MC;
      int16_t MD;
   };

   uint8_t asBytes[11 * 2];

   int32_t B5;    // computed from temperature

}  param;

void BMP085_initialize(void)
{
   uint16_t xmit = 0;
   enum Status status = i2c_readRegisters(&BMP085_BUS, BMP085_ADDRESS, BP_EEPROM, param.asBytes, sizeof(param), &xmit);
   assert(STATUS_OK == status);

   /*
    * Swap MSB / LSB
    */
   //param.AC1 = (int16_t)((((int32_t)((int8_t) param.asBytes[BMP180_CALIB_PARAM_AC1_MSB])) << 8) | param.asBytes[BMP180_CALIB_PARAM_AC1_LSB]);
   //param.AC2 = (int16_t)((((int32_t)((int8_t) param.asBytes[BMP180_CALIB_PARAM_AC2_MSB])) << 8) | param.asBytes[BMP180_CALIB_PARAM_AC2_LSB]);
   //...
   for (uint32_t ii = 0; ii < 11; ii ++)
   {
      uint8_t temp              = param.asBytes[2 * ii];
      param.asBytes[2 * ii]     = param.asBytes[2 * ii + 1];
      param.asBytes[2 * ii + 1] = temp;
   }
}

enum Status BMP085_getTemperature(float* temperature_C)
{
   *temperature_C = 0.0f;

   uint16_t xmit = 0;
   uint8_t cmd = 0x2e;
   enum Status status = i2c_writeRegisters(&BMP085_BUS, BMP085_ADDRESS, BP_CONTROL, &cmd, 1, &xmit);
   if (STATUS_OK == status)
   {
      fx3_suspendTask(5);

      uint8_t rawData[2];
      status = i2c_readRegisters(&BMP085_BUS, BMP085_ADDRESS, BP_DATA_MSB, rawData, sizeof(rawData), &xmit);
      if (STATUS_OK == status)
      {
         int32_t UT = (((uint32_t) rawData[0]) << 8) | rawData[1];

         int32_t X1 = ((UT - param.AC6) * param.AC5) >> 15;
         int32_t X2 = (param.MC << 11) / (X1 + param.MD);
         param.B5 = X1 + X2;
         int32_t T = (param.B5 + 8) >> 4;
         *temperature_C = T / 10.0f;
      }
   }

   return status;
}

#define OVERSAMPLE_MODE 2

enum Status BMP085_getPressure(int32_t* pressure_Pa)
{
   *pressure_Pa = 0;

   uint16_t xmit = 0;
   uint8_t cmd = 0x34 + (OVERSAMPLE_MODE << 6);
   enum Status status = i2c_writeRegisters(&BMP085_BUS, BMP085_ADDRESS, BP_CONTROL, &cmd, 1, &xmit);
   if (STATUS_OK == status)
   {
      fx3_suspendTask(14);

      uint8_t rawData[3];
      status = i2c_readRegisters(&BMP085_BUS, BMP085_ADDRESS, BP_DATA_MSB, rawData, sizeof(rawData), &xmit);
      if (STATUS_OK == status)
      {
         int32_t UP = ((((uint32_t) rawData[0]) << 16) | (((uint32_t) rawData[1]) << 8) | ((uint32_t) rawData[2])) >> (8 - OVERSAMPLE_MODE);

         int32_t B6 = param.B5 - 4000;
         int32_t X1 = (param.B2 * ((B6 * B6) >> 12)) >> 11;
         int32_t X2 = (param.AC2 * B6) >> 11;
         int32_t X3 = X1 + X2;
         int32_t B3 = (((param.AC1 * 4 + X2) << OVERSAMPLE_MODE) + 2) / 4;

         int32_t X1_ = (param.AC3 * B6) >> 13;
         int32_t X2_ = (param.B1 * ((B6 * B6) >> 12)) >> 16;
         int32_t X3_ = (X1 + X2 + 2) >> 2;
         uint32_t B4 = (param.AC4 * ((uint32_t) (X3 + 32768))) >> 15;
         uint32_t B7 = (uint32_t) ((UP - B3) * (50000 >> OVERSAMPLE_MODE));
         int32_t p;
         if (B7 < 0x8000000)
         {
            p = (B7 * 2) / B4;
         }
         else
         {
            p = (B7 / B4) * 2;
         }
         int32_t X1__ = (p >> 8) * (p >> 8);
         X1__ = (X1__ * 3038) >> 16;
         int32_t X2__ = (-7357 * p) >> 16;
         p = p + ((X1__ + X2__ + 3791) >> 4);

         *pressure_Pa = p;
      }
   }

   return status;
}
