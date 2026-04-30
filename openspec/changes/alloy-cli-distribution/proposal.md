# Proposal: alloy CLI Distribution

## Status
`open` — strategic priority for community adoption.

## Problem

Setting up an alloy project today requires:
1. `git clone` alloy repo
2. `git clone` alloy-devices repo (or let CMake FetchContent pull it)
3. Manually copy CMake boilerplate from examples
4. Know the exact CMake variables (`ALLOY_BOARD`, `ALLOY_DEVICES_ROOT`, etc.)

There is no installable `alloy` command. Discoverability of boards, devices, and
drivers requires reading source code. This friction blocks adoption — embedded
developers expect a CLI-first workflow (like `idf.py`, `west`, `platformio`).

## Proposed Solution

### pip-installable `alloy` CLI

```sh
pip install alloy-cli           # installs `alloy` command
alloy new --board nucleo_g071rb my_project
alloy build my_project
alloy flash my_project
alloy device add stm32h743zit6
alloy boards
alloy doctor
```

`alloy-cli` is a separate Python package (not embedded in the alloy repo).
It shells out to CMake, `arm-none-eabi-gcc`, `openocd`, `pyocd`, and `imgtool`.
The existing `alloyctl.py` becomes the backend implementation, refactored into
importable modules.

### Package layout

```
alloy_cli/
    __main__.py       # entry point: `python -m alloy_cli`
    commands/
        new.py        # alloy new
        build.py      # alloy build
        flash.py      # alloy flash
        device.py     # alloy device add/list/info/prefetch
        boards.py     # alloy boards
        doctor.py     # alloy doctor
        bundle.py     # alloy bundle
    core/
        board_discovery.py   # reads board.json (from board-manifest-declarative spec)
        device_registry.py   # reads device registry (from alloy-devices-package-registry spec)
        cmake_runner.py      # subprocess wrapper for cmake/ninja
        probe_runner.py      # openocd/pyocd/esptool subprocess wrappers
    data/
        cmake/           # bundled CMake modules (CMakeLists template, toolchain files)
        schemas/         # board-manifest v1.json schema
```

### `alloy new` — project scaffolding

```sh
alloy new --board nucleo_g071rb --name blink
# Creates:
#   blink/
#     CMakeLists.txt       (pre-filled with ALLOY_BOARD, find_package)
#     src/
#       main.cpp           (blink example using pin_handle)
#     .vscode/
#       c_cpp_properties.json
#       launch.json        (OpenOCD debug config from board.json)
```

Scaffolding reads `board.json` to populate:
- `ALLOY_BOARD` value
- Debug probe configuration
- Default linker script reference
- UART pin labels as comments

### `alloy device add <device_id>` — on-demand device package

```sh
alloy device add stm32h743zit6
# Downloads st-stm32h7-1.0.0.tar.gz → ~/.alloy/devices/
# Emits: [ok] stm32h743zit6 installed to ~/.alloy/devices/st-stm32h7/
```

Uses the FetchContent equivalent from `alloy-devices-package-registry` spec.

### `alloy device list --available` — registry browser

```sh
alloy device list --vendor st --family stm32h7
# Prints table: device_id | family | arch | status (cached/available/unknown)
```

Device registry JSON hosted at `alloy-rs.dev/registry/devices.json`:
```json
{
  "version": 1,
  "devices": [
    { "id": "stm32g071rb", "vendor": "st", "family": "stm32g0", "arch": "cortex-m0plus",
      "package_url": "https://github.com/alloy-rs/alloy-devices/releases/..." },
    ...
  ]
}
```

### `alloy boards` — board browser

```sh
alloy boards --vendor st
# Prints table from board.json discovery:
# nucleo_g071rb  | ST  | stm32g0  | tier 1 | cortex-m0plus
# nucleo_f401re  | ST  | stm32f4  | tier 1 | cortex-m4
```

### `alloy doctor` — environment check

Existing `cmd_doctor()` in alloyctl.py becomes `alloy doctor`:
- arm-none-eabi-gcc ≥ 12.0 ✓
- cmake ≥ 3.25 ✓
- openocd ✓ / ✗
- imgtool (warn if absent)
- alloy-devices reachable or locally cached ✓

### PyPI publishing

`pyproject.toml` with:
```toml
[project]
name = "alloy-cli"
version = "0.1.0"
dependencies = ["jsonschema>=4.0", "rich>=13.0"]

[project.scripts]
alloy = "alloy_cli.__main__:main"
```

GitHub Actions: on tag push → `python -m build` → `twine upload`.

## Non-goals

- GUI or web dashboard.
- IDE plugin (VS Code extension is a separate spec).
- Cross-compilation of the alloy library itself (still CMake-driven).
