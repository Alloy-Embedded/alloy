# STM32F4 Renode Boot Smoke

This scenario is the first ST-family Renode-backed validation layer for Alloy.

## Scope

- target board: `nucleo_f401re`
- target MCU: `STM32F401RE`
- current goal:
  - load the Alloy-generated startup
  - reach `main()`
  - run `board::init()`
  - configure the debug UART (`USART2`)
  - emit a deterministic boot banner
  - assert key RCC / GPIOA / USART2 register side effects from emulation

## Why STM32F4

The currently validated Renode bundle used by Alloy already ships:

- `platforms/cpus/stm32f4.repl`
- `platforms/boards/stm32f4_discovery.repl`

That makes `stm32f4` the lowest-risk ST family for the second Renode step after SAME70.

## Layout

- `tests/firmware/stm32f4/boot_smoke.cpp`
  - shared STM32F4 boot smoke firmware used by Renode and ELF validation
- `tests/emulation/renode/common/platforms/stm32_boot_smoke.repl.in`
  - shared ST overlay template over Renode's official CPU platforms
- `tests/emulation/renode/common/stm32_renode_helpers.cmake`
  - shared ST CMake glue for board match, REPL staging, and test registration
- `scripts/boot_smoke.robot`
  - STM32F4-specific robot scenario layered on top of the shared boot + STM32 boot assertions

## Build

Preferred path:

```bash
cmake --workflow --preset stm32f4-renode-smoke
```

For the current layered proof (`ELF startup inspection + Renode boot smoke`):

```bash
cmake --workflow --preset stm32f4-runtime-validation
```

Or step-by-step:

```bash
cmake -S . -B build-stm32f4-renode \
  -DALLOY_BOARD=nucleo_f401re \
  -DALLOY_BUILD_TESTS=ON \
  -DALLOY_ENABLE_RENODE_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build-stm32f4-renode --target stm32f4_elf_startup_smoke stm32f4_renode_boot_smoke
```

## Run

If `renode` and `renode-test` are installed:

```bash
python3 -m pip install -r /Applications/Renode.app/Contents/MacOS/tests/requirements.txt
cmake --preset stm32f4-renode-debug
cmake --build --preset stm32f4-runtime-validation
ctest --preset stm32f4-runtime-validation
```

If CMake picks a Python without `robot`, Alloy now falls back to `python3`/`python` from `PATH`
before disabling the scenario. To force a specific interpreter:

```bash
cmake --preset stm32f4-renode-debug \
  -DALLOY_RENODE_PYTHON_EXECUTABLE="$(command -v python3)"
```

## Limits

- this is still a narrow boot scenario; it is not a full STM32F4 peripheral test suite
- the platform intentionally stays a thin overlay over the official Renode assets
- ST-family overlay/assertion reuse now lives in `tests/emulation/renode/common/`; expansion should
  add new ST boards through the shared helper path instead of cloning scenario plumbing
