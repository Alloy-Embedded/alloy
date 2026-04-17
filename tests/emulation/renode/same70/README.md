# SAME70 Renode Boot Smoke

This scenario is the first Renode-backed validation layer for Alloy.

## Scope

- target board: `same70_xplained`
- target MCU: `ATSAME70Q21B`
- current goal:
  - load the Alloy-generated startup
  - reach `main()`
  - run `board::init()`
  - configure the debug UART (`USART0`)
  - emit a deterministic boot banner
  - prove the expected boot stage / marker / transmitted byte count from SRAM symbols

## Layout

- `tests/emulation/renode/common/robot/boot_assertions.resource`
  - shared Renode keywords for boot-style scenarios
- `tests/emulation/renode/common/scripts/run_renode_robot.py`
  - shared wrapper that stages files into a temp dir and resolves ELF symbols into Robot variables
- `tests/firmware/same70/boot_smoke.cpp`
  - shared boot smoke firmware used by both `elf` and `renode` validation layers
- `platforms/same70_boot_smoke.repl.in`
  - local Renode platform derived from the official `sam_e70` platform
- `scripts/boot_smoke.robot`
  - SAME70-specific robot scenario layered on top of the shared Renode boot assertions

## Build

Preferred path:

```bash
cmake --workflow --preset same70-renode-smoke
```

For the current layered proof (`ELF startup inspection + Renode boot smoke`):

```bash
cmake --workflow --preset same70-runtime-validation
```

Or step-by-step:

```bash
cmake -S . -B build-same70-renode \
  -DALLOY_BOARD=same70_xplained \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build-same70-renode --target same70_renode_boot_smoke
```

## Run

If `renode` and `renode-test` are installed:

```bash
cmake --preset same70-renode-debug
cmake --build --preset same70-renode-smoke
ctest --preset same70-renode-smoke
```

If CMake picks a Python without `robot`, Alloy now falls back to `python3`/`python` from `PATH`
before disabling the scenario. To force a specific interpreter:

```bash
cmake --preset same70-renode-debug \
  -DALLOY_RENODE_PYTHON_EXECUTABLE="$(command -v python3)"
```

### Apple Silicon note

On macOS `arm64`, install the `renode-<version>-dotnet.osx-arm64-portable.dmg` build and copy `Renode.app` to `/Applications`. Alloy now auto-detects:

- `/Applications/Renode.app/Contents/MacOS/renode`
- `/Applications/Renode.app/Contents/MacOS/renode-test`

If both paths exist, no extra `-DALLOY_RENODE_*` flags are required.

## Limits

- the current platform is intentionally narrow; it validates boot, first UART output, and a small set
  of deterministic boot-side observables
- this does not yet prove full peripheral correctness
- after this scenario is stable, the next step is to grow from boot smoke into clock/register assertions and then broader family-level Renode reuse
