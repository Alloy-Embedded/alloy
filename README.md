# alloy

[![ci](https://github.com/Alloy-Embedded/alloy/actions/workflows/ci.yml/badge.svg)](https://github.com/Alloy-Embedded/alloy/actions/workflows/ci.yml)

A from-scratch C++23 framework for microcontrollers: one portable app source, any supported
board, compile-time safety, no RTOS required, no IDE required.

**[Documentation →](https://alloy-embedded.github.io/alloy/)**

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
| ST STM32G0 | Cortex-M0+ @ 64 MHz | Nucleo-G071RB, G0B1RE | PLL via probe, GPIO, UART echo |
| ST STM32F7 | Cortex-M7 | Nucleo-F722ZE | clocks, UART |
| Microchip SAME70 | Cortex-M7 @ 150 MHz | SAM E70 Xplained | PLLA via probe, PIO, USART echo |
| Raspberry Pi RP2040 | 2× Cortex-M0+ @ 125 MHz | RP2040-Zero, Pico | boot2+CRC, WS2812 (±150 ns timing) |
| Espressif ESP32 | Xtensa LX6 @ 80 MHz | WROVER-KIT, DevKit | boot chain, watchdogs, UART echo |

Every fact above ships from [`alloy-devices`](https://github.com/Alloy-Embedded/alloy-devices)
data with provenance; the same 14-line `main.cpp` runs on all of them with zero `#ifdef`s
(enforced by CI).

## What's in the box

- **Peripherals** — GPIO, UART, SPI, I²C, ADC, PWM, DMA, timers, watchdog, RTC, DAC, CAN,
  on-chip flash/NVM, Ethernet + TCP/IP.
- **Async without an RTOS** — C++20 coroutines with no heap and no dynamic allocation.
- **Field updates** — A/B slots derived from chip data, a trial boot that rolls back on its own,
  and optional Ed25519-signed images, over a plain UART
  ([docs](https://alloy-embedded.github.io/alloy/guide/firmware-update/)).
- **A driver ecosystem** — sensors, displays and clocks vendored with `alloy lib add`, templated
  on concepts so they never name a chip.
- **Emulation as a gate** — firmware boots under Renode on a platform generated from the same
  chip data, and CI asserts on real UART output: drivers, coroutines, and the whole update
  lifecycle.
- **A VS Code extension** — visual pin/clock configurator, build/flash/debug, device update; it
  holds no domain logic, every fact comes from `alloy … --json`
  ([alloy-vscode](https://github.com/Alloy-Embedded/alloy-vscode)).

## Layout

| Path            | What                                                              |
| --------------- | ----------------------------------------------------------------- |
| `src/alloy/`    | Hand-written C++: concepts, HAL drivers (one per peripheral IP version), arch support, async, OTA |
| `tools/alloy/`  | The Python CLI + code generator (single `alloy` package)          |
| `boards/`       | One `board.json` per board — data only, zero hand-written C++     |
| `examples/`     | Portable examples — zero preprocessor conditionals, enforced by CI |
| `libs/`         | Reference driver libraries + the registry the ecosystem grows from |
| `tests/`        | Host unit tests, compile-fail guards, and the Renode behaviour oracle |
| `cmake/`        | Internal CMake templates rendered by the CLI (users never see CMake) |
| `scripts/`      | Contract gates (`check_contract.sh`) and dev helpers              |
| `docs/`         | The documentation site (Material for MkDocs)                      |

Device data (register maps per IP version, per-chip instances, pin routes, clock trees)
lives in the sibling [`alloy-devices`](../alloy-devices) repo and reaches this repo only
through generated headers.

Read [NORTH_STAR.md](NORTH_STAR.md) before contributing — it is the contract that keeps
this rebuild from repeating the old ecosystem's failures.

## Status

Active greenfield rebuild. Working today: 5 chip families / 8 boards through the full
data → codegen → HAL → CLI loop; `alloy new / build / flash / run / monitor / emulate / test`
with `--board` switching; UF2, esptool and OpenOCD/probe flash runners; the coroutine runtime;
the driver registry; and UART field update with trial/confirm/rollback and signed images. CI
runs every example × every board, contract gates, an out-of-repo scaffold build, and a blocking
Renode suite that asserts behaviour rather than compilation.

Not yet: nRF52840 (the "new family = data only" proof) and ESP32-C3 (RISC-V). The three flash
drivers behind field update are proven in emulation and by construction, not yet on silicon.

License: MIT OR Apache-2.0 (same as the previous alloy).
