## 1. Baseline and Safety Rails

- [x] 1.1 Build a baseline matrix for currently supported boards/examples through `ucore`.
- [x] 1.2 Add regression snapshots for current public CLI commands and CMake configure/build paths.
- [x] 1.3 Define deprecation policy document for legacy identifiers (`ALLOY_*`, legacy targets/macros).

## 2. Canonical Identity Unification

- [x] 2.1 Define canonical naming table (namespace, CMake variables, compile definitions, targets, docs terms).
- [x] 2.2 Implement compatibility aliases for legacy macros/variables and emit deprecation diagnostics.
- [x] 2.3 Normalize runtime namespace references in board/RTOS integration paths to canonical namespace.
- [x] 2.4 Update high-traffic docs (README, quickstart, architecture, migration) to canonical naming.

## 3. Build Graph and Public Target Contract

- [x] 3.1 Normalize internal target naming strategy and remove ambiguous duplicates where safe.
- [x] 3.2 Expose stable public targets (`microcore::hal`, `microcore::board::<board>`, `microcore::rtos` as applicable).
- [x] 3.3 Add compatibility aliases from legacy target names to public targets during migration window.
- [x] 3.4 Add deterministic example-target mapping table to avoid path-based heuristics.

## 4. Board Metadata Single Source of Truth

- [x] 4.1 Define/lock board metadata schema for board identity, pins, clocks, transport, flash/debug tooling.
- [x] 4.2 Generate board-derived artifacts (board config/header fragments and CMake fragments) from metadata.
- [x] 4.3 Ensure generated artifacts match metadata semantics through schema and consistency checks.
- [x] 4.4 Remove or isolate manual board config drift sources.

## 5. CLI Reliability and UX Contract

- [x] 5.1 Refactor `ucore build` target resolution to metadata-driven explicit mappings.
- [x] 5.2 Ensure nested examples resolve to canonical targets.
- [x] 5.3 Improve CLI errors with actionable alternatives (`did you mean`, list valid board/example/target).
- [x] 5.4 Add non-interactive safe mode for CI usage (no blocking prompts unless explicitly requested).
- [x] 5.5 Add CLI smoke tests for all supported board/example combinations in CI.

## 6. SDK Distribution and Consumer Mode

- [x] 6.1 Implement CMake `install()` rules and export package config for `find_package(microcore CONFIG REQUIRED)`.
- [x] 6.2 Publish and validate consumer mode profile that does not require Python/codegen execution.
- [x] 6.3 Add minimal external-consumer example repository/workspace test in CI.
- [x] 6.4 Document integration paths: `add_subdirectory`, `FetchContent`, `find_package`.

## 7. Codegen Boundary and Extraction Readiness

- [x] 7.1 Define stable codegen interface contract (CLI surface, input schema, artifact outputs, versioning).
- [x] 7.2 Separate generator internals from framework runtime deliverables in release layout.
- [x] 7.3 Add version pinning and compatibility checks between generated artifacts and framework version.
- [x] 7.4 Document extraction plan to dedicated repository (`microcore-codegen`) with rollback strategy.
- [x] 7.5 Evaluate extraction gate criteria and decide go/no-go for follow-up split.

## 8. Validation and Exit Criteria

- [x] 8.1 All baseline matrix builds pass with canonical identifiers enabled.
- [x] 8.2 Legacy compatibility path passes with deprecation warnings and no hard breaks.
- [x] 8.3 CLI smoke matrix passes in CI for supported boards/examples.
- [x] 8.4 External consumer integration test passes with installed package.
- [x] 8.5 Migration guide is published with timeline for legacy identifier removal.
