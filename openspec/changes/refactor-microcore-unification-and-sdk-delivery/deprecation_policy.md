# Deprecation Policy (Legacy Identifiers)

This policy defines how legacy `ALLOY_*` identifiers are handled while migrating to `MICROCORE_*`.

## Scope

- CMake variables (`ALLOY_BOARD`, `ALLOY_PLATFORM`, `ALLOY_BUILD_TESTS`, `ALLOY_MINIMAL_BUILD`)
- Compile definitions (`ALLOY_BOARD_*`, `ALLOY_PLATFORM_*`, `ALLOY_RTOS_ENABLED`)
- CMake helper variables (`ALLOY_HAL_LIBRARY`, `ALLOY_PLATFORM_DIR`, `ALLOY_BOARD_DIR`)

## Rules

1. Canonical names are `MICROCORE_*` and `UCORE_*`.
2. Legacy `ALLOY_*` inputs continue to work during migration.
3. Using legacy CMake inputs emits deprecation diagnostics at configure time.
4. If both legacy and canonical CMake variables are provided with different values, configuration fails fast with an actionable error.
5. Compatibility aliases remain until migration exit criteria are reached in this OpenSpec.

## Diagnostics Contract

- Deprecated legacy variable use:
  - `message(DEPRECATION "... ALLOY_* is deprecated; use MICROCORE_*")`
- Conflicting values:
  - `FATAL_ERROR` with both values and correction guidance

## Removal Timeline

- Phase A (current): compatibility + diagnostics enabled.
- Phase B (after migration guide publication and CI green on canonical path): disable legacy inputs in docs and examples.
- Phase C (final): remove `ALLOY_*` CMake input handling and legacy compile-definition emission.

## Consumer Guidance

- Prefer:
  - `-DMICROCORE_BOARD=<board>`
  - `-DMICROCORE_PLATFORM=<platform>` (or omit for auto-detect)
  - `-DMICROCORE_BUILD_TESTS=ON|OFF`
- Avoid:
  - `-DALLOY_*` configure inputs in new scripts and CI.
