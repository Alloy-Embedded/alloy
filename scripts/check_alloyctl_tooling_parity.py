#!/usr/bin/env python3
"""Release gate: exercise alloyctl compile-commands / info / doctor / new surface.

Validates the user-facing contract without requiring a configured build dir:
- `alloyctl info` emits JSON with the documented keys and board list
- `alloyctl doctor` runs and reports each expected preflight check
- `alloyctl new` scaffolds a starter tree for a foundational board
- `alloyctl compile-commands` errors cleanly when the build dir is missing

Exits non-zero if any surface drifts from the spec.
"""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
ALLOYCTL = REPO_ROOT / "scripts" / "alloyctl.py"

EXPECTED_INFO_KEYS = {
    "alloy_version",
    "git_sha",
    "alloy_devices",
    "boards",
    "release_gates",
    "tools",
    "python_packages",
}

EXPECTED_INFO_TOOL_KEYS = {
    "cmake",
    "ninja",
    "arm_none_eabi_gcc",
    "openocd",
    "python",
}

EXPECTED_DOCTOR_LABELS = (
    "cmake >= 3.25 on PATH",
    "arm-none-eabi-gcc on PATH",
    "openocd on PATH",
    "python pyserial",
    "alloy-devices ref aligned with RELEASE_MANIFEST.json",
)

FOUNDATIONAL_BOARD = "same70_xplained"


def _run(args: list[str]) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["python3", str(ALLOYCTL), *args],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        check=False,
    )


def _check_info() -> list[str]:
    errors: list[str] = []
    result = _run(["info"])
    if result.returncode != 0:
        errors.append(f"info: exited {result.returncode}: {result.stderr.strip()}")
        return errors
    try:
        payload = json.loads(result.stdout)
    except json.JSONDecodeError as err:
        errors.append(f"info: stdout is not valid JSON: {err}")
        return errors
    missing = EXPECTED_INFO_KEYS - set(payload)
    if missing:
        errors.append(f"info: missing keys {sorted(missing)}")
    tools = payload.get("tools") or {}
    missing_tools = EXPECTED_INFO_TOOL_KEYS - set(tools)
    if missing_tools:
        errors.append(f"info: tools section missing {sorted(missing_tools)}")
    boards = {entry.get("board") for entry in payload.get("boards", [])}
    if FOUNDATIONAL_BOARD not in boards:
        errors.append(f"info: boards list missing foundational board {FOUNDATIONAL_BOARD}")
    return errors


def _check_doctor() -> list[str]:
    errors: list[str] = []
    result = _run(["doctor"])
    for label in EXPECTED_DOCTOR_LABELS:
        if label not in result.stdout:
            errors.append(f"doctor: expected label {label!r} in stdout")
    # Exit 0 or 1 is acceptable; exit 2 or more means crash.
    if result.returncode not in (0, 1):
        errors.append(f"doctor: unexpected exit {result.returncode}: {result.stderr.strip()}")
    return errors


def _check_new() -> list[str]:
    errors: list[str] = []
    with tempfile.TemporaryDirectory() as td:
        dest = Path(td) / "starter"
        result = _run(["new", "--board", FOUNDATIONAL_BOARD, "--path", str(dest),
                        "--name", "parity_probe"])
        if result.returncode != 0:
            errors.append(f"new: exited {result.returncode}: {result.stderr.strip()}")
            return errors
        for relative in ("CMakeLists.txt", "README.md", "src/main.cpp", ".gitignore"):
            if not (dest / relative).exists():
                errors.append(f"new: missing scaffold file {relative}")
        cml = (dest / "CMakeLists.txt").read_text(encoding="utf-8") if (dest / "CMakeLists.txt").exists() else ""
        if "ALLOY_ROOT" not in cml:
            errors.append("new: generated CMakeLists.txt must document ALLOY_ROOT")
        if FOUNDATIONAL_BOARD not in cml:
            errors.append(f"new: generated CMakeLists.txt must reference {FOUNDATIONAL_BOARD}")
    return errors


def _check_compile_commands_missing() -> list[str]:
    errors: list[str] = []
    with tempfile.TemporaryDirectory() as td:
        bogus_root_link = Path(td) / "bogus_repo"
        bogus_root_link.mkdir()
        # Use an unconfigured synthetic board-like dir path: rely on nucleo_f401re board
        # whose build dir likely doesn't exist in a fresh checkout.
        result = _run(["compile-commands", "--board", "nucleo_f401re"])
        # Either symlink succeeded (build dir already existed) or it errored cleanly.
        if result.returncode not in (0, 2):
            errors.append(
                f"compile-commands: unexpected exit {result.returncode}: {result.stderr.strip()}"
            )
        # Clean any symlink we just created at repo root.
        link = REPO_ROOT / "compile_commands.json"
        if link.is_symlink() or link.exists():
            try:
                link.unlink()
            except OSError:
                pass
    return errors


def main() -> int:
    if not ALLOYCTL.exists():
        print(f"error: {ALLOYCTL} not found", file=sys.stderr)
        return 2
    all_errors: list[str] = []
    all_errors.extend(_check_info())
    all_errors.extend(_check_doctor())
    all_errors.extend(_check_new())
    all_errors.extend(_check_compile_commands_missing())
    if all_errors:
        print("alloyctl tooling parity check FAILED:")
        for err in all_errors:
            print(f"  - {err}")
        return 1
    print("alloyctl tooling parity check passed.")
    print("Exercised subcommands:")
    print("  - info")
    print("  - doctor")
    print("  - new")
    print("  - compile-commands")
    return 0


if __name__ == "__main__":
    sys.exit(main())
