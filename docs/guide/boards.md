# Boards

A **board** is pure data — one `board.json` that names a chip, a clock profile, and the roles
the board wires up (LED, button, debug UART, buses…). No hand-written C++ lives in a board.

## Supported boards

| Board id | Chip | Core | Roles wired | Notes |
| --- | --- | --- | --- | --- |
| `nucleo_g0b1re` | STM32G0B1RE | Cortex-M0+ | 14 | ST Nucleo-64 — the most complete board |
| `nucleo_g071rb` | STM32G071RB | Cortex-M0+ | 8 | ST Nucleo-64 |
| `same70_xplained` | ATSAME70Q21 | Cortex-M7 | 9 | Microchip, Ethernet + EEPROM |
| `nucleo_f722ze` | STM32F722ZE | Cortex-M7 | 5 | ST Nucleo-144 |
| `nucleo_f767zi` | STM32F767ZI | Cortex-M7 | 5 | ST Nucleo-144 — for the network examples |
| `esp_wrover_kit` | ESP32 | Xtensa LX6 | 4 | Espressif |
| `esp32_devkit` | ESP32 | Xtensa LX6 | 4 | Espressif DevKitC |
| `rp2040_zero` | RP2040 | 2× Cortex-M0+ | 2 | Waveshare, WS2812 |
| `raspberry_pi_pico` | RP2040 | 2× Cortex-M0+ | 2 | BOOTSEL flashing |

**Roles wired** is what the board file actually connects. A driver existing for your chip family
does not mean your board routes the pins for it — `alloy board-info` prints the exact list, and the
[README](https://github.com/Alloy-Embedded/alloy#supported-silicon) carries the per-family driver
matrix and how far each board has been proven.

List them from the CLI at any time (with `--json` for tooling/IDEs):

```console
$ alloy boards
$ alloy boards --json
```

## Selecting a board

The board lives in your project's `alloy.toml`:

```toml
[project]
name = "hello"

[board]
id = "nucleo_g071rb"
```

Change it three ways:

=== "Per command"

    ```console
    $ alloy build --board same70_xplained
    ```

=== "Change the default"

    ```console
    $ alloy set-board rp2040_zero
    ```

=== "Edit alloy.toml"

    Set `[board] id = "..."` by hand.

## What a board declares

A board only records *its own choices* — the chip's silicon facts (pin functions, addresses,
IRQ numbers) live in the [device database](adding-a-board.md), never in the board file:

```json
{
  "schema": "alloy.board.v1",
  "id": "nucleo_g071rb",
  "chip": "st/stm32g071rb",
  "clock_profile": "pll_64mhz",
  "roles": {
    "led":        { "pin": "pa5", "active": "high" },
    "button":     { "pin": "pc13", "active": "low" },
    "debug_uart": { "peripheral": "usart2", "tx": "pa2", "rx": "pa3", "baud": 115200 }
  },
  "probe": { "kind": "stlink", "runner": "openocd" }
}
```

That is the whole contract: pick a chip, pick a clock, wire the roles. See
[Adding a board](adding-a-board.md) to bring up one that isn't listed.

## What the board is, and what your project chooses

A `board.json` value is a **default**, not a decree. Most of what it says is
hardware — which pin the LED is on, that the button is active-low, which
peripheral the header exposes — and changing that means you have different
hardware, so you need a board of your own (`alloy board-clone`).

A few fields are different: the same board supports many values and your
application picks one. Those you set in `alloy.toml`, and the framework's board
keeps receiving upstream fixes instead of being forked to change a number.

```toml
[roles.debug_uart]
baud = 921600          # the board says 115200; this project wants faster

[roles.watchdog]
timeout_ms = 1500      # depends on your loop, not on the PCB

[clock]
mhz = 48               # or: profile = "hsi_16mhz"
```

| Role | What a project may choose |
| --- | --- |
| `debug_uart` | `baud` |
| `watchdog` | `timeout_ms` |
| `nvm`, `fs` | `bytes` — how much flash to reserve |
| (any board) | `[clock]` — a named `profile`, or a target `mhz` to solve for |

Anything else is refused, with the reason and the command that would actually
do it — silently ignoring `[roles.led] pin = "pb7"` would leave you with a dark
LED and no explanation.

`alloy board-info --json` reports the effective board *and* a
`project_overrides` block saying which values your project changed and what the
board itself says, so a tool can show both.
