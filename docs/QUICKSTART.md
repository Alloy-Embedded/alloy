# Alloy Quickstart

The shortest supported path from "nothing installed" to a flashing project.

## TL;DR

```bash
pipx install alloy-cli              # one-time
alloy sdk install v0.1.0            # downloads alloy + alloy-devices
alloy toolchain install arm-none-eabi-gcc
alloy new ./myproj --mcu STM32G071RBT6
cd myproj && code .                 # open in VS Code
cmake --preset debug && cmake --build --preset debug
alloy flash --board custom --target myproj --build-first
```

The rest of this document explains each step and points at deeper docs.

## Install the CLI

The user-facing entry point is the `alloy-cli` Python package. Install it once with `pipx`
or `uv`:

```bash
pipx install alloy-cli
# or:
uv tool install alloy-cli

# while alloy-cli is not yet on PyPI, install from a local checkout:
pipx install --editable tools/alloy-cli
```

Verify:

```bash
alloy --version
alloy --help
```

`alloy --help` lists the native commands (`new`, `boards`, `sdk`, `toolchain`) and the
runtime-delegated commands (`build`, `flash`, `monitor`, `doctor`, `info`, ...). For the
full subcommand reference see [CLI.md](CLI.md).

## Install an SDK and toolchain

`alloy sdk install <version>` downloads the alloy runtime and the matching
`alloy-devices` descriptors into `~/.alloy/sdk/<version>/`:

```bash
alloy sdk install v0.1.0
alloy sdk list           # active version is marked with *
alloy sdk path           # prints the active runtime path
```

`alloy toolchain install <name>` downloads a pinned cross-toolchain into
`~/.alloy/toolchains/<name>/<version>/`:

```bash
alloy toolchain install arm-none-eabi-gcc
alloy toolchain list
alloy toolchain which arm-none-eabi-gcc
```

> The `--version` and SHA pins in `_toolchain_pins.toml` are validated as part of each
> alloy release. Pre-release builds may ship pins marked `TODO`; in that case
> `alloy toolchain install` refuses with a clear message.

## Scaffold a project

Every new project owns its `board/` directory. The runtime stays untouched; you edit pin
assignments in your repo. Pick `--board` for a foundational board or `--mcu` for any MCU
that has an `alloy-devices` descriptor:

```bash
# Existing foundational board: copies boards/nucleo_g071rb/ from the SDK into board/
alloy new ./blink --board nucleo_g071rb

# MCU mapped to a foundational board (alias to the above)
alloy new ./blink --mcu STM32G071RBT6

# MCU without a foundational board: generates a skeleton you fill in
alloy new ./controller --mcu STM32G474RET6
```

The generated tree:

```
myproj/
├── CMakeLists.txt        # ALLOY_BOARD=custom + cache vars (see CUSTOM_BOARDS.md)
├── CMakePresets.json     # debug / release presets
├── board/                # board.hpp, board_config.hpp, board.cpp, syscalls.cpp, *.ld
├── src/main.cpp
├── .vscode/              # clangd, build/flash/monitor tasks, OpenOCD launch
├── .gitignore
└── README.md
```

If the `--mcu` lookup found a descriptor without memory regions, the linker script ships
with TODO markers and the CLI prints a one-line warning.

See [CUSTOM_BOARDS.md](CUSTOM_BOARDS.md) for the runtime contract the generated
CMakeLists wires up.

## Build, flash, monitor

From inside the project tree:

```bash
cmake --preset debug
cmake --build --preset debug

alloy flash   --board custom --target myproj --build-first
alloy monitor --board custom
```

The `flash` and `monitor` subcommands delegate to the active SDK; they expect a probe to
be reachable and (for `flash`) the toolchain on PATH. See
[BOARD_TOOLING.md](BOARD_TOOLING.md) for probe-specific notes.

## Validate the host environment

Before bringing up new hardware:

```bash
alloy doctor                  # toolchain, probe tooling, python deps, descriptor ref
alloy info                    # JSON environment report (handy for bug reports)
```

`doctor` exits non-zero with an actionable diagnostic when a prerequisite is missing.

## Working inside the alloy runtime checkout

If you are contributing to alloy itself rather than building an application, run the CLI
from inside the runtime checkout (or set `ALLOY_ROOT=$PWD`). The same subcommands work
without an installed SDK because the runtime locator falls back to the working directory:

```bash
cd /path/to/alloy
alloy configure --board nucleo_g071rb
alloy build --board nucleo_g071rb --target blink
```

The legacy `python3 scripts/alloyctl.py ...` form remains a working alias during the
transition; new docs and examples should use `alloy`.

## Where to next

- [CLI.md](CLI.md) -- full subcommand reference
- [CUSTOM_BOARDS.md](CUSTOM_BOARDS.md) -- the `ALLOY_BOARD=custom` contract
- [BOARD_TOOLING.md](BOARD_TOOLING.md) -- probe and flashing notes per board
- [CMAKE_CONSUMPTION.md](CMAKE_CONSUMPTION.md) -- alternative consumption paths
- [COOKBOOK.md](COOKBOOK.md) -- canonical runtime patterns
- [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md) -- foundational boards and known descriptors
