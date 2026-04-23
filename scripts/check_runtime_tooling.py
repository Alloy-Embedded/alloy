#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
PRESETS_PATH = REPO_ROOT / "CMakePresets.json"
ALLOYCTL_PATH = REPO_ROOT / "scripts" / "alloyctl.py"

REQUIRED_DOCS = (
    REPO_ROOT / "docs" / "QUICKSTART.md",
    REPO_ROOT / "docs" / "BOARD_TOOLING.md",
    REPO_ROOT / "docs" / "CMAKE_CONSUMPTION.md",
    REPO_ROOT / "boards" / "same70_xplained" / "README.md",
    REPO_ROOT / "boards" / "nucleo_g071rb" / "README.md",
    REPO_ROOT / "boards" / "nucleo_f401re" / "README.md",
)

REQUIRED_PATHS = (
    REPO_ROOT / "scripts" / "check_downstream_package.py",
    REPO_ROOT / "tests" / "downstream_package" / "CMakeLists.txt",
    REPO_ROOT / "tests" / "downstream_package" / "main.cpp",
)

REQUIRED_WORKFLOW_PRESETS = {
    "host-mmio-validation",
    "same70-runtime-validation",
    "same70-renode-smoke",
    "same70-zero-overhead",
    "stm32g0-runtime-validation",
    "stm32g0-renode-smoke",
    "stm32f4-runtime-validation",
    "stm32f4-renode-smoke",
}

REQUIRED_BUILD_PRESETS = {
    "host-mmio-validation",
    "same70-runtime-validation",
    "same70-zero-overhead",
    "stm32g0-runtime-validation",
    "stm32f4-runtime-validation",
}

REQUIRED_ALLOYCTL_COMMANDS = (
    "configure",
    "build",
    "bundle",
    "flash",
    "monitor",
    "gdbserver",
    "validate",
    "sweep",
)


def fail(message: str) -> int:
    print(f"Runtime tooling check failed: {message}")
    return 1


def load_presets() -> dict:
    return json.loads(PRESETS_PATH.read_text(encoding="utf-8"))


def run_help(*args: str) -> tuple[int, str]:
    completed = subprocess.run(
        [sys.executable, str(ALLOYCTL_PATH), *args],
        cwd=REPO_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    return completed.returncode, completed.stdout


def main() -> int:
    errors: list[str] = []

    for path in REQUIRED_DOCS:
        if not path.exists():
            errors.append(f"missing required tooling doc: {path.relative_to(REPO_ROOT)}")

    for path in REQUIRED_PATHS:
        if not path.exists():
            errors.append(f"missing required tooling path: {path.relative_to(REPO_ROOT)}")

    presets = load_presets()
    workflow_presets = {item["name"] for item in presets.get("workflowPresets", [])}
    build_presets = {item["name"] for item in presets.get("buildPresets", [])}

    for preset in sorted(REQUIRED_WORKFLOW_PRESETS - workflow_presets):
        errors.append(f"missing required workflow preset: {preset}")

    for preset in sorted(REQUIRED_BUILD_PRESETS - build_presets):
        errors.append(f"missing required build preset: {preset}")

    rc, help_output = run_help("--help")
    if rc != 0:
        errors.append("alloyctl --help failed")
    else:
        for command in REQUIRED_ALLOYCTL_COMMANDS:
            if command not in help_output:
                errors.append(f"alloyctl --help does not list command: {command}")

    for command in REQUIRED_ALLOYCTL_COMMANDS:
        rc, _ = run_help(command, "--help")
        if rc != 0:
            errors.append(f"alloyctl {command} --help failed")

    if errors:
        print("Runtime tooling violations found:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print("Runtime tooling check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())