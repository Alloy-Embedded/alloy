# Code Generation Contract

## Source of Truth

`alloy` does not generate MCU knowledge locally.

Device data comes from the published `alloy-devices` repository, which exposes:

- typed runtime artifacts for pins, peripheral instances, routes, register maps, register fields, clocks, and driver semantics

## Consumption Model

The runtime consumes generated headers through `src/device/selected.hpp` and `src/device/runtime.hpp`.

New runtime code should prefer:

- typed runtime ids, traits, refs, and driver semantics
- `runtime_connector`-resolved operation packs
- compile-time route and clock/reset bindings

It should not depend on reflection tables or legacy string payloads.

## Repository Boundary

- `alloy-codegen` fetches, normalizes, validates, and emits descriptors
- `alloy-devices` publishes generated artifacts
- `alloy` consumes those artifacts and implements behavior

## See Also

- [RUNTIME_DEVICE_BOUNDARY.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_DEVICE_BOUNDARY.md)
