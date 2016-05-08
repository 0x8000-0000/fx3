# @file Makefile
# @brief Master build file for FX3 and apps
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

.DEFAULT_GOAL=artifacts

ifndef COMPILER
	COMPILER:=gcc
endif

ifeq ($(RELEASE),true)
	FLAVOR:=.rel
else
	FLAVOR:=
endif

include tools/build/compiler-$(COMPILER).mk

.SUFFIXES:                                      # Delete the default suffixes
.SUFFIXES: .c .cpp .o .elf .map .img            # Define our suffix list

include config.mk

include source/kernel/kernel.mk

CHIP_FRAGMENTS:=$(wildcard source/chips/*/chip.mk)

include $(CHIP_FRAGMENTS)

BOARD_FRAGMENTS:=$(wildcard source/boards/*/board.mk)

include $(BOARD_FRAGMENTS)

APP_FRAGMENTS:=$(wildcard source/apps/*/app.mk)

include $(APP_FRAGMENTS)

COMPONENT_FRAGMENTS:=$(wildcard source/components/*/component.mk)

include $(COMPONENT_FRAGMENTS)

MYPATH=$(dir $(lastword $(MAKEFILE_LIST)))

#
# Arguments:
#    1: Application name
#    2: Board name
#    3: Components, separated by space
#
define TARGET_template=

$(1)_$(2)_OBJDIR:=$(MYPATH)obj.$(COMPILER)$(FLAVOR)

$$($(1)_$(2)_OBJDIR):
	mkdir -p $$($(1)_$(2)_OBJDIR)

TARGETS+=$$($(1)_$(2)_OBJDIR)/$$(APP_$(1)_TARGET).img $$($(1)_$(2)_OBJDIR)/tags

DIR_TO_CLEAN+=$$($(1)_$(2)_OBJDIR)

$(1)_$(2)_OBJ_LIST:=$(APP_$(1)_OBJECTS) $(BOARD_$(2)_OBJECTS) $(FX3_OBJECTS) $(foreach comp,$(3),$(COMPONENT_$(comp)_OBJECTS))

$(1)_$(2)_PREC_LIST:=$$($(1)_$(2)_OBJ_LIST:.o=.i)

$(1)_$(2)_OBJECTS:=$$(addprefix $$($(1)_$(2)_OBJDIR)/,$$($(1)_$(2)_OBJ_LIST))

$(1)_$(2)_PREC_FILES:=$$(addprefix $$($(1)_$(2)_OBJDIR)/,$$($(1)_$(2)_PREC_LIST))

$$($(1)_$(2)_OBJECTS): CFLAGS=$(foreach comp,$(3),$$(COMPONENT_$(comp)_CFLAGS)) $$(BOARD_$(2)_CFLAGS) $(COMPILER_CFLAGS) $$(BOARD_$(2)_INCLUDES) $(FX3_INCLUDES) $(foreach comp,$(3),$$(COMPONENT_$(comp)_INCLUDES)) -Ibuild/common-config

$$($(1)_$(2)_PREC_FILES): CFLAGS=$(foreach comp,$(3),$$(COMPONENT_$(comp)_CFLAGS)) $$(BOARD_$(2)_CFLAGS) $(COMPILER_CFLAGS) $$(BOARD_$(2)_INCLUDES) $(FX3_INCLUDES) $(foreach comp,$(3),$$(COMPONENT_$(comp)_INCLUDES)) -Ibuild/common-config

$$($(1)_$(2)_OBJECTS): AFLAGS=$$(BOARD_$(2)_AFLAGS) $(COMPILER_AFLAGS)

$$($(1)_$(2)_OBJDIR)/$$(APP_$(1)_TARGET).elf: LFLAGS=$$(BOARD_$(2)_LFLAGS) $(COMPILER_LFLAGS) -Wl,-Map=$$($(1)_$(2)_OBJDIR)/$$(APP_$(1)_TARGET).map

$$($(1)_$(2)_OBJDIR)/$$(APP_$(1)_TARGET).elf: $$($(1)_$(2)_OBJECTS)
ifeq ($(VERBOSE),true)
	$(LD) $$(LFLAGS) -o $$@ $$^ $$(LDLIBS)
else
	@echo LD $$@
	@$(LD) $$(LFLAGS) -o $$@ $$^ $$(LDLIBS)
endif
	@$(SIZE) $$@

$$($(1)_$(2)_OBJDIR)/$$(APP_$(1)_TARGET).img: $$($(1)_$(2)_OBJDIR)/$$(APP_$(1)_TARGET).elf
	@echo IMG $$@
	@$(OBJDUMP) $$< > $$@

$$($(1)_$(2)_OBJDIR)/tags: $$($(1)_$(2)_OBJDIR)/$$(APP_$(1)_TARGET).elf
	@echo tags $$($(1)_$(2)_OBJDIR)
	@cat $$($(1)_$(2)_OBJDIR)/*.d | tr " " "\n" | grep ".h$$$$" | sort | uniq > $$($(1)_$(2)_OBJDIR)/headers.list
	@cat $$($(1)_$(2)_OBJDIR)/sources.list $$($(1)_$(2)_OBJDIR)/headers.list | ctags -f $$(abspath $$@) --tag-relative=no -L -

$$($(1)_$(2)_OBJDIR)/%.o: %.S | $$($(1)_$(2)_OBJDIR)
ifeq ($(VERBOSE),true)
	$(AS) $$(AFLAGS) -c -o $$@ $$<
else
	@echo AS $$(<F)
	@$(AS) $$(AFLAGS) -c -o $$@ $$<
endif

$$($(1)_$(2)_OBJDIR)/%.o: %.c | $$($(1)_$(2)_OBJDIR)
ifeq ($(VERBOSE),true)
	$(CC) $$(CFLAGS) -c -o $$@ $$<
else
	@echo CC $$(<F)
	@$(CC) $$(CFLAGS) -c -o $$@ $$<
endif
	@echo $$< >> $$($(1)_$(2)_OBJDIR)/sources.list

$$($(1)_$(2)_OBJDIR)/%.i: %.c | $$($(1)_$(2)_OBJDIR)
	$(CC) $$(CFLAGS) -E -o $$@ $$<

vpath %.c \
	$(APP_$(1)_C_VPATH) \
	$(BOARD_$(2)_C_VPATH) \
	$(FX3_C_VPATH) \
	$(foreach comp,$(3),$(COMPONENT_$(comp)_C_VPATH))

vpath %.S \
	$(BOARD_$(2)_S_VPATH) \
	$(FX3_S_VPATH)

$(1)_$(2)_DEPS:=$$($(1)_$(2)_OBJECTS:.o=.d)
-include $$($(1)_$(2)_DEPS)

endef

TARGET_FRAGMENTS:=$(wildcard build/*/target.mk)

include $(TARGET_FRAGMENTS)

artifacts: $(TARGETS)

clean:
	$(RM) -r $(DIR_TO_CLEAN)

.PHONY: all clean
