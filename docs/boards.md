# Supported Boards

This document lists all officially supported development boards for Alloy Framework.

## Overview

Alloy currently supports 5 development boards across 4 different MCU vendors and 3 architectures:

| Board | MCU | Vendor | Architecture | CPU | Flash | RAM | Clock |
|-------|-----|--------|--------------|-----|-------|-----|-------|
| Blue Pill | STM32F103C8 | STMicroelectronics | ARM Cortex-M3 | 72 MHz | 64 KB | 20 KB | 72 MHz |
| STM32F4 Discovery | STM32F407VG | STMicroelectronics | ARM Cortex-M4F | 168 MHz | 1 MB | 192 KB | 168 MHz |
| Raspberry Pi Pico | RP2040 | Raspberry Pi | ARM Cortex-M0+ (Dual) | 133 MHz | 2 MB | 264 KB | 125-133 MHz |
| Arduino Zero | ATSAMD21G18 | Microchip | ARM Cortex-M0+ | 48 MHz | 256 KB | 32 KB | 48 MHz |
| ESP32 DevKit | ESP32-WROOM-32 | Espressif | Xtensa LX6 (Dual) | 240 MHz | 4 MB | 520 KB | 160-240 MHz |

## Board Details

### 1. Blue Pill (STM32F103C8)

**Status**: ✅ Fully Supported

**Description**: Ultra-popular, low-cost ARM Cortex-M3 development board. Commonly used for hobby projects and education.

**Key Features**:
- ARM Cortex-M3 @ 72 MHz
- 64 KB Flash, 20 KB RAM
- 37 GPIO pins
- 2× SPI, 2× I2C, 3× USART
- 2× 12-bit ADC (16 channels)
- USB 2.0 Full Speed device
- **On-board LED**: PC13 (active LOW)

**Toolchain**: `arm-none-eabi-gcc` (xPack recommended)

**Board ID**: `bluepill`

**Purchase**: ~$2-4 USD on AliExpress

---

### 2. STM32F4 Discovery (STM32F407VG)

**Status**: ✅ Fully Supported

**Description**: High-performance ARM Cortex-M4F board with FPU, DSP instructions, and rich peripherals. Official ST development board.

**Key Features**:
- ARM Cortex-M4F @ 168 MHz (with FPU)
- 1 MB Flash, 192 KB RAM (+ 64 KB CCM)
- 82 GPIO pins
- 3× SPI, 3× I2C, 4× USART
- 3× 12-bit ADC (24 channels)
- USB OTG Full Speed + High Speed
- MEMS accelerometer (LIS3DSH)
- MEMS audio sensor (MP45DT02)
- **On-board LEDs**: 4 LEDs (PD12/13/14/15 - Green/Orange/Red/Blue)

**Toolchain**: `arm-none-eabi-gcc` (xPack recommended)

**Board ID**: `stm32f407vg`

**Purchase**: ~$20-30 USD (official ST board)

---

### 3. Raspberry Pi Pico (RP2040)

**Status**: ✅ Fully Supported

**Description**: Modern, dual-core ARM Cortex-M0+ board from Raspberry Pi Foundation. Excellent performance-per-dollar ratio.

**Key Features**:
- Dual ARM Cortex-M0+ @ 133 MHz
- 2 MB Flash (external QSPI)
- 264 KB SRAM (6 banks)
- 26 GPIO pins (30 total with 4 ADC pins)
- 2× SPI, 2× I2C, 2× UART
- 3× 12-bit ADC (shared 500 ksps SAR)
- 16× PWM channels
- 8× Programmable I/O (PIO) state machines
- USB 1.1 device/host
- **On-board LED**: GPIO25 (active HIGH, green LED)

**Toolchain**: `arm-none-eabi-gcc` (xPack recommended)

**Board ID**: `rp_pico`

**Purchase**: ~$4 USD (official)

**Special Notes**:
- Requires second-stage bootloader (boot2) for external flash initialization
- Dual-core capable (currently only core 0 used by Alloy)

---

### 4. Arduino Zero (ATSAMD21G18)

**Status**: ⚠️ Partial Support (known toolchain issue)

**Description**: Arduino-compatible board based on ARM Cortex-M0+. Great for beginners transitioning from Arduino.

**Key Features**:
- ARM Cortex-M0+ @ 48 MHz
- 256 KB Flash, 32 KB RAM
- 20 GPIO pins (14 digital I/O + 6 analog)
- 1× SPI, 1× I2C, 2× UART
- 6× 12-bit ADC (350 ksps)
- 1× 10-bit DAC
- USB 2.0 Full Speed device/host
- **On-board LED**: GPIO13/PA17 (active HIGH, orange LED)

**Toolchain**: `arm-none-eabi-gcc` (xPack recommended)

**Board ID**: `arduino_zero`

**Purchase**: ~$40 USD (official Arduino) / ~$10 USD (clones)

**Known Issues**:
- xPack ARM toolchain v14.2.1+ has nosys.specs conflict (workaround in progress)
- See [troubleshooting guide](troubleshooting.md) for details

---

### 5. ESP32 DevKit (ESP32-WROOM-32)

**Status**: ⚠️ Partial Support (GPIO peripheral issue)

**Description**: WiFi/Bluetooth-enabled dual-core Xtensa board. Extremely popular for IoT applications.

**Key Features**:
- Dual Xtensa LX6 @ 240 MHz
- 4 MB Flash (external)
- 520 KB SRAM (+ 8 MB PSRAM optional)
- 34 GPIO pins
- 3× SPI, 2× I2C, 3× UART
- 18× 12-bit ADC
- 2× 8-bit DAC
- WiFi 802.11 b/g/n
- Bluetooth v4.2 BR/EDR + BLE
- **On-board LED**: GPIO2 (active HIGH, blue LED on many boards)

**Toolchain**: `xtensa-esp32-elf-gcc` (ESP-IDF)

**Board ID**: `esp32_devkit`

**Purchase**: ~$5-10 USD

**Known Issues**:
- GPIO peripheral structure incomplete in generated code (fix in progress)
- Requires ESP-IDF toolchain (larger dependency)
- See [troubleshooting guide](troubleshooting.md) for details

## Board Selection

To build for a specific board, use the `ALLOY_BOARD` CMake variable:

```bash
cmake -DALLOY_BOARD=<board_id> ..
```

Example:
```bash
cmake -DALLOY_BOARD=bluepill ..
make
```

Valid board IDs:
- `bluepill` - Blue Pill (STM32F103C8)
- `stm32f407vg` - STM32F4 Discovery
- `rp_pico` - Raspberry Pi Pico
- `arduino_zero` - Arduino Zero
- `esp32_devkit` - ESP32 DevKit

## Pin Mappings

See [Pin Mappings Documentation](pin_mappings.md) for detailed pinout diagrams and alternate function tables for each board.

## Clock Configurations

See [Clock Configuration Guide](clock_configurations.md) for clock tree diagrams and frequency settings for each MCU.

## Building and Flashing

- **Build Instructions**: See [Building for Boards](building_for_boards.md)
- **Flash Instructions**: See [Flashing Guide](flashing.md)

## Adding New Boards

To add a new board to Alloy:

1. Create board directory: `boards/<board_name>/`
2. Add linker script: `<board_name>.ld`
3. Add startup code: `startup.cpp`
4. Add board header: `board.hpp`
5. Create CMake config: `cmake/boards/<board_id>.cmake`
6. Add example: `examples/blink_<board_id>/`
7. Update build system and documentation

See existing boards as reference templates.
