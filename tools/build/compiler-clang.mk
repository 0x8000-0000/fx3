CC=clang
CXX=clang++
AS=arm-none-eabi-gcc
LD=arm-none-eabi-g++
OBJDUMP=llvm-objdump -triple=thumbv7 -disassemble -g
SIZE=arm-none-eabi-size

ifeq ($(RELEASE),true)
	CFLAGS+=-Os
	CXXFLAGS+=-Os
else
	CFLAGS+=-g -O0
	CXXFLAGS+=-g -O0
	LFLAGS+=-g
	AFLAGS+=-g
endif

CFLAGS+=-Wall -std=c11 \
		--target=arm-none-eabi -fshort-enums -isystem "C:/Libs/Embedded/GNU_Tools_ARM/5.2_2015q4/arm-none-eabi/include"
LFLAGS+=--specs=nosys.specs -Wl,-gc-sections -Wl,--cref
CXXFLAGS+=-Wall -std=c++14 -fno-exceptions -fno-rtti -Werror

