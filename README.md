# alloy

[![ci](https://github.com/Alloy-Embedded/alloy/actions/workflows/ci.yml/badge.svg)](https://github.com/Alloy-Embedded/alloy/actions/workflows/ci.yml)
[![docs](https://img.shields.io/badge/docs-alloy--embedded.github.io-blue)](https://alloy-embedded.github.io/alloy/)
[![license](https://img.shields.io/badge/license-MIT%20OR%20Apache--2.0-green)](LICENSE)

**One portable `main.cpp`. Any microcontroller.**

A from-scratch C++23 framework for MCUs. Write the application once; retarget it to different
silicon by changing one line of config. No `#ifdef`, no RTOS required, no IDE required, no vendor
code generator to fight.

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
$ uv tool install alloy-embedded          # the command it installs is `alloy`
$ alloy new hello --board nucleo_g071rb && cd hello
$ alloy run                               # build + flash + serial monitor
$ alloy build --board same70_xplained     # same source, different MCU — one flag
```

📖 **[Documentation](https://alloy-embedded.github.io/alloy/)** · [Get started](https://alloy-embedded.github.io/alloy/getting-started/) · [Portable code](https://alloy-embedded.github.io/alloy/guide/portable-code/) · [CLI reference](https://alloy-embedded.github.io/alloy/guide/cli/)

---

## Why alloy

**Facts are generated, behaviour is hand-written.** Register maps, pin routes and clock trees live
in a separate data repository ([alloy-devices](https://github.com/Alloy-Embedded/alloy-devices))
and reach the compiler only as generated headers. `scripts/check_contract.sh` fails CI on any
silicon address that appears in hand-written framework code — including in the code generator
itself. Vendor tools generate code you then own and hand-edit; alloy generates the *facts* and
refuses to let an address into the behaviour.

**One driver per peripheral IP version, selected by the type system.** Each driver is a partial
specialization constrained on the instance's IP (`requires std::same_as<typename Inst::ip, …>`),
and the primary template is left undefined. Bind a peripheral whose IP has no driver and you get a
compile error naming it. That is why one SPI driver serves 259 chip definitions across three
families with no per-chip code, and why adding a chip is a data change.

**Mistakes become compile errors, not bench sessions.** Routing a pin with no alternate function
fails the build *and names the pin*. A baud rate that the real divisor cannot hit within tolerance
fails the build, checked against each driver's own formula. A coroutine frame that does not fit its
declared storage fails the build. CI has acceptance tests asserting these errors still happen.

**Emulation is a blocking gate on behaviour, not on compilation.** Twelve [Renode](https://renode.io)
legs must pass on every commit, on platforms generated from the *same* chip data the firmware
compiles against — so the model and the driver cannot drift. They assert real UART output: an I²C
driver ACKing a bus slave (with a no-slave negative control), a SPI driver reading back what a
slave shifted out, two coroutines interleaving on two different vendors' cores, and the entire
firmware-update lifecycle through the real `alloy update` client.

**The maturity ladder is written down, not implied.** Every table below distinguishes *a driver
exists*, *CI compiles it*, *emulation asserts its behaviour*, and *someone ran it on the board* —
because those are four different things, and most of the interesting bugs live between them.

---

## Supported silicon

### Boards

| Board | Chip | Core | Roles wired | Flashed via | Proven |
| --- | --- | --- | --- | --- | --- |
| `nucleo_g0b1re` | STM32G0B1RE | Cortex-M0+ | 14 | ST-Link / OpenOCD | 🟢 silicon — 12 commits: I²C, ADC, DMA, PWM, RTC, DAC, CAN, watchdog, flash/NVM, filesystem |
| `nucleo_g071rb` | STM32G071RB | Cortex-M0+ | 8 | ST-Link / probe-rs | 🟢 silicon — GPIO, PLL, UART echo · 🔵 8 of 12 emulation gates |
| `same70_xplained` | ATSAME70Q21 | Cortex-M7 | 9 | CMSIS-DAP / OpenOCD | 🟢 silicon — GPIO, USART, I²C, ADC, DMA, watchdog, EEPROM, Ethernet ping¹ · 🔵 emulation gates |
| `esp_wrover_kit` | ESP32 | Xtensa LX6 | 4 | esptool | 🟢 silicon — boot chain, UART, LEDC PWM |
| `esp32_devkit` | ESP32 | Xtensa LX6 | 4 | esptool | 🟢 silicon — boot log + banner through `alloy run` |
| `rp2040_zero` | RP2040 | 2× Cortex-M0+ | 2 | BOOTSEL / UF2 | 🟢 silicon — CRC'd boot2, 125 MHz clock program, WS2812 at ±150 ns |
| `nucleo_f722ze` | STM32F722ZE | Cortex-M7 | 5 | ST-Link / OpenOCD | 🔵 emulation — boot + async runtime · ⚪ compiles |
| `raspberry_pi_pico` | RP2040 | 2× Cortex-M0+ | 2 | BOOTSEL / UF2 | ⚪ compiles — hardware validation pending |
| `nucleo_f767zi` | STM32F767ZI | Cortex-M7 | 5 | ST-Link / OpenOCD | ⚪ compiles — the lwIP examples only² |

🟢 **silicon** — a commit reports it running on that named board, with observed register values or
byte-level results. These are self-reports by the maintainer: there is no hardware CI runner.
🔵 **emulation** — a blocking Renode job asserts observable behaviour on every commit.
⚪ **compiles** — CI cross-compiles 30 examples for it; nothing asserts behaviour.

> ¹ SAME70 Ethernet: ping (ARP + ICMP) worked on silicon after six bring-up bugs
> ([f12c8df](https://github.com/Alloy-Embedded/alloy/commit/f12c8df)), with a known residual ~10%
> packet loss from marginal RMII sampling. Functional, not yet clean.
> ² `nucleo_f767zi` does not meet the project's own bar for "supported" (CI compiling `blink` +
> `uart_echo`); it exists to compile the network examples on a second MAC family.

**Roles wired** is the number of things the board file actually connects — LED, button, debug UART,
I²C, SPI, ADC… A driver existing for your family does not mean *your* board wires the pins for it.
Run `alloy board-info` to see exactly what a board offers.

### Peripheral drivers, by vendor and family

What has a **hand-written driver** plus **curated register data**, per chip family:

| Vendor / family | Chips in DB | Board? | GPIO | UART | I²C | SPI | ADC | PWM | DMA | Flash | WDT | RTC | DAC | CAN | Eth |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **ST** STM32G0 | 78 | ✅ 2 | ● | ● | ● | ● | ● | ● | ● | ● | ◐ | ◐ | ◐ | ◐ | · |
| **ST** STM32F7 | 89 | ✅ 2 | ● | ● | ● | ● | · | · | · | ● | ◐ | · | · | · | ◐ |
| **ST** STM32G4 | 93 | — | ● | ● | ● | ● | · | · | ● | ✗ | · | · | · | · | · |
| **ST** STM32F4 | 149 | — | ● | ● | ● | ● | · | · | · | ✗ | · | · | · | · | · |
| **Microchip** SAME70 | 1 | ✅ 1 | ● | ● | ● | ● | ● | · | ● | ● | ● | · | · | · | ● |
| **Espressif** ESP32 | 1 | ✅ 2 | ● | ● | ● | · | · | ● | · | · | · | · | · | · | · |
| **Raspberry Pi** RP2040 | 1 | ✅ 2 | ● | ● | · | · | · | · | · | · | · | · | · | · | · |

| | |
| --- | --- |
| **●** | driver + curated data across the whole family |
| **◐** | driver exists, but the data is curated only on the board-backed chip(s) of that family |
| **✗** | register data exists, driver not written yet |
| **·** | no driver |

**Read this table as availability, not as proof.** STM32G4 and STM32F4 have drivers and curated
data but **no board**, so no firmware containing them is ever built — the STM32F4 drivers say so in
their own headers ("not silicon-validated — tier-2, compile-checked"). Cross-reference the board
table above for what has actually run.

Known sharp edges, so they are not a surprise:

- **I²C and SPI are blocking, byte-at-a-time.** Only UART has an interrupt path, and only for RX.
  DMA completion is polled.
- **`st/i2c_v2` refuses transfers over 255 bytes**, and the ESP32 I²C driver is bounded by its
  32-byte FIFO. They fail loudly rather than corrupting a transfer.
- **CAN is a controller with a verified internal loopback** — no transceiver has been on the bus,
  the bit rate is fixed at 500 kbit/s, and only classic 11-bit frames are supported.
- **The ESP32 UART does not program the baud rate**; it inherits 115200 8N1 from the boot ROM.
- **Ethernet drivers are polled**, do no cache maintenance, and offer no offload or filtering.
- **Everything is built soft-float today**, including on the Cortex-M7 boards.

### Adding your own board

Your board is data, not code. If it uses a chip alloy already models, it costs **zero new C++**:

```console
$ alloy chips --vendor st                      # 412 chip definitions
$ alloy chip-info st/stm32g071rb               # which roles this silicon can fill, and with what
$ alloy board-clone nucleo_g071rb my_board     # start from something that works
$ alloy board-validate                         # every problem, located, with the pins that would work
```

---

## Quick start

### Install

```console
$ uv tool install alloy-embedded      # or: pipx install alloy-embedded
$ alloy setup                         # fetch cross-toolchains into ~/.alloy/tools
```

> ⚠️ Install **`alloy-embedded`**, not `alloy` — an unrelated project owns that name on PyPI.
> The command you get is still called `alloy`.

**CMake ≥ 3.25 and Ninja are yours to install** — `alloy setup` deliberately does not manage them;
it prints the command for your OS and stops. It also never edits your `PATH`: after installing a
toolchain, add `~/.alloy/tools/arm-gnu-toolchain/bin` (and `openocd/bin`) yourself, then confirm
with `alloy setup --check`.

### Five tips that save an hour

1. **Learn four verbs.** `alloy new`, `alloy build`, `alloy run` (= flash + monitor; leave the
   monitor with <kbd>Ctrl</kbd>+<kbd>]</kbd>), `alloy clean`. Plus one flag: `--board <id>`
   retargets any command for a single invocation without editing a file.
2. **Never write `#ifdef`** — CI rejects it in examples. Branch on `if constexpr (board::caps::x)`
   instead, and run `alloy board-info` to see exactly which capabilities the generated header will
   carry. Watch for the quiet case: a missing role usually degrades to a no-op stub, so the code
   compiles and does nothing.
3. **`alloy board-validate` before you debug.** It reports *every* problem at once, with the pins
   that would work (`debug_uart.tx: pa9 has no route to usart2 tx … try: pa2`). It is the
   compiler's `static_assert` moved to config time.
4. **`.alloy/` is disposable.** Everything under it is generated and keyed by board id;
   `alloy.toml` is the whole configuration. `alloy build --board other_board` can never reuse a
   stale tree, which is what makes retargeting an honest test of portability.
5. **`alloy matrix` is the portability claim, executable.** It builds the same `src/` for every
   board and prints one table — and it does not stop at the first failure.

---

## Beyond peripherals

| | |
| --- | --- |
| **[Async without an RTOS](https://alloy-embedded.github.io/alloy/guide/async/)** | C++20 coroutines whose frames are carved from caller-declared storage. `operator new` is deleted, so a task that would allocate does not compile. One executor, cooperative, `WFI` when idle. |
| **[Firmware update over UART](https://alloy-embedded.github.io/alloy/guide/firmware-update/)** | A/B slots derived from chip data, a trial boot that rolls back on its own when the new firmware misbehaves, a watchdog that turns a hung trial into a reset, and optional Ed25519-signed images. |
| **[Driver libraries](https://alloy-embedded.github.io/alloy/guide/libraries/)** | Sensors, displays and clocks vendored with `alloy lib add`, templated on concepts (`I2cBus`, `SpiBus`, `DelayNs`) so they never name a chip. Six reference drivers, each host-tested against mock buses. |
| **[Emulation](https://alloy-embedded.github.io/alloy/guide/emulation/)** | `alloy emulate` boots your firmware on a Renode platform generated from your chip data. It is also how CI asserts behaviour. |
| **[VS Code extension](https://alloy-embedded.github.io/alloy/guide/vscode/)** | Visual pin and clock configurator, build/flash/debug, device update. It holds no domain logic — every fact comes from `alloy … --json`. |
| **Networking & storage** | lwIP TCP/IP with socket, DHCP and HTTP facades, and a littlefs filesystem facade. Ethernet works on the SAME70 (see footnote 1); the STM32F7 MAC is compile-only. |
| **Building blocks** | `Result<T, E>` error handling, a strongly-typed `frequency` unit, an SPSC ring buffer, an NVM key/value store, IIR/FIR filters, control loops, capability tokens, and `ALLOY_FASTCODE` to run hot functions from RAM. |

---

## Layout

| Path | What |
| --- | --- |
| `src/alloy/` | Hand-written C++: concepts, 33 HAL drivers (one per peripheral IP version), 2 architectures, async runtime, OTA, net, fs |
| `tools/alloy/` | The Python CLI and code generator (the `alloy-embedded` package) |
| `boards/` | One `board.json` per board — data only, zero hand-written C++ |
| `examples/` | 36 portable examples, zero preprocessor conditionals, enforced by CI |
| `libs/` | Reference driver libraries and the registry the ecosystem grows from |
| `tests/` | Host unit tests, compile-fail acceptance tests, and the Renode behaviour oracle |
| `docs/` | The documentation site (Material for MkDocs) |
| `scripts/` | The contract gates and dev helpers |

Device data — register maps per IP version, per-chip instances, pin routes, clock trees — lives in
the sibling [`alloy-devices`](https://github.com/Alloy-Embedded/alloy-devices) repo and reaches
this one only through generated headers. The VS Code extension lives in
[`alloy-vscode`](https://github.com/Alloy-Embedded/alloy-vscode).

Read [NORTH_STAR.md](NORTH_STAR.md) before contributing — it is the contract that keeps this
rebuild from repeating the old ecosystem's failures.

---

## Status

Active greenfield rebuild. **412 chip definitions in the database; 7 of them board-backed.** Five
families run on real hardware; two more (STM32F4, STM32G4) have drivers and data but no board yet.

Honest about what is *not* done:

- **No hardware CI.** Every 🟢 above is a maintainer's self-report in a commit message, not an
  artifact from a test rig.
- **49 of 54 register files are a deliberate bring-up subset**, not a complete model of the IP.
- **The async runtime has never been reported running on physical silicon** — only under emulation,
  on three boards across two vendors.
- **Firmware update is proven unevenly.** The full bad-image → exhausted trials → rollback
  lifecycle and the signed-image oracle run on `nucleo_g071rb`; SAME70 runs the install/confirm
  half. The STM32G0 flash driver's register sequences are *not* exercised in emulation (Renode has
  no G0 flash-controller model), and none of the three flash drivers has run on silicon.
- **ESP32 Wi-Fi and Bluetooth are not implemented**, and [CONNECTIVITY.md](CONNECTIVITY.md)
  explains at length why that is a hard problem rather than a to-do item.
- **Next up:** nRF52840 (the "new family = data only" proof) and ESP32-C3 (RISC-V).

Where alloy is behind the alternatives: CubeMX's pin and clock coverage is far deeper, Zephyr has
an order of magnitude more drivers and real hardware-in-the-loop farms, and Rust's embassy is well
ahead on async breadth and on guarantees C++ cannot express. alloy's bet is a different one — that
a small, generated, honestly-labelled core beats a large one you cannot audit.

---

License: MIT OR Apache-2.0.
