# Codegen Extraction Plan (`microcore-codegen`)

This plan describes how code generation can move to a dedicated repository
without breaking framework consumers.

## Objectives

- Keep framework consumer builds independent from Python/codegen runtime.
- Allow independent codegen release cadence.
- Preserve deterministic runtime artifact compatibility.

## Scope Boundary

- Runtime SDK deliverables:
  - headers/libraries/board metadata/generated board artifacts
- Codegen boundary artifacts:
  - contract files, compatibility validator, regeneration scripts
- Full generator internals remain contributor-side and are not required in
  consumer delivery profile.

## Phased Migration

1. Phase A (current repository)
   - Keep `tools/codegen` in-tree.
   - Enforce stable contract and artifact compatibility checks in CMake/CI.
   - Publish explicit contract and manifest.

2. Phase B (new repository: `microcore-codegen`)
   - Mirror contract files and scripts to new repository.
   - Tag first external codegen release aligned to current contract version.
   - Update framework contributor workflows to consume pinned external release.

3. Phase C (steady state)
   - Framework CI validates generated artifacts against pinned contract version.
   - Runtime SDK remains codegen-runtime-free.
   - Contract changes require coordinated version bump and migration note.

## Rollback Strategy

If incompatibility is detected:

1. Re-pin to previous known-good codegen contract version.
2. Restore matching generated artifacts (`boards/generated`, `cmake/generated`)
   from last green commit/release tag.
3. Re-run:
   - `python3 tools/codegen/scripts/generate_board_artifacts.py --check`
   - `python3 tools/codegen/scripts/validate_codegen_contract.py`
4. Block release until compatibility checks pass again.

## Release Gates

Extraction is allowed only if all are true:

- Contract file is versioned and validated in CI.
- Generated artifact compatibility check is green in CI and configure-time.
- Consumer profile package does not require codegen runtime tooling.
- Rollback steps are documented and tested on at least one release candidate.
