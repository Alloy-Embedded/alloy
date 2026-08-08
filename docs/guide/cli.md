# The `alloy` CLI

One command drives the whole workflow — scaffold, configure, build, flash, monitor, emulate,
test, update. You never write CMake; it is generated into a throwaway `.alloy/build-tree/`.

Every command that takes a board accepts `--board <id>` to override `alloy.toml` for that
invocation, and `--project <dir>` to work on a project other than the current directory. Commands
marked **JSON** emit a versioned envelope on stdout for editors and scripts.

## Start a project

| Command | What it does |
| --- | --- |
| `alloy new <name> --board <id>` | scaffold from a curated board (the scaffold *is* the CI-built blink) |
| `alloy new <name> --chip <id> [--clock <profile>]` | scaffold a **clean, editable board** for any MCU in the database |
| `alloy boards` — **JSON** | list curated boards |
| `alloy chips [--vendor st]` — **JSON** | list every MCU you can scaffold for |
| `alloy set-board <id>` | change the board in `alloy.toml` |
| `alloy setup [--check] [--family <f>]` | verify / install cross-toolchains into `~/.alloy/tools` |

```console
$ alloy new blinky --board rp2040_zero && cd blinky
$ alloy run                 # build → flash → open the monitor
```

## Build and run

| Command | What it does |
| --- | --- |
| `alloy build` | generate + render CMake + compile |
| `alloy flash` | build + program the board |
| `alloy monitor [--json]` | bidirectional serial monitor; `--json` streams NDJSON and needs no terminal |
| `alloy run` | flash + monitor |
| `alloy gen` | regenerate `.alloy/generated` from the device database |
| `alloy clean [--all]` | remove per-board build trees |
| `alloy test [--no-sanitize]` | build + run the host unit tests (ASan/UBSan on by default) |

`build`, `flash`, `run` and `gen` share two placement flags:

- `--ram` links every section into RAM and loads it with the debugger — no flash erase, for fast
  iteration.
- `--slot bl|a|b` links into the bootloader region or an A/B firmware slot. See
  [Firmware update](firmware-update.md).

Retarget with a single flag — same source, different silicon:

```console
$ alloy build --board nucleo_g071rb
$ alloy build --board esp32_devkit
```

## Inspect what you built

| Command | What it does |
| --- | --- |
| `alloy size` — **JSON** | flash/RAM of the last build against the chip's real memories, and whether the image fits its update slot |
| `alloy matrix [--boards a,b]` — **JSON** | build this project for **every** board and table the result |
| `alloy frame-audit` | coroutine frame sizes vs the `task_storage<N>` you declared |
| `alloy crash [--line "…"] [--pc 0x…] [--elf app.elf]` — **JSON** | decode a device's crash report: pc/lr to file:line via addr2line, CFSR bits into words — see [Crash reports](crash-reports.md) |
| `alloy svd [--chip <id>] [-o out]` | write a CMSIS-SVD file so a debugger can show peripheral registers |
| `alloy debug-info` — **JSON** | debug-server facts for an IDE launch config |

```console
$ alloy size
flash      2340 /   131072 B  (1.8%)
ram        2184 /    36864 B  (5.9%)
slot_a          47104 B  image 2852 B — fits
slot_b          47104 B  image 2852 B — fits
```

`alloy matrix` is the portability claim, executable — one table, one `src/`, no `#ifdef`:

```console
$ alloy matrix --boards nucleo_g071rb,nucleo_f722ze,same70_xplained
board                       flash               ram  time
nucleo_g071rb       2.3K / 128.0K      2.1K / 36.0K  8.1s
nucleo_f722ze       2.6K / 512.0K     2.4K / 256.0K  1.9s
same70_xplained     2.5K / 2048.0K     2.3K / 384.0K  1.3s

3 of 3 boards — the same src/, no #ifdef
```

A board that fails to build does not end the sweep — it gets a row with the reason, so one table
tells you what fits where.

## Configure a board

| Command | What it does |
| --- | --- |
| `alloy board-info [<id>]` — **JSON** | roles, capabilities, used pins and problems of any board |
| `alloy board-validate [<id>] [--file f\|-]` — **JSON** | every problem located, with the pins that *would* work |
| `alloy board-clone <src> <new>` | copy a curated board into your project as an editable one |
| `alloy chip-info <chip>` — JSON | clock profiles, pin map with alternate functions, peripherals, role catalogue |
| `alloy clock --chip <id> --mhz <n> [--hse <mhz>]` — JSON | solve a PLL for a target frequency |
| `alloy clock --chip <id> --graph [--profile p]` — **JSON** | the whole clock: sources, buses, and what each peripheral is fed |

`board-validate --file -` reads a candidate `board.json` from stdin, which is how an editor
checks a configuration *before* writing it. These verbs are what the
[VS Code configurator](vscode.md) is built on — it holds no chip knowledge of its own.

### The whole clock, not just the PLL

`--graph` answers the question that comes after "what frequency": where the clock goes. Bus
prescalers, and the kernel clock each peripheral is actually fed — with what that implies.

```console
$ alloy clock --chip st/stm32f767 --graph --profile pll_180mhz
SYSCLK 180 MHz → AHB ÷1 180 MHz → eth
                 APB ÷4  45 MHz → usart3   115200 baud → 115089 (0.10% error)
                 APB2 ÷2 90 MHz
```

The baud error is computed with the driver's own rounding, so it is the error you will measure, not
a different one. Peripherals whose feed the chip data does not state (an independent watchdog on its
own oscillator) are listed as unplaceable rather than left out.

## Emulate

| Command | What it does |
| --- | --- |
| `alloy emulate [--emit-only]` | run the firmware headless in [Renode](https://renode.io) on a data-generated platform |

```console
$ alloy emulate --board nucleo_f722ze
alloy uart_echo ready
```

The platform is generated from the same chip data the firmware compiles against, so the emulated
memory map and UART cannot drift from the real target. See [Emulation](emulation.md).

## Update a device in the field

| Command | What it does |
| --- | --- |
| `alloy keygen [-o key]` | generate an Ed25519 update-signing keypair |
| `alloy image <app.elf\|bin> --set-version <n> [--sign key]` | pack an update image |
| `alloy ports` — **JSON** | list serial ports |
| `alloy update [--image-a a.img] [--image-b b.img] --port <p>` | stream an image into the device's inactive slot |

```console
$ alloy image build/app.elf --set-version 3 --sign keys/update.key -o app_b.img
$ alloy update --image-b app_b.img --port /dev/ttyUSB0
```

The full story — slots, trial boots, rollback, signing — is in
[Firmware update](firmware-update.md).

## Give a board its identity

| Command | What it does |
| --- | --- |
| `alloy provision write --serial <sn> [--mac <m>] [--hw-rev N] [--batch N]` | program this board's factory identity through the probe and verify it by reading it back |
| `alloy provision write … -o <file>` | **offline**: encode the record to a file (no probe, no board) — mass pre-programming and test fixtures |
| `alloy provision read [--file <dump>]` | read the identity back — always safe, never writes |

```console
$ alloy provision write --serial ALY-0001-A7 --mac 02:1a:2b:3c:4d:5e
  identity page: 0x08007800 +2048 B (from the slot layout)
verified by readback: serial 'ALY-0001-A7'  mac 02:1a:2b:3c:4d:5e  hw_rev 0  batch 0
```

Identity lives outside both firmware slots, so updates never touch it. **Provision before
`alloy secure apply`** — locking the part first makes this impossible. The whole production
line order, and a worked script for it, is in [Firmware update](firmware-update.md).

## Lock a production unit

| Command | What it does |
| --- | --- |
| `alloy secure status` | read + decode the RDP level and WRP ranges over the probe — always safe |
| `alloy secure apply [--rdp N] [--wrp-bootloader]` | program option bytes — destructive transitions refuse without explicit flags and a typed phrase |

Signed images mean nothing if the flash can be read or replaced over SWD. The guards are the
point: read [Security](security.md) before using `apply` — especially what `--rdp 2` means.

## Give the project its own CI

| Command | What it does |
| --- | --- |
| `alloy ci-init [--boards a,b] [--all] [--force]` | write a GitHub Actions workflow for this project |

The framework's CI is what keeps every board building; a project built *on* alloy starts with
nothing. This writes the equivalent — validate the boards it defines, then build its sources for
each board it targets:

```console
$ alloy ci-init
wrote .github/workflows/alloy.yml
  builds on: nucleo_g071rb
```

It targets what the project targets, not every supported board; `--all` widens it. Toolchain steps
follow the same rule (an ARM-only project gets no ESP download) and are pinned to the versions alloy
itself is tested against.

## Libraries

| Command | What it does |
| --- | --- |
| `alloy lib list` — **JSON** | browse the driver registry |
| `alloy lib search <text>` | filter by name, summary or category |
| `alloy lib info <name>` | manifest + required concepts |
| `alloy lib add <name>` | vendor into `./libs` and wire the build |

See [Driver libraries](libraries.md).

## Editor integration

- The `--json` envelopes are versioned (`alloy.boards.v1`, `alloy.board_info.v1`,
  `alloy.chip_info.v1`, `alloy.size.v1`, …). Tools check the schema string and fail loudly rather
  than misreading a newer CLI.
- `.alloy/build-tree/` carries `compile_commands.json`, so clangd and IntelliSense work with no
  setup.
- The [VS Code extension](vscode.md) wraps all of the above.
