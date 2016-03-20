TARGET=blinky

ROOT_DIR:=../..

include $(ROOT_DIR)/tools/build/common_variables.mk
include $(ROOT_DIR)/source/boards/STM32F4DISCOVERY/board.mk
include $(ROOT_DIR)/source/kernel/kernel.mk

C_VPATH+=$(ROOT_DIR)/source/apps/blinky

OBJECTS+=blinky.o

CFLAGS+=-DTEST_TIMER_WRAP

include $(ROOT_DIR)/tools/build/common_rules.mk



