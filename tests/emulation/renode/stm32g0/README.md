# STM32G0 Renode Boot Smoke

This scenario is the first STM32G0 Renode-backed validation layer for Alloy.

## Scope

- target board: `nucleo_g071rb`
- target MCU: `STM32G071RB`
- current goal:
  - load the Alloy-generated startup
  - reach `main()`
  - run `board::init()`
  - configure the debug UART (`USART2`)
  - emit a deterministic boot banner
  - assert key RCC / GPIOA / USART2 register side effects from emulation

## Why STM32G0 now

The validated Renode bundle used by Alloy ships a CPU-level `stm32g0` platform:

- `platforms/cpus/stm32g0.repl`

After extracting the shared Renode CMake helper, STM32G0 became the right next target to prove
that a third family can be added with a thin overlay instead of duplicating the whole scenario
plumbing again.

## Layout

- `tests/firmware/stm32g0/boot_smoke.cpp`
  - shared STM32G0 boot smoke firmware used by Renode and ELF validation
- `tests/emulation/renode/common/platforms/stm32_boot_smoke.repl.in`
  - shared ST overlay template over Renode's official CPU platforms
- `tests/emulation/renode/common/stm32_renode_helpers.cmake`
  - shared ST CMake glue for board match, REPL staging, and test registration
- `scripts/boot_smoke.robot`
  - STM32G0-specific robot scenario layered on top of the shared boot + STM32 boot assertions

## Build

Preferred path:

```bash
cmake --workflow --preset stm32g0-renode-smoke
```

For the current layered proof (`ELF startup inspection + Renode boot smoke`):

```bash
cmake --workflow --preset stm32g0-runtime-validation
```

## Run

If `renode` and `renode-test` are installed:

```bash
python3 -m pip install -r /Applications/Renode.app/Contents/MacOS/tests/requirements.txt
cmake --preset stm32g0-renode-debug
cmake --build --preset stm32g0-runtime-validation
ctest --preset stm32g0-runtime-validation
```

If CMake picks a Python without `robot`, Alloy now falls back to `python3`/`python` from `PATH`
before disabling the scenario. To force a specific interpreter:

```bash
cmake --preset stm32g0-renode-debug \
  -DALLOY_RENODE_PYTHON_EXECUTABLE="$(command -v python3)"
```

## Limits

- this is still a narrow boot scenario; it is not a full STM32G0 peripheral test suite
- the platform intentionally stays a thin overlay over the official Renode CPU asset
- the current board default is the published `safe_hsi16` profile, so the scenario asserts the
  modeled HSI16 bring-up state (`RCC_CR`, `RCC_CFGR`, `FLASH_ACR`) plus GPIO/UART side effects
  instead of relying on RCC gate readback or the older 64 MHz PLL path
- ST-family overlay/assertion reuse now lives in `tests/emulation/renode/common/`; further
  expansion should build on that shared path instead of adding new per-board scaffolding
