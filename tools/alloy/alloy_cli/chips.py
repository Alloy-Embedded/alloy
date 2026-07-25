"""Chip catalogue + clean-board scaffolding.

The framework ships curated *boards*, but the device database has hundreds of
*chips*. `alloy chips` lists them and `alloy new --chip <id>` scaffolds a project
with a project-LOCAL "clean" board (just the MCU + a safe clock, empty roles) so
a user can target any supported silicon and fill in pins themselves — free to
build whatever they want, without a curated board.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from .emit.common import EmitError

_FAMILY = re.compile(r"^family:\s*(\S+)", re.M)
_CORE = re.compile(r"name:\s*(cm0plus|cm0|cm3|cm4|cm7|cm33|lx6|lx7)\b")


def _chips_dir(devices_root: Path) -> Path:
    d = devices_root / "chips"
    if not d.is_dir():
        raise EmitError(f"no chips/ under the device database at {devices_root}")
    return d


def list_chips(devices_root: Path) -> list[dict[str, Any]]:
    """Every chip as {id, vendor, chip, family, core} — cheap regex read (no full
    YAML parse) so listing all ~400 stays fast."""
    rows: list[dict[str, Any]] = []
    for f in sorted(_chips_dir(devices_root).glob("*/*.yaml")):
        text = f.read_text()
        fam = _FAMILY.search(text)
        core = _CORE.search(text)
        rows.append({
            "id": f"{f.parent.name}/{f.stem}",
            "vendor": f.parent.name,
            "chip": f.stem,
            "family": fam.group(1) if fam else f.stem,
            "core": core.group(1) if core else None,
        })
    return rows


def chip_clock(devices_root: Path, chip_id: str) -> tuple[list[str], str]:
    """(profile names, default) for a chip. Default = the first profile, which
    is the boot-safe (no-PLL) one by database convention."""
    import yaml  # noqa: PLC0415

    vendor, _, name = chip_id.partition("/")
    f = _chips_dir(devices_root) / vendor / f"{name}.yaml"
    if not f.exists():
        raise EmitError(f"unknown chip '{chip_id}' — try `alloy chips`")
    data = yaml.safe_load(f.read_text())
    profiles = list((data.get("clock") or {}).get("profiles", {}).keys())
    if not profiles:
        raise EmitError(f"chip '{chip_id}' has no clock profiles in the database")
    return profiles, profiles[0]


def clean_board_json(board_id: str, chip_id: str, clock_profile: str) -> str:
    """A minimal, valid board: the chip, a safe clock, and no roles yet."""
    return json.dumps(
        {
            "schema": "alloy.board.v1",
            "id": board_id,
            "name": f"Custom ({chip_id})",
            "chip": chip_id,
            "clock_profile": clock_profile,
            "roles": {},
        },
        indent=2,
    ) + "\n"
