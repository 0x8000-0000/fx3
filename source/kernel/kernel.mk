# @file kernel.mk
# @brief Build file fragment for FX3 kernel
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

FX3_C_VPATH:=\
	source/kernel/src \
	source/modules/src

FX3_S_VPATH:=\
	source/kernel/src/gcc

FX3_INCLUDES:=\
	-Isource/kernel/inc \
	-Isource/modules/inc

FX3_OBJECTS:=\
	priority_queue.o buffer.o synchronization.o \
	context_switch.o faults.o fx3.o fx3_cortex.o
