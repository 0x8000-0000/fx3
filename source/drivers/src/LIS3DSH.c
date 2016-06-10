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
#include <string.h>

#include <task.h>
#include <board.h>

#include <spi.h>

#include <LIS3DSH.h>

#define READ_REGISTER_CMD      ((uint8_t) 0x80)
#define MULTIPLE_SELECT        ((uint8_t) 0x40)

enum LIS3DSH_register
{
   REG_INFO1      = 0x0D,
   REG_INFO2      = 0x0E,
   REG_WHO_AM_I   = 0x0F,

   CTRL_REG4_ADDR = 0x20,
   CTRL_REG1_ADDR = 0x21,
   CTRL_REG2_ADDR = 0x21,
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

   FIFO_CTRL_REG  = 0x2E,
   FIFO_SRC_REG   = 0x2F,
};

enum LIS3DSH_CTRL_REG3_bits
{
   REG3_DR_EN   = 0x80,
   REG3_IEA     = 0x40,
   REG3_IEL     = 0x20,
   REG3_INT2_EN = 0x10,
   REG3_INT1_EN = 0x08,
   REG3_VFILT   = 0x04,
   REG3_STRT    = 0x01,
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
   REG6_FIFO_EN    = 0x40,
   REG6_IF_ADD_INC = 0x10,
   REG6_BOOT_INT2  = 0x01,
};

enum LIS3DSH_FIFO_CTRL_bits
{
   FIFO_CTRL_OFF        = 0x00,
   FIFO_CTRL_ONE_SHOT   = 0x01 << 5,
   FIFO_CTRL_CIRCULAR   = 0x02 << 5,
};

enum LIS3DSH_FIFO_SRC_bits
{
   FIFO_SRC_WTM          = 0x80,
   FIFO_SRC_OVRN         = 0x40,
   FIFO_SRC_EMPTY        = 0x20,
   FIFO_SRC_SAMPLES_MASK = 0x1F,
};

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
   REG6_BOOT | REG6_BOOT_INT2,
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

static const uint8_t setSamplingRateAndEnable[] =
{
   CTRL_REG4_ADDR,
   REG4_ODR_100_HZ | REG4_ZEN | REG4_YEN | REG4_XEN,
};

static const uint8_t enableInterrupts[] =
{
   CTRL_REG3_ADDR,
   REG3_DR_EN | REG3_IEA | REG3_IEL | REG3_INT1_EN,
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
   }

   fx3_suspendTask(20);       // wake-up time: 10 ms

   if (STATUS_OK == status);
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) enableMultibyteAutoincrement, sizeof(enableMultibyteAutoincrement), &xmit);
      deselectChip();

      bsp_delay(8);
   }

   if (STATUS_OK == status);
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) enable2gScale, sizeof(enable2gScale), &xmit);
      deselectChip();

      bsp_delay(8);
   }

   if (STATUS_OK == status);
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) setSamplingRateAndEnable, sizeof(setSamplingRateAndEnable), &xmit);
      deselectChip();
   }

   if (STATUS_OK == status);
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) enableInterrupts, sizeof(enableInterrupts), &xmit);
      deselectChip();
   }

   /*
    * check settings
    */
   if (STATUS_OK == status)
   {
      const uint8_t cmd = READ_REGISTER_CMD | CTRL_REG4_ADDR;

      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

      uint8_t configuredRegisters[7] = { 0 };

      if (STATUS_OK == status)
      {
         status = spi_read(&LIS3DSH_BUS, configuredRegisters, sizeof(configuredRegisters), &xmit);

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

enum Status LIS3DSH_getChipId(uint32_t* expectedId, uint32_t* actualId)
{
   *expectedId = 0x37;
   *actualId   = 0xff;

   enum Status status = reserveBus();
   if (STATUS_OK != status)
   {
      return status;
   }

   uint32_t xmit = 0;

   const uint8_t cmd = READ_REGISTER_CMD | MULTIPLE_SELECT | REG_INFO1;

   uint8_t rawData[3] = { 0 };

   selectChip();
   status = spi_write(&LIS3DSH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

   if (STATUS_OK == status)
   {
      status = spi_read(&LIS3DSH_BUS, rawData, sizeof(rawData), &xmit);
   }

   deselectChip();
   releaseBus();

   if (STATUS_OK == status)
   {
      *actualId =
         (((uint32_t) rawData[0]) << 16) |
         (((uint32_t) rawData[1]) << 8) |
         ((uint32_t) rawData[2]);
   }

   return status;
}

enum Status LIS3DSH_getSensitivity(uint8_t* sensitivity)
{
   *sensitivity = 2;
   return STATUS_OK;
}

enum Status LIS3DSH_getRawCounts(uint8_t* dataStatus, struct LIS3DSH_rawData* rawData)
{
   enum Status status = reserveBus();
   if (STATUS_OK == status)
   {
      uint32_t xmit = 0;

      const uint8_t cmd = READ_REGISTER_CMD | STATUS;

      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

      uint8_t data[7] = { 0 };

      if (STATUS_OK == status)
      {
         status = spi_read(&LIS3DSH_BUS, data, sizeof(data), &xmit);
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

static enum Status getRawCounts(struct LIS3DSH_rawData* rawData)
{
   uint32_t xmit = 0;

   const uint8_t cmd = READ_REGISTER_CMD | OUT_X_L_ADDR;

   selectChip();
   enum Status status = spi_write(&LIS3DSH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

   uint8_t data[6] = { 0 };

   if (STATUS_OK == status)
   {
      status = spi_read(&LIS3DSH_BUS, data, sizeof(data), &xmit);
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

void LIS3DSH_computeAcceleration(const struct LIS3DSH_rawData* rawData, uint32_t dataSize, uint8_t sensitivity, struct acceleration* accel)
{
   if (dataSize)
   {
      const float sensitivityAdjustment = (32768.0f / (float) (sensitivity));

      /*
       * we're averaging up to 32 int16_t values; we can add them all up,
       * without fear of overflow
       */
      int32_t sumOfX = 0;
      int32_t sumOfY = 0;
      int32_t sumOfZ = 0;

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

static const uint8_t enableFIFO[] =
{
   CTRL_REG6_ADDR,
   REG6_FIFO_EN | REG6_IF_ADD_INC,
};

static const uint8_t setFIFOMode[] =
{
   FIFO_CTRL_REG,
   FIFO_CTRL_CIRCULAR,
};

static const uint8_t disableFIFO[] =
{
   CTRL_REG6_ADDR,
   REG6_FIFO_EN | REG6_IF_ADD_INC,
};

enum Status LIS3DSH_enableFIFO(void)
{
   enum Status status = reserveBus();

   uint32_t xmit = 0;

   if (STATUS_OK == status)
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) enableFIFO, sizeof(enableFIFO), &xmit);
      deselectChip();

      if (STATUS_OK == status)
      {
         selectChip();
         status = spi_write(&LIS3DSH_BUS, (uint8_t*) setFIFOMode, sizeof(setFIFOMode), &xmit);
         deselectChip();
      }

      releaseBus();
   }

   return status;
}

enum Status LIS3DSH_disableFIFO(void)
{
   uint32_t xmit = 0;

   enum Status status = reserveBus();

   if (STATUS_OK == status)
   {
      selectChip();
      status = spi_write(&LIS3DSH_BUS, (uint8_t*) disableFIFO, sizeof(disableFIFO), &xmit);
      deselectChip();

      releaseBus();
   }

   return status;
}

enum Status LIS3DSH_readFIFO(struct LIS3DSH_rawData* data, uint32_t capacity, uint32_t* valuesCount)
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
         status = spi_write(&LIS3DSH_BUS, (uint8_t*) &cmd, sizeof(cmd), &xmit);

         if (STATUS_OK == status)
         {
            status = spi_read(&LIS3DSH_BUS, &fifoSrc, sizeof(fifoSrc), &xmit);
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

