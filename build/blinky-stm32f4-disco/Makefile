TARGET=blinky

ROOT_DIR:=../..

include $(ROOT_DIR)/tools/build/common_variables.mk
include $(ROOT_DIR)/source/boards/STM32F4DISCOVERY/board.mk
include $(ROOT_DIR)/source/kernel/kernel.mk

C_VPATH+=$(ROOT_DIR)/source/apps/blinky

INCLUDES+=-I../common-config
CFLAGS+=-DTEST_TIMER_WRAP

OBJECTS+=blinky.o

include $(ROOT_DIR)/tools/build/common_rules.mk



