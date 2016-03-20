# FX3 RTOS

## Scope

This is a new RTOS developed from the ground-up. It presentely targets ARM Cortex-M3/4 microcontrollers. The examples use inexpensive STM32 Discovery boards.

## License

Apache License, Version 2.0

## Pre-requisites

You will need:
* ARM CMSIS 4.5
* Vendor drivers (in the case of STM32, you'll need the [STM32CubeF4](http://www.st.com/web/en/catalog/tools/PF259243) and [STM32CubeF3](http://www.st.com/web/en/catalog/tools/PF260613) drivers).
* A compiler: FX3 is developed using [GCC ARM Embedded](https://launchpad.net/gcc-arm-embedded) version 5.2.1 .
* GNU Make version 4.0 or later.
* [CppUTest](https://cpputest.github.io/) . Have CPPUTEST\_HOME environment variable point to it.

The libraries need to be expanded into a folder pointed to by the EMBEDDED environmnent variable. See 'config.mk' for details.

## Other tools

For debugging the Discovery boards, you'll need [ST-Link](http://www.st.com/web/catalog/tools/FM146/CL1984/SC724/SS1677/PF251168) drivers and utilities.
My main debugger is [WinIDEA Open](http://www.isystem.com/download/winideaopen) version 9.2.256 .

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
