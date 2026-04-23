#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
MANIFEST_PATH = REPO_ROOT / "docs" / "RELEASE_MANIFEST.json"
SUPPORT_MATRIX_PATH = REPO_ROOT / "docs" / "SUPPORT_MATRIX.md"
RELEASE_DISCIPLINE_PATH = REPO_ROOT / "docs" / "RELEASE_DISCIPLINE.md"
RELEASE_CHECKLIST_PATH = REPO_ROOT / "docs" / "RELEASE_CHECKLIST.md"

ALLOWED_TIERS = {"foundational", "representative", "experimental", "deprecated"}
WORKFLOW_FILES = (
    REPO_ROOT / ".github" / "workflows" / "ci.yml",
    REPO_ROOT / ".github" / "workflows" / "build.yml",
)


def fail(message: str) -> int:
    print(f"Release discipline check failed: {message}")
    return 1


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_manifest() -> dict:
    return json.loads(read_text(MANIFEST_PATH))


def validate_manifest(manifest: dict) -> list[str]:
    errors: list[str] = []

    if manifest.get("schema_version") != 1:
        errors.append("schema_version must be 1")

    alloy_devices = manifest.get("alloy_devices", {})
    ref = alloy_devices.get("ref", "")
    if not re.fullmatch(r"[0-9a-f]{40}", ref):
        errors.append("alloy_devices.ref must be a 40-character lowercase git commit")
    if alloy_devices.get("policy") != "pinned-commit":
        errors.append("alloy_devices.policy must be 'pinned-commit'")

    tiers = manifest.get("support_tiers", [])
    if set(tiers) != ALLOWED_TIERS:
        errors.append("support_tiers must contain foundational, representative, experimental, deprecated")

    release_gates = manifest.get("release_gates", {})
    if not release_gates:
        errors.append("release_gates must not be empty")

    boards = manifest.get("boards", {})
    if not boards:
        errors.append("boards must not be empty")

    foundational_boards = []
    for board_name, board_data in boards.items():
        tier = board_data.get("tier")
        if tier not in ALLOWED_TIERS:
            errors.append(f"board {board_name} uses invalid tier {tier!r}")
        gates = board_data.get("required_gates", [])
        if not gates:
            errors.append(f"board {board_name} must declare required_gates")
        for gate in gates:
            if gate not in release_gates:
                errors.append(f"board {board_name} references unknown gate {gate!r}")
        examples = board_data.get("required_examples", [])
        if tier == "foundational":
            foundational_boards.append(board_name)
            if "descriptor-contract-smoke" not in gates:
                errors.append(f"foundational board {board_name} must require descriptor-contract-smoke")
            if not examples:
                errors.append(f"foundational board {board_name} must declare required_examples")

    if not foundational_boards:
        errors.append("at least one foundational board must be declared")

    peripheral_classes = manifest.get("peripheral_classes", {})
    if not peripheral_classes:
        errors.append("peripheral_classes must not be empty")

    for class_name, class_data in peripheral_classes.items():
        tier = class_data.get("tier")
        if tier not in ALLOWED_TIERS:
            errors.append(f"peripheral class {class_name} uses invalid tier {tier!r}")
        gates = class_data.get("required_gates", [])
        if not gates:
            errors.append(f"peripheral class {class_name} must declare required_gates")
        for gate in gates:
            if gate not in release_gates:
                errors.append(f"peripheral class {class_name} references unknown gate {gate!r}")

    return errors


def validate_docs(manifest: dict) -> list[str]:
    errors: list[str] = []

    support_matrix = read_text(SUPPORT_MATRIX_PATH)
    release_discipline = read_text(RELEASE_DISCIPLINE_PATH)
    release_checklist = read_text(RELEASE_CHECKLIST_PATH)

    for board_name in manifest["boards"].keys():
        if board_name not in support_matrix:
            errors.append(f"support matrix must mention board {board_name}")

    for class_name in manifest["peripheral_classes"].keys():
        if class_name not in support_matrix:
            errors.append(f"support matrix must mention peripheral class {class_name}")

    for required_text in (
        "RELEASE_MANIFEST.json",
        "descriptor contract smoke",
        "host-MMIO",
        "Renode runtime validation",
        "zero-overhead",
    ):
        if required_text not in release_checklist:
            errors.append(f"release checklist must mention {required_text}")

    for required_text in (
        "Compatibility Policy",
        "Release Tiers",
        "Release Gates",
        "Breaking Change Discipline",
    ):
        if required_text not in release_discipline:
            errors.append(f"release discipline doc must contain section {required_text}")

    return errors


def validate_workflow_refs(expected_ref: str) -> list[str]:
    errors: list[str] = []
    pattern = re.compile(r"ALLOY_DEVICES_REF:\s*([0-9a-f]{40})")

    for workflow in WORKFLOW_FILES:
        text = read_text(workflow)
        match = pattern.search(text)
        if match is None:
            errors.append(f"workflow {workflow.relative_to(REPO_ROOT)} must define ALLOY_DEVICES_REF")
            continue
        actual_ref = match.group(1)
        if actual_ref != expected_ref:
            errors.append(
                f"workflow {workflow.relative_to(REPO_ROOT)} pins ALLOY_DEVICES_REF={actual_ref}, expected {expected_ref}"
            )

    return errors


def main() -> int:
    manifest = parse_manifest()
    errors = [
        *validate_manifest(manifest),
        *validate_docs(manifest),
        *validate_workflow_refs(manifest["alloy_devices"]["ref"]),
    ]

    if errors:
        print("Release discipline violations found:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print("Release discipline check passed.")
    print(f"Validated manifest: {MANIFEST_PATH.relative_to(REPO_ROOT)}")
    print(f"Pinned alloy-devices ref: {manifest['alloy_devices']['ref']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())