/**
 * @file board.h
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

#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdbool.h>
#include <stdint.h>

/** @addtogroup BSP
 * @{
 */

/** @addtogroup System
 * @{
 */

void bsp_initialize(void);

__attribute__((noreturn)) void bsp_reset(void);

void bsp_startMultitasking(uint32_t firstPSP, void (* firstHandler)(const void* arg), const void* firstArg);

//void bsp_scheduleContextSwitch(void);

void bsp_sleep(void);

// busy-loop
void bsp_delay(uint32_t loops);

/**
 * @}
 */

/** @addtogroup Input/Output
 * @{
 */

void bsp_turnOnLED(uint32_t ledId);

void bsp_turnOffLED(uint32_t ledId);

void bsp_toggleLED(uint32_t ledId);

void bsp_initializeOutputPin(uint32_t outputPin);

void bsp_setOutputPin(uint32_t outputPin, bool high);

/**
 * @}
 */

/** @addtogroup Clocks
 * @{
 */

/*
 * Declared as "static inline" for performance
 */
//uint32_t bsp_getTicksForMS(void);

//uint32_t bsp_getTimestamp_ticks(void);

uint64_t bsp_getTimestamp64_ticks(void);

/** "Blocks" until wakeup.
 *
 * @param duration how much to sleep, in ticks
 * @return duration slept (less than input, if other interrupt)
 */
uint32_t bsp_sleep_ticks(uint32_t duration_ticks);

void bsp_startMainClock(void);

/** Suspend the system timer interrupts
 */
void bsp_disableSystemTimer(void);

/** Start the system timer interrupts
 */
void bsp_enableSystemTimer(void);


/** Schedule a wake-up alarm at the given timestamp.
 *
 * @param timestamp_ticks indicates when to call bsp_onWokenUp
 */
void bsp_wakeUpAt_ticks(uint32_t timestamp_ticks);

/** Called by BSP on alarm
 *
 */
bool bsp_onWokenUp(void);

// Event
bool bsp_onEpochRollover(void);

/*
 * Round-robin support
 */
void bsp_requestRoundRobinSliceTimeout_ticks(uint32_t timestamp_ticks);

void bsp_cancelRoundRobinSliceTimeout(void);

// Event
bool bsp_onRoundRobinSliceTimeout(void);

/*
 * Debounce support
 */
void bsp_requestDebounceTimeout_ticks(uint32_t timestamp_ticks);

void bsp_cancelDebounceTimeout(void);

bool bsp_onDebounceIntervalTimeout(void);
/**
 * @}
 */

/**
 * @}
 */

#include <board_local.h>

#endif // __BOARD_H__

