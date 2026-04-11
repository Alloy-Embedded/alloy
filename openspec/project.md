# Project Context

## Purpose
`alloy` is the runtime half of the Alloy ecosystem.

Its job is not to know raw MCU facts or regenerate device support. That work belongs to
`alloy-codegen` and `alloy-devices`.

`alloy` must consume the published device descriptors and provide:
- a clean, vendor-agnostic C++ runtime
- typed peripheral connection and ownership
- board bring-up and startup behavior
- a small, coherent public HAL API
- a scalable build model for many vendors and families

The long-term goal is to make Alloy the clearest C++ bare-metal runtime for multi-vendor MCU
projects without leaking vendor quirks into the public API.

## Tech Stack
- C++23 for the runtime and public API
- CMake for build and device selection
- `alloy-devices` as a versioned device-descriptor source
- GitHub Actions for CI validation and examples

## Project Conventions

### Code Style
- Prefer explicit, typed C++ over macro-heavy or enum-heavy indirection
- Keep public APIs small and coherent
- Avoid parallel API layers that solve the same problem differently
- Keep vendor-specific logic out of the public runtime surface

### Architecture Patterns
- Descriptor-driven runtime: generated device facts are consumed, not regenerated
- Runtime owns behavior; device repo owns hardware facts
- Drivers are written by peripheral class and capability, not by vendor namespace
- Boards are declarative and thin
- Startup behavior is architecture-driven and data-fed

### Testing Strategy
- Descriptor consumption must be covered by compile tests
- Board bring-up must be covered by focused smoke examples
- Foundational families must stay green before breadth expansion
- Public API changes require example and compile-coverage updates

### Git Workflow
- Architecture, boundary, API, or build model changes require OpenSpec first
- Large cleanup and code deletion require an explicit migration plan
- Multi-vendor scaling work must preserve the runtime/device boundary

## Domain Context
The old repo shape mixed:
- handwritten vendor HAL code
- generated device headers inside the runtime repo
- board bring-up logic
- multiple overlapping API layers
- build logic that hardcoded vendor/family choices in CMake

That model does not scale once `alloy-devices` becomes the source of truth.

The new runtime must scale across:
- ST
- Microchip
- NXP
- future vendors

without rewriting the public API or duplicating drivers per vendor.

## Important Constraints
- `alloy` must not become a second code generator
- `alloy` must not depend on vendor-specific public APIs for normal usage
- public HAL should expose one main API surface with defaults, not multiple overlapping tiers
- board code must not perform hardware side effects in headers
- startup algorithms belong in `alloy`; startup data belongs in `alloy-devices`
- the architecture should be allowed to delete incompatible legacy code when that is the simplest path

## External Dependencies
- `alloy-devices` as a submodule or equivalent checked-out descriptor tree
- architecture-specific compiler and linker toolchains
- host test tooling and compile smoke coverage
