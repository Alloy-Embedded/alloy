# The `alloy` CLI

One command drives the whole workflow — scaffold, build, flash, monitor, emulate, test. Users
never write CMake; it is generated into a throwaway `.alloy/build-tree/`.

## Commands

| Command | What it does |
| --- | --- |
| `alloy new <name> --board <id>` | scaffold a portable project (the scaffold *is* the CI-built blink) |
| `alloy boards [--json]` | list known boards |
| `alloy build [--board <id>]` | generate + render CMake + compile |
| `alloy flash [--board <id>]` | build + program the board |
| `alloy monitor` | bidirectional serial monitor |
| `alloy run` | flash + monitor |
| `alloy emulate [--board <id>]` | boot the firmware headless in [Renode](https://renode.io) |
| `alloy gen` | regenerate `.alloy/generated` from the device database |
| `alloy clean [--all]` | remove per-board build trees |
| `alloy set-board <id>` | change the board in `alloy.toml` |
| `alloy setup` | verify / install cross-toolchains into `~/.alloy/tools` |
| `alloy test` | build + run the host unit tests |
| `alloy debug-info [--json]` | debug-server facts for an IDE launch config |

## Everyday flow

```console
$ alloy new blinky --board rp2040_zero
$ cd blinky
$ alloy run                 # build → flash → open the monitor
```

Retarget with a single flag — same source, different silicon:

```console
$ alloy build --board nucleo_g071rb
$ alloy build --board esp32_devkit
```

## Emulate without hardware

```console
$ alloy emulate --board nucleo_f722ze
alloy uart_echo ready
```

The Renode platform is **generated from the same chip data** the firmware uses, so the emulated
memory map and UART cannot drift from the real target.

## Editor integration

- `alloy boards --json` and `alloy debug-info --json` emit versioned JSON envelopes for IDE
  tooling.
- The `.alloy/build-tree` carries a `compile_commands.json`, so clangd/IntelliSense work out of
  the box.
- A companion **VS Code extension** wraps these commands (build/flash/run/monitor tasks +
  Cortex-Debug launch).
