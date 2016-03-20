C_VPATH+=$(ROOT_DIR)/source/kernel/src \
			$(ROOT_DIR)/source/modules/src

S_VPATH+=$(ROOT_DIR)/source/kernel/src/gcc

INCLUDES+=-I$(ROOT_DIR)/source/kernel/inc \
			-I$(ROOT_DIR)/source/modules/inc

OBJECTS+=priority_queue.o synchronization.o \
			context_switch.o faults.o fx3.o
