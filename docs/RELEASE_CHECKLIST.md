# Alloy Release Checklist

Use this checklist before cutting a release or tagging a version that claims production readiness for the active foundational runtime path.

## Compatibility

- confirm the `alloy-devices` compatibility pin in [docs/RELEASE_MANIFEST.json](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_MANIFEST.json)
- confirm the same ref is used in CI workflows that validate runtime behavior
- if the `alloy-devices` pin changed, document the reason and rerun the full foundational release ladder

## Foundational Gates

- pass `python3 scripts/check_runtime_device_boundary.py`
- pass `python3 scripts/check_release_discipline.py`
- pass descriptor contract smoke for:
  - `same70_xplained`
  - `nucleo_g071rb`
  - `nucleo_f401re`
- pass host-MMIO validation for:
  - `same70_xplained`
  - `nucleo_g071rb`
  - `nucleo_f401re`
- pass Renode runtime validation for:
  - `same70`
  - `stm32g0`
  - `stm32f4`
- pass the SAME70 zero-overhead gate before making any zero-overhead release claim

## Example And Build Coverage

- verify the required foundational examples declared in [docs/RELEASE_MANIFEST.json](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_MANIFEST.json)
- confirm canonical examples still match the public runtime shape:
  - `time_probe`
  - `dma_probe`
- do not claim a board as foundational if its required example coverage is failing or intentionally skipped

## Support Matrix

- update [docs/SUPPORT_MATRIX.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/SUPPORT_MATRIX.md) if board or peripheral tier claims changed
- verify the support matrix still matches the machine-readable release manifest
- downgrade claims instead of hand-waving around missing coverage

## Breaking Changes

- if the change breaks the public HAL shape, build/selection story, runtime/device contract expectations, or board migration path:
  - add migration notes
  - update at least one canonical example
  - call out the break in the changelog or release notes

## Release Notes

- describe the validated `alloy-devices` compatibility pin or policy
- call out any downgraded support tiers
- avoid claiming production support for paths that are only `representative` or `experimental`
- check that `CHANGELOG.md` at the repo root has a section matching the version in
  `CMakeLists.txt` — enforced by the `changelog-present` release gate
  (`scripts/check_changelog_present.py`)
- add `docs/RELEASE_NOTES_vX.Y.Z.md` as the canonical release-body text and reference it from
  the GitHub release draft
- run the docs site gate (`scripts/check_docs_site.py`) and the tooling-parity gate
  (`scripts/check_alloyctl_tooling_parity.py`) before tagging