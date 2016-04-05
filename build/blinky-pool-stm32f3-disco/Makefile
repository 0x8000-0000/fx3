TARGET=blinky

ROOT_DIR:=../..

include $(ROOT_DIR)/tools/build/common_variables.mk
include $(ROOT_DIR)/source/boards/STM32F3DISCOVERY/board.mk
include $(ROOT_DIR)/source/kernel/kernel.mk

C_VPATH+=$(ROOT_DIR)/source/apps/blinky-pool

INCLUDES+=-I../common-config

OBJECTS+=blinky-pool.o

include $(ROOT_DIR)/tools/build/common_rules.mk



