## Context

Top embedded libraries win on two axes:

- technical architecture
- developer experience

`alloy` is now becoming credible on architecture, but users still need too much implicit project
knowledge.

The runtime already has good generated diagnostics available from the contract:

- connectors
- clock profiles
- capabilities
- board selection

This change turns that into a product-level user experience.

## Goals

- stable board-oriented build/flash/debug flows
- actionable diagnostics for invalid configuration
- downstream-consumable CMake packaging
- explainable resource/clock/target differences
- canonical examples for the public HAL surface
- explicit support matrix and docs
- migration-friendly user guides

## Non-Goals

- replacing CMake with a separate monolithic CLI
- turning `alloy` into a second generator
- hiding all advanced details from expert users

## Decision 1: Board-Oriented Entry Points Must Be Stable

Users SHALL have stable, documented ways to:

- configure a board build
- build examples and smoke targets
- flash when supported
- run validation where available

These entry points may be implemented through:

- CMake presets
- named CMake targets
- scripts

but they must be documented as one supported story.

## Decision 2: Diagnostics Must Use The Generated Contract

Connector, clock, and ownership diagnostics SHALL be surfaced from the generated runtime contract
rather than rebuilt as handwritten guesswork.

This keeps the user-facing story aligned with the actual source of truth.

The preferred outcome is a user-facing experience that can answer:

- why a connector or clock choice is invalid
- what valid alternatives exist
- where the underlying fact came from when provenance is available

This may be exposed through compiler diagnostics, scripts, or future CLI entry points such as
`alloy explain` and `alloy diff`.

## Decision 3: CMake Consumption Must Become First-Class

`alloy` SHALL support a documented downstream CMake integration story.

At minimum, the repo should converge toward a supported package-style flow so an application can
consume the runtime without reverse-engineering internal target names.

## Decision 4: Examples Must Be Product Documentation

Examples SHALL function as the canonical usage documentation for the public HAL.

Each major peripheral class should have:

- a minimal example
- a representative board target
- a short explanation of the official runtime path

## Decision 5: Support Status Must Be Explicit

The repo SHALL publish support status explicitly for:

- boards
- peripheral classes
- validation coverage

The user should not need to infer support from source layout.

## Decision 6: Migration Guides Matter

To displace vendor and legacy libraries, `alloy` SHALL explain how its model maps from:

- STM32Cube HAL/LL style
- `modm` style
- register-level libraries such as `libopencm3`

## Validation

At minimum this change must prove:

- documented presets and commands actually work
- examples build for their claimed boards
- diagnostics are exercised in tests
- downstream CMake integration is documented and tested if claimed
- support matrix docs match reality
