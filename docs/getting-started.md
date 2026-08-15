# Get started

From nothing to a blinking LED in a few minutes.

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

## Build, flash, run

=== "Run (build + flash + monitor)"

    ```console
    $ cd hello
    $ alloy run
    ```

    Builds the firmware, programs the board, and opens a serial monitor so you see output
    immediately.

=== "Build only"

    ```console
    $ alloy build
    built .alloy/build-tree/nucleo_g071rb/out/hello.elf
    ```

=== "Retarget to another board"

    ```console
    $ alloy build --board same70_xplained
    ```

    The **same source** — no edits — compiled for a different MCU.

## Your first change

Open `src/main.cpp`. The scaffold blinks and echoes over the debug UART:

```cpp title="src/main.cpp"
#include <alloy/board.hpp>
#include <cstdint>

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy hello: blinking + echoing\r\n");

    std::uint32_t last = alloy::uptime_ms();
    while (true) {
        std::uint8_t byte{};
        if (uart.read(byte)) {
            uart.write(byte);              // echo whatever you type
        }
        if (alloy::uptime_ms() - last >= 500u) {
            board::led.toggle();           // blink
            last = alloy::uptime_ms();
        }
    }
}
```

Everything you touch here — `board::led`, `board::debug_uart` — is a **role** the board data
provides. On a board that lacks a role, the code still compiles (the role degrades to a no-op
stub or a compile-time error), which is why the *same file* works everywhere.

<div class="grid cards" markdown>

-   :material-book-open-page-variant:{ .lg .middle } __How portability works__

    ---

    Board roles, capabilities, and `if constexpr` — the model behind "one `main.cpp`".

    [:octicons-arrow-right-24: Portable code](guide/portable-code.md)

-   :material-chip:{ .lg .middle } __Use the peripherals__

    ---

    GPIO, UART, SPI, I²C, ADC, PWM, DMA — with copy-pasteable snippets.

    [:octicons-arrow-right-24: Peripherals](guide/peripherals.md)

-   :material-fan:{ .lg .middle } __Drive a motor__

    ---

    Carrier resolution, centre-aligned counting, an ADC trigger, and the
    three-phase bridge with a dead time you cannot forget to state.

    [:octicons-arrow-right-24: PWM and motor control](guide/pwm.md)

</div>

## Try it without hardware

Don't have the board yet? alloy can boot your firmware in the [Renode](https://renode.io)
emulator:

```console
$ alloy emulate
…
17:55:33.6919 [INFO] nucleo_g071rb: Machine started.
17:55:33.8461 [INFO] usart2: […] alloy hello: blinking + echoing
```

That last line is your firmware's own banner, coming out of an emulated USART2. Type `q` at the
Renode prompt to quit.

The emulated machine is generated from the same chip data your firmware compiles against — see
[Emulation](guide/emulation.md), which is also how alloy's own CI proves driver behaviour.

## Next

- Add a part: [driver libraries](guide/libraries.md) — `alloy lib add sht31`.
- Prefer an IDE? The [VS Code extension](guide/vscode.md) has a visual pin and clock configurator.
- Shipping a product? Set up [firmware update over UART](guide/firmware-update.md) before you
  need it.
