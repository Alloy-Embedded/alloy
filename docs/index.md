---
hide:
  - navigation
  - toc
---

<div class="hero" markdown>

# One `main.cpp`. Any microcontroller.

A from-scratch **C++23** framework for microcontrollers: write your app once, recompile it
for a different board by changing **one line** — no `#ifdef`, no RTOS required, no IDE required.
Mistakes are caught at **compile time**, in errors a beginner can read.

[Get started :material-rocket-launch:](getting-started.md){ .md-button .md-button--primary }
[Browse the guide :material-book-open-variant:](guide/portable-code.md){ .md-button }
[View on GitHub :fontawesome-brands-github:](https://github.com/Alloy-Embedded/alloy){ .md-button }

</div>

```cpp title="src/main.cpp — identical bytes on every supported board"
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
$ uv tool install alloy-embedded         # or: pipx install alloy-embedded
$ alloy new hello --board esp32_devkit && cd hello
$ alloy run                              # build + flash + serial monitor
$ alloy build --board nucleo_g071rb      # same code, different MCU — one flag
```

## Why alloy

<div class="grid cards" markdown>

-   :material-swap-horizontal:{ .lg .middle } __Truly portable__

    ---

    The same `main.cpp` recompiles for another board by changing one config line.
    Portability comes from board *roles* and `if constexpr (board::caps::x)` — **zero
    preprocessor conditionals**, enforced by CI.

    [:octicons-arrow-right-24: Portable code](guide/portable-code.md)

-   :material-shield-check:{ .lg .middle } __Safe at compile time__

    ---

    A wrong pin route is a **compile error that names the pin**, not a runtime surprise.
    Peripherals are selected by type, with no vtables and no heap — the zero-cost claim is
    real.

    [:octicons-arrow-right-24: Architecture](reference/architecture.md)

-   :material-rocket-launch:{ .lg .middle } __Trivially easy__

    ---

    `uv tool install alloy-embedded && alloy new && alloy run` → a blinking LED in minutes.
    No IDE, no vendor code generator, no CMake to hand-write.

    [:octicons-arrow-right-24: Get started](getting-started.md)

-   :material-database-cog:{ .lg .middle } __Data-driven__

    ---

    **Facts are generated, behavior is hand-written.** Adding a chip that reuses known
    peripheral IP costs *data only* — no new C++, no new emitter branches.

    [:octicons-arrow-right-24: Adding a board](guide/adding-a-board.md)

-   :material-update:{ .lg .middle } __Updatable in the field__

    ---

    A/B firmware slots computed from chip data, a trial boot that **rolls back on its own**, and
    optional Ed25519-signed images — over a plain UART.

    [:octicons-arrow-right-24: Firmware update](guide/firmware-update.md)

-   :material-play-box-outline:{ .lg .middle } __Testable without hardware__

    ---

    Firmware boots on an emulated MCU generated from the same chip data, and CI **asserts on real
    UART output** — drivers, coroutines and the whole update lifecycle.

    [:octicons-arrow-right-24: Emulation](guide/emulation.md)

</div>

## Supported boards

The `main.cpp` at the top of this page compiles for every one of these — verified board by
board, not asserted.

!!! warning "Read this column before you trust it"
    A <span class="status-pill ok">silicon</span> cell means **the maintainer reports the
    peripheral running on that named board** during bring-up, with observed register values or
    byte-level results. There is no hardware CI runner: nothing in that column is re-checked by a
    machine, and none of it is re-checked at all once it is written down. Where a feature page
    disagrees with this table, **the feature page wins** — several of them state plainly that
    they have never touched silicon ([PWM bridge](guide/pwm.md#what-has-actually-been-proven-and-what-has-not),
    [async](guide/async.md), [firmware update](guide/firmware-update.md),
    [security](guide/security.md)). <span class="status-pill beta">emulation</span> means a
    blocking CI leg runs it under [Renode](guide/emulation.md), which is the strongest
    *automated* evidence this project has.

| Family | Core | Board | Reported working |
| --- | --- | --- | --- |
| ST STM32G0 | Cortex-M0+ | Nucleo-G0B1RE | <span class="status-pill ok">silicon</span> I²C, ADC, DMA, PWM, RTC, DAC, CAN, watchdog, flash |
| ST STM32G0 | Cortex-M0+ | Nucleo-G071RB | <span class="status-pill ok">silicon</span> PLL, GPIO, UART echo · 8 emulation gates |
| Microchip SAME70 | Cortex-M7 | SAM E70 Xplained | <span class="status-pill ok">silicon</span> PLLA, PIO, USART, I²C, ADC, DMA, EEPROM, Ethernet |
| Espressif ESP32 | Xtensa LX6 | WROVER-KIT, DevKit | <span class="status-pill ok">silicon</span> boot chain, UART, LEDC PWM |
| Raspberry Pi RP2040 | 2× Cortex-M0+ | RP2040-Zero | <span class="status-pill ok">silicon</span> boot2+CRC, 125 MHz clock, WS2812 |
| ST STM32F7 | Cortex-M7 | Nucleo-F722ZE | <span class="status-pill beta">emulation</span> boot + async runtime |
| Raspberry Pi RP2040 | 2× Cortex-M0+ | Raspberry Pi Pico | <span class="status-pill beta">compiles</span> hardware validation pending |
| ST STM32F7 | Cortex-M7 | Nucleo-F767ZI | <span class="status-pill beta">compiles</span> the network examples only |

See the [README](https://github.com/Alloy-Embedded/alloy#supported-silicon) for the per-family
driver matrix, and [Testing](guide/testing.md) for how alloy separates *built* from *proven*.

!!! tip "Your board isn't listed?"
    alloy is designed so a new board is **data, not code**. See
    [Adding a board](guide/adding-a-board.md) — if it reuses a peripheral IP alloy already
    models, it costs zero new C++.

## What you get

- **Peripherals**: GPIO, UART, SPI, I²C, ADC, PWM, DMA, timers, watchdog, RTC, DAC, CAN, on-chip flash/NVM — [coverage varies by family](https://github.com/Alloy-Embedded/alloy#peripheral-drivers-by-vendor-and-family); GPIO and UART are everywhere, CAN and DAC are one chip.
- **Async without an RTOS**: C++20 coroutines with [no heap and no dynamic allocation](guide/async.md).
- **A single CLI**: `new`, `build`, `flash`, `monitor`, `run`, `emulate`, `image`, `update`, `test` — [see all commands](guide/cli.md).
- **A driver ecosystem**: sensors, displays and clocks you [vendor with one command](guide/libraries.md) — grown outside the core, portable by construction.
- **Field updates**: A/B slots, trial boot with automatic rollback, and [signed images](guide/firmware-update.md).
- **An IDE, if you want one**: a [VS Code extension](guide/vscode.md) with a visual pin/clock configurator — driven entirely by the CLI.
- **Host-testable app logic**: the scheduler and drivers run on your laptop against fakes, so you unit-test without hardware.
- **CI that executes, not just compiles**: firmware is booted under [Renode](https://renode.io) and asserted on real UART output.
