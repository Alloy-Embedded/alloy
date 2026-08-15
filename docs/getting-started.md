# Get started

From nothing to firmware that samples an input, reports over a serial line, and recompiles for a
different microcontroller without a single edit to your source.

You do **not** need a board for any of it. alloy generates a [Renode](https://renode.io) machine
from the same chip data your firmware is compiled against, so every step below has a
no-hardware path — and that path is the one alloy's own CI uses to prove its drivers.

!!! info "The path, end to end"
    1. [Install](#install) the CLI and a cross-toolchain.
    2. [Create a project](#create-a-project) for a board.
    3. [Run it](#run-it) — on the board, or in the emulator.
    4. [Read what you got](#read-what-you-got): roles, and why the file has no `#ifdef`.
    5. [Make it do something](#make-it-do-something) — sample an input and report it.
    6. [Move it to another MCU](#move-it-to-another-mcu) without touching `src/`.
    7. [See where the board facts live](#where-the-board-facts-live), which is why step 6 worked.
    8. [Test your logic on your laptop](#test-your-logic-on-your-laptop).
    9. [Understand what is actually proven](#before-you-trust-it) before you ship anything.

    Steps 1–4 are the tutorial. Steps 5–9 are the part that decides whether alloy is any use to
    you.

## Install

alloy ships as a Python CLI, published as **`alloy-embedded`**. Install it into its own isolated
environment with [uv](https://docs.astral.sh/uv/) or [pipx](https://pipx.pypa.io/):

```console
$ uv tool install alloy-embedded
```

```console
$ pipx install alloy-embedded
```

!!! warning "Mind the package name"
    An unrelated project owns `alloy` on PyPI. `pipx install alloy` installs *that*, not this —
    always install `alloy-embedded`. The command it gives you is still called `alloy`.

Then let alloy fetch the cross-toolchains it needs (Arm GCC, and the Xtensa toolchain for
ESP32). Toolchains install into `~/.alloy/tools` and never touch your system:

```console
$ alloy setup
```

`alloy setup --check` tells you where you stand without downloading anything — it covers the two
cross-toolchains plus the three host tools alloy shells out to:

```console
$ alloy setup --check
arm-gnu-toolchain    ok (PATH)                /…/bin/arm-none-eabi-gcc
xtensa-esp-elf       ok (~/.alloy/tools)      /…/.alloy/tools/xtensa-esp-elf/bin/xtensa-esp-elf-gcc
openocd              ok (PATH)                /opt/homebrew/bin/openocd
cmake                ok (PATH)                /opt/homebrew/bin/cmake
ninja                ok (PATH)                /opt/homebrew/bin/ninja
```

**CMake** and **Ninja** are required for every build; **OpenOCD** only for the flash path on
boards whose probe runner needs it. All three are one package away on every OS.

!!! note "Integrity-checked downloads"
    `alloy setup` refuses to install a toolchain whose checksum is not pinned. If you are
    working from an unreleased checkout you can opt in for a one-off with
    `ALLOY_ALLOW_UNPINNED=1 alloy setup`.

!!! warning "Working from a git checkout instead of the wheel"
    The published wheel embeds the framework, so an installed `alloy` finds itself. A checkout
    does not — run `alloy new` outside the repo and you get:

    ```
    error: could not find the alloy framework — run inside the repo, set ALLOY_ROOT, or
    install the alloy package (the wheel embeds the framework)
    ```

    Point it at your checkouts once and the error goes away:

    ```console
    $ export ALLOY_ROOT=/path/to/alloy
    $ export ALLOY_DEVICES_ROOT=/path/to/alloy-devices   # only if you also cloned the chip DB
    ```

    Only `alloy new` needs this. The project it scaffolds records the framework path in its
    `alloy.toml`, so `alloy build`, `alloy test` and `alloy emulate` work inside it with no
    environment at all.

## Create a project

Pick a board and scaffold a project. The scaffold is a portable *blink + echo* — the same code
that CI compiles for every board, so it is guaranteed to build:

```console
$ alloy new hello --board nucleo_g071rb
created hello/ (board: nucleo_g071rb, framework: /…/alloy)
next:  cd hello && alloy run
```

That gives you:

```
hello/
├── alloy.toml        # project name + board selection
├── src/
│   └── main.cpp      # your portable application
└── .gitignore
```

Don't know the board id? List them:

```console
$ alloy boards
esp32_devkit             ESP32 DevKit (WROOM-32, 30-pin)  chip=espressif/esp32
esp_wrover_kit           Espressif ESP-WROVER-KIT         chip=espressif/esp32
nucleo_f722ze            ST Nucleo-F722ZE (Nucleo-144)    chip=st/stm32f722
nucleo_f767zi            ST Nucleo-F767ZI (Nucleo-144)    chip=st/stm32f767
nucleo_g071rb            ST Nucleo-G071RB                 chip=st/stm32g071rb
nucleo_g0b1re            ST Nucleo-G0B1RE                 chip=st/stm32g0b1re
raspberry_pi_pico        Raspberry Pi Pico                chip=raspberrypi/rp2040
rp2040_zero              Waveshare RP2040-Zero            chip=raspberrypi/rp2040
same70_xplained          Microchip SAM E70 Xplained       chip=microchip/atsame70q21
```

Nine boards ship curated. If yours is not one of them, `alloy new mychip --chip st/stm32g431rb`
scaffolds an editable board you fill in yourself — see [Adding a board](guide/adding-a-board.md).

## Run it

Two paths. Pick the one that matches what is on your desk.

=== "With a board"

    ```console
    $ cd hello
    $ alloy run
    ```

    Builds the firmware, programs the board over its probe, and opens a serial monitor so you
    see output immediately. `alloy build`, `alloy flash` and `alloy monitor` are the same three
    steps separately.

    ```console
    $ alloy build
       text	   data	    bss	    dec	    hex	filename
       1944	      0	   2224	   4168	   1048	…/hello.elf

    built …/.alloy/build-tree/nucleo_g071rb/out/hello.elf
    ```

=== "Without a board"

    ```console
    $ cd hello
    $ alloy emulate
    platform: …/.alloy/build-tree/nucleo_g071rb/out/nucleo_g071rb.repl
    script:   …/.alloy/build-tree/nucleo_g071rb/out/nucleo_g071rb.resc
    uart:     sysbus.usart2
    …
    17:55:33.6919 [INFO] nucleo_g071rb: Machine started.
    17:55:33.8461 [INFO] usart2: […] alloy hello: blinking + echoing
    ```

    That last line is **your firmware's own banner**, produced by the real UART driver writing
    to Renode's model of USART2. Type `q` at the Renode prompt to quit.

    Renode has to be findable: put `renode` on your `PATH` or drop an install under
    `~/.alloy/tools/`. `alloy emulate --emit-only` writes the platform and script and stops,
    which is what you want if you drive Renode yourself. See [Emulation](guide/emulation.md).

!!! warning "The emulator's clock is not your board's clock"
    Renode runs the instruction set faithfully, not in real time. Measured on `nucleo_g071rb`:
    the machine advanced roughly one *virtual* second per four wall-clock seconds, and the
    firmware's own `alloy::uptime_ms()` advanced one second per two virtual seconds — SysTick
    under the emulated clock does not track the target's. Use emulation to check **what** your
    firmware does, never **how fast**. Timing is one of the things only a board can settle.

## Read what you got

Open `src/main.cpp`. The scaffold blinks and echoes over the debug UART:

```cpp title="src/main.cpp"
// Portable hello — blink AND echo, so the board never looks dead after a
// flash. Identical bytes on every supported board; zero #ifdefs.
#include <alloy/board.hpp>

#include <cstdint>

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy hello: blinking + echoing\r\n");

    std::uint32_t last_toggle = alloy::uptime_ms();
    while (true) {
        std::uint8_t byte{};
        if (uart.read(byte)) {
            uart.write(byte);
        }
        if (alloy::uptime_ms() - last_toggle >= 500u) {
            board::led.toggle();
            last_toggle = alloy::uptime_ms();
        }
    }
}
```

Three things in that file are worth noticing, because they are the whole design:

- **`board::led` and `board::debug_uart` are *roles*, not peripherals.** Nothing here names a
  pin, a register, a peripheral instance or a vendor. The board data binds each role to real
  hardware, and a wrong binding is a compile error that names the pin.
- **There is no `#ifdef`.** Not in this file and not in the framework. Where boards differ, the
  difference is a compile-time boolean you branch on with `if constexpr`.
- **There is no heap and no RTOS.** `alloy::uptime_ms()` is a SysTick counter; `uart.write` goes
  straight at the peripheral.

Change `500u` to `100u`, rebuild, and the LED blinks five times faster. That is the tutorial
done. The rest of this page is the part that matters.

## Make it do something

A blinking LED proves the toolchain. This proves the framework: **read an input, report it, and
still be portable.** Replace `src/main.cpp` with:

```cpp title="src/main.cpp"
// A thermostat-shaped app: blink, echo, and report a sample once a second.
// Identical bytes on every supported board — no #ifdef anywhere.
#include <alloy/board.hpp>

#include <cstdint>

namespace {
// There is no <cstdio> here: print an unsigned number a digit at a time.
template <class Uart>
void write_u32(const Uart& uart, std::uint32_t value) {
    char buf[10];
    unsigned n = 0;
    do { buf[n++] = static_cast<char>('0' + value % 10u); value /= 10u; } while (value != 0u);
    while (n != 0u) { uart.write(static_cast<std::uint8_t>(buf[--n])); }
}

// The whole application. `sample` is whatever the board could give us — a real
// ADC read where the board has that role, a constant where it does not.
template <class Uart, class Sample>
[[noreturn]] void run(const Uart& uart, Sample sample) {
    std::uint32_t last = alloy::uptime_ms();
    while (true) {
        std::uint8_t byte{};
        if (uart.read(byte)) {
            uart.write(byte);  // echo, so a real board proves its RX path too
        }
        if (alloy::uptime_ms() - last >= 1000u) {
            last = alloy::uptime_ms();
            board::led.toggle();
            uart.write("t=");
            write_u32(uart, last / 1000u);
            uart.write("s sample=");
            write_u32(uart, sample());
            uart.write("\r\n");
        }
    }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("thermo: up\r\n");

    if constexpr (board::caps::adc) {
        auto adc = board::adc::open();               // opened once, not per loop
        run(uart, [&] { return std::uint32_t{adc.read(3)}; });
    } else {
        uart.write("thermo: no adc role on this board\r\n");
        run(uart, [] { return std::uint32_t{0}; });
    }
}
```

Then `alloy run` (with a board) or `alloy emulate` (without one):

```console
$ alloy emulate
…
usart2: […] thermo: up
usart2: […] t=1s sample=0
usart2: […] t=2s sample=0
usart2: […] t=3s sample=0
```

`sample=0` is correct here: nothing is driving the emulated ADC pin, and an unfed channel reads
zero. On a real Nucleo-G071RB it is the conversion of ADC channel 3.

Two things in that file are worth copying into your own code:

**The peripheral is opened once, outside the loop, but the loop is written once too.** That is
what `run()` being a template buys: the capability branch happens at the top, where you build
things, and the body below it never asks again. Opening a peripheral inside a hot loop is the
most common shape of this mistake — on the STM32G0 the ADC's calibration sequence alone made an
emulated iteration take about 26 wall-clock seconds.

**`board::caps::adc` is a `constexpr bool`, so the branch you did not take does not exist in the
binary.** Compile this for the RP2040-Zero, which declares no `adc` role, and there is no ADC
code in the image at all — not disabled, absent. That is what replaces `#ifdef` here; see
[Portable code](guide/portable-code.md).

!!! tip "One sample per second is the slow way"
    Polling the ADC from the main loop costs you a CPU that is awake for nothing. When you need
    a *stream* — a control loop sampling under a PWM carrier, say — `adc.ring()` hands the
    conversions to a DMA engine and gives you filled halves to work on. See
    [Streaming data without the CPU](guide/dma.md), which carries a per-board table of what is
    available and a per-capability table of what has actually been witnessed. Read the second
    one before you build a product on it: how well that path is proven differs on every engine.

## Move it to another MCU

This is the claim the whole framework rests on, so do it rather than believe it. **Do not touch
`src/`.** Change the board:

```console
$ alloy set-board nucleo_g0b1re
board set to nucleo_g0b1re
```

(That edits one line of `alloy.toml`. `alloy build --board <id>` does the same thing for a
one-off without changing the file.) Now run it again:

```console
$ alloy emulate
…
usart2: […] thermo: up
usart2: […] t=1s sample=0
usart2: […] t=2s sample=0
```

Same source, a different die — 512 KB of flash instead of 128 KB, a different UART instance
behind the same `board::debug_uart` role. Try `same70_xplained` next: a Cortex-M7 from another
vendor, with a Microchip USART instead of an ST one, and the same file still runs.

To check every board at once, `alloy matrix` builds your `src/` for each of them and reports
what fits where:

```console
$ alloy matrix --boards nucleo_g071rb,nucleo_g0b1re,same70_xplained,rp2040_zero,esp32_devkit
…
board                       flash               ram  time
nucleo_g071rb       2.2K / 128.0K      2.2K / 36.0K  8.4s
nucleo_g0b1re       3.2K / 512.0K     3.2K / 144.0K  0.2s
same70_xplained    2.2K / 2048.0K     2.3K / 384.0K  0.2s
rp2040_zero        2.7K / 2048.0K     2.1K / 264.0K  0.2s
esp32_devkit       2.9K / 1984.0K     0.3K / 272.0K  0.2s

5 of 5 boards — the same src/, no #ifdef
```

Three architectures — Cortex-M0+, Cortex-M7 and Xtensa LX6 — from one file. Run `alloy matrix`
with no `--boards` and it does all nine. `alloy ci-init` writes a GitHub Actions workflow that
runs the same check on every push, which is how alloy keeps its own portability claim from
rotting.

## Where the board facts live

Step 6 worked because none of the differences between those MCUs are in your source. They are in
the board's `board.json`, and this is the file to read when you want to know what a role actually
is:

```json title="boards/nucleo_g071rb/board.json (excerpt)"
{
  "id": "nucleo_g071rb",
  "chip": "st/stm32g071rb",
  "clock_profile": "pll_64mhz",
  "roles": {
    "led":        { "pin": "pa5", "active": "high", "label": "LD4 (green, D13)" },
    "button":     { "pin": "pc13", "active": "low", "label": "B1 USER" },
    "debug_uart": { "peripheral": "usart2", "tx": "pa2", "rx": "pa3", "baud": 115200 },
    "adc":        { "peripheral": "adc", "label": "internal vref/temp channels" },
    "i2c":        { "peripheral": "i2c1", "scl": "pb8", "sda": "pb9" }
  },
  "dma": {
    "adc.conv":      { "controller": "dma1", "channel": 1 },
    "debug_uart.rx": { "controller": "dma1", "channel": 2 },
    "debug_uart.tx": { "controller": "dma1", "channel": 3 }
  },
  "probe": { "kind": "stlink", "runner": "probe-rs", "chip_id": "STM32G071RBTx" }
}
```

`"active": "low"` on the button is why `board::button.is_active()` returns `true` when it is
pressed even though the pin reads low — the polarity is a board fact, not something your app
carries. The `"dma"` block is what makes `adc.ring()` and DMA-driven UART transmit available on
this board and absent on one that does not declare them.

You do not have to open the file to ask what a board offers:

```console
$ alloy board-info nucleo_g071rb
  warning: [roles] pa5 is used by led and led_pwm — only one of them can drive it at a time
nucleo_g071rb  ST Nucleo-G071RB
  chip     st/stm32g071rb (STM32G071RB)
  source   framework  (read-only)
  clock    pll_64mhz — HSI16 /1 x8 /2 = 64 MHz via PLLR, 2 wait states (order hardware-verified…)
  roles    adc, button, debug_uart, i2c, led, led_pwm, spi, watchdog
  caps     adc, button, debug_uart, dma, i2c, irq, led, led_pwm, spi, watchdog
```

Every name under `caps` is a `board::caps::` boolean you can branch on. That warning is real and
harmless: this Nucleo's LED sits on PA5, which both the `led` (plain GPIO) and `led_pwm` (TIM2
channel 1) roles claim — you may use either, just not both at once.

Your own hardware becomes a board the same way: `alloy board-clone nucleo_g071rb myboard` to
start from a close one, or `alloy new myproject --chip <id>` for an empty one, then
`alloy board-validate` — which reports every problem at once, located, with the pins that would
have worked. See [Boards](guide/boards.md) and [Adding a board](guide/adding-a-board.md).

## Test your logic on your laptop

The part of a firmware worth testing is rarely the part that touches registers. alloy scaffolds
that split for you:

```console
$ alloy new thermo --board nucleo_g071rb --with-tests
created thermo/ (board: nucleo_g071rb, framework: /…/alloy)
  + src/app.hpp
  + src/main.cpp
  + tests/test_app.cpp
  + tests/CMakeLists.txt
next:  cd thermo && alloy test     # runs on your laptop, no board
       cd thermo && alloy run      # and on the target
```

`src/app.hpp` holds a real hysteresis thermostat, templated on alloy's *concepts*
(`alloy::I2cBus`, `alloy::OutputPin`) rather than on any chip. So the same source compiles
against the generated silicon driver on the target and against a fake on your laptop:

```console
$ cd thermo && alloy test
…
1/1 Test #1: thermo_tests .....................   Passed    0.32 sec

100% tests passed, 0 tests failed out of 1
```

The five tests it ships are exactly the cases that are miserable on a bench and trivial here:
that the output does not chatter inside the hysteresis band, and that **a sensor which stops
answering does not move the output**. Reproducing that second one on hardware means pulling a
wire at the right moment; in the test it is one flag. See [Testing](guide/testing.md).

## Before you trust it

alloy is explicit about the difference between *built*, *tested* and *proven*, and you should
read a feature's evidence before you put it in a product. Three words appear throughout the
docs, and they mean different things:

| | What it means | What it does not mean |
| --- | --- | --- |
| **Renode-proven** | A blocking CI job boots the firmware on an emulated die and asserts on real UART output produced by real driver code. | That the model is the silicon. Renode models what its authors implemented; unmodelled bits are silently absent. |
| **Host-tested** | Real driver code runs on your laptop against a register double. Proves sequencing and bookkeeping. | Anything about the register map being right. |
| **Compiles only** | It builds and links. | Nothing else. |

And the fourth statement, which is not a tier but a footer on all of it: **almost nothing in
this framework has been run on physical silicon by this project.** The exception is the
[message bus](guide/bus.md), which carries bench measurements from a SAM E70 Xplained, plus two
driver quirks that were found on that same board and are marked as such in the headers that own
them. Everything else — the PWM bridge, the flash drivers, the async runtime, the security
verbs — says so on its own page. `alloy chip-status <chip>` ends every run with the line
*"None of these is evidence from silicon."*

That is not a warning against using alloy. It is the information you need to decide what to
verify yourself, and it is why [Emulation](guide/emulation.md) lists which boards run which
assertions rather than claiming a family "works".

## Where to go next

<div class="grid cards" markdown>

-   :material-book-open-page-variant:{ .lg .middle } __How portability works__

    ---

    Board roles, capabilities and `if constexpr` — the model behind "one `main.cpp`",
    and the compile errors it buys you.

    [:octicons-arrow-right-24: Portable code](guide/portable-code.md)

-   :material-chip:{ .lg .middle } __Use the peripherals__

    ---

    GPIO, UART, SPI, I²C, ADC, PWM, timers, watchdog — with snippets that compile.

    [:octicons-arrow-right-24: Peripherals](guide/peripherals.md)

-   :material-fan:{ .lg .middle } __Drive a motor__

    ---

    Carrier resolution, centre-aligned counting, an ADC trigger, and the three-phase
    bridge with a dead time you cannot forget to state.

    [:octicons-arrow-right-24: PWM and motor control](guide/pwm.md)

-   :material-waveform:{ .lg .middle } __Sample without the CPU__

    ---

    `adc.ring()`, `uart.rx_ring()` and DMA-fed buffers — with a per-board table of what
    exists and a per-capability table of what is proven.

    [:octicons-arrow-right-24: Streaming data without the CPU](guide/dma.md)

-   :material-package-variant:{ .lg .middle } __Add a part__

    ---

    Sensors, displays and clocks vendored with one command: `alloy lib add sht31`.

    [:octicons-arrow-right-24: Driver libraries](guide/libraries.md)

-   :material-update:{ .lg .middle } __Ship and update it__

    ---

    A/B slots, a trial boot that rolls back on its own, and signed images over a plain UART.
    Set this up before you need it.

    [:octicons-arrow-right-24: Firmware update](guide/firmware-update.md)

</div>

Prefer an IDE? The [VS Code extension](guide/vscode.md) drives the same CLI behind a visual pin
and clock configurator. Want to see more code? [Examples](guide/examples.md) indexes what is in
the repository — CI builds them across the board matrix on every push, and runs a subset of them
under Renode with assertions.
