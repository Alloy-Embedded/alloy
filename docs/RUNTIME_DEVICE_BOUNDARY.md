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

- typed runtime routes, clocks, registers, fields, and driver semantics
- generated startup sources, startup descriptors, and typed system-clock profiles
- optional reflection artifacts that stay outside the `alloy` runtime boundary

### `alloy`

`alloy` is responsible for:

- `connect()` and claim semantics
- GPIO, UART, SPI, I2C, and DMA runtime behavior
- architecture-local hooks that run after generated startup
- board bring-up orchestration
- public API shape and configuration defaults

Today `DMA` consumes:

- typed runtime DMA bindings
- typed DMA driver semantics
- schema-driven controller configuration in `alloy`

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

- includes of deleted legacy paths such as `hal/vendors/...`
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
2. consume generated startup and system-clock sources from `alloy-devices`
3. rebuild connector and claim kernel
4. rebuild drivers on descriptors
5. rewrite boards and examples
6. delete legacy vendor-centric glue and any dead startup/runtime residue

## Current Supported Entry Point

For the rebuilt runtime path, `src/device` is the only supported source-level
entry point into selected published descriptors.

## Vendor-Light Validation

The runtime is considered vendor-light only if both of these stay true:

- the runtime boundary check passes:
  - `python3 scripts/check_runtime_device_boundary.py`
- foundational descriptor builds pass on the same runtime core without new
  public API forks:
  - `nucleo_g071rb`: `alloy-device-contract-smoke`, `blink`
  - `nucleo_f401re`: `alloy-device-contract-smoke`, `blink`, `dma_probe`
  - `same70_xpld`: `alloy-device-contract-smoke`, `uart_logger`, `dma_probe`

This is the gate used to verify that one runtime shape still supports `stm32g0`,
`stm32f4`, and `same70`.

## Vendor-4 Admission Criteria

A fourth vendor is admitted to the rebuilt runtime only if all of the following
are true:

- the publish provides the same `runtime` surface already consumed by the
  foundational vendors:
  - `generated/runtime/types.hpp`
  - `generated/runtime/devices/<device>/{peripheral_instances,pins,registers,register_fields,routes,clock_bindings,dma_bindings}.hpp`
  - `generated/runtime/devices/<device>/driver_semantics/{common,gpio,uart,i2c,spi,dma}.hpp`
- `alloy` consumes that publish only through `src/device/**`
- the vendor brings no new public HAL tier, no vendor-specific public API, and
  no reflection fallback in the hot path
- the foundational validation shape still works unchanged:
  - compile smoke through `alloy-device-contract-smoke`
  - one canonical board/example path on the new runtime
  - zero-overhead behavior remains descriptor-driven rather than table-scanned

If a new vendor needs a new public API shape, a new reflection escape hatch, or
direct generated-family imports in runtime code, the runtime architecture is not
ready for that vendor yet.
