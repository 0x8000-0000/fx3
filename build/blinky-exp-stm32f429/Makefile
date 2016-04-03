TARGET=blinky

ROOT_DIR:=../..

include $(ROOT_DIR)/tools/build/common_variables.mk
include $(ROOT_DIR)/source/boards/EXP-STM32F429II/board.mk
include $(ROOT_DIR)/source/kernel/kernel.mk

C_VPATH+=$(ROOT_DIR)/source/apps/blinky

INCLUDES+=-I../common-config

OBJECTS+=blinky.o

include $(ROOT_DIR)/tools/build/common_rules.mk



