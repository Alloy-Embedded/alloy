# alloy

[![ci](https://github.com/Alloy-Embedded/alloy/actions/workflows/ci.yml/badge.svg)](https://github.com/Alloy-Embedded/alloy/actions/workflows/ci.yml)

A from-scratch C++23 framework for microcontrollers: one portable app source, any supported
board, compile-time safety, no RTOS required, no IDE required.

```cpp
// src/main.cpp — identical bytes on every supported board
#include <alloy/board.hpp>
using namespace alloy::literals;

int main() {
    board::init();
    while (true) {
        board::led.toggle();
        alloy::sleep_for(500ms);
    }
}
```

```console
$ alloy new hello --board esp32_devkit && cd hello
$ alloy run                        # build + flash + serial monitor
$ alloy build --board nucleo_g071rb   # same code, different MCU — one flag
```

## Hardware-validated families

| Family | Core | Board(s) | Validated on silicon |
| --- | --- | --- | --- |
| ST STM32G0 | Cortex-M0+ @ 64 MHz | Nucleo-G071RB | PLL via probe, GPIO, UART echo |
| Microchip SAME70 | Cortex-M7 @ 150 MHz | SAM E70 Xplained | PLLA via probe, PIO, USART echo |
| Raspberry Pi RP2040 | 2× Cortex-M0+ @ 125 MHz | RP2040-Zero, Pico | boot2+CRC, WS2812 (±150 ns timing) |
| Espressif ESP32 | Xtensa LX6 @ 80 MHz | WROVER-KIT, DevKit | boot chain, watchdogs, UART echo |

Every fact above ships from [`alloy-devices`](https://github.com/Alloy-Embedded/alloy-devices)
data with provenance; the same 14-line `main.cpp` runs on all of them with zero `#ifdef`s
(enforced by CI).

## Layout

| Path            | What                                                              |
| --------------- | ----------------------------------------------------------------- |
| `src/alloy/`    | Hand-written C++: concepts, HAL drivers (one per peripheral IP version), arch support |
| `tools/alloy/`  | The Python CLI + code generator (single `alloy` package)          |
| `boards/`       | One `board.json` per board — data only, zero hand-written C++     |
| `examples/`     | Portable examples — zero preprocessor conditionals, enforced by CI |
| `cmake/`        | Internal CMake templates rendered by the CLI (users never see CMake) |
| `scripts/`      | Contract gates (`check_contract.sh`) and dev helpers              |

Device data (register maps per IP version, per-chip instances, pin routes, clock trees)
lives in the sibling [`alloy-devices`](../alloy-devices) repo and reaches this repo only
through generated headers.

Read [NORTH_STAR.md](NORTH_STAR.md) before contributing — it is the contract that keeps
this rebuild from repeating the old ecosystem's failures.

## Status

Active greenfield rebuild (2026-07). Working today: 4 chip families / 6 boards through the
full data → codegen → HAL → CLI loop; `alloy new / build / flash / run / monitor` with
`--board` switching; UF2, esptool and OpenOCD/probe flash runners; CI matrix of every
example × every board plus contract gates and an out-of-repo scaffold build.

Next up: nRF52840 (the "new family = data only" proof), ESP32-C3 (RISC-V), packaged
distribution (`pipx install alloy`).

License: MIT OR Apache-2.0 (same as the previous alloy).
