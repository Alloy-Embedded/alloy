## 1. OpenSpec Baseline

- [ ] 1.1 Add `board-support-avr128da32`, `board-support-esp32c3`,
      `board-support-esp32s3`, and `board-support-rp2040` capability specs under
      `openspec/changes/expand-board-portfolio/specs/board-support/`.

## 2. AVR128DA32 Curiosity Nano Board Support

- [ ] 2.1 Add `boards/avr128da32_curiosity_nano/board_config.hpp` — clock config
      (16 MHz internal oscillator, 24 MHz CLKCTRL target), LED pin (PC6), UART
      baud constants.
- [ ] 2.2 Add `boards/avr128da32_curiosity_nano/board.hpp` + `board.cpp` —
      `board::init()` (disable WDT, configure CLKCTRL, SysTick if available),
      `board::led::on/off/toggle()`.
- [ ] 2.3 Add `boards/avr128da32_curiosity_nano/board_uart.hpp` — `make_debug_uart()`
      wired to USART0 (PA0 TX / PA1 RX, 115200-8-N-1).
- [ ] 2.4 Add `boards/avr128da32_curiosity_nano/CMakeLists.txt` — links
      `alloy-hal`, selects `ALLOY_BOARD_AVR128DA32_CURIOSITY_NANO`.
- [ ] 2.5 Add `review-avr128da32` CMake preset (AVR-GCC, `mmcu=avr128da32`,
      board = `avr128da32_curiosity_nano`).
- [ ] 2.6 Add compile smoke target for AVR128DA32 in
      `tests/compile/contract_smoke.cmake`.

## 3. ESP32-C3 DevKitM-1 Board Support

- [ ] 3.1 Add `boards/esp32c3_devkitm/board_config.hpp` — 160 MHz CPU clock
      (PLL), GPIO8 LED, GPIO0 button, UART0 pins.
- [ ] 3.2 Add `boards/esp32c3_devkitm/board.hpp` + `board.cpp` —
      `board::init()` (configure UART clocks via ESP32-C3 SYSTEM peripheral,
      WDT disable, LED init).
- [ ] 3.3 Add `boards/esp32c3_devkitm/board_uart.hpp` — `make_debug_uart()`
      wired to UART0 (GPIO20 TX / GPIO21 RX, 115200-8-N-1).
- [ ] 3.4 Add `boards/esp32c3_devkitm/CMakeLists.txt`.
- [ ] 3.5 Add `review-esp32c3` CMake preset (riscv32-esp-elf-gcc or ESP-IDF
      toolchain, board = `esp32c3_devkitm`).
- [ ] 3.6 Add compile smoke target for ESP32-C3.

## 4. ESP32-S3 DevKitC-1 Board Support

- [ ] 4.1 Add `boards/esp32s3_devkitc/board_config.hpp` — 240 MHz CPU clock
      (PLL), GPIO48 LED, GPIO0 button, UART0 pins (GPIO43/GPIO44).
- [ ] 4.2 Add `boards/esp32s3_devkitc/board.hpp` + `board.cpp`.
- [ ] 4.3 Add `boards/esp32s3_devkitc/board_uart.hpp`.
- [ ] 4.4 Add `boards/esp32s3_devkitc/CMakeLists.txt`.
- [ ] 4.5 Add `review-esp32s3` CMake preset (xtensa-esp32s3-elf-gcc or
      ESP-IDF toolchain, board = `esp32s3_devkitc`).
- [ ] 4.6 Add compile smoke target for ESP32-S3.

## 5. Raspberry Pi Pico (RP2040) Board Support

- [x] 5.1 Add `boards/raspberry_pi_pico/board_config.hpp` — 125 MHz system
      clock (XOSC 12 MHz + PLL), GPIO25 LED, GPIO0/GPIO1 UART0 pins.
- [x] 5.2 Add `boards/raspberry_pi_pico/board.hpp` + `board.cpp` —
      `board::init()` (XOSC enable, PLL configure, UART clock enable,
      SysTick init, LED init).
- [x] 5.3 Add `boards/raspberry_pi_pico/board_uart.hpp` — `make_debug_uart()`
      wired to UART0 (GPIO0 TX / GPIO1 RX, 115200-8-N-1).
- [x] 5.4 Add `boards/raspberry_pi_pico/CMakeLists.txt`.
- [x] 5.5 Add `review-rp2040` CMake preset (arm-none-eabi-gcc,
      `cortex-m0plus`, RP2040 linker script, board = `raspberry_pi_pico`).
- [x] 5.6 Add compile smoke target for RP2040.

## 6. CI Build Matrix Expansion

- [ ] 6.1 Add `review-avr128da32`, `review-esp32c3`, `review-esp32s3`,
      `review-rp2040` jobs to `.github/workflows/build.yml`.
- [ ] 6.2 Confirm each new target builds cleanly with zero errors on CI
      (build-only matrix, no silicon required).

## 7. Validation

- [ ] 7.1 `openspec validate expand-board-portfolio --strict`.
- [ ] 7.2 Run `driver_at24mac402_probe` on AVR128DA32 Curiosity Nano (I2C probe
      via TWI0, PA2/PA3); confirm PASS.
- [ ] 7.3 Run `driver_at24mac402_probe` on ESP32-C3 DevKitM-1 (I2C probe via
      I2C0); confirm PASS.
- [ ] 7.4 Run `driver_at24mac402_probe` on Raspberry Pi Pico (I2C probe via
      I2C0, GPIO4/GPIO5); confirm PASS.
