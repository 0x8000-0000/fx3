/**
 * @file board_local.h
 * @brief Board Support Package interface
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

#ifndef __BOARD_LOCAL_H__
#define __BOARD_LOCAL_H__

#include <stdint.h>
#include <stdbool.h>

#include <synchronization.h>

#include <circular_buffer.h>

#include <stm32f4xx.h>

static inline uint32_t bsp_getTicksForMS(uint32_t time_ms)
{
   return 2 * time_ms;
}

static inline uint32_t bsp_getTimestamp_ticks(void)
{
   return TIM2->CNT;
}

static inline uint32_t bsp_computeInterval_ticks(uint32_t start_ticks, uint32_t end_ticks)
{
   uint32_t duration_ticks = 0;
   if (end_ticks >= start_ticks)
   {
      duration_ticks = end_ticks - start_ticks;
   }
   else
   {
#ifdef TEST_TIMER_WRAP
      duration_ticks = (0x10000 + end_ticks) - start_ticks;
#else
      duration_ticks = end_ticks + 1 + (0xffffffff - start_ticks);
#endif
   }
   return duration_ticks;
}

static inline void bsp_scheduleContextSwitch(void)
{
   SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Set PendSV to pending
   __ISB();
}

static inline bool bsp_computeWakeUp_ticks(uint32_t duration_ticks, uint32_t* wakeupAt_ticks)
{
   uint32_t timestamp_ticks = bsp_getTimestamp_ticks();
#ifdef TEST_TIMER_WRAP
   *wakeupAt_ticks = (timestamp_ticks + duration_ticks) & 0xffff;
#else
   *wakeupAt_ticks = (timestamp_ticks + duration_ticks);
#endif
   return *wakeupAt_ticks < timestamp_ticks;
}

enum BOARD_LED
{
   LED_ID_GREEN,
   LED_ID_ORANGE,
   LED_ID_RED,
   LED_ID_BLUE,

   LED_COUNT,
};

struct USARTHandle
{
   UART_HandleTypeDef         huart;
   IRQn_Type                  uartIRQ;

   DMA_HandleTypeDef          transmitDMA;
   IRQn_Type                  transmitDMAIRQ;
   uint32_t                   transmitDMAChannel;
   struct CircularBuffer      transmitBuffer;
   bool                       transmitBufferIsFull;

   uint32_t                   currentTransmitTail;
   bool                       transmitInProgress;

   struct
   {
      uint32_t                started;
      uint32_t                completed;
   }  transmitStatus;

   DMA_HandleTypeDef          receiveDMA;
   IRQn_Type                  receiveDMAIRQ;
   uint32_t                   receiveDMAChannel;
   struct CircularBuffer      receiveBuffer;
   struct semaphore           receiveBufferNotEmpty;
   uint32_t                   receiveBufferOverflow;
   bool                       readerIsWaiting;
};

#define CONSOLE_USART usart2

#endif // __BOARD_LOCAL_H__

