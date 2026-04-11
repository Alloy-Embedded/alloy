# Code Generation Contract

## Source of Truth

`alloy` does not generate MCU knowledge locally.

Device data comes from the published `alloy-devices` repository, which exposes:

- family-scoped metadata and generated descriptors
- device-scoped descriptors such as pins, peripheral instances, register maps, register fields, interrupt bindings, DMA bindings, and startup descriptors

## Consumption Model

The runtime consumes generated headers through `src/device/selected.hpp` and `src/device/descriptors.hpp`.

New runtime code should prefer:

- typed runtime profiles
- typed connector tables
- typed register and field descriptors
- typed instance, interrupt, DMA, and startup descriptors

It should not parse legacy string payloads when typed descriptors already exist.

## Repository Boundary

- `alloy-codegen` fetches, normalizes, validates, and emits descriptors
- `alloy-devices` publishes generated artifacts
- `alloy` consumes those artifacts and implements behavior

## See Also

- [RUNTIME_DEVICE_BOUNDARY.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_DEVICE_BOUNDARY.md)
