# Runtime Device Boundary

## Purpose

This document defines the permanent boundary between `alloy` runtime code and
published hardware data in `alloy-devices`.

The rule is simple:

- `alloy-devices` owns hardware facts
- `alloy` owns runtime behavior
- boards own local choices

## Allowed Ownership

### `alloy-devices`

`alloy-devices` is the source of truth for:

- typed runtime-lite routes, clocks, registers, fields, and driver semantics
- generated startup vectors
- optional reflection artifacts that stay outside the `alloy` runtime boundary

### `alloy`

`alloy` is responsible for:

- `connect()` and claim semantics
- GPIO, UART, SPI, I2C, and DMA runtime behavior
- startup algorithm and architecture bootstrap
- board bring-up orchestration
- public API shape and configuration defaults

## Import Rule

Runtime code SHALL import selected device data only through:

- [`src/device/selected.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/device/selected.hpp)
- [`src/device/import.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/device/import.hpp)
- [`src/device/runtime.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/device/runtime.hpp)
- [`src/device/traits.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/device/traits.hpp)

Generated vendor and family paths SHALL NOT appear directly in new runtime code.

## Forbidden Patterns In New Runtime Code

The following are not allowed outside the device import layer and generator
integration code:

- includes of `hal/vendors/.../generated/...`
- includes of published family paths such as `st/...`, `microchip/...`, or
  `nxp/...`
- filesystem references to `alloy-devices` from runtime source files
- new cross-vendor handwritten signal or pin registries

## Transitional Scope

This boundary is enforced immediately for:

- `src/device/**`
- `src/arch/**`
- `tests/compile_tests/**`

Legacy trees still being migrated are documented in the cleanup audit and are
not yet fully blocked:

- `boards/**`
- `examples/**`
- `src/hal/**`

## Migration Direction

The cleanup proceeds in this order:

1. stabilize import and build selection
2. keep startup vectors as generated sources, not reflection imports
3. rebuild connector and claim kernel
4. rebuild drivers on descriptors
5. rewrite boards and examples
6. delete legacy vendor-centric glue and any dead startup/runtime residue

## Current Supported Entry Point

For the rebuilt runtime path, `src/device` is the only supported source-level
entry point into selected published descriptors.
