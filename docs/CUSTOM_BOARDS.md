# Custom Boards

Alloy supports two ways to declare a board:

1. **In-tree foundational boards** -- maintained inside the runtime under `boards/<name>/`
   and selected with `-DALLOY_BOARD=<name>`. These are the boards Alloy validates as part
   of the project's release discipline.
2. **Custom boards** -- declared in a downstream project's own repository and selected with
   `-DALLOY_BOARD=custom`. The downstream project owns its `board/` directory and edits
   pin assignments freely without forking the runtime.

Both paths use the same `board -> public HAL -> alloy-devices` flow. The only difference is
where the board sources live.

If you scaffold a project with `alloy new`, this contract is wired up for you. The recipe
below documents the contract for users who consume Alloy without the CLI.

## Required CMake variables

When `ALLOY_BOARD=custom`, the runtime reads its inputs from a small set of cache
variables that the downstream project sets *before* `add_subdirectory(<alloy>)`:

| Variable                       | Required | Notes                                                                      |
|--------------------------------|----------|----------------------------------------------------------------------------|
| `ALLOY_CUSTOM_BOARD_HEADER`    | yes      | Absolute path to your `board.hpp` (e.g. `${CMAKE_SOURCE_DIR}/board/board.hpp`). |
| `ALLOY_CUSTOM_LINKER_SCRIPT`   | yes      | Absolute path to your linker script.                                       |
| `ALLOY_DEVICE_VENDOR`          | yes      | e.g. `st`, `microchip`, `raspberrypi`, `espressif`.                        |
| `ALLOY_DEVICE_FAMILY`          | yes      | e.g. `stm32g0`, `same70`, `rp2040`.                                        |
| `ALLOY_DEVICE_NAME`            | yes      | The descriptor directory name in `alloy-devices` (e.g. `stm32g071rb`).     |
| `ALLOY_DEVICE_ARCH`            | yes      | One of `cortex-m0plus`, `cortex-m4`, `cortex-m7`, `riscv32`, `xtensa-lx6` (ESP32 classic), `xtensa-lx7` (ESP32-S2/S3), `avr`, `native`. |
| `ALLOY_DEVICE_MCU`             | no       | Canonical part number used in diagnostics. Defaults to `ALLOY_DEVICE_NAME`. |
| `ALLOY_FLASH_SIZE_BYTES`       | no       | Declared flash size in bytes. Defaults to `0`.                             |

The runtime fails fast at configure time if any required variable is missing, if a path is
relative, if the architecture is outside the accepted set, or if the
`<vendor>/<family>/<device>` tuple has no descriptor under `ALLOY_DEVICES_ROOT`.

## Minimal recipe

```cmake
cmake_minimum_required(VERSION 3.25)

set(ALLOY_BOARD "custom")
set(ALLOY_CUSTOM_BOARD_HEADER  "${CMAKE_CURRENT_SOURCE_DIR}/board/board.hpp")
set(ALLOY_CUSTOM_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/board/STM32G071RBT6.ld")
set(ALLOY_DEVICE_VENDOR "st")
set(ALLOY_DEVICE_FAMILY "stm32g0")
set(ALLOY_DEVICE_NAME   "stm32g071rb")
set(ALLOY_DEVICE_ARCH   "cortex-m0plus")
set(ALLOY_DEVICE_MCU    "STM32G071RBT6")
set(ALLOY_FLASH_SIZE_BYTES 131072)

project(my_firmware LANGUAGES C CXX ASM)

add_subdirectory(${ALLOY_ROOT} ${CMAKE_BINARY_DIR}/_alloy)

add_executable(my_firmware src/main.cpp)
target_link_libraries(my_firmware PRIVATE alloy::alloy)
```

The `board/` directory follows the same shape as in-tree boards (e.g.
[`boards/nucleo_g071rb/`](../boards/nucleo_g071rb/)): `board.hpp`, `board_config.hpp`,
`board.cpp`, optional `syscalls.cpp`, and a linker script. Copy that layout as a starting
point and edit the pin assignments for your hardware.

## CLI shortcut

```bash
alloy new ./my_firmware --mcu STM32G071RBT6
```

scaffolds a project that already wires the contract above. If the MCU has a foundational
board in the runtime, `alloy new` copies it into `<project>/board/` so you can extend it
with peripherals the in-tree version does not yet expose. If the MCU has a descriptor but
no foundational board, the CLI generates a minimal skeleton you fill in.

## Smoke test

A configure-only smoke test under [`tests/custom_board/`](../tests/custom_board/) exercises
this path. To run it manually:

```bash
python3 scripts/check_custom_board.py
```

The script requires an `alloy-devices` checkout at `../alloy-devices` (or pass
`--devices-root <path>`).
