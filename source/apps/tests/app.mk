# @file Makefile
# @brief Build file fragment for hardware test apps
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


APP_CHARGEN_TARGET:=chargen

APP_CHARGEN_OBJECTS:=chargen.o

APP_CHARGEN_C_VPATH:=source/apps/tests


APP_HELLO_WORLD_TARGET:=hello

APP_HELLO_WORLD_OBJECTS:=hello_world.o

APP_HELLO_WORLD_C_VPATH:=source/apps/tests


APP_ECHO_TARGET:=echo

APP_ECHO_OBJECTS:=echo.o

APP_ECHO_C_VPATH:=source/apps/tests


APP_TEST_LISTS_TARGET:=test_lists

APP_TEST_LISTS_OBJECTS:=test_lists.o

APP_TEST_LISTS_C_VPATH:=source/apps/tests


APP_MPU_6050_TARGET:=mpu_6050

APP_MPU_6050_OBJECTS:=test_mpu_6050.o MPU-6050.o

APP_MPU_6050_C_VPATH:=source/apps/tests


APP_DS3231M_TARGET:=ds3231m

APP_DS3231M_OBJECTS:=test_ds3231m.o DS3231M.o

APP_DS3231M_C_VPATH:=source/apps/tests


APP_BMP085_TARGET:=bmp085

APP_BMP085_OBJECTS:=test_bmp085.o BMP085.o

APP_BMP085_C_VPATH:=source/apps/tests
