# Get started

From nothing to a blinking LED in a few minutes.

## Install

alloy ships as a Python CLI. The recommended install is [pipx](https://pipx.pypa.io/), which
keeps it in its own isolated environment:

```console
$ pipx install alloy
```

Then let alloy fetch the cross-toolchains it needs (Arm GCC, and the Xtensa toolchain for
ESP32). Toolchains install into `~/.alloy/tools` and never touch your system:

```console
$ alloy setup
```

!!! note "Integrity-checked downloads"
    `alloy setup` refuses to install a toolchain whose checksum is not pinned. If you are
    working from an unreleased checkout you can opt in for a one-off with
    `ALLOY_ALLOW_UNPINNED=1 alloy setup`.

You will also need **CMake** and **Ninja** on your `PATH` (both are one package away on every
OS).

## Create a project

Pick a board and scaffold a project. The scaffold is a portable *blink + echo* — the same code
that CI compiles for every board, so it is guaranteed to build:

```console
$ alloy new hello --board nucleo_g071rb
created hello/ (board: nucleo_g071rb)
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
nucleo_g071rb   ST Nucleo-G071RB   chip=st/stm32g071rb
same70_xplained SAM E70 Xplained   chip=microchip/atsame70q21
...
```

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

</div>

## Try it without hardware

Don't have the board yet? alloy can boot your firmware in the [Renode](https://renode.io)
emulator:

```console
$ alloy emulate --board nucleo_f722ze
...
alloy uart_echo ready
```
