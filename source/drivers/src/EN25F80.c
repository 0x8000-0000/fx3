/**
 * @file EN25F80.c
 * @brief Driver for Eon Silicon EN25F80 serial flash memory
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
 * Based on EN25F80 data sheet Rev. E, Issue Date: 2007/11/23
 */

#include <assert.h>
#include <board.h>

#include <EN25F80.h>

#include <spi.h>

extern struct SPIBus EN25F80_BUS;

static inline void selectChip(void)
{
   bsp_setOutputPin(EN25F80_CHIP_SELECT, false);
}

static inline void deselectChip(void)
{
   bsp_setOutputPin(EN25F80_CHIP_SELECT, true);     // chip-select high
}

static inline enum Status reserveBus(void)
{
   return spi_reserveBus(&EN25F80_BUS, false, false);
}

static inline void releaseBus(void)
{
   deselectChip();   // just in case
   spi_releaseBus(&EN25F80_BUS);
}

enum Status EN25F80_initialize(void)
{
   bsp_initializeOutputPin(EN25F80_CHIP_SELECT);
   return STATUS_OK;
}

enum Status EN25F80_getChipId(uint32_t* chipId)
{
   *chipId = 0;

   enum Status status = reserveBus();

   uint32_t xmit = 0;

   if (STATUS_OK == status)
   {
      uint8_t readIdCmd = 0x9f;
      selectChip();
      status = spi_write(&EN25F80_BUS, &readIdCmd, sizeof(readIdCmd), &xmit);

      if (STATUS_OK == status)
      {
         uint8_t rawResult[3] = { 0 };

         status = spi_read(&EN25F80_BUS, rawResult, sizeof(rawResult), &xmit);

         if (STATUS_OK == status)
         {
            *chipId = (((uint32_t) rawResult[0]) << 16) |
               (((uint32_t) rawResult[1]) << 8) |
               (((uint32_t) rawResult[2]));
         }
      }

      deselectChip();
      releaseBus();
   }

   return status;
}

enum Status EN25F80_releaseFromDeepSleep(void)
{
   uint32_t xmit = 0;
   enum Status status = reserveBus();

   if (STATUS_OK == status)
   {
      uint8_t readElectronicSignatureCmd = 0xab;
      selectChip();
      status = spi_write(&EN25F80_BUS, &readElectronicSignatureCmd, sizeof(readElectronicSignatureCmd), &xmit);

      if (STATUS_OK == status)
      {
         uint8_t signature[4] = { 0 };
         status = spi_read(&EN25F80_BUS, signature, 4, &xmit);
         assert(0x13 == signature[4]);
      }

      deselectChip();
      releaseBus();
   }

   return status;
}

enum Status EN25F80_eraseChip(void)
{
   reserveBus();
   releaseBus();
   return STATUS_NOT_IMPLEMENTED;
}

enum Status EN25F80_eraseSector(uint32_t address)
{
   reserveBus();
   releaseBus();
   return STATUS_NOT_IMPLEMENTED;
}

enum Status EN25F80_enableWrite(void)
{
   reserveBus();
   releaseBus();
   return STATUS_NOT_IMPLEMENTED;
}

enum Status EN25F80_disableWrite(void)
{
   reserveBus();
   releaseBus();
   return STATUS_NOT_IMPLEMENTED;
}

enum Status EN25F80_readByte(uint32_t address, uint8_t* value)
{
   reserveBus();
   releaseBus();
   return STATUS_NOT_IMPLEMENTED;
}

enum Status EN25F80_writeByte(uint32_t address, uint8_t value)
{
   reserveBus();
   releaseBus();
   return STATUS_NOT_IMPLEMENTED;
}

