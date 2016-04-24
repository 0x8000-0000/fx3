# @file Makefile
# @brief Build file fragment for STM32Fxx chips
# @author Florin Iucha <florin@signbit.net>
# @copyright Apache License, Version 2.0

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file is part of FX3 RTOS for ARM Cortex-M4

CHIP_STM32FXX_DIR:=source/chips/stm32fxxx

CHIP_STM32FXX_OBJECTS:=stm32_chp.o

CHIP_STM32FXX_CFLAGS:=$(ARM_CORTEX_M4_CFLAGS)
CHIP_STM32FXX_AFLAGS:=$(ARM_CORTEX_M4_AFLAGS)
CHIP_STM32FXX_LFLAGS:=$(ARM_CORTEX_M4_LFLAGS)
