# Porting a New Board

## Goal

A board port should be mostly declarative. CMake discovers boards from `board.json` manifests
automatically — no edits to `cmake/board_manifest.cmake` are required.

The board layer should select:

- board name and device tuple (vendor / family / device from `alloy-devices`)
- linker script and toolchain
- board-level aliases such as LED pin and debug UART connector

## Rules

- Prefer consuming existing generated descriptors over handwritten board-specific register code.
- Keep bring-up explicit in board code.
- Avoid static initialization with side effects.
- Do not add new vendor-specific runtime APIs for a board port.

## Step 1 — Create `boards/<board_id>/board.json`

This is the single source of truth for CMake, the CLI, and the validator.

```json
{
  "$schema": "https://alloy-rs.dev/schemas/board-manifest/v1.json",
  "board_id": "nucleo_g071rb",
  "display_name": "Nucleo-G071RB",
  "vendor": "st",
  "family": "stm32g0",
  "device": "stm32g071rb",
  "arch": "cortex-m0plus",
  "linker_script": "STM32G071RBT6.ld",
  "board_header": "boards/nucleo_g071rb/board.hpp",
  "toolchain": "arm-none-eabi",
  "mcu": "STM32G071RBT6",
  "flash_size_bytes": 131072,
  "debug": {
    "probe": "stlink",
    "openocd_cfg": "interface/stlink.cfg target/stm32g0x.cfg"
  },
  "uart": {
    "debug": { "peripheral": "USART2", "tx": "PA2", "rx": "PA3", "baud": 115200 }
  },
  "serial_globs": ["/dev/cu.usbmodem*", "/dev/ttyACM*"],
  "leds": [
    { "name": "ld4", "pin": "PA5", "active_high": true }
  ],
  "firmware_targets": ["blink", "uart_logger"],
  "tier": 3
}
```

### Required fields

| Field | Description |
|---|---|
| `board_id` | Snake-case identifier (matches directory name) |
| `vendor` | `st`, `microchip`, `raspberrypi`, `espressif`, `nordic` |
| `family` | e.g. `stm32g0`, `same70`, `rp2040` |
| `device` | alloy-devices descriptor name (e.g. `stm32g071rb`) |
| `arch` | `cortex-m0plus`, `cortex-m4`, `cortex-m7`, `riscv32`, `xtensa-lx6`, `xtensa-lx7`, `avr`, `native` |
| `linker_script` | Relative to `boards/<board_id>/` |
| `board_header` | Relative to repo root |
| `toolchain` | `arm-none-eabi`, `riscv32-esp-elf`, `xtensa-esp32-elf`, `xtensa-esp32s3-elf`, `avr-gcc`, `native` |
| `tier` | 1 = actively maintained; 2 = community-supported; 3 = placeholder |

### Validate

```bash
python3 scripts/validate_board_manifest.py
```

## Step 2 — Create `board.hpp`

Point to the generated device descriptor and define board aliases:

```cpp
#pragma once
#include "alloy/device.hpp"

namespace board {
    // Make the LED available as a typed alias
    using Led = alloy::hal::Gpio<alloy::device::GpioPort::A, 5>;

    inline void init() {
        // minimal bring-up: clock + GPIO init
    }
}
```

## Step 3 — Create the linker script

Start from an existing script for the same device family under `boards/`.
Required sections: `.text`, `.data`, `.bss`, stack/heap.

## Step 4 — Create `board.cpp` (optional)

Keep this to orchestration code only (clock setup, peripheral init).
Avoid logic that belongs in `board.hpp`.

## Step 5 — Wire it up

```bash
# CMake discovers the new board automatically from board.json
cmake -DALLOY_BOARD=my_new_board -B build
cmake --build build --target blink
```

`alloy boards` lists it immediately after the board.json is created:

```bash
python3 scripts/alloyctl.py boards --vendor st
```

## References

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [RUNTIME_DEVICE_BOUNDARY.md](RUNTIME_DEVICE_BOUNDARY.md)
- [CUSTOM_BOARDS.md](CUSTOM_BOARDS.md) — downstream custom board path
- [cmake/schemas/board-manifest/v1.json](../cmake/schemas/board-manifest/v1.json) — full JSON Schema
