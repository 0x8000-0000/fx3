/**
 * @file power.h
 * @brief Power management interface
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

#ifndef __POWER_H__
#define __POWER_H__

enum PowerMode
{
   POWER_MODE_OFF,            /// device is not powered
   POWER_MODE_SLEEP,          /// device configuration maintained only
   POWER_MODE_STANDBY,        /// interrupts are enabled; device can wake-up
   POWER_MODE_OPERATIONAL,    /// device is fully powered and operational
};

#endif // __POWER_H__

