#!/usr/bin/env python3
"""Configure-only smoke test for the ALLOY_BOARD=custom path.

Runs a synthetic external CMake project (tests/custom_board/) against the alloy
runtime checkout. Exercises the positive case (all required cache variables set,
descriptor present) and a representative negative case (missing required variable
must abort with a one-line FATAL_ERROR naming the variable).

The test is configure-only: it does not build a firmware image, so it does not
require a cross-toolchain. ALLOY_DEVICES_ROOT must point at an alloy-devices
checkout that contains the descriptor for the device tuple used below.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
PROJECT_DIR = REPO_ROOT / "tests" / "custom_board"


def run(command: list[str], *, expect_fail: bool = False) -> subprocess.CompletedProcess:
    print("+", " ".join(command))
    proc = subprocess.run(command, cwd=REPO_ROOT, capture_output=True, text=True)
    sys.stdout.write(proc.stdout)
    sys.stderr.write(proc.stderr)
    if expect_fail and proc.returncode == 0:
        raise SystemExit("expected configure to fail but it succeeded")
    if not expect_fail and proc.returncode != 0:
        raise SystemExit(f"configure failed unexpectedly (exit {proc.returncode})")
    return proc


def configure_args(build_dir: Path, devices_root: Path, *, extra: list[str] | None = None) -> list[str]:
    args = [
        "cmake",
        "-S",
        str(PROJECT_DIR),
        "-B",
        str(build_dir),
        "-G",
        "Ninja",
        f"-DALLOY_ROOT={REPO_ROOT}",
        f"-DALLOY_DEVICES_ROOT={devices_root}",
    ]
    if extra:
        args.extend(extra)
    return args


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--devices-root",
        type=Path,
        default=REPO_ROOT.parent / "alloy-devices",
        help="path to an alloy-devices checkout (default: ../alloy-devices)",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=REPO_ROOT / "build" / "custom-board-smoke",
        help="build dir for the positive configure run",
    )
    args = parser.parse_args()

    devices_root = args.devices_root.resolve()
    if not devices_root.is_dir():
        raise SystemExit(
            f"alloy-devices checkout not found at {devices_root}; "
            f"clone Alloy-Embedded/alloy-devices or pass --devices-root."
        )

    pos_build = args.build_dir
    neg_build = args.build_dir.parent / (args.build_dir.name + "-neg")
    for path in (pos_build, neg_build):
        if path.exists():
            shutil.rmtree(path)

    print("=== positive: all required variables set, descriptor present ===")
    run(configure_args(pos_build, devices_root))

    print("=== negative: missing ALLOY_CUSTOM_BOARD_HEADER must abort ===")
    proc = run(
        configure_args(
            neg_build,
            devices_root,
            extra=["-UALLOY_CUSTOM_BOARD_HEADER", "-DALLOY_CUSTOM_BOARD_HEADER="],
        ),
        expect_fail=True,
    )
    if "ALLOY_CUSTOM_BOARD_HEADER" not in (proc.stderr + proc.stdout):
        raise SystemExit(
            "negative test passed for the wrong reason: "
            "expected diagnostic to name ALLOY_CUSTOM_BOARD_HEADER"
        )

    print("custom-board smoke check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
