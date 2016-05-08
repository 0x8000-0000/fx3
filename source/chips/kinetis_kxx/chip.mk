# @file Makefile
# @brief Build file fragment for Kinetis Kxy Cortex-M4 chips
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

CHIP_KINETIS_KXX_DIR:=source/chips/kinetis_kxx

CHIP_KINETIS_KXX_OBJECTS:=cortex_timer.o kinetis_chp.o

CHIP_KINETIS_KXX_CFLAGS:=$(ARM_CORTEX_M4_CFLAGS)
CHIP_KINETIS_KXX_AFLAGS:=$(ARM_CORTEX_M4_AFLAGS) -D__STARTUP_CLEAR_BSS
CHIP_KINETIS_KXX_LFLAGS:=$(ARM_CORTEX_M4_LFLAGS)

CHIP_KINETIS_KXX_C_VPATH:=\
	$(ARM_CORTEX_M4_C_VPATH) \
	$(CHIP_KINETIS_KXX_DIR)/src
