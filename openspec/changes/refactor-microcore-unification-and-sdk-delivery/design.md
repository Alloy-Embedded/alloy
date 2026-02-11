# Design: MicroCore Unification and SDK Delivery

## Context

The repository currently contains production-quality components mixed with legacy naming/models from earlier architecture phases. This causes integration failures and erodes confidence for external adopters.

The design goal is to establish a stable public contract while preserving migration safety for existing code.

## Goals / Non-Goals

- Goals:
  - Define canonical public identifiers and build contracts.
  - Guarantee deterministic CLI behavior for supported workflows.
  - Make board configuration data-driven with one source of truth.
  - Provide installable/consumable SDK package for external projects.
  - Establish codegen extraction boundary for future repository split.
- Non-Goals:
  - Rewriting all HAL/peripheral implementations.
  - Introducing new peripheral families as primary objective.
  - Immediate removal of all legacy aliases in this change.

## Decisions

### D1. Canonical Identity Matrix

- Canonical preprocessor/config prefix: `MICROCORE_`.
- Canonical namespace root: `ucore`.
- Canonical CLI: `ucore`.
- Legacy compatibility:
  - Keep `ALLOY_*` macros/variables as compatibility aliases for defined deprecation window.
  - Keep legacy target aliases where needed.
  - Emit deprecation diagnostics when legacy identifiers are used in supported paths.

### D2. Build Graph Contract

Define stable exported targets for consumers:

- `microcore::hal`
- `microcore::board::<board>`
- `microcore::rtos` (when enabled)

Implementation internals may keep current target names, but public targets must resolve consistently and be exportable by `install(EXPORT ...)`.

### D3. Board Metadata Source of Truth

Board metadata SHALL be canonical in board manifest files (`board.yaml`/schema-governed format). Generated derivatives (`board_config.hpp`, CMake fragments, CLI board catalogs) are generated artifacts and must not diverge semantically from metadata.

### D4. CLI Target Resolution

CLI must resolve build target by explicit mapping, not by path heuristics:

- Each example has canonical target metadata.
- Nested examples map to explicit targets.
- Missing mappings return actionable errors with valid alternatives.

### D5. SDK Distribution Profiles

Introduce two release/consumption profiles:

1. **Consumer Mode**:
   - No Python/codegen execution required.
   - Uses pre-generated HAL artifacts.
   - Supports `find_package(microcore CONFIG REQUIRED)`.
2. **Contributor Mode**:
   - Includes codegen/dev dependencies and regeneration pipeline.

### D6. Codegen Extraction Boundary

Split feasibility is accepted, but extraction is phased:

- Phase A (this program): formalize generator interfaces, artifact contract, version pinning policy, and CI handoff points.
- Phase B (optional follow-up): move generator implementation to dedicated repo (`microcore-codegen`), keep framework consuming released artifacts by pinned version.

This avoids forcing end users to manage generator internals while enabling independent tool evolution.

## Risks / Trade-offs

- Risk: compatibility layer prolongs legacy usage.
  - Mitigation: deprecation deadlines, CI warnings, migration docs.
- Risk: board metadata migration can introduce silent mismatches.
  - Mitigation: generated artifact validation and consistency checks in CI.
- Risk: package/export changes can break current local workflows.
  - Mitigation: maintain in-tree build flow and add compatibility aliases during transition.

## Migration Plan

1. Introduce canonical identifiers and compatibility aliases.
2. Normalize build targets and export public SDK targets.
3. Move board consumption paths to metadata-generated artifacts.
4. Update CLI resolution and add smoke tests for all supported examples.
5. Add `install`/`find_package` path and publish consumer-mode release profile.
6. Implement codegen boundary and document extraction gate criteria.

## Extraction Gate (Codegen Repo Split)

Codegen can be moved to a separate repo only after:

- Stable generator CLI/API contract is versioned.
- Artifact compatibility matrix is tested in CI.
- Consumer mode release builds require no generator runtime.
- Rollback path exists (pin previous generator artifacts).
