# @file Makefile
# @brief Build file fragment for GNU ARM Tools
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

CC:=arm-none-eabi-gcc
CXX:=arm-none-eabi-g++
AS:=arm-none-eabi-gcc
LD:=arm-none-eabi-g++
OBJDUMP:=arm-none-eabi-objdump -d
SIZE:=arm-none-eabi-size

ifeq ($(RELEASE),true)
	COMPILER_CFLAGS+=-Os
	COMPILER_CXXFLAGS+=-Os
else
	COMPILER_CFLAGS+=-g -Og
	COMPILER_CXXFLAGS+=-g -Og
	COMPILER_LFLAGS+=-g
	COMPILER_AFLAGS+=-g

ifeq ($(WINIDEA),true)
	COMPILER_CFLAGS+=-gdwarf-2 -gstrict-dwarf
else
	COMPILER_CFLAGS+=-gdwarf-4 -fvar-tracking -fvar-tracking-assignments
endif

endif

COMPILER_CFLAGS+=-Wall -std=c11 \
		-ffunction-sections \
		-pedantic -Werror \
		-Wno-unused-but-set-variable -Wno-unused-variable \
		-nostdlib -ffreestanding \
		-MMD
COMPILER_LFLAGS+=--specs=nosys.specs -Wl,-gc-sections -Wl,--cref
COMPILER_CXXFLAGS+=-Wall -std=c++14 -fno-exceptions -fno-rtti -Werror

