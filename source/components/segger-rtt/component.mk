# @file component.mk
# @brief Component definition for Segger RTT and SystemView
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

COMPONENT_SEGGER_RTT_INCLUDES:=\
	-I$(SEGGER)/SEGGER \
	-I$(SEGGER)/Config

COMPONENT_SEGGER_RTT_CFLAGS:=-DFX3_RTT_TRACE -DUSE_CYCCNT_TIMESTAMP

COMPONENT_SEGGER_RTT_C_VPATH:=\
	$(SEGGER)/SEGGER \
	source/components/segger-rtt/src

COMPONENT_SEGGER_RTT_OBJECTS:=\
	SEGGER_RTT.o \
	SEGGER_SYSVIEW.o \
	SEGGER_SYSVIEW_Config_FX3.o
