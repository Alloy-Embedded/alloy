# Alloy Runtime Validation

`tests/` owns the runtime-validation stack for Alloy. The structure is intentionally split by validation layer so the project can scale from compile smoke to emulation and a small number of hardware spot-checks.

Release support claims are published separately in [docs/SUPPORT_MATRIX.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/SUPPORT_MATRIX.md) and backed by [docs/RELEASE_MANIFEST.json](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_MANIFEST.json).

## Validation Taxonomy

- `compile/`
  - owns compile-only validation that must build for a selected device contract
  - currently wraps the legacy `compile_tests/` sources while the migration is in progress
- `unit/`
  - host-only Catch2 tests for core types and concepts
- `integration/`
  - host-only Catch2 tests that validate multi-component behavior
- `regression/`
  - host-only Catch2 tests for fixed bugs
- `host_mmio/`
  - test-only MMIO simulation framework and register-sequence validation
  - the host target can override `alloy/device/selected_config.hpp` per test binary so native execution still exercises a real published device contract
  - current coverage includes `same70`, `stm32g071rb`, and `stm32f401re`
  - ST-family bring-up scenarios share `framework/stm32_gpio_uart_expect.hpp` so board-backed
    GPIO/UART assertions stay aligned across `stm32g0` and `stm32f4`
- `elf/`
  - ELF/startup inspection scenarios
  - current family coverage: `same70`, `stm32g0`, `stm32f4`
- `emulation/renode/`
  - SAME70-first Renode scenarios
  - `common/` holds reusable Renode robot resources, launch helpers, and shared CMake glue
  - ST-family scenarios also share `common/stm32_renode_helpers.cmake`,
    `common/platforms/stm32_boot_smoke.repl.in`, and
    `common/robot/stm32_boot_assertions.resource`
  - current family coverage: `same70`, `stm32g0`, `stm32f4`
  - current scenarios assert boot markers plus family-specific register/UART observables
- `hardware/`
  - representative board spot-check runbooks for silicon validation
- `perf/assembly/`
  - dedicated zero-overhead assembly/size validation gates
  - current SAME70 gate compares manual register access and runtime hot paths with `nm` +
    `objdump`

## Current Coverage

The current runtime-validation ladder is representative, not exhaustive. Alloy does **not** claim
Renode bring-up coverage for every published MCU today.

| Layer | Current boards | What is proved |
| --- | --- | --- |
| `host_mmio/` | `same70_xplained`, `nucleo_g071rb`, `nucleo_f401re` | Recorded bring-up ordering and final MMIO state for clock/reset/GPIO/UART on native host |
| `elf/` | `same70_xplained`, `nucleo_g071rb`, `nucleo_f401re` | Vector table, reset entry, required sections, and startup readiness |
| `emulation/renode/` | `same70_xplained`, `nucleo_g071rb`, `nucleo_f401re` | Boot to `main`, debug UART banner, boot markers, UART byte count, and family-specific register side effects |
| `perf/assembly/` | `same70_xplained` | Zero-overhead assembly/size checks for runtime hot paths |

Current UART coverage by board:

- `same70_xplained`
  - banner observed on `USART1` EDBG VCOM
  - boot marker/stage verified
  - exact UART byte count verified
- `nucleo_g071rb`
  - banner observed on `USART2` terminal
  - exact UART byte count verified
  - `USART2_BRR` and `USART2_CR1` programmed as expected
- `nucleo_f401re`
  - banner observed on `USART2` terminal
  - exact UART byte count verified
  - `USART2_BRR` and `USART2_CR1` programmed as expected

## Build and Run

### Preset workflow

For the current SAME70 Renode smoke path, the shortest flow is:

```bash
cmake --workflow --preset same70-renode-smoke
```

For the SAME70 zero-overhead path, use the dedicated preset:

```bash
cmake --workflow --preset same70-zero-overhead
```

To run the current SAME70 validation ladder (`ELF + Renode`):

```bash
cmake --workflow --preset same70-runtime-validation
```

For the first STM32F4 validation ladder (`ELF + Renode`):

```bash
cmake --workflow --preset stm32f4-runtime-validation
```

For the STM32G0 validation ladder (`ELF + Renode`):

```bash
cmake --workflow --preset stm32g0-runtime-validation
```

You can also split it by phase:

```bash
cmake --preset same70-renode-debug
cmake --build --preset same70-runtime-validation
ctest --preset same70-runtime-validation
```

STM32F4 follows the same split:

```bash
cmake --preset stm32f4-renode-debug
cmake --build --preset stm32f4-runtime-validation
ctest --preset stm32f4-runtime-validation
```

STM32G0 follows the same split:

```bash
cmake --preset stm32g0-renode-debug
cmake --build --preset stm32g0-runtime-validation
ctest --preset stm32g0-runtime-validation
```

The zero-overhead gate stays separate from behavioral validation and does not require Renode:

```bash
cmake --preset same70-analysis-debug
cmake --build --preset same70-zero-overhead
ctest --preset same70-zero-overhead
```

### Host Catch2 suites

Preferred path for the runtime-contract host MMIO ladder:

```bash
cmake --workflow --preset host-mmio-validation
```

Or split it by phase:

```bash
cmake --preset host-validation-debug
cmake --build --preset host-mmio-validation
ctest --preset host-mmio-validation
```

Full host validation remains available with the manual build dir flow:

```bash
cmake -S . -B build-tests-host -DALLOY_BOARD=host -DALLOY_BUILD_TESTS=ON
cmake --build build-tests-host
ctest --test-dir build-tests-host --output-on-failure
```

The current `host_mmio` target overrides the selected runtime contract to `microchip/same70/atsame70q21b`, so the native host run validates real SAME70 runtime GPIO/UART/watchdog bring-up code without switching the whole workspace away from `ALLOY_BOARD=host`.

The host MMIO layer now also carries an ST descriptor-driven target that overrides the selected
contract to `st/stm32g0/stm32g071rb`, covering RCC/GPIO/USART bring-up on the native host through
the production `nucleo_g071rb` debug-UART connector path.

It also carries a second ST-family contract override for `st/stm32f4/stm32f401re`, covering the
legacy-style STM32F4 RCC/GPIO/USART path on the native host through the production
`nucleo_f401re` debug-UART connector path.

### Filter by validation category

```bash
ctest --test-dir build-tests-host --output-on-failure -L unit
ctest --test-dir build-tests-host --output-on-failure -L integration
ctest --test-dir build-tests-host --output-on-failure -L regression
ctest --test-dir build-tests-host --output-on-failure -L host-mmio
ctest --test-dir build-same70-renode --output-on-failure -L elf
ctest --test-dir build-same70-renode --output-on-failure -L emulation
ctest --test-dir build/presets/same70-analysis-debug --output-on-failure -L assembly
```

### Hardware spot-check runbooks

Hardware validation is intentionally representative, not exhaustive. The initial runbooks live under:

- `tests/hardware/common/README.md`
- `tests/hardware/same70/README.md`
- `tests/hardware/stm32g0/README.md`
- `tests/hardware/stm32f4/README.md`

Current foundation boards:

- `same70_xplained`
  - full validation bundle: `same70_hardware_validation_bundle`
- `nucleo_g071rb`
  - full validation bundle: `stm32g0_hardware_validation_bundle`
- `nucleo_f401re`
  - full validation bundle: `stm32f4_hardware_validation_bundle`

Representative configure/build pattern:

```bash
cmake -S . -B build/hw/<board> \
  -DALLOY_BOARD=<board> \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build/hw/<board> --target <targets...> --parallel 8
```

The repo currently stops at build artifacts (`.elf`, `.hex`, `.bin`). Flashing and UART capture are
board-lab responsibilities and are intentionally documented in the runbooks instead of being faked as
portable CI targets.

### Apple Silicon Renode

On macOS `arm64`, prefer the `dotnet.osx-arm64-portable` Renode app. After copying it to `/Applications/Renode.app`, Alloy will auto-detect:

- `/Applications/Renode.app/Contents/MacOS/renode`
- `/Applications/Renode.app/Contents/MacOS/renode-test`

`renode-test` also needs its Python dependencies. If the `robot` module is missing, Alloy now skips
registration of Renode runtime tests and prints a configure-time warning. Alloy first tries the
CMake-selected Python interpreter, then falls back to `python3`/`python` from `PATH` if one of them
already has `robot` installed. You can also force the interpreter with
`-DALLOY_RENODE_PYTHON_EXECUTABLE=/path/to/python`.

Install the bundled requirements with:

```bash
python3 -m pip install -r /Applications/Renode.app/Contents/MacOS/tests/requirements.txt
```

The legacy Mono/macOS bundle exposes `Renode.exe` instead and is not the recommended default on Apple Silicon.

### Compile contract smoke

Compile validation is registered as an object library because the signal is successful compilation, not a runtime assertion:

```bash
cmake -S . -B build-device -DALLOY_BOARD=nucleo_g071rb
cmake --build build-device --target alloy-device-contract-smoke
```

## Design Rules

- host MMIO must remain test-only and must not add indirection to production MMIO paths
- emulation starts with `same70` in Renode before expanding to more families
- Renode scenarios should reuse `tests/emulation/renode/common/` for file staging, symbol lookup, and boot assertions
- `stm32f4` is the selected second Renode family after `same70`; the initial `nucleo_f401re` boot smoke now runs through the same shared Renode harness
- hardware spot-checks are board-focused runbooks with mandatory `blink` + `uart_logger`, plus `dma_probe` on boards that expose typed DMA helpers today
- compile validation stays strict about device boundary imports
- zero-overhead validation stays in `tests/perf/assembly/` with its own presets and labels instead
  of piggybacking on behavioral ladders
- hardware coverage is representative, not exhaustive
