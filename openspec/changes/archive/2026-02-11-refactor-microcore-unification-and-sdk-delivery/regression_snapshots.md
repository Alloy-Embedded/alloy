# Regression Snapshots (CLI + CMake)

Date: 2026-02-10

## Snapshot Verifier

`scripts/verify_cli_cmake_snapshots.sh`

The verifier covers:

- Public CLI surface snapshots:
  - `./ucore --help`
  - `./ucore list boards`
  - `./ucore list examples`
- Canonical CMake configure/build path:
  - Configure with `MICROCORE_BOARD`
  - Build `blink`
  - Assert cache values (`MICROCORE_BOARD`, `MICROCORE_PLATFORM`)
- Legacy compatibility path:
  - Configure with `ALLOY_BOARD`
  - Assert deprecation diagnostics
  - Assert canonical cache resolution

## Expected Stability Contract

- Snapshot checks are token/marker based (not full raw stdout equality).
- ANSI colors are stripped before checks.
- Dynamic data (paths/tool versions) are intentionally not snapshotted.
