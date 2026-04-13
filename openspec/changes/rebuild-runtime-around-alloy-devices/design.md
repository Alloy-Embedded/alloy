## Context

The current runtime still assumes that `alloy` owns too much hardware knowledge.

Evidence in the repo:
- dead legacy comments and docs still refer to `src/hal/interface/`, even though the tree has
  already been removed
- [boards/nucleo_g071rb/board.cpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/nucleo_g071rb/board.cpp) handwrites clock and startup-adjacent logic
- [src/startup](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/startup) still duplicates part of the startup ownership that is moving into `src/arch/cortex_m`
- [CMakeLists.txt](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/CMakeLists.txt) and [cmake/platform_selection.cmake](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/cmake/platform_selection.cmake) still encode family and startup selection manually

That shape is incompatible with the final boundary defined by
[codegen-alloy-boundary.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy-codegen/docs/codegen-alloy-boundary.md).

The right target is:
- `alloy-devices` owns facts
- `alloy` owns behavior
- boards own local choices

## Goals / Non-Goals

### Goals

- make `alloy` a descriptor-driven runtime that scales across vendors without rewriting its public
  API
- expose one coherent public HAL API per peripheral class
- keep board code declarative and side-effect safe
- move startup behavior to architecture runtime fed by device descriptors
- allow aggressive deletion of legacy code that conflicts with the new boundary
- make adding a new vendor mostly a codegen and descriptor-consumption exercise, not a runtime
  rewrite

### Non-Goals

- preserving every current include path or API surface
- keeping `simple`, `expert`, and `fluent` APIs in parallel
- continuing to store generated device support inside `alloy`
- adding RTOS or async as part of this rebuild
- adding vendor 4 before the foundational runtime path is stable

## Decisions

### Decision 1: Favor a cleanup-first rebuild over compatibility layering

The current runtime contains too many incompatible assumptions. Trying to preserve all of them
will create a dual architecture and prolong confusion.

This proposal explicitly allows:
- deleting obsolete vendor headers and API layers
- replacing large CMake selection blocks
- rewriting board code to consume the new runtime directly

The guiding rule is:
- keep only what still fits the runtime/device boundary
- delete what fights the new boundary

### Decision 2: One public HAL API

The public runtime SHALL expose one primary API per peripheral class.

That API may use:
- configuration structs with defaults
- named helper aliases
- default template arguments

But it SHALL NOT expose distinct public API families such as:
- `simple`
- `expert`
- `fluent`

The runtime may still provide convenience helpers, but those helpers are thin wrappers over the
same underlying API and not separate conceptual layers.

### Decision 3: Runtime layers

The target runtime is:

```text
alloy-devices
  -> published hardware descriptors

src/device
  -> selected device import and stable aliases

src/hal
  -> connector, claim, gpio, uart, spi, i2c, dma

src/arch
  -> Cortex-M runtime and startup algorithms

boards/<board>
  -> declarative board resource choices and bring-up policy
```

### Decision 4: Stable import layer for `alloy-devices`

`alloy` SHALL not scatter direct references to vendor/family paths across boards and drivers.

Instead it SHALL provide a stable import layer, for example:
- `alloy::device::selected::pins`
- `alloy::device::selected::peripherals`
- `alloy::device::selected::instances`
- `alloy::device::selected::startup`

This isolates the runtime from descriptor layout churn and makes boards and drivers read like
runtime code rather than generator integration code.

### Decision 5: Generic `connect()` over generated descriptors

The runtime core SHALL stop owning cross-vendor signal enums and handwritten registries.

`connect()` SHALL operate over generated descriptor tables:
- connection candidates
- connection groups
- route requirements
- route operations
- capability overlays

The runtime implements:
- typed selection
- compile-time validity checks
- application of route operations
- ownership claims

The device repo provides:
- the legal graph

### Decision 6: Claims are first-class

To avoid double configuration and scale across more peripherals, the runtime SHALL introduce
resource claims for:
- pins
- peripheral instances
- DMA routes
- interrupt bindings

The public API may hide claim details in common cases, but the underlying model SHALL remain
single-owner.

### Decision 7: Drivers are capability-driven, not vendor-driven

Drivers SHALL be written by peripheral class and capability:
- `gpio`
- `uart`
- `spi`
- `i2c`
- `dma`

They consume:
- instance descriptors
- IP-version descriptors
- capability overlays
- connector tables
- clock/reset descriptors

They SHALL NOT require per-vendor public driver classes for foundational use cases.

### Decision 8: Boards are declarative

Boards SHALL:
- select default pins, peripherals, and profiles
- expose stable aliases such as `board::led`, `board::debug_uart`, `board::button`
- orchestrate bring-up using runtime APIs

Boards SHALL NOT:
- perform hardware side effects in headers
- include raw vendor register access for normal bring-up
- implement handwritten per-board copies of startup behavior

### Decision 9: Startup runtime belongs to `alloy`

`alloy-devices` publishes startup descriptors and vector data.

`alloy` SHALL implement:
- reset handler algorithm
- `.data` copy / `.bss` zero logic
- vector-table integration
- architecture bootstrap

The algorithm belongs in `src/arch`, while the data comes from `alloy-devices`.

### Decision 10: Build selection must be data-driven

The giant `if/elseif` model in CMake does not scale.

The new build system SHALL:
- import `alloy-devices`
- select board, vendor, family, device, and architecture through a compact board manifest path
- add the selected device descriptors and startup vectors to the build
- stop hardcoding family startup source files in top-level `CMakeLists.txt`

## Target Layout

```text
src/
  device/
    selected.hpp
    import.hpp
    descriptors.hpp
    traits.hpp
  hal/
    connect/
    claim/
    gpio/
    uart/
    spi/
    i2c/
    dma/
  arch/
    cortex_m/
  board/
    bringup.hpp
    clock_profile.hpp
boards/
  <board>/
    board.hpp
    board.cpp
cmake/
  alloy_devices.cmake
  board_manifest.cmake
  device_selection.cmake
```

## Public API Shape

Example target shape:

```cpp
using DebugUart = alloy::connector<
    alloy::device::selected::peripherals::usart2,
    alloy::tx<alloy::device::selected::pins::pa2>,
    alloy::rx<alloy::device::selected::pins::pa3>
>;

auto uart = alloy::uart::open<DebugUart>({
    .baud_rate = 115200,
});
```

Important points:
- one public `open/configure/connect` path
- config structs carry defaults
- no separate "simple API"

## Migration Strategy

### Phase 1: Boundary and cleanup scaffold
- create the OpenSpec baseline
- introduce `src/device/*`
- import `alloy-devices` into the build
- add compile smoke coverage against the selected device contract

### Phase 2: Connector and claim kernel
- implement generic `connect()` and claim model
- stop centralizing vendor enums in the core

### Phase 3: Core drivers
- implement descriptor-driven `gpio`, `uart`, `spi`, `i2c`, `dma`
- remove overlapping API layers

### Phase 4: Startup and boards
- move startup algorithm to `src/arch`
- rebuild boards to be declarative
- move examples to the official path only

### Phase 5: Deletion
- remove legacy `hal/vendors/*` public runtime dependencies
- remove `simple/fluent/expert` split
- remove stale universal include indirection and obsolete board/platform CMake logic

## Gates

### Gate R1: Runtime boundary reset

Closed when:
- `alloy` has a stable `src/device` import layer for the selected device
- no new code depends on handwritten cross-vendor pin/signal enums in the runtime core

### Gate R2: Single public API

Closed when:
- `gpio`, `uart`, `spi`, `i2c`, and `dma` each expose one primary public API
- `simple`, `expert`, and `fluent` are not part of the active public architecture

### Gate R3: Descriptor-driven bring-up

Closed when:
- boards and examples bring up foundational targets without raw register paths
- `connect()` and claim logic work through generated descriptors

### Gate R4: Startup and build integration

Closed when:
- startup behavior is data-fed from `alloy-devices`
- CMake no longer hardcodes startup and device selection through large family switches

### Gate R5: Foundational runtime completeness

Closed when:
- foundational boards for `stm32g0`, `stm32f4`, and `same70` build and use the new runtime path
- the architecture is generic enough that adding more vendors is runtime-light rather than
  runtime-heavy

## Risks / Trade-offs

- Breaking a lot at once is risky
  - Mitigation: phase gates and foundational-family compile coverage
- Deleting compatibility layers may temporarily shrink supported surfaces
  - Mitigation: treat that as intentional until the new public path is stable
- Some current examples and boards will need full rewrites, not patches
  - Mitigation: prioritize the canonical path over breadth

## Open Questions

- Whether host support should be rebuilt on the same descriptor path or kept as a thinner adapter
- Whether the first post-reset board should be `nucleo_g071rb` or `same70_xplained`
