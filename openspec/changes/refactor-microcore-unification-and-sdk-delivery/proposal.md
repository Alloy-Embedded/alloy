# Proposal: Refactor MicroCore Unification and SDK Delivery

**Change ID**: `refactor-microcore-unification-and-sdk-delivery`  
**Status**: Proposed  
**Priority**: High  
**Scope**: Framework identity, build consistency, board metadata, CLI reliability, SDK packaging, codegen product boundary

## Why

MicroCore already has strong technical foundations (HAL layers, concepts, code generation, multi-board support), but adoption friction is high because the project behaves like multiple partially merged products:

- Naming and namespace inconsistencies (`Alloy`, `MicroCore`, `ucore`) across code, docs, CMake variables, and macros.
- Build graph inconsistencies (different target names and legacy aliases) that make integration brittle.
- Board configuration split across multiple sources with drift risk.
- CLI behavior that is not fully deterministic for nested examples/targets.
- No clear SDK distribution contract (`install`/`find_package`) for external projects.

This change focuses on **productization and integration reliability** rather than introducing new peripherals.

## What Changes

1. Define canonical framework identity for new code paths (`MICROCORE_*`, `ucore::` namespace, `ucore` CLI) and introduce a controlled compatibility layer for legacy identifiers.
2. Normalize CMake target graph and export stable public targets for both in-tree and external consumption.
3. Establish board metadata as single source of truth and generate derived artifacts from it.
4. Make `ucore` deterministic for example/target resolution and board/example discovery.
5. Add SDK distribution contract (`install()`, exported CMake package, consumer-mode release profile).
6. Define and implement a codegen boundary to support eventual extraction to a dedicated repository.

## Codegen Repository Strategy

### Decision
Move code generation tooling toward a dedicated repository **in phases**, while keeping generated HAL artifacts consumable from the main framework repository.

### Rationale
- End users should not need Python/tooling internals to consume MicroCore.
- Generator contributors need faster independent iteration and release cadence.
- Splitting immediately without a compatibility boundary would create breakage.

### Migration Model
1. **Phase A (inside current repo)**: make `tools/codegen` a standalone product boundary with stable CLI/API and versioned artifact contract.
2. **Phase B (optional extraction)**: publish/maintain codegen in a separate repository, with pinned versions consumed by main repo CI/release flow.

## Impact

- Affected specs:
  - `project-structure` (modified via added requirements)
  - `board-support` (modified via added requirements)
  - `codegen-system` (modified via added requirements)
  - `testing-infrastructure` (modified via added requirements)
  - `cli-workflow` (new capability)
  - `sdk-distribution` (new capability)
- Affected code:
  - Root CMake and platform/board selection modules
  - CLI (`ucore`)
  - Board metadata and generators
  - Packaging/release/CI scripts
  - Documentation and migration guides

## Breaking Changes

No immediate hard breaks are required in this change. Legacy aliases/macros/targets remain available during migration with deprecation warnings and defined removal windows.
