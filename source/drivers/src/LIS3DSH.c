/**
 * @file LIS3DSH.c
 * @brief Driver for ST LIS3DSH accelerometer
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
 * Developed based on data sheet and application note
 *
 *    "LIS3DSH"
 *    DocID 022405 Rev 2
 *    October 2015
 *
 *    "AN3393"
 *    DocID 018750 Rev 4
 *    October 2014
 */

#include <assert.h>
#include <task.h>
#include <board.h>

#include <spi.h>

#include <LIS3DSH.h>

#define READ_REGISTER_CMD      ((uint8_t) 0x80)

enum LIS3DSH_register
{
   WHO_AM_I       = 0x0F,
   CTRL_REG4_ADDR = 0x20,

   CTRL_REG3_ADDR = 0x23,
   CTRL_REG5_ADDR = 0x24,
   CTRL_REG6_ADDR = 0x25,

   STATUS         = 0x27,
   OUT_X_L_ADDR   = 0x28,
   OUT_X_H_ADDR   = 0x29,
   OUT_Y_L_ADDR   = 0x2A,
   OUT_Y_H_ADDR   = 0x2B,
   OUT_Z_L_ADDR   = 0x2C,
   OUT_Z_H_ADDR   = 0x2D,
};

enum LIS3DSH_CTRL_REG3_bits
{
   REG3_STRT = 1,
};

enum LIS3DSH_CTRL_REG4_bits
{
   REG4_XEN = 1,
   REG4_YEN = 2,
   REG4_ZEN = 4,
   REG4_BDU = 8,

   REG4_ODR_POWER_DOWN = 0 << 4,     // 0  |  0   |  0   |  0   | Power Down (Default)
   REG4_ODR_3_125_HZ   = 1 << 4,     // 0  |  0   |  0   |  1   | 3.125 Hz
   REG4_ODR_6_25_HZ    = 2 << 4,     // 0  |  0   |  1   |  0   | 6.25 Hz
   REG4_ODR_12_5_HZ    = 3 << 4,     // 0  |  0   |  1   |  1   | 12.5 Hz
   REG4_ODR_25_HZ      = 4 << 4,     // 0  |  1   |  0   |  0   | 25 Hz
   REG4_ODR_50_HZ      = 5 << 4,     // 0  |  1   |  0   |  1   | 50 Hz
   REG4_ODR_100_HZ     = 6 << 4,     // 0  |  1   |  1   |  0   | 100 Hz
   REG4_ODR_400_HZ     = 7 << 4,     // 0  |  1   |  1   |  1   | 400 Hz
   REG4_ODR_800_HZ     = 8 << 4,     // 1  |  0   |  0   |  0   | 800 Hz
   REG4_ODR_1600_HZ    = 9 << 4,     // 1  |  0   |  0   |  1   | 1600 Hz
};

enum LIS3DSH_CTRL_REG5_bits
{
   REG5_FSCALE_2g      = 0 << 3,
   REG5_FSCALE_4g      = 1 << 3,
   REG5_FSCALE_6g      = 2 << 3,
   REG5_FSCALE_8g      = 3 << 3,
   REG5_FSCALE_16g     = 4 << 3,
};

enum LIS3DSH_CTRL_REG6_bits
{
   REG6_BOOT       = 0x80,
   REG6_IF_ADD_INC = 0x10,
};

#define LIS3DSH_SENSITIVITY_0_06G            0.00006f /* 0.06 mg/digit */
#define LIS3DSH_SENSITIVITY_0_12G            0.00012f /* 0.12 mg/digit */
#define LIS3DSH_SENSITIVITY_0_18G            0.00018f /* 0.18 mg/digit */
#define LIS3DSH_SENSITIVITY_0_24G            0.00024f /* 0.24 mg/digit */
#define LIS3DSH_SENSITIVITY_0_73G            0.00073f /* 0.73 mg/digit */

extern struct SPIBus LIS3DSH_BUS;

static inline void selectChip(void)
{
   bsp_setOutputPin(LIS3DSH_CHIP_SELECT, false);
}

static inline void deselectChip(void)
{
   bsp_setOutputPin(LIS3DSH_CHIP_SELECT, true);     // chip-select high
}

static inline enum Status reserveBus(void)
{
   return spi_reserveBus(&LIS3DSH_BUS, false, false);
}

static inline void releaseBus(void)
{
   deselectChip();   // just in case
   spi_releaseBus(&LIS3DSH_BUS);
}

static const uint8_t resetSequence[] =
{
   CTRL_REG6_ADDR,
   REG6_BOOT,
};

static const uint8_t enableMultibyteAutoincrement[] =
{
   CTRL_REG6_ADDR,
   REG6_IF_ADD_INC,
};

static const uint8_t enable2gScale[] =
{
   CTRL_REG5_ADDR,
   REG5_FSCALE_2g,
};

static const uint8_t enableXYZSamplingAt100Hz[] =
{
   CTRL_REG4_ADDR,
   REG4_ODR_100_HZ | REG4_BDU | REG4_ZEN | REG4_YEN | REG4_XEN,
};

enum Status LIS3DSH_initialize(void)
{
   bsp_initializeOutputPin(LIS3DSH_CHIP_SELECT);

   enum Status status = reserveBus();

   uint32_t xmit = 0;

   if (STATUS_OK == status)
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) resetSequence, sizeof(resetSequence), &xmit);
      deselectChip();
      fx3_suspendTask(10);
   }

   if (STATUS_OK == status);
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) enableMultibyteAutoincrement, sizeof(enableMultibyteAutoincrement), &xmit);
      deselectChip();

      bsp_delay(16);
   }

   if (STATUS_OK == status);
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) enable2gScale, sizeof(enable2gScale), &xmit);
      deselectChip();

      bsp_delay(16);
   }

   if (STATUS_OK == status);
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) enableXYZSamplingAt100Hz, sizeof(enableXYZSamplingAt100Hz), &xmit);
      deselectChip();

      fx3_suspendTask(10);       // settle time 1/ODR
   }

   if (STATUS_OK == status)
   {
      const uint8_t cmd = READ_REGISTER_CMD | CTRL_REG4_ADDR;

      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

      uint8_t ctrl4Reg = 0;

      if (STATUS_OK == status)
      {
         status = spi_read(&LIS3DSH_BUS, &ctrl4Reg, sizeof(ctrl4Reg), &xmit);
         if (enableXYZSamplingAt100Hz[1] != ctrl4Reg)
         {
            status = STATUS_HARDWARE_CONFIGURATION_FAILED;
         }
      }

      deselectChip();
   }

   releaseBus();

   return status;
}

enum Status LIS3DSH_getChipId(uint8_t* chipId)
{
   *chipId = 0xff;

   enum Status status = reserveBus();
   if (STATUS_OK != status)
   {
      return status;
   }

   uint32_t xmit = 0;

   const uint8_t cmd = READ_REGISTER_CMD | WHO_AM_I;

   selectChip();
   status = spi_write(&LIS3DSH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

   if (STATUS_OK == status)
   {
      status = spi_read(&LIS3DSH_BUS, chipId, sizeof(*chipId), &xmit);
   }

   deselectChip();
   releaseBus();

   return status;
}

static struct LIS3DSH_rawData
{
   int16_t x;
   int16_t y;
   int16_t z;
}  rawAccelerometerData;

enum Status LIS3DSH_getAcceleration(struct acceleration* accel)
{
   accel->x_g = 0;
   accel->y_g = 0;
   accel->z_g = 0;

   enum Status status = reserveBus();
   if (STATUS_OK != status)
   {
      return status;
   }

   uint32_t xmit = 0;

   const uint8_t cmd = READ_REGISTER_CMD | STATUS;

   selectChip();
   status = spi_write(&LIS3DSH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

   uint8_t rawData[7] = { 0 };

   if (STATUS_OK == status)
   {
      status = spi_read(&LIS3DSH_BUS, rawData, sizeof(rawData), &xmit);
   }

   deselectChip();
   releaseBus();

   if (STATUS_OK == status)
   {
      rawAccelerometerData.x = ((int16_t) ((((uint16_t) rawData[2]) << 8) | rawData[1]));
      rawAccelerometerData.y = ((int16_t) ((((uint16_t) rawData[4]) << 8) | rawData[3]));
      rawAccelerometerData.z = ((int16_t) ((((uint16_t) rawData[6]) << 8) | rawData[5]));

      /* low first, then high */
      accel->x_g = rawAccelerometerData.x * (2.0f / 32768);
      accel->y_g = rawAccelerometerData.y * (2.0f / 32768);
      accel->z_g = rawAccelerometerData.z * (2.0f / 32768);
   }

   return status;
}

