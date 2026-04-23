# Alloy

Modern C++20 bare-metal runtime built around generated, typed device contracts.

## What It Is

`alloy` consumes generated hardware facts from `alloy-devices` and keeps runtime
behavior in this repo.

Current direction:

- generated device facts, routes, clocks, startup, and driver semantics
- typed runtime import through `src/device/**`
- zero-overhead HAL paths built on compile-time traits instead of handwritten
  vendor policy trees
- board bring-up and examples on top of the same runtime surface

See:

- [Quickstart](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/QUICKSTART.md)
- [Board Tooling](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/BOARD_TOOLING.md)
- [Cookbook](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/COOKBOOK.md)
- [Downstream CMake Consumption](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/CMAKE_CONSUMPTION.md)
- [Migration Guide](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/MIGRATION_GUIDE.md)
- [Runtime Device Boundary](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_DEVICE_BOUNDARY.md)
- [Runtime Release Discipline](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_DISCIPLINE.md)
- [Support Matrix](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/SUPPORT_MATRIX.md)
- [Code Generation Contract](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/CODE_GENERATION.md)
- [Architecture](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/ARCHITECTURE.md)

## Quickstart

Fastest supported path to `blink` on a foundational board:

```bash
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target blink --build-first
python3 scripts/alloyctl.py monitor --board nucleo_g071rb
```

The supported board-oriented flow is documented in [docs/QUICKSTART.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/QUICKSTART.md) and [docs/BOARD_TOOLING.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/BOARD_TOOLING.md).

For canonical usage paths and migration notes, use [docs/COOKBOOK.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/COOKBOOK.md) and [docs/MIGRATION_GUIDE.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/MIGRATION_GUIDE.md).

## Validated Targets

Foundational runtime path:

- `nucleo_g071rb`
- `nucleo_f401re`
- `same70_xplained`
- `host` for host-MMIO validation

Additional board packages exist in [boards](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards), but the boards above are the ones used as the active runtime validation set.

Release support tiers and peripheral-class status now live in [docs/SUPPORT_MATRIX.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/SUPPORT_MATRIX.md).

## Current Examples

- `blink`
- `time_probe`
- `uart_logger`
- `i2c_scan`
- `spi_probe`
- `dma_probe`
- `timer_pwm_probe`
- `systick_demo`
- `timing/basic_delays`
- `rtos/simple_tasks`

Examples live under [examples](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples).

## Blocking, Event, Async

`alloy` keeps one primary HAL shape in `src/hal/*`.

- blocking code calls the HAL directly
- event-driven code uses the same HAL operation and waits on typed runtime completion tokens
- async integration is optional and lives in separate adapters under `src/async.hpp`

See [Runtime Async Model](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_ASYNC_MODEL.md) and [examples](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/README.md).

## Build

### Host validation

```bash
cmake --preset host-validation-debug
cmake --build --preset host-mmio-validation
ctest --preset host-mmio-validation
```

### Embedded validation

STM32F4:

```bash
cmake --preset stm32f4-renode-debug
cmake --build build/presets/stm32f4-renode-debug --target alloy-device-contract-smoke blink
```

STM32G0:

```bash
cmake --preset stm32g0-renode-debug
cmake --build build/presets/stm32g0-renode-debug --target alloy-device-contract-smoke blink
```

SAME70:

```bash
cmake --preset same70-analysis-debug
cmake --build build/presets/same70-analysis-debug --target alloy-device-contract-smoke uart_logger
```

### Direct board selection

```bash
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DALLOY_BOARD=nucleo_g071rb

cmake --build build --target blink
```

## Board Workflow

Use [Board Tooling](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/BOARD_TOOLING.md).

Short path:

```bash
python3 scripts/alloyctl.py bundle --board same70_xplained
python3 scripts/alloyctl.py flash --board same70_xplained --target uart_logger
python3 scripts/alloyctl.py monitor --board same70_xplained
```

## Runtime Shape

The active source-level boundary is:

- `src/device/**`: generated contract import wrappers
- `src/hal/**`: runtime behavior on typed traits
- `src/arch/**`: architecture-local hooks
- `boards/**`: board choices and orchestration

The legacy `src/hal/vendors/**` tree is gone from the active runtime path.

## Generated Contract

`alloy` does not generate MCU metadata locally.

It consumes published artifacts from `alloy-devices`, including:

- startup sources and startup contracts
- system clock and clock profile contracts
- typed ids for peripherals, pins, registers, and fields
- routes, DMA bindings, connector diagnostics, and capabilities
- driver semantics for GPIO, UART, I2C, SPI, DMA, timer, PWM, ADC, DAC, RTC,
  watchdog, and CAN

Import happens through:

- [src/device/selected.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/device/selected.hpp)
- [src/device/import.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/device/import.hpp)
- [src/device/runtime.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/device/runtime.hpp)

## Validation Gates

Core checks used by the rebuilt runtime:

- `python3 scripts/check_runtime_device_boundary.py`
- `python3 scripts/check_release_discipline.py`
- host-MMIO suites
- compile smoke through `alloy-device-contract-smoke`
- selected board/example builds
- SAME70 zero-overhead assembly gate
- Renode/ELF validation presets where enabled

## Documentation

- [Architecture](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/ARCHITECTURE.md)
- [Quickstart](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/QUICKSTART.md)
- [Board Tooling](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/BOARD_TOOLING.md)
- [Cookbook](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/COOKBOOK.md)
- [Downstream CMake Consumption](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/CMAKE_CONSUMPTION.md)
- [Migration Guide](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/MIGRATION_GUIDE.md)
- [Runtime Async Model](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_ASYNC_MODEL.md)
- [Runtime Release Discipline](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_DISCIPLINE.md)
- [Support Matrix](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/SUPPORT_MATRIX.md)
- [Release Checklist](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_CHECKLIST.md)
- [Runtime Device Boundary](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_DEVICE_BOUNDARY.md)
- [Runtime Cleanup Audit](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_CLEANUP_AUDIT.md)
- [Code Generation Contract](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/CODE_GENERATION.md)
- [Porting a New Board](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/PORTING_NEW_BOARD.md)
- [Porting a New Platform](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/PORTING_NEW_PLATFORM.md)
- [Nucleo-G071RB board page](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/nucleo_g071rb/README.md)
- [Nucleo-F401RE board page](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/nucleo_f401re/README.md)
- [SAME70 Xplained board page](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/README.md)

## Status

This repo is mid-migration toward a single runtime-first architecture. The active work is:

- consuming more of the published typed contract directly in `alloy`
- removing remaining legacy glue
- keeping the foundational boards on one runtime shape

## License

MIT. See [LICENSE](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/LICENSE).
