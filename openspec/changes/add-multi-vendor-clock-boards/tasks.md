# Implementation Tasks

## 1. STM32F103 (Blue Pill) - ARM Cortex-M3, 72MHz

### 1.1 Clock Implementation
- [x] 1.1.1 Create `src/hal/st/stm32f1/clock.hpp`
- [x] 1.1.2 Implement RCC register definitions
- [x] 1.1.3 Implement HSE (8MHz external crystal) setup
- [x] 1.1.4 Implement PLL configuration (HSE × 9 = 72MHz)
- [x] 1.1.5 Implement flash latency configuration (2 wait states @ 72MHz)
- [x] 1.1.6 Implement AHB/APB1/APB2 bus dividers
- [x] 1.1.7 Implement peripheral clock enable/disable
- [x] 1.1.8 Implement frequency query functions

### 1.2 Board Support
- [x] 1.2.1 Create `boards/stm32f103c8/` directory
- [x] 1.2.2 Create `STM32F103C8.ld` linker script (64K flash, 20K RAM)
- [x] 1.2.3 Create `startup.cpp` with vector table and reset handler
- [x] 1.2.4 Create `board.hpp` with pin definitions (LED on PC13)
- [x] 1.2.5 Create CMakeLists.txt for board

### 1.3 Blink Example
- [x] 1.3.1 Create `examples/blink_stm32f103/main.cpp`
- [x] 1.3.2 Initialize clock to 72MHz
- [x] 1.3.3 Configure GPIO PC13 as output
- [x] 1.3.4 Blink LED at 1Hz
- [x] 1.3.5 Create CMakeLists.txt for example

## 2. ESP32 (DevKit) - Xtensa LX6 Dual-Core, 240MHz

### 2.1 Clock Implementation
- [x] 2.1.1 Create `src/hal/espressif/esp32/clock.hpp`
- [x] 2.1.2 Implement RTC_CNTL and DPORT register definitions
- [x] 2.1.3 Implement 40MHz XTAL setup
- [x] 2.1.4 Implement PLL configuration (40MHz → 160/240MHz)
- [x] 2.1.5 Implement CPU frequency selection
- [x] 2.1.6 Implement APB frequency configuration
- [x] 2.1.7 Implement peripheral clock enable/disable
- [x] 2.1.8 Implement frequency query functions

### 2.2 Board Support
- [x] 2.2.1 Create `boards/esp32_devkit/` directory
- [x] 2.2.2 Create `esp32.ld` linker script (with IRAM/DRAM sections)
- [x] 2.2.3 Create `startup.cpp` with minimal ESP32 startup
- [x] 2.2.4 Create `board.hpp` with pin definitions (LED on GPIO2)
- [x] 2.2.5 Create CMakeLists.txt with xtensa toolchain

### 2.3 Blink Example
- [x] 2.3.1 Create `examples/blink_esp32/main.cpp`
- [x] 2.3.2 Initialize clock to 160MHz
- [x] 2.3.3 Configure GPIO2 as output
- [x] 2.3.4 Blink LED at 1Hz
- [x] 2.3.5 Create CMakeLists.txt for example

## 3. STM32F407 (Discovery) - ARM Cortex-M4F, 168MHz

### 3.1 Clock Implementation
- [x] 3.1.1 Create `src/hal/st/stm32f4/clock.hpp`
- [x] 3.1.2 Implement RCC register definitions
- [x] 3.1.3 Implement HSE (8MHz external crystal) setup
- [x] 3.1.4 Implement PLL configuration (HSE → 168MHz with VCO)
- [x] 3.1.5 Implement flash latency configuration (5 wait states @ 168MHz)
- [x] 3.1.6 Implement AHB/APB1/APB2 bus dividers
- [x] 3.1.7 Implement peripheral clock enable/disable
- [x] 3.1.8 Implement frequency query functions

### 3.2 Board Support
- [x] 3.2.1 Create `boards/stm32f407vg/` directory
- [x] 3.2.2 Create `STM32F407VG.ld` linker script (1M flash, 192K RAM)
- [x] 3.2.3 Create `startup.cpp` with vector table
- [x] 3.2.4 Create `board.hpp` with pin definitions (LED on PD12/13/14/15)
- [x] 3.2.5 Create CMakeLists.txt for board

### 3.3 Blink Example
- [x] 3.3.1 Create `examples/blink_stm32f407/main.cpp`
- [x] 3.3.2 Initialize clock to 168MHz
- [x] 3.3.3 Configure GPIO PD12 as output
- [x] 3.3.4 Blink LED at 1Hz
- [x] 3.3.5 Create CMakeLists.txt for example

## 4. ATSAMD21 (Arduino Zero) - ARM Cortex-M0+, 48MHz

### 4.1 Clock Implementation
- [x] 4.1.1 Create `src/hal/microchip/samd21/clock.hpp`
- [x] 4.1.2 Implement GCLK and PM register definitions
- [x] 4.1.3 Implement 32kHz XOSC32K setup
- [x] 4.1.4 Implement DFLL48M configuration (32kHz → 48MHz)
- [x] 4.1.5 Implement generic clock generators
- [x] 4.1.6 Implement peripheral clock enable/disable
- [x] 4.1.7 Implement frequency query functions

### 4.2 Board Support
- [x] 4.2.1 Create `boards/arduino_zero/` directory
- [x] 4.2.2 Create `ATSAMD21G18.ld` linker script (256K flash, 32K RAM)
- [x] 4.2.3 Create `startup.cpp` with vector table
- [x] 4.2.4 Create `board.hpp` with pin definitions (LED on PA17)
- [x] 4.2.5 Create CMakeLists.txt for board

### 4.3 Blink Example
- [x] 4.3.1 Create `examples/blink_arduino_zero/main.cpp`
- [x] 4.3.2 Initialize clock to 48MHz
- [x] 4.3.3 Configure GPIO PA17 as output
- [x] 4.3.4 Blink LED at 1Hz
- [x] 4.3.5 Create CMakeLists.txt for example

## 5. RP2040 (Raspberry Pi Pico) - ARM Cortex-M0+ Dual-Core, 133MHz

### 5.1 Clock Implementation
- [x] 5.1.1 Create `src/hal/raspberry/rp2040/clock.hpp`
- [x] 5.1.2 Implement CLOCKS and PLL register definitions
- [x] 5.1.3 Implement 12MHz XOSC setup
- [x] 5.1.4 Implement PLL_SYS configuration (12MHz → 125-133MHz)
- [x] 5.1.5 Implement clock generators (ref, sys, peri, usb)
- [x] 5.1.6 Implement peripheral clock enable/disable
- [x] 5.1.7 Implement frequency query functions

### 5.2 Board Support
- [x] 5.2.1 Create `boards/pico/` directory
- [x] 5.2.2 Create `rp2040.ld` linker script (2M flash, 264K RAM)
- [x] 5.2.3 Create `boot2.S` second-stage bootloader (or copy from pico-sdk)
- [x] 5.2.4 Create `startup.cpp` with dual-core support
- [x] 5.2.5 Create `board.hpp` with pin definitions (LED on GPIO25)
- [x] 5.2.6 Create CMakeLists.txt for board

### 5.3 Blink Example
- [x] 5.3.1 Create `examples/blink_pico/main.cpp`
- [x] 5.3.2 Initialize clock to 125MHz
- [x] 5.3.3 Configure GPIO25 as output
- [x] 5.3.4 Blink LED at 1Hz
- [x] 5.3.5 Create CMakeLists.txt for example

## 6. Build System Integration

- [ ] 6.1 Modify root `CMakeLists.txt` to add board selection option
- [ ] 6.2 Create `boards/CMakeLists.txt` with board configurations
- [ ] 6.3 Add toolchain detection (arm-none-eabi-gcc, xtensa-esp32-elf-gcc)
- [ ] 6.4 Create board-specific compile flags
- [ ] 6.5 Add flash memory layout checks
- [ ] 6.6 Create upload/flash targets for each board
- [ ] 6.7 Add board-specific startup and linker script to target

## 7. Common Startup Code

- [ ] 7.1 Create `src/startup/startup_common.hpp` with shared functions
- [ ] 7.2 Implement .data section copy from flash to RAM
- [ ] 7.3 Implement .bss section zero initialization
- [ ] 7.4 Implement C++ global constructors call (__libc_init_array)
- [ ] 7.5 Create default exception handlers
- [ ] 7.6 Create weak default interrupt handlers

## 8. Toolchain and Cross-Compilation

- [ ] 8.1 Create `cmake/arm-none-eabi-toolchain.cmake`
- [ ] 8.2 Create `cmake/xtensa-esp32-toolchain.cmake`
- [ ] 8.3 Detect toolchain availability
- [ ] 8.4 Set compiler flags for each architecture
- [ ] 8.5 Configure linker flags (nosys, nostartfiles if needed)

## 9. Documentation

- [ ] 9.1 Write `docs/boards.md` with board list and specs
- [ ] 9.2 Write `docs/building_for_boards.md` with build instructions
- [ ] 9.3 Write `docs/flashing.md` with flash instructions (OpenOCD, esptool, etc)
- [ ] 9.4 Document pin mappings for each board
- [ ] 9.5 Document clock configurations for each MCU
- [ ] 9.6 Create troubleshooting guide

## 10. Testing and Validation

- [ ] 10.1 Build all 5 examples without errors
- [ ] 10.2 Verify linker generates correct memory layout
- [ ] 10.3 Flash and test STM32F103 blink
- [ ] 10.4 Flash and test ESP32 blink
- [ ] 10.5 Flash and test STM32F407 blink
- [ ] 10.6 Flash and test ATSAMD21 blink
- [ ] 10.7 Flash and test RP2040 blink
- [ ] 10.8 Measure clock frequencies with oscilloscope/logic analyzer
- [ ] 10.9 Verify LED blink timing is accurate (1Hz)
- [ ] 10.10 Document results and lessons learned
