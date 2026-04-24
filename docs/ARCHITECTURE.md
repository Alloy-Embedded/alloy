# Alloy Architecture

## Direction

`alloy` is being rebuilt around `alloy-devices`.

The split is:

- `alloy-devices`: generated hardware facts and typed descriptors
- `alloy`: runtime behavior, ownership, connection logic, drivers, startup algorithm, and boards

The runtime must scale by descriptor schema, not by vendor or family switches.

## Layers

- `src/device`
  Imports the selected device contract and exposes stable aliases for the runtime.
- `src/arch`
  Architecture-level startup and core support.
- `src/hal`
  Descriptor-driven connection, claim, and peripheral APIs.
- `boards/*`
  Board-level aliases and bring-up choices.

## Current Migration Rules

- New runtime code must consume generated descriptors from `alloy-devices`.
- New runtime code must not introduce family detection such as `is_stm32f4_family()`.
- New runtime code must not hardcode register offsets that already exist in `alloy-devices`.
- Legacy handwritten registries and vendor glue are being removed incrementally.

## Type-Driven Runtime Policy

- Performance-sensitive runtime paths in `src/hal`, `src/runtime`, and `src/device` must select behavior through types, enums, and templates.
- Public HAL routing, connector resolution, and backend register or field selection must not depend on textual matching such as `std::string_view`, suffix parsing, or name lookup tables.
- Text is acceptable only in explicitly text-oriented surfaces such as docs, logs, CLI output, diagnostics, OpenSpec metadata, and developer tooling under `scripts/`.
- Compile-time descriptor metadata may expose names for reporting or introspection, but runtime control flow must consume typed identifiers instead of comparing those names.
- If a backend needs schema-specific branching, encode that choice in descriptor traits or typed tags at compile time rather than vendor or family string switches.

## Key References

- [RUNTIME_DEVICE_BOUNDARY.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_DEVICE_BOUNDARY.md)
- [RUNTIME_CLEANUP_AUDIT.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_CLEANUP_AUDIT.md)
- [OpenSpec change: rebuild-runtime-around-alloy-devices](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/openspec/changes/rebuild-runtime-around-alloy-devices/proposal.md)
