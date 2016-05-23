#
# Board: TWR-K64F120M (with MK64FN1M0VMD12)
# http://www.nxp.com/products/sensors/accelerometers/3-axis-accelerometers/kinetis-k64-mcu-tower-system-module:TWR-K64F120M
#

ifneq ("$(wildcard $(KINETIS_K64))","")

BOARD_TWR_K64F120M_MCU:=CPU_MK64FN1M0VMD12

BOARD_TWR_K64F120M_DIR:=source/boards/TWR-K64F120M

BOARD_TWR_K64F120M_INCLUDES:=\
	-Isource/arch/inc \
	-Isource/boards/inc \
	-I$(BOARD_TWR_K64F120M_DIR)/inc \
	-I$(CHIP_KINETIS_KXX_DIR)/inc \
	-I$(KINETIS_K64)/devices/MK64F12 \
	-I$(KINETIS_K64)/devices/MK64F12/drivers \
	-I$(CMSIS)/Include

BOARD_TWR_K64F120M_CFLAGS:=$(CHIP_KINETIS_KXX_CFLAGS) -D$(BOARD_TWR_K64F120M_MCU) -DTWR_K64F120M
BOARD_TWR_K64F120M_AFLAGS:=$(CHIP_KINETIS_KXX_AFLAGS) -D__HEAP_SIZE=1024
BOARD_TWR_K64F120M_LFLAGS:=$(CHIP_KINETIS_KXX_LFLAGS) -T $(BOARD_TWR_K64F120M_DIR)/linker/gcc/MK64FN1M0xxx12_flash.ld

BOARD_TWR_K64F120M_C_VPATH:=\
	$(KINETIS_K64)/devices/MK64F12 \
	$(KINETIS_K64)/devices/MK64F12/drivers \
	$(CHIP_KINETIS_KXX_C_VPATH) \
	$(BOARD_TWR_K64F120M_DIR)/src

BOARD_TWR_K64F120M_S_VPATH:=\
	source/arch/cortex-m4/gcc \
	$(BOARD_TWR_K64F120M_DIR)/src/gcc

BOARD_TWR_K64F120M_OBJECTS:=\
	$(CHIP_KINETIS_KXX_OBJECTS) \
	board_twr_k64f120m.o \
	bitops.o \
	system_MK64F12.o \
	startup_MK64F12.o \
	fsl_clock.o \
	fsl_gpio.o

else
$(warning KINETIS_K64 does not contain libraries)
endif
