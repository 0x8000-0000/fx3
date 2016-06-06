/**
 * @file DS3231M.c
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

/*
 * Developed based on:
 *    "DS3231MPMB1 Peripheral Module"
 *    19-6337; Rev 0; 5/12
 *
 *    "DS3231M ±5ppm, I2C Real-Time Clock"
 *    19-5312; Rev 7; 3/15
 */

#include <string.h>

#include <board.h>

#include <i2c.h>

#include <DS3231M.h>

extern struct I2CHandle DS3231M_BUS;

#define DS3231M_ADDRESS 0xD0

enum DS3231M_Registers
{
   DS_SECONDS   = 0,
   DS_TEMP_HI   = 0x11,
   DS_TEMP_LO   = 0x12,
};

void DS3231M_initialize(void)
{
   // empty
}

enum Status DS3231M_getTemperature(float* temperature)
{
   uint16_t xmit;

   uint8_t rawData[2];
   enum Status status = i2c_readRegisters(&DS3231M_BUS, DS3231M_ADDRESS, DS_TEMP_HI, rawData, 2, &xmit);
   if (STATUS_OK == status)
   {
      *temperature = ((int8_t) rawData[0]) + ((rawData[1] >> 6) * 0.25f);
   }
   else
   {
      *temperature = 0;
   }

   return status;
}

static inline uint8_t bcd2bin(uint8_t val)
{
   return (val & 0xF) + ((val >> 4) * 10);
}

static inline uint8_t bin2bcd(uint8_t val)
{
   return (uint8_t) (((val / 10) << 4) | (val % 10));
}

enum Status DS3231M_getTime(struct tm* dsTime)
{
   uint16_t xmit;

   uint8_t rawData[7];
   enum Status status = i2c_readRegisters(&DS3231M_BUS, DS3231M_ADDRESS, DS_SECONDS, rawData, 7, &xmit);

   if (STATUS_OK == status)
   {
      dsTime->tm_sec   = bcd2bin(rawData[0] & 0x7F);
      dsTime->tm_min   = bcd2bin(rawData[1] & 0x7F);
      dsTime->tm_hour  = bcd2bin(rawData[2] & 0x3F);
      dsTime->tm_wday  = (rawData[3] & 0x7);
      dsTime->tm_mday  = bcd2bin(rawData[4] & 0x3F);
      dsTime->tm_mon   = (bcd2bin(rawData[5] & 0x1f)) - 1;
      uint8_t century  = (rawData[5] >> 7) * 100;
      dsTime->tm_year  = bcd2bin(rawData[6]) + century;

      dsTime->tm_yday  = 0;
      dsTime->tm_isdst = 0;
   }
   else
   {
      memset(dsTime, 0, sizeof(*dsTime));
   }

   return status;
}

enum Status DS3231M_setTime(struct tm* dsTime)
{
   uint8_t rawData[7] = { 0 };

   rawData[0] = bin2bcd(dsTime->tm_sec);
   rawData[1] = bin2bcd(dsTime->tm_min);
   rawData[2] = bin2bcd(dsTime->tm_hour);
   rawData[3] = bin2bcd(dsTime->tm_wday);
   rawData[4] = bin2bcd(dsTime->tm_mday);
   rawData[5] = bin2bcd(dsTime->tm_mday + 1) | 0x80;
   rawData[6] = bin2bcd(dsTime->tm_year - 100);

   uint16_t xmit;
   enum Status status = i2c_writeRegisters(&DS3231M_BUS, DS3231M_ADDRESS, DS_SECONDS, rawData, 7, &xmit);
   return status;
}
