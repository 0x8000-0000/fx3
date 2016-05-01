#
# Board: EXP-STM32F429II (with 429)
# https://www.iar.com/globalassets/iar-embedded-workbench/add-ons-and-integrations/kits/stm32f429ii-exp_rev_a.pdf
#
#

# MCU model: STM32F429IIT6
BOARD_EXP_STM32F429II_MCU=STM32F429xx

BOARD_EXP_STM32F429II_DIR:=source/boards/EXP-STM32F429II

BOARD_EXP_STM32F429II_INCLUDES:=\
	-Isource/arch/inc \
	-Isource/boards/inc \
	-I$(BOARD_EXP_STM32F429II_DIR)/inc \
	-I$(CHIP_STM32FXX_DIR)/inc \
	-I$(STM32F4HAL)/CMSIS/Device/ST/STM32F4xx/Include \
	-I$(CMSIS)/Include \
	-I$(STM32F4CUBE)/Drivers/STM32F4xx_HAL_Driver/Inc

BOARD_EXP_STM32F429II_CFLAGS:=$(CHIP_STM32FXX_CFLAGS) -D$(BOARD_EXP_STM32F429II_MCU) -DUSE_HAL_DRIVER -DCAN_SLEEP_UNDER_DEBUGGER
BOARD_EXP_STM32F429II_AFLAGS:=$(CHIP_STM32FXX_AFLAGS) -D__HEAP_SIZE=1024
BOARD_EXP_STM32F429II_LFLAGS:=$(CHIP_STM32FXX_LFLAGS) -T $(BOARD_EXP_STM32F429II_DIR)/linker/gcc/$(BOARD_EXP_STM32F429II_MCU).ld

BOARD_EXP_STM32F429II_C_VPATH:=\
	$(BOARD_EXP_STM32F429II_DIR)/src \
	$(CHIP_STM32FXX_DIR)/src \
	$(STM32F4CUBE)/Drivers/STM32F4xx_HAL_Driver/Src

BOARD_EXP_STM32F429II_S_VPATH:=\
	source/arch/cortex-m4/gcc \
	$(BOARD_EXP_STM32F429II_DIR)/src/gcc

BOARD_EXP_STM32F429II_OBJECTS:=\
	$(CHIP_STM32FXX_OBJECTS) \
	board_exp_stm32f429.o \
	bitops.o \
	system_stm32f4xx.o \
	stm32f4xx_hal.o \
	stm32f4xx_hal_cortex.o \
	stm32f4xx_hal_pwr.o \
	stm32f4xx_hal_rcc.o \
	stm32f4xx_hal_gpio.o \
	startup_stm32f429xx.o

