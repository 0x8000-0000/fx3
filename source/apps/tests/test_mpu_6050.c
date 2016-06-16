/**
 * @file test_mpu_6050.c
 * @brief Test MPU-6050 driver
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
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <board.h>
#include <task.h>

#include <usart.h>
#include <i2c.h>

#include <MPU-6050.h>

static const struct USARTConfiguration usartConfig =
{
   .baudRate    = 115200,
   .flowControl = USART_FLOW_CONTROL_NONE,
   .bits        = 8,
   .parity      = USART_PARITY_NONE,
   .stopBits    = 1,
};

static const struct I2CConfiguration i2cConfig =
{
   .speed      = 400 * 1000,        // 400 kHz
};

static const uint8_t APP_BANNER[] = "Test MPU-6050 application\r\n";

static char outBuffer[80];

struct acceleration accel;
struct rotation     gyro;

static void testMPU6050(const void* arg)
{
   struct USARTHandle* usart = (struct USARTHandle*) arg;

   uint32_t bytesWritten = 0;
   enum Status status = usart_write(usart, APP_BANNER, sizeof(APP_BANNER) - 1, &bytesWritten);
   assert(STATUS_OK == status);
   assert((sizeof(APP_BANNER) - 1) == bytesWritten);

   status = mpu6050_initialize();

   while (true)
   {
      if (STATUS_OK == status)
      {
         status = mpu6050_getAcceleration(&accel);

         if (STATUS_OK == status)
         {
            int len = snprintf(outBuffer, sizeof(outBuffer), "Accel[XYZ]: (%6.4f, %6.4f, %6.4f)", accel.x_g, accel.y_g, accel.z_g);
            status = usart_write(usart, (const uint8_t*) outBuffer, (uint32_t) len, &bytesWritten);
            assert(STATUS_OK == status);
            assert((uint32_t) len == bytesWritten);

            status = mpu6050_getRotation(&gyro);
            if (STATUS_OK == status)
            {
               len = snprintf(outBuffer, sizeof(outBuffer), "   Gyro[XYZ]: (%8.2f, %8.2f, %8.2f)\r\n", gyro.x_deg, gyro.y_deg, gyro.z_deg);
               status = usart_write(usart, (const uint8_t*) outBuffer, (uint32_t) len, &bytesWritten);
               assert(STATUS_OK == status);
               assert((uint32_t) len == bytesWritten);

               bsp_toggleLED(LED_ID_GREEN);
            }
         }
      }
      else
      {
         bsp_toggleLED(LED_ID_RED);
      }

      fx3_suspendTask(1000);
   }
}

static uint8_t mpu6050TestStack[2048] __attribute__ ((aligned (16)));

extern struct USARTHandle CONSOLE_USART;

extern struct I2CHandle MPU_6050_BUS;

static const struct task_config mpu6050TestConfig =
{
   .name            = "Test MPU-6050",
   .handler         = testMPU6050,
   .argument        = &CONSOLE_USART,
   .priority        = 4,
   .stackBase       = mpu6050TestStack,
   .stackSize       = sizeof(mpu6050TestStack),
   .timeSlice_ticks = 0,
};

static struct task_control_block mpu6050TestTCB;

int main(void)
{
   bsp_initialize();
   usart_initialize(&CONSOLE_USART, &usartConfig);
   i2c_initialize(&MPU_6050_BUS, &i2cConfig);

   fx3_initialize();

   fx3_createTask(&mpu6050TestTCB, &mpu6050TestConfig);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}

