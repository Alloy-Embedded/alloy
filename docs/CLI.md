# Alloy CLI Reference

The `alloy` command is the user-facing entry point for the Alloy multi-vendor bare-metal
runtime. It is shipped as the [`alloy-cli`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tools/alloy-cli/) Python package and installed once via `pipx install
alloy-cli`. See [QUICKSTART.md](QUICKSTART.md) for the end-to-end flow.

This document is the reference for every subcommand. For installation, see
[QUICKSTART.md](QUICKSTART.md). For the runtime contract that scaffolded projects rely on,
see [CUSTOM_BOARDS.md](CUSTOM_BOARDS.md).

## Runtime resolution order

Most subcommands need to locate an Alloy runtime checkout. Resolution order:

1. The `ALLOY_ROOT` environment variable, when set.
2. A walk up from the current working directory looking for `scripts/alloyctl.py` and
   `cmake/board_manifest.cmake`.
3. The version selected with `alloy sdk use` (default: the most recently installed).

If none resolve, the CLI exits with an error pointing you at `alloy sdk install`.

## Native subcommands

These are implemented in `alloy-cli` itself.

### `alloy new <path> (--board <name> | --mcu <part>) [...]`

Scaffolds a self-contained downstream project at `<path>`. Every project owns its
`board/` directory.

| Flag | Meaning |
|---|---|
| `--board <name>` | Copy `boards/<name>/` from the active SDK into `<project>/board/`. |
| `--mcu <part>` | Resolve the MCU. If the catalog matches, alias to `--board`; otherwise generate a board skeleton from the `alloy-devices` descriptor. |
| `--name <project>` | Override the project name. Defaults to the destination basename. |
| `--alloy-root <path>` | Skip the SDK lookup; use this checkout directly. |
| `--devices-root <path>` | Skip the SDK's bundled `alloy-devices`; use this checkout. |
| `--arch <arch>` | Override architecture for raw-MCU scaffolding when the descriptor lacks it. Valid values: `cortex-m0plus`, `cortex-m4`, `cortex-m7`, `riscv32`, `xtensa`, `avr`, `native`. |

Output: project tree with `CMakeLists.txt`, `CMakePresets.json`, `src/main.cpp`, `board/`,
`.vscode/`, `README.md`, `.gitignore`. See [QUICKSTART.md](QUICKSTART.md) for next steps.

### `alloy boards`

Lists boards supported by `alloy new --board`, including the MCU each one declares.

### `alloy sdk install <version> [--runtime-ref <ref>] [--devices-ref <ref>] [--force]`

Downloads the runtime and `alloy-devices` checkouts into
`$ALLOY_HOME/sdk/<version>/{runtime,devices}/`. Pinned commit SHAs are recorded in
`$ALLOY_HOME/sdk/<version>/manifest.toml`. The first installed version is automatically
made active.

`$ALLOY_HOME` defaults to `~/.alloy/` and can be overridden with the `ALLOY_HOME`
environment variable. Source repository URLs default to the `Alloy-Embedded` GitHub
organisation and are stored in `$ALLOY_HOME/config.toml`.

### `alloy sdk list`

Lists installed versions. The active one is marked with `*`.

### `alloy sdk use <version>`

Selects the active version.

### `alloy sdk path`

Prints the active runtime path. Exit code 1 if no version is active.

### `alloy toolchain install <name>[@<version>] [--force]`

Downloads a pinned cross-toolchain into `$ALLOY_HOME/toolchains/<name>/<version>/`.
Verifies the archive against the sha256 in
[`_toolchain_pins.toml`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tools/alloy-cli/src/alloy_cli/_toolchain_pins.toml) and refuses to install if the pin
is marked `TODO`.

`<version>` defaults to the toolchain's `default_version`. Pass `--force` to reinstall.

The pin file ships `arm-none-eabi-gcc` and `openocd` entries by default; tests and
maintainers can override the file via the `ALLOY_TOOLCHAIN_PINS` environment variable.

### `alloy toolchain list`

Lists installed toolchains.

### `alloy toolchain which <name>[@<version>]`

Prints the absolute path to the toolchain's primary binary.

## Delegated subcommands

These pass through to `scripts/alloyctl.py` inside the active runtime. The CLI shape is
the contract; behaviour matches the legacy script and will continue to do so until that
script is retired.

| Subcommand | Purpose |
|---|---|
| `alloy doctor` | Preflight: toolchain, probe tooling, Python deps, descriptor ref. |
| `alloy info` | JSON environment report (handy for bug reports). |
| `alloy configure --board <board>` | Prepare a CMake build directory for a board. |
| `alloy build --board <board> --target <t>` | Build a target. |
| `alloy bundle --board <board>` | Build the board's foundational target bundle. |
| `alloy flash --board <board> --target <t> [--build-first]` | Flash via OpenOCD or the STM32 programmer (board-dependent). |
| `alloy recover --board <board> --target <t>` | Recover an STM32 board trapped by bad firmware. |
| `alloy monitor --board <board> [--port <p>] [--baud <b>]` | Open a serial monitor. |
| `alloy gdbserver --board <board> [--gdb-port <p>]` | Start an OpenOCD GDB server. |
| `alloy validate --board <board> [--kind <k>]` | Run runtime validation suites. |
| `alloy sweep --board <board> [...]` | Build/flash/monitor a list of targets in sequence. |
| `alloy explain --board <board> [...]` | Explain connector / clock / peripheral facts. |
| `alloy diff --from <a> --to <b>` | Diff release-relevant facts between two boards. |
| `alloy compile-commands --board <board> [--configure] [--copy]` | Expose `compile_commands.json` at the repo root for clangd. |
| `alloy run --target <t> [...]` | Build, flash, and open a monitor in one step. |

For the canonical CLI inputs and outputs of each delegated subcommand, run
`alloy <subcommand> --help`. Behaviour is preserved verbatim from the legacy script
during the transition.

## Top-level flags

| Flag | Meaning |
|---|---|
| `-h`, `--help` | Print top-level help. |
| `-V`, `--version` | Print the CLI version. |

## Environment variables

| Variable | Meaning |
|---|---|
| `ALLOY_ROOT` | Force the runtime checkout location. Wins over walk-up and the active SDK. |
| `ALLOY_HOME` | Override the per-user CLI directory. Default: `~/.alloy/`. |
| `ALLOY_TOOLCHAIN_PINS` | Override the shipped pin file. Used by tests; rarely needed otherwise. |

## Exit codes

| Code | Meaning |
|---|---|
| `0` | Success. |
| `1` | User-facing error (validation, missing inputs, network failures). |
| `2` | Cannot locate a runtime checkout, or `argparse` rejected the invocation. |

## Stability

The native subcommand surface (`new`, `boards`, `sdk *`, `toolchain *`) is part of the
[`runtime-tooling`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/openspec/specs/runtime-tooling/spec.md) contract and changes only through OpenSpec. Delegated
subcommands inherit their stability from `scripts/alloyctl.py`; they will eventually be
folded into the native surface, at which point flags may be normalised. Any breaking
change will be announced in the release notes and gated behind a deprecation cycle.
