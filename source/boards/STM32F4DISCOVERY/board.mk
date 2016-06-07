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
BOARD_STM32F4DISCOVERY_MCU=STM32F407xx

BOARD_STM32F4DISCOVERY_DIR:=source/boards/STM32F4DISCOVERY

BOARD_STM32F4DISCOVERY_INCLUDES:=\
	-Isource/arch/inc \
	-Isource/boards/inc \
	-I$(BOARD_STM32F4DISCOVERY_DIR)/inc \
	-I$(CHIP_STM32FXX_DIR)/inc \
	-I$(STM32F4HAL)/CMSIS/Device/ST/STM32F4xx/Include \
	-I$(CMSIS)/Include \
	-I$(STM32F4CUBE)/Drivers/STM32F4xx_HAL_Driver/Inc

BOARD_STM32F4DISCOVERY_CFLAGS:=$(CHIP_STM32FXX_CFLAGS) -D$(BOARD_STM32F4DISCOVERY_MCU) -DUSE_HAL_DRIVER
BOARD_STM32F4DISCOVERY_AFLAGS:=$(CHIP_STM32FXX_AFLAGS) -D__HEAP_SIZE=1024
BOARD_STM32F4DISCOVERY_LFLAGS:=$(CHIP_STM32FXX_LFLAGS) -T $(BOARD_STM32F4DISCOVERY_DIR)/linker/gcc/$(BOARD_STM32F4DISCOVERY_MCU).ld

BOARD_STM32F4DISCOVERY_C_VPATH:=\
	$(BOARD_STM32F4DISCOVERY_DIR)/src \
	$(CHIP_STM32FXX_DIR)/src \
	$(STM32F4CUBE)/Drivers/STM32F4xx_HAL_Driver/Src

BOARD_STM32F4DISCOVERY_S_VPATH:=\
	source/arch/cortex-m4/gcc \
	$(BOARD_STM32F4DISCOVERY_DIR)/src/gcc

BOARD_STM32F4DISCOVERY_OBJECTS:=\
	$(CHIP_STM32FXX_OBJECTS) \
	board_stm32f4disco.o \
	stm32f4_driver_usart.o \
	stm32f4_driver_i2c.o \
	stm32f4_driver_spi.o \
	bitops.o \
	system_stm32f4xx.o \
	stm32f4xx_hal.o \
	stm32f4xx_hal_cortex.o \
	stm32f4xx_hal_pwr.o \
	stm32f4xx_hal_rcc.o \
	stm32f4xx_hal_gpio.o \
	stm32f4xx_hal_uart.o \
	stm32f4xx_hal_dma.o \
	stm32f4xx_hal_i2c.o \
	stm32f4xx_hal_spi.o \
	startup_stm32f407xx.o \

BOARD_STM32F4DISCOVERY_OPENOCD_BOARD_SCRIPT:=board/stm32f4discovery.cfg
