# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## vx.y.z (T-B-D)

### Added

   - Input debouncer for switches, and quadrature encoder
   - SPI drivers for LIS3DH and LIS3DSH accelerometers
   - SPI driver for STM32F4 chips
   - I2C driver for STM32F4 chips
   - Drivers for BMP085 (pressure sensor), DS3231M (real-time clock) and MPU-6050 (accelerometer / gyroscope)

## v0.4.0 (2016-06-02)

### Added

   - Added implementation for semaphores
   - Changed the kernel-implementation to be lock-free
   - Added UART driver for STM32F4 boards (Nucleo and STM32F4-DISCO)
   - Added support for [NUCLEO-F401RE](http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/LN1847/PF260000)
   - Added support for [NXP Kinetis TWR-K64F120M](http://www.nxp.com/products/sensors/accelerometers/3-axis-accelerometers/kinetis-k64-mcu-tower-system-module:TWR-K64F120M)
   - Added support for [Segger SystemView](https://www.segger.com/systemview.html)
   - Refactored build system to use non-recursive makefile

## v0.3.0 (2016-04-04)

### Added

   - Lock-free bit allocator
   - Lock-free buffer allocator
   - Task-pool helper
   - Lock-free task message queue

## v0.2.0 (2016-03-19)

### Added

   - Round-robin scheduling

## v0.1.0 (2016-03-14)

### Added

   - Context switching
   - Task sleep
   - Board support for [STM32F4-Discovery](http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/PF252419)
   - Buffer interface (implementation is pending)

## v0.0.0 (2016-03-12)

### Added

   - Board support for [STM32F3-Discovery](http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/PF254044)
