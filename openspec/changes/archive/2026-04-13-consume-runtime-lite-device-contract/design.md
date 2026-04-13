## Context

The current runtime migration improved the public API and removed a lot of vendor glue, but the
hot path is still not in its final form.

Evidence:

- [src/hal/detail/runtime_lite_ops.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/detail/runtime_lite_ops.hpp)
  is the runtime executor surface that must stay typed and minimal
- [src/hal/connect/runtime_connector.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/connect/runtime_connector.hpp)
  is the route resolution surface that must stay compile-time-only
- generated descriptor inventories such as connector and register-field tables are large enough
  that using them as the default runtime contract will hurt compile times and risks footprint

The runtime should instead consume a minimal compile-time contract that lets used code collapse
to direct register access.

## Goals

- make foundational runtime paths compile down to code equivalent to direct register use
- stop requiring family-wide lookup tables in normal driver and board paths
- preserve one public API while changing the implementation boundary underneath

## Non-Goals

- removing reflection or debug capability from the ecosystem
- preserving transitional runtime_backend helpers forever
- optimizing every optional debugging path before the main runtime path is fixed

## Decision: Runtime Hot Path Targets Runtime-Lite Only

The production runtime path SHALL consume only the runtime-lite contract for:

- instance selection
- route selection
- register and field access
- clock/reset binding
- startup-side descriptor consumption used in hot bring-up code

Reflection tables MAY still be consumed by:

- compile smoke tests
- debug utilities
- optional inspection tools

## Decision: No Generic Family Table Walk In Normal Driver Paths

Foundational drivers SHALL NOT perform generic scans over family-wide tables to resolve one
instance or route in normal usage.

Instead they SHALL rely on:

- trait specializations
- template aliases
- `constexpr` route/operation packs
- direct generated ids and refs

## Decision: Public API Stays Small While Implementation Gets More Compile-Time

The user-facing HAL remains one coherent API per peripheral class. The implementation under it
may become more template-driven, but the public surface SHALL remain small and readable.

This means:

- convenience remains in config defaults and named board aliases
- compile-time machinery lives behind the API, not as a second public programming model

## Decision: Device Import Layer Exposes Runtime-Lite Only

`src/device` SHALL expose runtime-lite imports for the library runtime.

Reflection artifacts MAY continue to exist in `alloy-devices` for tooling, but they SHALL NOT be
re-exported by `alloy`.

## Decision: Zero-Overhead Gate

The runtime migration SHALL treat the following as architectural failures:

- requiring `std::span` table iteration to open GPIO/UART/SPI/I2C in normal board code
- carrying descriptor strings or reflection payload into foundational hot-path code
- requiring dynamic branchy lookup where the board-selected type already determines the result at
  compile time

## Migration Order

1. Introduce the runtime-lite import layer in `src/device`
2. Rebuild the runtime executor around `runtime_lite_ops` and `runtime_connector`
3. Migrate `gpio` and `uart`
4. Migrate `spi` and `i2c`
5. Remove reflection-table dependency from board defaults and examples
6. Demote reflection consumption to smoke/debug-only code
