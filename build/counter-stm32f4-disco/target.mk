# @file Makefile
# @brief Build file fragment for quadrature encoder test app on stm32f4-disco board
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

#
# Note: the STM32F4-Disovery board includes a ST/Link v2 debugger.
# To use Segger SystemView you need to change the firmware on the debugger
# to Segger for ST-Link: https://www.segger.com/jlink-st-link.html
#

$(eval $(call TARGET_template,TEST_ENCODER,STM32F4DISCOVERY,SEGGER_RTT SIGNAL_INPUT))

