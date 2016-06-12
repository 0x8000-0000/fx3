/**
 * @file heartbeat.c
 * @brief Blink an LED to indicate liveness
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

#include <board.h>
#include <task.h>

#ifndef HEARTBEAT_INTERVAL_MS
#define HEARTBEAT_INTERVAL_MS 500
#endif

static void blinkHeartbeatLED(const void* arg __attribute__((unused)))
{
   while (true)
   {
      bsp_turnOnLED(LED_ID_BLUE);

      fx3_suspendTask(HEARTBEAT_INTERVAL_MS);

      bsp_turnOffLED(LED_ID_BLUE);

      fx3_suspendTask(HEARTBEAT_INTERVAL_MS);
   }
}

static uint8_t heartbeatStack[128] __attribute__ ((aligned (16)));

static const struct task_config heartbeatTaskConfig =
{
   .name            = "Heartbeat",
   .handler         = blinkHeartbeatLED,
   .argument        = NULL,
   .priority        = 0xff00,
   .stackBase       = heartbeatStack,
   .stackSize       = sizeof(heartbeatStack),
   .timeSlice_ticks = 0,
};

static struct task_control_block heartbeatTCB;

void utl_startHeartbeat(void)
{
   fx3_createTask(&heartbeatTCB, &heartbeatTaskConfig);
}
