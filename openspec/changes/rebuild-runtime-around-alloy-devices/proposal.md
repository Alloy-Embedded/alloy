## Why

`alloy-codegen` and `alloy-devices` now define a descriptor-first hardware contract, but the
runtime repo still reflects an older architecture:

- vendor-specific generated code lives inside `alloy`
- boards include vendor headers directly
- startup, clocks, pinmux, and connection rules are handwritten per family
- public HAL is split across overlapping layers (`simple`, `expert`, `fluent`, legacy interface headers)
- CMake selects boards and startup code through large manual switch blocks

That architecture does not scale to many families and vendors. It also undermines the main value
of the new ecosystem split: `alloy-devices` should own device facts and `alloy` should own
behavior.

If we keep adapting the old structure incrementally, we will preserve the wrong seams:
- the runtime will keep leaking vendor details
- drivers will continue to drift by family
- boards and examples will keep bypassing the intended path
- new vendors will multiply maintenance cost instead of exercising one generic architecture

This change rebuilds the runtime around the published device-descriptor contract, starting with a
cleanup-first reset.

## What Changes

- **BREAKING** replace the current platform/vendor-centric runtime structure with a
  descriptor-driven runtime that consumes `alloy-devices`
- **BREAKING** collapse the public HAL surface to one primary API per peripheral class; remove the
  `simple`/`expert`/`fluent` split as a public architectural concept
- **BREAKING** move board bring-up to a declarative board layer over shared runtime primitives
- **BREAKING** replace handwritten per-family startup logic with architecture runtime plus
  descriptor-fed startup data
- **BREAKING** replace manual signal registries and vendor enums in the core with connector and
  claim mechanisms driven by generated descriptors
- **NEW** define a strict runtime/device boundary:
  - `alloy-devices` publishes descriptors and reports
  - `alloy` imports those descriptors and implements behavior
- **NEW** define a cleanup-first migration plan that explicitly allows deletion of incompatible
  legacy code when that is simpler and safer than compatibility layering

## Impact

- Affected specs:
  - `runtime-device-boundary`
  - `public-hal-api`
  - `board-bringup`
  - `build-and-selection`
  - `startup-runtime`
  - `migration-cleanup`
- Affected code:
  - `src/hal/**`
  - `src/arch/**`
  - `boards/**`
  - `examples/**`
  - `cmake/**`
  - legacy vendor/runtime glue throughout the repo
