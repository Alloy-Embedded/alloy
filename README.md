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

- [Runtime Device Boundary](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_DEVICE_BOUNDARY.md)
- [Code Generation Contract](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/CODE_GENERATION.md)
- [Architecture](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/ARCHITECTURE.md)

## Validated Targets

Foundational runtime path:

- `nucleo_g071rb`
- `nucleo_f401re`
- `same70_xplained`
- `host` for host-MMIO validation

Additional board packages exist in [boards](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards), but the boards above are the ones used as the active runtime validation set.

## Current Examples

- `blink`
- `uart_logger`
- `i2c_scan`
- `spi_probe`
- `dma_probe`
- `timer_pwm_probe`
- `systick_demo`
- `timing/basic_delays`
- `rtos/simple_tasks`

Examples live under [examples](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples).

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
- host-MMIO suites
- compile smoke through `alloy-device-contract-smoke`
- selected board/example builds
- SAME70 zero-overhead assembly gate
- Renode/ELF validation presets where enabled

## Documentation

- [Architecture](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/ARCHITECTURE.md)
- [Runtime Device Boundary](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_DEVICE_BOUNDARY.md)
- [Runtime Cleanup Audit](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_CLEANUP_AUDIT.md)
- [Code Generation Contract](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/CODE_GENERATION.md)
- [Porting a New Board](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/PORTING_NEW_BOARD.md)
- [Porting a New Platform](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/PORTING_NEW_PLATFORM.md)

## Status

This repo is mid-migration toward a single runtime-first architecture. The active work is:

- consuming more of the published typed contract directly in `alloy`
- removing remaining legacy glue
- keeping the foundational boards on one runtime shape

## License

MIT. See [LICENSE](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/LICENSE).
