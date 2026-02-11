#!/usr/bin/env python3
"""
Validate compatibility between generated board artifacts and framework version.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any


PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_CONTRACT = PROJECT_ROOT / "tools" / "codegen" / "contracts" / "board_artifacts_contract.json"
DEFAULT_MANIFEST = PROJECT_ROOT / "boards" / "generated" / "codegen_contract_manifest.json"
DEFAULT_CATALOG = PROJECT_ROOT / "boards" / "generated" / "board_catalog.json"
DEFAULT_CMAKE_METADATA = PROJECT_ROOT / "cmake" / "generated" / "board_metadata.cmake"
DEFAULT_CMAKE_LISTS = PROJECT_ROOT / "CMakeLists.txt"


def _read_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        payload = json.load(handle)
    if not isinstance(payload, dict):
        raise ValueError(f"Expected JSON object at {path}")
    return payload


def _parse_framework_version(cmake_lists: Path) -> tuple[str, int, int, int]:
    content = cmake_lists.read_text(encoding="utf-8")
    match = re.search(r"project\(\s*microcore[\s\S]*?VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)", content)
    if not match:
        raise ValueError("Unable to parse framework version from CMakeLists.txt")

    version = match.group(1)
    major, minor, patch = (int(part) for part in version.split("."))
    return version, major, minor, patch


def _ensure(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def _validate_runtime_artifacts(
    contract: dict[str, Any],
    boards: list[str],
    errors: list[str],
) -> None:
    patterns = contract["outputs"]["runtime_artifacts"]
    for pattern in patterns:
        if "<board>" in pattern:
            for board in boards:
                resolved = PROJECT_ROOT / pattern.replace("<board>", board)
                _ensure(resolved.exists(), f"Missing board artifact: {resolved}", errors)
            continue

        if "*" in pattern:
            matches = list(PROJECT_ROOT.glob(pattern))
            _ensure(bool(matches), f"No files match artifact glob: {pattern}", errors)
            continue

        resolved = PROJECT_ROOT / pattern
        _ensure(resolved.exists(), f"Missing artifact: {resolved}", errors)


def _validate_cmake_metadata(
    cmake_metadata: Path,
    manifest: dict[str, Any],
    errors: list[str],
) -> None:
    content = cmake_metadata.read_text(encoding="utf-8")

    expected_pairs = {
        "MICROCORE_GENERATED_CODEGEN_CONTRACT_ID": manifest["contract_id"],
        "MICROCORE_GENERATED_CODEGEN_CONTRACT_VERSION": manifest["contract_version"],
        "MICROCORE_GENERATED_CODEGEN_VERSION": manifest["generator"]["version"],
        "MICROCORE_GENERATED_BOARD_SCHEMA_VERSION": manifest["board_schema_version"],
        "MICROCORE_GENERATED_FRAMEWORK_VERSION": manifest["framework_version"]["value"],
        "MICROCORE_GENERATED_FRAMEWORK_VERSION_MAJOR": str(manifest["framework_version"]["major"]),
        "MICROCORE_GENERATED_FRAMEWORK_VERSION_MINOR": str(manifest["framework_version"]["minor"]),
        "MICROCORE_GENERATED_FRAMEWORK_VERSION_PATCH": str(manifest["framework_version"]["patch"]),
    }

    for key, value in expected_pairs.items():
        pattern = f'set({key} "{value}")'
        _ensure(pattern in content, f"Missing CMake compatibility variable: {pattern}", errors)


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate generated codegen contract compatibility")
    parser.add_argument("--contract", type=Path, default=DEFAULT_CONTRACT)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--cmake-metadata", type=Path, default=DEFAULT_CMAKE_METADATA)
    parser.add_argument("--cmake-lists", type=Path, default=DEFAULT_CMAKE_LISTS)
    args = parser.parse_args()

    errors: list[str] = []

    try:
        contract = _read_json(args.contract)
        manifest = _read_json(args.manifest)
        catalog = _read_json(args.catalog)
        framework_version, fw_major, fw_minor, fw_patch = _parse_framework_version(args.cmake_lists)
    except Exception as exc:
        print(f"[error] Failed to load contract artifacts: {exc}")
        return 1

    # Contract identity and pinning
    _ensure(manifest["contract_id"] == contract["contract_id"], "Contract ID mismatch", errors)
    _ensure(
        manifest["contract_version"] == contract["contract_version"],
        "Contract version mismatch between source and manifest",
        errors,
    )
    _ensure(
        manifest["generator"]["version"] == contract["generator"]["version"],
        "Generator version mismatch between source contract and manifest",
        errors,
    )

    # Framework version pinning
    manifest_framework = manifest["framework_version"]
    _ensure(manifest_framework["value"] == framework_version, "Manifest/framework version mismatch", errors)
    _ensure(manifest_framework["major"] == fw_major, "Manifest/framework major mismatch", errors)
    _ensure(manifest_framework["minor"] == fw_minor, "Manifest/framework minor mismatch", errors)
    _ensure(manifest_framework["patch"] == fw_patch, "Manifest/framework patch mismatch", errors)

    # Compatibility range policy
    compat = contract["framework_compatibility"]
    _ensure(fw_major == compat["compatible_major"], "Framework major not compatible with contract", errors)
    _ensure(
        compat["compatible_minor_min"] <= fw_minor <= compat["compatible_minor_max"],
        "Framework minor not compatible with contract range",
        errors,
    )

    # Catalog linkage
    artifact_contract = catalog.get("artifact_contract", {})
    _ensure(
        artifact_contract.get("contract_version") == contract["contract_version"],
        "Board catalog contract version mismatch",
        errors,
    )
    _ensure(
        artifact_contract.get("framework_version") == framework_version,
        "Board catalog framework version mismatch",
        errors,
    )
    _ensure(
        artifact_contract.get("board_schema_version") == manifest["board_schema_version"],
        "Board catalog board schema version mismatch",
        errors,
    )

    # Runtime artifact coverage
    boards = manifest.get("boards", [])
    _ensure(bool(boards), "Manifest board list is empty", errors)
    _validate_runtime_artifacts(contract=contract, boards=boards, errors=errors)

    # Generated CMake metadata consistency
    _validate_cmake_metadata(args.cmake_metadata, manifest, errors)

    if errors:
        print("Codegen compatibility validation: FAILED")
        for issue in errors:
            print(f"  - {issue}")
        return 1

    print("Codegen compatibility validation: OK")
    print(f"  Contract: {contract['contract_id']}@{contract['contract_version']}")
    print(f"  Framework: {framework_version}")
    print(f"  Boards: {len(boards)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
