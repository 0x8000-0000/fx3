/**
 * @file test_LIS3DH.c
 * @brief Test LIS3DH driver
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
#include <spi.h>

#include <LIS3DH.h>
#include <mems.h>

static const struct USARTConfiguration usartConfig =
{
   .baudRate    = 115200,
   .flowControl = USART_FLOW_CONTROL_NONE,
   .bits        = 8,
   .parity      = USART_PARITY_NONE,
   .stopBits    = 1,
};

static const struct SPIConfiguration spiConfig =
{
   .speed      = 400 * 1000,        // 400 kHz
};

#define DISPLAY_RAW_VALUES 1

static const uint8_t APP_BANNER[] = "Test LIS3DH application\r\n"
#ifdef DISPLAY_RAW_VALUES
   "s,rx,ry,rz,"
#endif
   "x,y,z"
#ifdef COMPUTE_TILT
   ",pitch,roll"
#endif
   "\r\n";

static char outBuffer[80];

static volatile uint32_t expectedChipId;
static volatile uint32_t actualChipId;

static uint8_t sensitivity;
static uint8_t dataStatus;

#define USE_LIS3DH_FIFO
#define COMPUTE_TILT

static struct LIS3DH_rawData rawAccel[LIS3DH_FIFO_SIZE];
static uint32_t valueCount;

static struct acceleration accel;
#ifdef COMPUTE_TILT
static struct tilt tilt;
#endif

static void testHandler(const void* arg)
{
   struct USARTHandle* usart = (struct USARTHandle*) arg;

   uint32_t bytesWritten = 0;
   enum Status status = usart_write(usart, APP_BANNER, sizeof(APP_BANNER) - 1, &bytesWritten);
   assert(STATUS_OK == status);
   assert((sizeof(APP_BANNER) - 1) == bytesWritten);

   status = LIS3DH_initialize();

   status = LIS3DH_getChipId((uint32_t*) &expectedChipId, (uint32_t*) &actualChipId);

   LIS3DH_getSensitivity(&sensitivity);

#ifdef USE_LIS3DH_FIFO
   status = LIS3DH_enableFIFO();
   fx3_suspendTask(200);   // give us time to read something
#endif

   while (true)
   {
      if (STATUS_OK == status)
      {
         valueCount = 1;

#ifdef USE_LIS3DH_FIFO
         memset(rawAccel, 0, sizeof(rawAccel));
         status = LIS3DH_readFIFO(rawAccel, LIS3DH_FIFO_SIZE, &valueCount);
         assert(valueCount <= LIS3DH_FIFO_SIZE);
#else
         status = LIS3DH_getRawCounts(&dataStatus, rawAccel);
#endif

         if (STATUS_OK == status)
         {
            LIS3DH_computeAcceleration(rawAccel, valueCount, sensitivity, &accel);

            int len = 0;

#ifndef USE_LIS3DH_FIFO
            struct LIS3DH_rawData* data = &rawAccel[0];
#ifdef DISPLAY_RAW_VALUES
            len = snprintf(outBuffer, sizeof(outBuffer), "%u,%02x,%d,%d,%d", sensitivity, dataStatus, (int) data->x, (int) data->y, (int) data->z);
            status = usart_write(usart, (const uint8_t*) outBuffer, (uint32_t) len, &bytesWritten);
            assert(STATUS_OK == status);
            assert((uint32_t) len == bytesWritten);
#endif
#endif

#ifdef COMPUTE_TILT
            computeTilt(&accel, &tilt);
            len = snprintf(outBuffer, sizeof(outBuffer), ",%9.5f,%9.5f", tilt.pitch_deg, tilt.roll_deg);
            status = usart_write(usart, (const uint8_t*) outBuffer, (uint32_t) len, &bytesWritten);
            assert(STATUS_OK == status);
            assert((uint32_t) len == bytesWritten);
#endif

            len = snprintf(outBuffer, sizeof(outBuffer), "%9.7f,%9.7f,%9.7f\r\n", accel.x_g, accel.y_g, accel.z_g);
            status = usart_write(usart, (const uint8_t*) outBuffer, (uint32_t) len, &bytesWritten);
            assert(STATUS_OK == status);
            assert((uint32_t) len == bytesWritten);

            bsp_toggleLED(LED_ID_GREEN);
         }
      }
      else
      {
         bsp_toggleLED(LED_ID_RED);
      }

      fx3_suspendTask(200);
   }
}

static uint8_t testStack[2048] __attribute__ ((aligned (16)));

extern struct USARTHandle CONSOLE_USART;

extern struct SPIBus LIS3DH_BUS;

static const struct task_config testConfig =
{
   .name            = "Test MPU-6050",
   .handler         = testHandler,
   .argument        = &CONSOLE_USART,
   .priority        = 4,
   .stackBase       = testStack,
   .stackSize       = sizeof(testStack),
   .timeSlice_ticks = 0,
};

static struct task_control_block testTCB;

int main(void)
{
   bsp_initialize();
   usart_initialize(&CONSOLE_USART, &usartConfig);
   spi_initialize(&LIS3DH_BUS, &spiConfig);

   fx3_initialize();

   fx3_createTask(&testTCB, &testConfig);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}

