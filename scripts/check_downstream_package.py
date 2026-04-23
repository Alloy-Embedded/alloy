#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def run(command: list[str]) -> None:
    print("+", " ".join(command))
    subprocess.run(command, cwd=REPO_ROOT, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Alloy build-tree CMake package consumption")
    parser.add_argument("--alloy-build-dir", default="build/presets/host-validation-debug")
    parser.add_argument("--downstream-build-dir", default="build/downstream-package-smoke")
    parser.add_argument("--generator", default="Ninja")
    args = parser.parse_args()

    alloy_build_dir = (REPO_ROOT / args.alloy_build_dir).resolve()
    package_dir = alloy_build_dir / "generated" / "cmake"
    downstream_build_dir = (REPO_ROOT / args.downstream_build_dir).resolve()
    downstream_source_dir = REPO_ROOT / "tests" / "downstream_package"

    if not (package_dir / "AlloyConfig.cmake").exists():
        raise SystemExit(f"missing Alloy package config: {package_dir / 'AlloyConfig.cmake'}")

    if downstream_build_dir.exists():
        shutil.rmtree(downstream_build_dir)

    run(
        [
            "cmake",
            "-S",
            str(downstream_source_dir),
            "-B",
            str(downstream_build_dir),
            "-G",
            args.generator,
            f"-DAlloy_DIR={package_dir}",
        ]
    )
    run(["cmake", "--build", str(downstream_build_dir)])
    print("Downstream Alloy package check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())