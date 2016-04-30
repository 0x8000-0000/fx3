# @file Makefile
# @brief Build file fragment for ARM Cortex-M4 MCUs
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

ARM_CORTEX_M4_FLAGS:=-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mno-unaligned-access

ARM_CORTEX_M4_CFLAGS:=$(ARM_CORTEX_M4_FLAGS) -D__CORTEX_M4 -D__USE_CMSIS
ARM_CORTEX_M4_AFLAGS:=$(ARM_CORTEX_M4_FLAGS)
ARM_CORTEX_M4_LFLAGS:=$(ARM_CORTEX_M4_FLAGS)

