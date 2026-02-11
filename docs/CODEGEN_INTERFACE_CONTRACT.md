# Codegen Interface Contract

This document defines the stable interface between framework runtime delivery and
board artifact generation.

## Contract Source of Truth

- Machine-readable contract: `tools/codegen/contracts/board_artifacts_contract.json`
- Generated compatibility manifest: `boards/generated/codegen_contract_manifest.json`
- Generated CMake metadata: `cmake/generated/board_metadata.cmake`

## Stable CLI Surface (Pinned)

The following commands are the supported codegen boundary for board artifacts:

```bash
python3 tools/codegen/scripts/generate_board_artifacts.py
python3 tools/codegen/scripts/generate_board_artifacts.py --check
python3 tools/codegen/scripts/validate_codegen_contract.py
```

## Input Contract

- Board manifests: `boards/*/board.yaml`
- Schema: `boards/board-schema.json`
- Framework version source: `CMakeLists.txt` (`project(microcore VERSION x.y.z)`)

## Output Contract

Required runtime artifacts:

- `boards/*/board_config.hpp`
- `boards/generated/board_catalog.json`
- `boards/generated/<board>/board_metadata.hpp`
- `boards/generated/codegen_contract_manifest.json`
- `cmake/generated/board_metadata.cmake`
- `cmake/generated/boards/<board>.cmake`

## Versioning Policy

- Contract version: semantic version in `board_artifacts_contract.json`.
- Generator version: pinned in contract and validated against runtime codegen
  version (`tools/codegen/core/version.py`).
- Framework compatibility policy: major+minor pinned.
  - Framework major must match contract `compatible_major`.
  - Framework minor must be within `[compatible_minor_min, compatible_minor_max]`.
  - Generated artifacts must match the current framework major/minor.

## Enforcement Points

- `tools/codegen/scripts/validate_codegen_contract.py` checks contract/manifest
  consistency and required artifact presence.
- Root CMake configure enforces artifact compatibility via
  `cmake/codegen_contract.cmake`.
- CI release path runs both:
  - `generate_board_artifacts.py --check`
  - `validate_codegen_contract.py`
