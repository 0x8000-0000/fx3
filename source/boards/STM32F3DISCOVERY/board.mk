#
# Board: STM32F3DISCOVERY
# http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/PF254044
#

# MCU model: STM32F303VCT6
MCU=STM32F303xC

include $(ROOT_DIR)/source/chips/cortex-m4.mk

BOARD_DIR:=$(ROOT_DIR)/source/boards/STM32F3DISCOVERY

CHIP_DIR:=$(ROOT_DIR)/source/chips/stm32fxxx

CFLAGS+=-D$(MCU) $(CFLAGS_ARCH) -DUSE_HAL_DRIVER
AFLAGS+=$(AFLAGS_ARCH) -D__HEAP_SIZE=1024
LFLAGS+=-T $(BOARD_DIR)/linker/gcc/STM32F303XC.ld \
	$(LFLAGS_ARCH)

INCLUDES+=\
	-I$(ROOT_DIR)/source/arch/inc \
	-I$(ROOT_DIR)/source/boards/inc \
	-I$(BOARD_DIR)/inc \
	-I$(CHIP_DIR)/inc \
	-I$(STM32F3HAL)/CMSIS/Device/ST/STM32F3xx/Include \
	-I$(CMSIS)/Include \
	-I$(STM32F3CUBE)/Drivers/STM32F3xx_HAL_Driver/Inc

C_VPATH+=$(BOARD_DIR)/src \
	$(CHIP_DIR)/src \
	$(STM32F3CUBE)/Drivers/STM32F3xx_HAL_Driver/Src

S_VPATH+= \
	$(ROOT_DIR)/source/arch/cortex-m4/gcc \
	$(BOARD_DIR)/src/gcc

OBJECTS+=board.o \
			bitops.o \
			system_stm32f3xx.o \
			stm32_chp.o \
			stm32f3xx_hal.o \
			stm32f3xx_hal_cortex.o \
			stm32f3xx_hal_pwr.o \
			stm32f3xx_hal_rcc.o \
			stm32f3xx_hal_gpio.o \
			startup_stm32f303xc.o

OPENOCD_BOARD_SCRIPT:=board/stm32f3discovery.cfg
