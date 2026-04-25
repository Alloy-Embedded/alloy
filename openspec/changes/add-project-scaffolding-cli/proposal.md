# Add Project Scaffolding CLI

## Why
Today, starting a new application on top of Alloy is a multi-step, manual process: clone the
runtime, clone `alloy-devices`, install an ARM toolchain, copy a `CMakeLists.txt` from an
example, hand-edit board flags, and figure out the correct configure/build/flash incantation.
The existing `scripts/alloyctl.py new` command produces a starter project but assumes the SDK
and toolchain are already in place and only generates a thin CMake stub.

For Alloy to be the clearest multi-vendor C++ bare-metal runtime, the bring-up cost for a new
project must be effectively zero. A user should pick an MCU or board, run one command, open
VS Code, and start writing application code. Toolchain acquisition, SDK fetch, descriptor
fetch, build presets, debug configuration, and editor integration must be handled by the
runtime tooling layer rather than by the user.

This change establishes the user-facing project scaffolding entry point as a stable, supported
piece of the runtime tooling product, and lays the multi-step path to get there without
breaking the existing `alloyctl.py` flows.

## What Changes
- Introduce a stable `alloy` CLI as the primary user entry point for project lifecycle
  (`new`, `build`, `flash`, `monitor`, `doctor`, `sdk`), distributed independently of the
  runtime checkout (installable via `pipx`/`uv tool install` and a one-line installer).
- Add a project scaffolding command that generates a complete, self-contained downstream
  project (CMakeLists, `CMakePresets.json`, `src/main.cpp`, `.vscode/`, `.gitignore`, README)
  for a chosen board or raw MCU target.
- Add an SDK manager (`alloy sdk install/update/use`) that fetches and pins the `alloy`
  runtime and `alloy-devices` descriptors into a shared, versioned cache (default
  `~/.alloy/sdk/<version>`) with an optional per-project vendored mode for CI reproducibility.
- Add a toolchain manager (`alloy doctor --fix`, `alloy toolchain install`) that downloads
  and pins required cross-toolchains (xPack `arm-none-eabi-gcc`, `avr-gcc`, OpenOCD) into
  `~/.alloy/toolchains/<name>/<version>` and exposes them to generated CMake presets.
- Generate a working VS Code workspace (`launch.json` for OpenOCD/J-Link debugging,
  `settings.json` for clangd, `tasks.json` for build/flash/monitor) per scaffolded project.
- Extend the board model to support an opt-in raw-MCU scaffolding path: a user may pass
  `--mcu STM32G474RET6` and receive a project wired to an `alloy-devices` descriptor plus a
  generated minimal board layer, without requiring a pre-cadastered board manifest entry.
- Document and validate the scaffolded project shape so downstream consumption stays inside
  the `runtime-tooling` contract instead of drifting into ad-hoc patterns.

## Impact
- Affected specs:
  - `runtime-tooling` (extended with scaffolding, SDK management, toolchain management,
    editor integration, and raw-MCU bring-up requirements)
- Affected code:
  - `scripts/alloyctl.py` (subsumed by the new CLI; behavior preserved during transition)
  - new `tools/alloy-cli/` package (Python, `pyproject.toml`, installable via `pipx`)
  - `cmake/` (presets and toolchain-file generation reused by scaffolding)
  - `docs/` (`QUICKSTART.md`, `CMAKE_CONSUMPTION.md`, `BOARD_TOOLING.md` updated to point at
    the new entry point)
- Out of scope for this change:
  - rewriting the CLI in a non-Python language (tracked separately if/when adopted)
  - a VS Code extension (file-based integration only in this change)
  - adding new vendors or boards beyond the currently supported set
