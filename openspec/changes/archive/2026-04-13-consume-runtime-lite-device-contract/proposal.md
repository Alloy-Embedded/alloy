## Why

`alloy` currently consumes the generated device contract in a way that is still too reflective.
The runtime now includes typed data, but it still leans on:

- family-wide descriptor tables
- `std::span`-backed inventories
- lookup helpers
- transitional runtime glue that rediscovers compile-time facts

That is not acceptable for the final runtime path. Normal usage of the library should not carry
more runtime overhead or larger footprint than direct register code for the same peripheral
bring-up path.

The `alloy` side therefore needs an explicit migration to the new `runtime-lite` contract
published by `alloy-devices`.

## What Changes

- **BREAKING** move the runtime hot path from reflection-table consumption to `runtime-lite`
  generated traits and specializations
- **BREAKING** forbid family-wide descriptor-table scans in normal GPIO/UART/SPI/I2C bring-up
- **NEW** define zero-overhead expectations for the public runtime path
- **NEW** keep reflection descriptors available only for tooling, smoke checks, and optional
  debugging paths
- **NEW** rebuild `connect()`, claims, and foundational drivers around compile-time-resolved
  runtime-lite data

## Impact

- Affected specs:
  - `runtime-device-boundary`
  - `public-hal-api`
  - `build-and-selection`
  - `migration-cleanup`
  - `zero-overhead-runtime`
- Affected code:
  - `src/device/**`
  - `src/hal/detail/runtime_lite_ops.hpp`
  - `src/hal/connect/runtime_connector.hpp`
  - `src/hal/gpio/**`
  - `src/hal/uart/**`
  - `src/hal/spi/**`
  - `src/hal/i2c/**`
  - board helpers and compile smoke coverage
