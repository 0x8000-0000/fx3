# @file Makefile
# @brief Common makefile fragment
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

SPACE:=
SPACE+=
TARGET_DIR:=$(lastword $(subst /,$(SPACE),$(CURDIR)))

ifndef COMPILER
	COMPILER:=gcc
endif

ifeq ($(RELEASE),true)
	FLAVOR:=.rel
else
	FLAVOR:=
endif

.SUFFIXES:                                      # Delete the default suffixes
.SUFFIXES: .img                                 # Define our suffix list

APP_FRAGMENTS:=$(wildcard ../../source/apps/*/app.mk)

include $(APP_FRAGMENTS)

local_artifacts: tags obj.$(COMPILER)$(FLAVOR)/$(APP_$(TARGET_APP)_TARGET).img

obj.$(COMPILER)$(FLAVOR)/$(APP_$(TARGET_APP)_TARGET).img: force
	@$(MAKE) --no-print-directory -C ../.. build/$(TARGET_DIR)/obj.$(COMPILER)$(FLAVOR)/$(APP_$(TARGET_APP)_TARGET).img

tags: force
	@$(MAKE) --no-print-directory -C ../.. build/$(TARGET_DIR)/tags

clean:
	@$(RM) -r obj.$(COMPILER)$(FLAVOR)

.PHONY: clean force local_artifacts
