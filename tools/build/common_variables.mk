include $(ROOT_DIR)/config.mk

ifndef COMPILER
	COMPILER:=gcc
endif

ifeq ($(RELEASE),true)
	FLAVOR:=.rel
else
	FLAVOR:=
endif

OBJDIR:=obj.$(COMPILER)$(FLAVOR)

include $(ROOT_DIR)/tools/build/compiler-$(COMPILER).mk

.DEFAULT_GOAL=artifacts

.SUFFIXES:                       # Delete the default suffixes
.SUFFIXES: .c .cpp .o .elf .map  # Define our suffix list

OPENOCD_BIN:=$(OPENOCD_HOME)$(SEP)bin$(SEP)openocd.exe -s $(OPENOCD_HOME)$(SEP)share$(SEP)openocd$(SEP)scripts
