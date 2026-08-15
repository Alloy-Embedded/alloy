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
| `alloy new <name> ... --with-tests` | also scaffold a host test suite wired to the concept fakes ([guide](testing.md)) |
| `alloy boards` — **JSON** | list curated boards |
| `alloy chips [--vendor st]` — **JSON** | list every MCU you can scaffold for |
| `alloy set-board <id>` | change the board in `alloy.toml` |
| `alloy devices` — **JSON** | which chip database this project resolves: root, declared version, content digest |
| `alloy devices --pin [--version-only\|--digest-only]` | freeze that database into `alloy.toml` `[devices]`, so a shipped product's facts cannot move — see [API stability](../reference/stability.md#pinning-the-chip-database) |
| `alloy setup [--check] [--family <f>]` | verify / install cross-toolchains into `~/.alloy/tools` |
| `alloy setup --fetch <dir> [--platform <key>]` | populate a local mirror of the pinned archives, for an air-gapped host — see [Supply chain](supply-chain.md) |
| `alloy setup --from <dir> [--offline]` | install from that mirror, same sha256 verification, no network |

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
| `alloy test [--no-sanitize]` | build + run the host unit tests — the PROJECT's if it has a `tests/`, else the framework's (ASan/UBSan on by default) |
| `alloy test --coverage` | the same, instrumented, plus a line/branch report over the project's `src/` (needs gcovr) |
| `alloy test --framework` | run the framework's own suite even from inside a project |

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
| `alloy symbols [--board <id>]` | what is in the last build and **where** — sections with their load vs run addresses, the largest symbols, and any `[budget]` in `alloy.toml` |
| `alloy frame-audit` | coroutine frame sizes vs the `task_storage<N>` you declared |
| `alloy crash [--line "…"] [--pc 0x…] [--elf app.elf]` — **JSON** | decode a device's crash report: pc/lr to file:line via addr2line, CFSR bits into words — see [Crash reports](crash-reports.md) |
| `alloy svd [--chip <id>] [-o out]` | write a CMSIS-SVD file so a debugger can show peripheral registers |
| `alloy debug-info` — **JSON** | debug-server facts for an IDE launch config |
| `alloy sbom [--format notice\|spdx\|cyclonedx\|json] [--out f] [--strict]` | what the built image actually contains, derived from what it compiles and links — see [Supply chain](supply-chain.md) |

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

`alloy symbols` answers the two questions `size` cannot: *where* does a section live, and *what*
is filling it. The load address (`lma`) and the run address (`vma`) differ for anything copied at
startup, which is how you see that a section is not loaded at all:

```console
$ alloy symbols
section          size      vma        lma
.isr_vector         192  0x08000000 0x08000000
.text              2016  0x080000c0 0x080000c0
.bss                340  0x20000000 0x080008a0  zeroed/reserved, not loaded
.noinit              36  0x20000154 0x080008a0  zeroed/reserved, not loaded
._heap_stack       2048  0x20000178 0x080008a0  zeroed/reserved, not loaded

largest symbols:
   312  0x08000164  .text        main
   266  0x080006e4  .text        __udivsi3
   196  0x080005c4  .text        Reset_Handler
   192  0x08000000  .isr_vector  g_vector_table
```

## Configure a board

| Command | What it does |
| --- | --- |
| `alloy board-info [<id>]` — **JSON** | roles, capabilities, used pins and problems of any board |
| `alloy board-validate [<id>] [--file f\|-]` — **JSON** | every problem located, with the pins that *would* work |
| `alloy product-validate [<name>]` — **JSON** | the same, for a product TOML: every problem located, with a way out — see [Products](products.md) |
| `alloy board-clone <src> <new>` | copy a curated board into your project as an editable one |
| `alloy chip-info <chip>` — JSON | clock profiles, pin map with alternate functions, peripherals, role catalogue |
| `alloy chip-status <chip> [--json]` | per peripheral: curated register data? HAL driver? Renode model? reachable from a board role? — see [Chip coverage](../reference/chip-coverage.md) |
| `alloy clock --chip <id> --mhz <n> [--hse <mhz>]` — JSON | solve a PLL for a target frequency |
| `alloy clock --chip <id> --graph [--profile p]` — **JSON** | the whole clock: sources, buses, and what each peripheral is fed |

`board-validate --file -` reads a candidate `board.json` from stdin, which is how an editor
checks a configuration *before* writing it. These verbs are what the
[VS Code configurator](vscode.md) is built on — it holds no chip knowledge of its own.

### The whole clock, not just the PLL

`--graph` answers the question that comes after "what frequency": where the clock goes. Bus
prescalers, and the kernel clock each peripheral is actually fed — with what that implies.

The verb emits **JSON only** — there is no human-readable renderer and no `--text` flag. It is
built for the [VS Code clock view](vscode.md) and for scripts; read it with `jq` or pipe it
through a few lines of Python.

```console
$ alloy clock --chip st/stm32f767 --graph --profile pll_180mhz
{
  "schema": "alloy.clock_graph.v1",
  "chip": "st/stm32f767",
  "profile": "pll_180mhz",
  "nodes": [
    { "name": "sysclk", "label": "SYSCLK",        "hz": 180000000, "parent": null,     "divider": null },
    { "name": "ahb",    "label": "AHB · HCLK",    "hz": 180000000, "parent": "sysclk", "divider": 1 },
    { "name": "apb",    "label": "APB · PCLK",    "hz":  45000000, "parent": "ahb",    "divider": 4 },
    { "name": "apb2",   "label": "APB2 · PCLK2",  "hz":  90000000, "parent": "ahb",    "divider": 2 }
  ],
  "consumers": [
    { "peripheral": "eth",    "class": "eth",  "node": "ahb",  "hz": 180000000, "notes": [] },
    { "peripheral": "usart3", "class": "uart", "node": "apb",  "hz":  45000000,
      "notes": [ { "level": "info", "text": "115200 baud → 115089 (0.10% error)" } ] }
  ],
  "unstated": ["adc123_common", "dma1", "dma2", "flash", "gpioa", "…", "iwdg", "rcc"]
}
```

Two fields carry the value. The `notes` on a consumer are computed with **the driver's own
rounding**, so a baud error there is the error you will measure, not a different one. And
`unstated` is the honest half: peripherals whose feed the chip data does not state (an
independent watchdog on its own oscillator, a block whose kernel-clock node was never curated)
are **listed as unplaceable rather than left out**, so a consumer missing from the graph is
always visible as a gap in the data rather than as silence.

## Emulate

| Command | What it does |
| --- | --- |
| `alloy emulate [--emit-only]` | run the firmware headless in [Renode](https://renode.io) on a data-generated platform |

```console
$ alloy emulate --board nucleo_f722ze
platform: /…/.alloy/build-tree/nucleo_f722ze/out/nucleo_f722ze.repl
script:   /…/.alloy/build-tree/nucleo_f722ze/out/nucleo_f722ze.resc
uart:     sysbus.usart3
…Renode's own INFO/WARNING lines…
<your firmware's banner, then whatever it prints>
```

Those three header lines are the whole contract with Renode: they are also exactly what a
`renode-test` invocation needs, which is why `--emit-only` prints them and stops. What follows
them is your program's own output — the banner in the block above is whichever example you built,
not something the CLI prints.

The platform is generated from the same chip data the firmware compiles against, so the emulated
memory map and UART cannot drift from the real target. **It works for seven of the nine shipped
boards**; the two ESP32 boards have no Renode model and `alloy emulate` refuses them by name
rather than booting a machine it cannot map. See [Emulation](emulation.md).

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
| `alloy bus {validate,list,manifest}` | the wire contract for `libs/bus` datagrams, from `bus.toml` — see [The message bus](bus.md) |

See [Driver libraries](libraries.md).

## Editor integration

- The `--json` envelopes are versioned (`alloy.boards.v1`, `alloy.board_info.v1`,
  `alloy.chip_info.v1`, `alloy.size.v1`, …). Tools check the schema string and fail loudly rather
  than misreading a newer CLI.
- `.alloy/build-tree/` carries `compile_commands.json`, so clangd and IntelliSense work with no
  setup.
- The [VS Code extension](vscode.md) wraps all of the above.
