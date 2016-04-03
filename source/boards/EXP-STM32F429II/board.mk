#
# Board: EXP-STM32F429II (with 429)
# https://www.iar.com/globalassets/iar-embedded-workbench/add-ons-and-integrations/kits/stm32f429ii-exp_rev_a.pdf
#
#

# MCU model: STM32F429IIT6
MCU=STM32F429xx

include $(ROOT_DIR)/source/chips/cortex-m4.mk

BOARD_DIR:=$(ROOT_DIR)/source/boards/EXP-STM32F429II

CHIP_DIR:=$(ROOT_DIR)/source/chips/stm32fxxx

CFLAGS+=-D$(MCU) $(CFLAGS_ARCH) -DUSE_HAL_DRIVER
AFLAGS+=$(AFLAGS_ARCH) -D__HEAP_SIZE=1024
LFLAGS+=-T $(BOARD_DIR)/linker/gcc/$(MCU).ld \
	$(LFLAGS_ARCH)

INCLUDES+=\
	-I$(ROOT_DIR)/source/arch/inc \
	-I$(ROOT_DIR)/source/boards/inc \
	-I$(BOARD_DIR)/inc \
	-I$(CHIP_DIR)/inc \
	-I$(STM32F4HAL)/CMSIS/Device/ST/STM32F4xx/Include \
	-I$(CMSIS)/Include \
	-I$(STM32F4CUBE)/Drivers/STM32F4xx_HAL_Driver/Inc

C_VPATH+=$(BOARD_DIR)/src \
	$(CHIP_DIR)/src \
	$(STM32F4CUBE)/Drivers/STM32F4xx_HAL_Driver/Src

S_VPATH+= \
	$(ROOT_DIR)/source/arch/cortex-m4/gcc \
	$(BOARD_DIR)/src/gcc

OBJECTS+=board.o \
			bitops.o \
			system_stm32f4xx.o \
			stm32_chp.o \
			stm32f4xx_hal.o \
			stm32f4xx_hal_cortex.o \
			stm32f4xx_hal_pwr.o \
			stm32f4xx_hal_rcc.o \
			stm32f4xx_hal_gpio.o \
			startup_stm32f429xx.o

