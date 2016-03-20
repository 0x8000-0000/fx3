#
# Board: STM32F4DISCOVERY (with 407)
# http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/PF252419?sc=internet/evalboard/product/252419.jsp
#
# Note, there is an older STM32F4DISCOVERY using 401:
# http://www.st.com/web/catalog/tools/FM116/CL1620/SC959/SS1532/LN1848/PF259098
# The peripherals are different, but more notably it has only 256KB of
# flash and 64KB of SRAM (1/4 and 1/3 respectively)
#

# MCU model: STM32F407VGT6
MCU=STM32F407xx

include $(ROOT_DIR)/source/chips/cortex-m4.mk

BOARD_DIR:=$(ROOT_DIR)/source/boards/STM32F4DISCOVERY

CHIP_DIR:=$(ROOT_DIR)/source/chips/stm32fxxx

CFLAGS+=-D$(MCU) $(CFLAGS_ARCH) -DUSE_HAL_DRIVER
AFLAGS+=$(AFLAGS_ARCH) -D__HEAP_SIZE=1024
LFLAGS+=-T $(BOARD_DIR)/linker/gcc/STM32F407XX.ld \
	$(LFLAGS_ARCH)

INCLUDES+=\
	-I$(ROOT_DIR)/source/boards/inc \
	-I$(BOARD_DIR)/inc \
	-I$(CHIP_DIR)/inc \
	-I$(STM32F4HAL)/CMSIS/Device/ST/STM32F4xx/Include \
	-I$(CMSIS)/Include \
	-I$(STM32F4CUBE)/Drivers/STM32F4xx_HAL_Driver/Inc

C_VPATH+=$(BOARD_DIR)/src \
	$(CHIP_DIR)/src \
	$(STM32F4CUBE)/Drivers/STM32F4xx_HAL_Driver/Src

S_VPATH+=$(BOARD_DIR)/src/gcc

OBJECTS+=board.o \
			system_stm32f4xx.o startup_stm32f407xx.o \
			stm32_chp.o \
			stm32f4xx_hal.o \
			stm32f4xx_hal_cortex.o \
			stm32f4xx_hal_pwr.o \
			stm32f4xx_hal_rcc.o \
			stm32f4xx_hal_gpio.o
