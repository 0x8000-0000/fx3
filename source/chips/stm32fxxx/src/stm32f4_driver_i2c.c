/**
 * @file stm32f4_driver_i2c.c
 * @brief I2C driver implementation for STM32F4 chips
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

#include <i2c.h>

#include <board_local.h>

void i2c_initialize(struct I2CHandle* handle, const struct I2CConfiguration* config)
{
   /*
    * force reset the I2C registers
    */
   volatile uint32_t* const CR1 = &(handle->hi2c.Instance->CR1);
   *CR1 |= 0x8000;
   *CR1 &= 0x7fff;

   /*
    * standard init
    */
   handle->hi2c.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
   handle->hi2c.Init.ClockSpeed      = config->speed;
   handle->hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
   handle->hi2c.Init.DutyCycle       = I2C_DUTYCYCLE_2;
   handle->hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
   handle->hi2c.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
   handle->hi2c.Init.OwnAddress1     = 0x0;  // not relevant for master
   handle->hi2c.Init.OwnAddress2     = 0x0;  // not relevant for master

   if (HAL_I2C_Init(&handle->hi2c) != HAL_OK)
   {
      assert(false);
   }

   fx3_initializeSemaphore(&handle->isAvailable, 1);

#if 0
   HAL_NVIC_SetPriority(handle->erIRQ, 1, 0);
   HAL_NVIC_EnableIRQ(handle->erIRQ);
   HAL_NVIC_SetPriority(handle->evIRQ, 2, 0);
   HAL_NVIC_EnableIRQ(handle->evIRQ);
#endif
}

void i2c_acquireBus(struct I2CHandle* handle)
{
   fx3_waitOnSemaphore(&handle->isAvailable);
}

void i2c_releaseBus(struct I2CHandle* handle)
{
   fx3_signalSemaphore(&handle->isAvailable);
}

enum Status i2c_readRegisters(struct I2CHandle* handle, uint16_t deviceAddress, uint16_t registerAddress, uint8_t* buffer, uint16_t bufferSize, uint16_t* bytesReceived)
{
   HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&handle->hi2c, (deviceAddress << 1), registerAddress, 1, buffer, bufferSize, 128);
   if (HAL_OK == status)
   {
      *bytesReceived = bufferSize;
      return STATUS_OK;
   }
   else
   {
      *bytesReceived = 0;
      return STATUS_COMMUNICATION_FAILED;
   }
}

enum Status i2c_writeRegisters(struct I2CHandle* handle, uint16_t deviceAddress, uint16_t registerAddress, const uint8_t* buffer, uint16_t bufferSize, uint16_t* bytesSent)
{
   HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&handle->hi2c, (deviceAddress << 1), registerAddress, 1, (uint8_t*) buffer, bufferSize, 128);
   if (HAL_OK == status)
   {
      *bytesSent = bufferSize;
      return STATUS_OK;
   }
   else
   {
      *bytesSent = 0;
      return STATUS_COMMUNICATION_FAILED;
   }
}


extern struct I2CHandle i2c1;

void I2C1_EV_IRQHandler(void)
{
   HAL_I2C_EV_IRQHandler(&i2c1.hi2c);
}

void I2C1_ER_IRQHandler(void)
{
   HAL_I2C_ER_IRQHandler(&i2c1.hi2c);
}
