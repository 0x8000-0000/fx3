# FX3 RTOS

## Scope

This is a new RTOS developed from the ground-up with two main goals: being easy to learn and review, and being efficient in terms of resource usage.
It presentely targets ARM Cortex-M3/4 microcontrollers. The examples use inexpensive [STM32 Discovery](www.st.com/stm32discovery-pr) boards.

## License

Apache License, Version 2.0

## Pre-requisites

You will need:
* ARM CMSIS 4.5
* Vendor drivers (in the case of STM32, you'll need the [STM32CubeF4](http://www.st.com/web/en/catalog/tools/PF259243) and [STM32CubeF3](http://www.st.com/web/en/catalog/tools/PF260613) drivers).
* A compiler: FX3 is developed using [GCC ARM Embedded](https://launchpad.net/gcc-arm-embedded) version 5.2.1 . It can be compiled with [CLANG 3.8](http://clang.llvm.org/) but the GNU ARM linker and support libraries are required.
* GNU Make version 4.0 or later.
* [CppUTest](https://cpputest.github.io/) . Have CPPUTEST\_HOME environment variable point to it.

The libraries need to be expanded into a folder pointed to by the EMBEDDED environmnent variable. See 'config.mk' for details. The build system is OS agnostic, tested on Windows and Linux.

## Other tools

For debugging the Discovery boards, you'll need [ST-Link](http://www.st.com/web/catalog/tools/FM146/CL1984/SC724/SS1677/PF251168) drivers and utilities. For other boards, I recommend the excellent [Segger J-Link](https://www.segger.com/jlink-debug-probes.html) for their versatility and cross-platform support.
My main debugger is [WinIDEA Open](http://www.isystem.com/download/winideaopen) version 9.2.256 and I'm also exploring [Segger Ozone](https://www.segger.com/ozone.html).

## Building Examples

Assuming you have set up the environment, you should be able to run 'make' from the top level folder.

## Documentation, Plans

Design documents are in the doc folder.

## References

* Books

  * "The Definitive Guide to ARM® Cortex®-M3 and Cortex®-M4 Processors, Third Edition" by Joseph Yiu
  * "Programming Pearls, Second Edition" by Jon Bentley


Happy Hacking,
florin@signbit.net
