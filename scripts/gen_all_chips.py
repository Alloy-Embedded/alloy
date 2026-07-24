#!/usr/bin/env python3
"""Gen-all smoke: run the chip-level codegen over EVERY chip in the database.

NORTH_STAR goal #2 promises adding a chip that reuses known IP versions costs
"zero new C++ — only data". That promise is only real if the emitter can
actually consume every shipped chip. The board×example CI matrix compiles only
the handful of chips that boards reference, so the ~400 builder-generated chips
were never generated, let alone compiled — an emitter bug on any of them (e.g.
dereferencing an uncurated stub's missing `ip` key) shipped silently.

This script closes that gap: it runs the same chip-level emitters that ship
firmware (`alloy_cli.emit.emit_chip_check`) over `db.chips`, into a throwaway
tree, and fails CI if any chip crashes the generator. It is generate-only (no
cross-compile) — fast, and enough to catch the KeyError class of bug. Compile
coverage of generated output stays with the board matrix.

Run: `python scripts/gen_all_chips.py` (needs the alloy_cli + alloy_devices
packages importable, e.g. `uv run --project tools/alloy python scripts/...`).
Honors ALLOY_DEVICES_ROOT; otherwise expects alloy-devices beside the repo.
"""

from __future__ import annotations

import os
import sys
import tempfile
import traceback
from pathlib import Path

from alloy_cli.emit import emit_chip_check
from alloy_devices.loader import load_database

ALLOY_ROOT = Path(__file__).resolve().parent.parent


def _devices_root() -> Path:
    if env := os.environ.get("ALLOY_DEVICES_ROOT"):
        return Path(env).resolve()
    sibling = ALLOY_ROOT.parent / "alloy-devices"
    if (sibling / "chips").is_dir():
        return sibling
    raise SystemExit(
        "could not find the alloy-devices database — set ALLOY_DEVICES_ROOT or "
        f"clone alloy-devices beside the framework ({sibling})"
    )


def main() -> int:
    db = load_database(_devices_root())
    if not db.chips:
        raise SystemExit("no chips loaded — is the database path correct?")

    schema_errors = [i for i in db.errors]
    if schema_errors:
        print(f"database has {len(schema_errors)} schema error(s) before codegen:")
        for issue in schema_errors[:20]:
            print(f"  {issue}")
        return 1

    failures: list[tuple[str, str]] = []
    with tempfile.TemporaryDirectory(prefix="alloy-gen-all-") as tmp:
        out = Path(tmp)
        for key in sorted(db.chips):
            try:
                emit_chip_check(db.chips[key], db, out / key.replace("/", "_"), ALLOY_ROOT)
            except Exception:  # noqa: BLE001 — report every chip, don't abort on the first
                failures.append((key, traceback.format_exc().strip().splitlines()[-1]))

    total = len(db.chips)
    if failures:
        print(f"gen-all FAILED: {len(failures)}/{total} chips crashed the emitter\n")
        for key, err in failures:
            print(f"  {key}: {err}")
        return 1

    print(f"gen-all OK: {total} chips generated cleanly")
    return 0


if __name__ == "__main__":
    sys.exit(main())
