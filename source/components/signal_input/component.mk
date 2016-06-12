# @file component.mk
# @brief Component definition for input signal debouncing
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

COMPONENT_SIGNAL_INPUT_INCLUDES:=\
	-Isource/components/signal_input/inc

#COMPONENT_SIGNAL_INPUT_CFLAGS:=

COMPONENT_SIGNAL_INPUT_C_VPATH:=\
	source/components/signal_input/src

COMPONENT_SIGNAL_INPUT_OBJECTS:=\
	input_debouncer.o

