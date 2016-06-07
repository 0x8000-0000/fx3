/**
 * @file stm32f4_driver_spi.c
 * @brief SPI driver implementation for STM32F4 chips
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

#include <assert.h>
#include <string.h>

#include <spi.h>

#include <board_local.h>

void spi_initialize(struct SPIBus* bus, const struct SPIConfiguration* config)
{
   (void) config;

   fx3_initializeSemaphore(&bus->isAvailable, 1);

   SPI_HandleTypeDef* handle = &bus->halHandle;

   handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
   handle->Init.Direction         = SPI_DIRECTION_2LINES;
   handle->Init.CLKPhase          = SPI_PHASE_1EDGE;
   handle->Init.CLKPolarity       = SPI_POLARITY_LOW;
   handle->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
   handle->Init.CRCPolynomial     = 7;
   handle->Init.DataSize          = SPI_DATASIZE_8BIT;
   handle->Init.FirstBit          = SPI_FIRSTBIT_MSB;
   handle->Init.NSS               = SPI_NSS_SOFT;
   handle->Init.TIMode            = SPI_TIMODE_DISABLED;
   handle->Init.Mode              = SPI_MODE_MASTER;

   HAL_StatusTypeDef status = HAL_SPI_Init(handle);
   assert(HAL_OK == status);
}

enum Status spi_reserveBus(struct SPIBus* bus, bool polarity, bool phase)
{
   fx3_waitOnSemaphore(&bus->isAvailable);

   SPI_HandleTypeDef* handle = &bus->halHandle;

   uint32_t desiredCLKPhase;
   uint32_t desiredCLKPolarity;

   if (phase)
   {
      desiredCLKPhase = SPI_PHASE_2EDGE;
   }
   else
   {
      desiredCLKPhase = SPI_PHASE_1EDGE;
   }
   if (polarity)
   {
      desiredCLKPolarity = SPI_POLARITY_HIGH;
   }
   else
   {
      desiredCLKPolarity = SPI_POLARITY_LOW;
   }

   if ((handle->Init.CLKPhase != desiredCLKPhase) || (handle->Init.CLKPolarity != desiredCLKPolarity))
   {
      HAL_StatusTypeDef status = HAL_SPI_DeInit(handle);
      if (HAL_OK != status)
      {
         goto failed;
      }

      handle->Init.CLKPhase    = desiredCLKPhase;
      handle->Init.CLKPolarity = desiredCLKPolarity;

      status = HAL_SPI_Init(handle);
      if (HAL_OK != status)
      {
         goto failed;
      }
   }

   return STATUS_OK;

failed:

   fx3_signalSemaphore(&bus->isAvailable);
   return STATUS_HARDWARE_CONFIGURATION_FAILED;
}

enum Status spi_releaseBus(struct SPIBus* bus)
{
   fx3_signalSemaphore(&bus->isAvailable);

   return STATUS_OK;
}

enum Status spi_read(struct SPIBus* bus, uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesRead)
{
   HAL_StatusTypeDef halStatus = HAL_SPI_Receive(&bus->halHandle, buffer, bufferSize, 128);
   if (HAL_OK == halStatus)
   {
      return STATUS_OK;
   }
   else
   {
      return STATUS_COMMUNICATION_FAILED;
   }
}

enum Status spi_write(struct SPIBus* bus, const uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesWritten)
{
   HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(&bus->halHandle, (uint8_t*) buffer, bufferSize, 128);
   if (HAL_OK == halStatus)
   {
      return STATUS_OK;
   }
   else
   {
      return STATUS_COMMUNICATION_FAILED;
   }
}

