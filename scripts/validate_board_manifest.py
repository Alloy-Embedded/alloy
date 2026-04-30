#!/usr/bin/env python3
"""
validate_board_manifest.py — Validate all boards/**/board.json files against
the alloy board-manifest JSON Schema v1.

Usage:
    python scripts/validate_board_manifest.py              # validate all
    python scripts/validate_board_manifest.py boards/nucleo_g071rb/board.json

Exit code 0 = all valid; 1 = one or more failures.

Requires:
    pip install jsonschema

Without jsonschema installed, the script does structural validation only
(required fields present, arch enum, etc.) using a built-in fallback checker.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCHEMA_PATH = ROOT / "cmake" / "schemas" / "board-manifest" / "v1.json"

VALID_ARCHES = {
    "cortex-m0plus", "cortex-m4", "cortex-m7",
    "riscv32", "xtensa-lx6", "xtensa-lx7", "avr", "native",
}
VALID_TOOLCHAINS = {
    "arm-none-eabi", "riscv32-esp-elf", "xtensa-esp32-elf",
    "xtensa-esp32s3-elf", "avr-gcc", "native",
}
REQUIRED_FIELDS = {
    "board_id", "vendor", "family", "device", "arch",
    "linker_script", "board_header", "toolchain", "tier",
}


def _builtin_validate(data: dict, path: Path) -> list[str]:
    """Structural validation without jsonschema dependency."""
    errors: list[str] = []

    # Required fields
    for field in sorted(REQUIRED_FIELDS):
        if field not in data:
            errors.append(f"missing required field '{field}'")

    # Enum checks
    arch = data.get("arch", "")
    if arch and arch not in VALID_ARCHES:
        errors.append(f"'arch' = '{arch}' not in {sorted(VALID_ARCHES)}")

    toolchain = data.get("toolchain", "")
    if toolchain and toolchain not in VALID_TOOLCHAINS:
        errors.append(f"'toolchain' = '{toolchain}' not in {sorted(VALID_TOOLCHAINS)}")

    tier = data.get("tier")
    if tier is not None and not isinstance(tier, int):
        errors.append(f"'tier' must be an integer, got {type(tier).__name__}")
    elif isinstance(tier, int) and not (1 <= tier <= 3):
        errors.append(f"'tier' = {tier} out of range 1-3")

    board_id = data.get("board_id", "")
    if board_id and not all(c.isalnum() or c == "_" for c in board_id):
        errors.append(f"'board_id' = '{board_id}' contains invalid characters (only a-z, 0-9, _)")

    # board_id should match directory name
    expected_id = path.parent.name
    if board_id and board_id != expected_id:
        errors.append(
            f"'board_id' = '{board_id}' does not match directory name '{expected_id}'"
        )

    return errors


def validate_one(manifest_path: Path, schema: dict | None) -> list[str]:
    """Validate a single board.json. Returns list of error strings (empty = ok)."""
    try:
        data = json.loads(manifest_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        return [f"invalid JSON: {exc}"]
    except OSError as exc:
        return [f"cannot read file: {exc}"]

    if schema is not None:
        try:
            import jsonschema  # type: ignore[import]
            validator = jsonschema.Draft7Validator(schema)
            return [str(e.message) for e in validator.iter_errors(data)]
        except ImportError:
            pass  # fall through to built-in checker

    return _builtin_validate(data, manifest_path)


def main(argv: list[str] | None = None) -> int:
    paths: list[Path]
    if argv:
        paths = [Path(a) for a in argv]
    else:
        paths = sorted(ROOT.glob("boards/**/board.json"))

    if not paths:
        print("No board.json files found under boards/")
        return 0

    schema: dict | None = None
    if SCHEMA_PATH.exists():
        try:
            schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            print(f"[warn] could not parse schema at {SCHEMA_PATH}", file=sys.stderr)

    total = len(paths)
    failures = 0

    for p in paths:
        errors = validate_one(p, schema)
        rel = p.relative_to(ROOT) if p.is_relative_to(ROOT) else p
        if errors:
            failures += 1
            print(f"[FAIL] {rel}")
            for err in errors:
                print(f"       - {err}")
        else:
            print(f"[ok]   {rel}")

    print(f"\n{total - failures}/{total} board.json files valid.")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:] or None))
