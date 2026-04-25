## Why

The v0.1 release proves the alloy HAL on SAME70, STM32 G0/F4, and NXP iMXRT. The
`alloy-devices` repository already carries runtime device contracts for three more
vendor families:

| Vendor      | Family   | Device        | Published runtime contract |
|-------------|----------|---------------|---------------------------|
| Microchip   | AVR-DA   | AVR128DA32    | `microchip/avr-da/`       |
| Espressif   | ESP32-C3 | ESP32-C3      | `espressif/esp32c3/`      |
| Espressif   | ESP32-S3 | ESP32-S3      | `espressif/esp32s3/`      |
| Raspberry Pi | RP2040  | RP2040 / Pico | `raspberrypi/rp2040/`     |

Without board support in the alloy repo, these contracts are unreachable — there is no
target to build against, no `board::init()` to hook the clock, and no way to exercise
the examples or compile smoke tests on these families.

Adding board support for one representative hardware target per device proves:
1. The device contract compiles against real toolchain constraints (AVR-GCC, Xtensa
   GCC, ARM Cortex-M0+ with the appropriate ABI).
2. The alloy public HAL surface is portable to RISC-V (ESP32-C3), Xtensa (ESP32-S3),
   and the AVR architecture — not just ARM Cortex-M.
3. Adopters on popular hobbyist platforms (Arduino-Nano-every-sized AVR, any ESP32
   devkit, Raspberry Pi Pico) have a tested starting point.

## What Changes

### Microchip AVR128DA32 — Curiosity Nano (DM164151)

- Board: AVR128DA32 Curiosity Nano — USB CDC UART on USART0 (PA0/PA1), user LED on
  PC6 (active-low), user button on PC7 (active-low).
- Add `boards/avr128da32_curiosity_nano/` with `board.hpp`, `board.cpp`,
  `board_uart.hpp`, `board_config.hpp`, CMakeLists.txt.
- Add a `review-avr128da32` CMake build preset (AVR-GCC toolchain, `mmcu=avr128da32`).
- Add compile smoke target `alloy_add_device_contract_smoke(avr128da32-smoke)`.

### Espressif ESP32-C3 — DevKitM-1

- Board: ESP32-C3-DevKitM-1 — USB UART on UART0 (GPIO20/GPIO21), user LED on GPIO8
  (RGB WS2812, treat as GPIO output for probe purposes), user button on GPIO9.
- Add `boards/esp32c3_devkitm/` with `board.hpp`, `board.cpp`, `board_uart.hpp`,
  `board_config.hpp`, CMakeLists.txt.
- Add a `review-esp32c3` CMake build preset (RISC-V ESP-IDF toolchain or
  riscv32-esp-elf-gcc).
- Add compile smoke target `alloy_add_device_contract_smoke(esp32c3-smoke)`.

### Espressif ESP32-S3 — DevKitC-1

- Board: ESP32-S3-DevKitC-1 — USB UART on UART0 (GPIO43/GPIO44), user LED on GPIO48
  (RGB, treat as GPIO output), user button on GPIO0.
- Add `boards/esp32s3_devkitc/` with `board.hpp`, `board.cpp`, `board_uart.hpp`,
  `board_config.hpp`, CMakeLists.txt.
- Add a `review-esp32s3` CMake build preset (Xtensa ESP-IDF toolchain or
  xtensa-esp32s3-elf-gcc).
- Add compile smoke target `alloy_add_device_contract_smoke(esp32s3-smoke)`.

### Raspberry Pi — Pico (RP2040)

- Board: Raspberry Pi Pico — UART0 on GPIO0/GPIO1, user LED on GPIO25 (active-high).
- Add `boards/raspberry_pi_pico/` with `board.hpp`, `board.cpp`, `board_uart.hpp`,
  `board_config.hpp`, CMakeLists.txt.
- Add a `review-rp2040` CMake build preset (ARM GCC, `cortex-m0plus`, RP2040 linker
  script).
- Add compile smoke target `alloy_add_device_contract_smoke(rp2040-smoke)`.

### Shared infrastructure

- Extend `tests/compile/contract_smoke.cmake` `alloy_add_device_contract_smoke` with
  per-family `#if` guards so the new smoke targets compile only the tests that apply
  to the selected device (no TWIHS-specific assertions on AVR, etc.).
- Extend the `code-quality.yml` workflow with a build matrix entry for each new target
  (build-only, no silicon required for CI).
- Extend `examples/` with per-board `CMakeLists.txt` guards where needed.

Out of scope for this change:

- Full silicon validation of all examples on every new board (separate
  `close-foundational-hardware-validation` change per family).
- RTOS / DMA / ADC peripheral coverage on the new targets (follow-on specs).
- ESP-IDF component wrapper or Arduino compatibility shim.

## Outcome

Four new compile-validated board targets join the alloy build matrix. Adopters on
AVR128DA32, ESP32-C3, ESP32-S3, and Raspberry Pi Pico have board support to copy from,
and the compile smoke suite catches regressions on each family independently.

## Impact

- Affected specs: new `board-support-avr128da32`, `board-support-esp32c3`,
  `board-support-esp32s3`, `board-support-rp2040`
- New files:
  - `boards/avr128da32_curiosity_nano/` (board.hpp, board.cpp, board_uart.hpp, board_config.hpp, CMakeLists.txt)
  - `boards/esp32c3_devkitm/` (same set)
  - `boards/esp32s3_devkitc/` (same set)
  - `boards/raspberry_pi_pico/` (same set)
  - `CMakePresets.json` entries: review-avr128da32, review-esp32c3, review-esp32s3, review-rp2040
  - `.github/workflows/build.yml` matrix expansion
