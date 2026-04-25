## Capability: board-support-avr128da32

### Goal

The alloy build matrix includes a compile-validated target for the
Microchip AVR128DA32 Curiosity Nano. `board::init()`, `board::make_debug_uart()`,
and `board::led::*` compile and link under AVR-GCC with `-mmcu=avr128da32`.

### Requirements

- `boards/avr128da32_curiosity_nano/board_config.hpp` exists and defines a
  `ClockConfig` struct with `cpu_freq_hz` set to the CLKCTRL output frequency.
- `boards/avr128da32_curiosity_nano/board.hpp` declares `board::init()`,
  `board::led::on/off/toggle()`, and `board::BoardSysTick` (or equivalent delay
  primitive available on AVR).
- `boards/avr128da32_curiosity_nano/board_uart.hpp` declares `board::make_debug_uart()`
  returning a `hal::uart::port_handle` valid for USART0 at 115200 baud.
- A CMake preset named `review-avr128da32` exists in `CMakePresets.json`,
  selects this board, and specifies the AVR-GCC toolchain.
- The compile smoke target `avr128da32-smoke` builds without errors.

---

## Capability: board-support-esp32c3

### Goal

The alloy build matrix includes a compile-validated target for the
Espressif ESP32-C3-DevKitM-1. `board::init()`, `board::make_debug_uart()`,
and `board::led::*` compile and link under the RISC-V ESP toolchain.

### Requirements

- `boards/esp32c3_devkitm/board_config.hpp` exists and defines a
  `ClockConfig` struct with `cpu_freq_hz` ≥ 80 MHz.
- `boards/esp32c3_devkitm/board.hpp` declares `board::init()` and
  `board::led::on/off/toggle()`.
- `boards/esp32c3_devkitm/board_uart.hpp` declares `board::make_debug_uart()`
  returning a `hal::uart::port_handle` valid for UART0 at 115200 baud.
- A CMake preset named `review-esp32c3` exists in `CMakePresets.json`.
- The compile smoke target `esp32c3-smoke` builds without errors.

---

## Capability: board-support-esp32s3

### Goal

The alloy build matrix includes a compile-validated target for the
Espressif ESP32-S3-DevKitC-1.

### Requirements

- `boards/esp32s3_devkitc/board_config.hpp` exists and defines a
  `ClockConfig` struct with `cpu_freq_hz` ≥ 160 MHz.
- `boards/esp32s3_devkitc/board.hpp` declares `board::init()` and
  `board::led::on/off/toggle()`.
- `boards/esp32s3_devkitc/board_uart.hpp` declares `board::make_debug_uart()`.
- A CMake preset named `review-esp32s3` exists in `CMakePresets.json`.
- The compile smoke target `esp32s3-smoke` builds without errors.

---

## Capability: board-support-rp2040

### Goal

The alloy build matrix includes a compile-validated target for the
Raspberry Pi Pico (RP2040). `board::init()`, `board::make_debug_uart()`,
and `board::led::*` compile and link under arm-none-eabi-gcc with
`-mcpu=cortex-m0plus`.

### Requirements

- `boards/raspberry_pi_pico/board_config.hpp` exists and defines a
  `ClockConfig` struct with `cpu_freq_hz = 125'000'000`.
- `boards/raspberry_pi_pico/board.hpp` declares `board::init()`,
  `board::led::on/off/toggle()`, and `board::BoardSysTick`.
- `boards/raspberry_pi_pico/board_uart.hpp` declares `board::make_debug_uart()`
  valid for UART0 at 115200 baud (GPIO0 TX / GPIO1 RX).
- A CMake preset named `review-rp2040` exists in `CMakePresets.json`.
- The compile smoke target `rp2040-smoke` builds without errors.
