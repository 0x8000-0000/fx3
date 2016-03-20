CC=arm-none-eabi-gcc
CXX=arm-none-eabi-g++
AS=arm-none-eabi-gcc
LD=arm-none-eabi-g++
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size

ifeq ($(RELEASE),true)
	CFLAGS+=-Os
	CXXFLAGS+=-Os
else
	CFLAGS+=-g -Og
	CXXFLAGS+=-g -Og
	LFLAGS+=-g
	AFLAGS+=-g
endif

CFLAGS+=-Wall -std=c11 \
		-ffunction-sections \
		-pedantic -Werror \
		-Wno-unused-but-set-variable -Wno-unused-variable \
		-nostdlib -ffreestanding
LFLAGS+=--specs=nosys.specs -Wl,-gc-sections -Wl,--cref
CXXFLAGS+=-Wall -std=c++14 -fno-exceptions -fno-rtti -Werror

