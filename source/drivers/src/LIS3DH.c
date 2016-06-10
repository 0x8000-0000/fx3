/**
 * @file LIS3DH.c
 * @brief Driver for ST LIS3DH accelerometer
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
 *    "LIS3DH"
 *    Doc ID 17530 Rev 1
 *    May 2010
 *
 *    "AN3308"
 *    Doc ID 18198 Rev 1
 *    January 2011
 */

#include <assert.h>
#include <string.h>

#include <task.h>
#include <board.h>

#include <spi.h>

#include <LIS3DH.h>

#define READ_REGISTER_CMD      ((uint8_t) 0x80)
#define MULTIPLE_SELECT        ((uint8_t) 0x40)

enum LIS3DH_register
{
   STATUS_AUX     = 0x07,
   OUT_1_L        = 0x08,
   OUT_1_H        = 0x09,
   OUT_2_L        = 0x0A,
   OUT_2_H        = 0x0B,
   OUT_3_L        = 0x0C,
   OUT_3_H        = 0x0D,
   INT_COUNTER    = 0x0E,
   WHO_AM_I       = 0x0F,

   CTRL_REG1      = 0x20,
   CTRL_REG2      = 0x21,
   CTRL_REG3      = 0x22,
   CTRL_REG4      = 0x23,
   CTRL_REG5      = 0x24,
   CTRL_REG6      = 0x25,

   STATUS         = 0x27,
   OUT_X_L_ADDR   = 0x28,
   OUT_X_H_ADDR   = 0x29,
   OUT_Y_L_ADDR   = 0x2A,
   OUT_Y_H_ADDR   = 0x2B,
   OUT_Z_L_ADDR   = 0x2C,
   OUT_Z_H_ADDR   = 0x2D,

   FIFO_CTRL_REG  = 0x2E,
   FIFO_SRC_REG   = 0x2F,

};

enum LIS3DH_CTRL_REG1_bits
{
   REG1_XEN = 1,
   REG1_YEN = 2,
   REG1_ZEN = 4,
   REG1_LP  = 8,

   REG1_ODR_POWER_DOWN = 0 << 4,     // Power Down (Default)
   REG1_ODR_1_HZ       = 1 << 4,     // Normal / Low 1 Hz
   REG1_ODR_10_HZ      = 2 << 4,     // Normal / Low 10 Hz
   REG1_ODR_25_HZ      = 3 << 4,     // Normal / Low 25 Hz
   REG1_ODR_50_HZ      = 4 << 4,     // Normal / Low 50 Hz
   REG1_ODR_100_HZ     = 5 << 4,     // Normal / Low 100 Hz
   REG1_ODR_200_HZ     = 6 << 4,     // Normal / Low 200 Hz
   REG1_ODR_400_HZ     = 7 << 4,     // Normal / Low 400 Hz
   REG1_ODR_1600_HZ    = 8 << 4,     // Low power mode 1.6 kHz
   REG1_ODR_1250_HZ    = 9 << 4,     // Normal (1.25 kHz) / Low power 5 kHz
};

enum LIS3DH_CTRL_REG4_bits
{
   REG4_FS_2G  = 0 << 4,
   REG4_FS_4G  = 1 << 4,
   REG4_FS_8G  = 2 << 4,
   REG4_FS_16G = 4 << 4,
};

enum LIS3DH_CTRL_REG5_bits
{
   REG5_BOOT       = 0x80,
   REG5_FIFO_EN    = 0x40,
};

enum LIS3DH_FIFO_CTRL_bits
{
   BYPASS_MODE    = 0x0 << 6,
   FIFO_MODE      = 0x1 << 6,
   STREAM_MODE    = 0x2 << 6,
   TRIGGER_MODE   = 0x3 << 6,
};

enum LIS3DH_FIFO_SRC_bits
{
   FIFO_SRC_WTM          = 0x80,
   FIFO_SRC_OVRN         = 0x40,
   FIFO_SRC_EMPTY        = 0x20,
   FIFO_SRC_SAMPLES_MASK = 0x1F,
};

extern struct SPIBus LIS3DH_BUS;

static inline void selectChip(void)
{
   bsp_setOutputPin(LIS3DH_CHIP_SELECT, false);
}

static inline void deselectChip(void)
{
   bsp_setOutputPin(LIS3DH_CHIP_SELECT, true);     // chip-select high
}

static inline enum Status reserveBus(void)
{
   return spi_reserveBus(&LIS3DH_BUS, false, false);
}

static inline void releaseBus(void)
{
   deselectChip();   // just in case
   spi_releaseBus(&LIS3DH_BUS);
}

static const uint8_t resetSequence[] =
{
   CTRL_REG5,
   REG5_BOOT,
};

static const uint8_t setSamplingRateAndEnable[] =
{
   CTRL_REG1,
   REG1_ODR_100_HZ | REG1_ZEN | REG1_YEN | REG1_XEN,
};

static const uint8_t enable2gScale[] =
{
   CTRL_REG4,
   REG4_FS_2G,
};

enum Status LIS3DH_initialize(void)
{
   bsp_initializeOutputPin(LIS3DH_CHIP_SELECT);

   enum Status status = reserveBus();

   uint32_t xmit = 0;

   if (STATUS_OK == status)
   {
      selectChip();
      status = spi_write(&LIS3DH_BUS, (uint8_t*) resetSequence, sizeof(resetSequence), &xmit);
      deselectChip();
   }

   fx3_suspendTask(20);       // wake-up time: 10 ms

   if (STATUS_OK == status);
   {
      selectChip();
      status = spi_write(&LIS3DH_BUS, (uint8_t*) setSamplingRateAndEnable, sizeof(setSamplingRateAndEnable), &xmit);
      deselectChip();
   }

   if (STATUS_OK == status);
   {
      selectChip();
      status = spi_write(&LIS3DH_BUS, (uint8_t*) enable2gScale, sizeof(enable2gScale), &xmit);
      deselectChip();

      bsp_delay(8);
   }

   /*
    * check settings
    */
   if (STATUS_OK == status)
   {
      const uint8_t cmd = READ_REGISTER_CMD | MULTIPLE_SELECT | CTRL_REG1;

      selectChip();
      status = spi_write(&LIS3DH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

      uint8_t configuredRegisters[6] = { 0 };

      if (STATUS_OK == status)
      {
         status = spi_read(&LIS3DH_BUS, configuredRegisters, sizeof(configuredRegisters), &xmit);

         if (setSamplingRateAndEnable[1] != configuredRegisters[0])
         {
            status = STATUS_HARDWARE_CONFIGURATION_FAILED;
         }
      }

      deselectChip();
   }

   releaseBus();

   if (STATUS_OK == status)
   {
      fx3_suspendTask(10);       // settle time 1/ODR
   }

   return status;
}

enum Status LIS3DH_getChipId(uint32_t* expectedId, uint32_t* actualId)
{
   *expectedId = 0x33;
   *actualId   = 0xff;

   enum Status status = reserveBus();
   if (STATUS_OK != status)
   {
      return status;
   }

   uint32_t xmit = 0;

   const uint8_t cmd = READ_REGISTER_CMD | WHO_AM_I;

   uint8_t rawData = 0;

   selectChip();
   status = spi_write(&LIS3DH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

   if (STATUS_OK == status)
   {
      status = spi_read(&LIS3DH_BUS, &rawData, sizeof(rawData), &xmit);
   }

   deselectChip();
   releaseBus();

   if (STATUS_OK == status)
   {
      *actualId = rawData;
   }

   return status;
}

enum Status LIS3DH_getSensitivity(uint8_t* sensitivity)
{
   *sensitivity = 2;
   return STATUS_OK;
}

enum Status LIS3DH_getRawCounts(uint8_t* dataStatus, struct LIS3DH_rawData* rawData)
{
   enum Status status = reserveBus();
   if (STATUS_OK == status)
   {
      uint32_t xmit = 0;

      const uint8_t cmd = READ_REGISTER_CMD | STATUS;

      selectChip();
      status = spi_write(&LIS3DH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

      uint8_t data[7] = { 0 };

      if (STATUS_OK == status)
      {
         status = spi_read(&LIS3DH_BUS, data, sizeof(data), &xmit);
      }

      deselectChip();
      releaseBus();

      if (STATUS_OK == status)
      {
         *dataStatus  = data[0];

         rawData->x = ((int16_t) ((((uint16_t) data[2]) << 8) | data[1]));
         rawData->y = ((int16_t) ((((uint16_t) data[4]) << 8) | data[3]));
         rawData->z = ((int16_t) ((((uint16_t) data[6]) << 8) | data[5]));
      }
      else
      {
         *dataStatus  = 0;
         memset(rawData, 0, sizeof(*rawData));
      }
   }

   return status;
}

void LIS3DH_computeAcceleration(const struct LIS3DH_rawData* rawData, uint32_t dataSize, uint8_t sensitivity, struct acceleration* accel)
{
   if (dataSize)
   {
      const float sensitivityAdjustment = (32768.0f / (float) (sensitivity));

      /*
       * we're averaging up to 32 int16_t values; we can add them all up,
       * without fear of overflow
       */
      int sumOfX = 0;
      int sumOfY = 0;
      int sumOfZ = 0;

      for (uint32_t ii = 0; ii < dataSize; ii ++)
      {
         sumOfX += rawData[ii].x;
         sumOfY += rawData[ii].y;
         sumOfZ += rawData[ii].z;
      }

      float averageX = sumOfX / (float) dataSize;
      float averageY = sumOfY / (float) dataSize;
      float averageZ = sumOfZ / (float) dataSize;

      accel->x_g = ((float) averageX) / sensitivityAdjustment;
      accel->y_g = ((float) averageY) / sensitivityAdjustment;
      accel->z_g = ((float) averageZ) / sensitivityAdjustment;
   }
   else
   {
      accel->x_g = 0.0f;
      accel->y_g = 0.0f;
      accel->z_g = 0.0f;
   }
}

/*
 * Follow these steps for FIFO mode configuration:
 *   1. Turn on FIFO by setting the FIFO_En bit to "1" in control
 *   register 5 (0x24). After this operation the FIFO buffer is enabled
 *   but isn't collecting data, output registers are frozen to the last
 *   samples set loaded.
 *   2. Activate stream mode by setting the FN[1:0] field to "10" in the
 *   FIFO control register (0x2E).
 */
static const uint8_t enableFIFO[] =
{
   CTRL_REG5,
   REG5_FIFO_EN,
};

static const uint8_t setFIFOMode[] =
{
   FIFO_CTRL_REG,
   STREAM_MODE | 0xF,        // watermark at 15
};

enum Status LIS3DH_enableFIFO(void)
{
   enum Status status = reserveBus();

   uint32_t xmit = 0;

   if (STATUS_OK == status)
   {
      selectChip();
      status = spi_write(&LIS3DH_BUS, (uint8_t*) enableFIFO, sizeof(enableFIFO), &xmit);
      deselectChip();

      if (STATUS_OK == status)
      {
         selectChip();
         status = spi_write(&LIS3DH_BUS, (uint8_t*) setFIFOMode, sizeof(setFIFOMode), &xmit);
         deselectChip();
      }

      releaseBus();
   }

   return status;
}

enum Status LIS3DH_disableFIFO(void)
{
   return STATUS_NOT_IMPLEMENTED;
}

static enum Status getRawCounts(struct LIS3DH_rawData* rawData)
{
   uint32_t xmit = 0;

   const uint8_t cmd = READ_REGISTER_CMD | MULTIPLE_SELECT | OUT_X_L_ADDR;

   selectChip();
   enum Status status = spi_write(&LIS3DH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

   uint8_t data[6] = { 0 };

   if (STATUS_OK == status)
   {
      status = spi_read(&LIS3DH_BUS, data, sizeof(data), &xmit);
   }

   deselectChip();

   if (STATUS_OK == status)
   {
      rawData->x = ((int16_t) ((((uint16_t) data[1]) << 8) | data[0]));
      rawData->y = ((int16_t) ((((uint16_t) data[3]) << 8) | data[2]));
      rawData->z = ((int16_t) ((((uint16_t) data[5]) << 8) | data[4]));
   }
   else
   {
      memset(rawData, 0, sizeof(*rawData));
   }

   return status;
}

enum Status LIS3DH_readFIFO(struct LIS3DH_rawData* data, uint32_t capacity, uint32_t* valuesCount)
{
   uint32_t xmit = 0;

   uint32_t readCount = 0;
   enum Status status = reserveBus();

   if (STATUS_OK == status)
   {
      uint8_t fifoSrc = 0;

      {
         const uint8_t cmd = READ_REGISTER_CMD | FIFO_SRC_REG;

         selectChip();
         status = spi_write(&LIS3DH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

         if (STATUS_OK == status)
         {
            status = spi_read(&LIS3DH_BUS, &fifoSrc, sizeof(fifoSrc), &xmit);
         }

         deselectChip();
      }

      uint32_t samplesCount = (fifoSrc & FIFO_SRC_SAMPLES_MASK);

      while ((STATUS_OK == status) && samplesCount && (readCount < capacity))
      {
         status = getRawCounts(&data[readCount]);

         readCount ++;
         samplesCount --;
      }

      releaseBus();
   }

   *valuesCount = readCount;

   return status;
}

