"""Light, read-only access to the device database.

``alloy_devices.loader.load_database`` parses and schema-validates all 412 chips
(~8.5 s on this machine). That is the right price for `gen`/`build`, which write
code, but far too slow for the introspection verbs an IDE calls on every panel
open. These loaders read ONE chip plus the register files (~56 ms) with no
schema validation.

The trade is deliberate and safe in one direction only: nothing here emits code.
Anything that produces a build still goes through ``load_database``, so bad data
is still rejected before it can reach a device.
"""

from __future__ import annotations

from functools import lru_cache
from pathlib import Path
from typing import Any

import yaml

try:  # libyaml C loader when available (same choice the real loader makes)
    _Loader = yaml.CSafeLoader
except AttributeError:  # pragma: no cover - pure-python fallback
    _Loader = yaml.SafeLoader

from .emit.common import EmitError


def _read(path: Path) -> Any:
    return yaml.load(path.read_text(), Loader=_Loader)


def chip_path(devices_root: Path, chip_id: str) -> Path:
    """Path of a chip file, or a helpful error naming the verb that lists them."""
    vendor, _, part = chip_id.partition("/")
    path = devices_root / "chips" / vendor / f"{part}.yaml"
    if not path.exists():
        raise EmitError(f"unknown chip '{chip_id}' — try `alloy chips`")
    return path


@lru_cache(maxsize=8)
def load_chip(devices_root: Path, chip_id: str) -> dict[str, Any]:
    return _read(chip_path(devices_root, chip_id))


@lru_cache(maxsize=2)
def load_registers(devices_root: Path) -> dict[str, dict[str, Any]]:
    """Every curated IP register document, keyed ``<vendor>/<ip>`` — the same
    keys ``chip['peripherals'][x]['ip']`` uses."""
    regs: dict[str, dict[str, Any]] = {}
    for path in sorted((devices_root / "registers").glob("*/*.yaml")):
        regs[f"{path.parent.name}/{path.stem}"] = _read(path)
    return regs


def ip_class(registers: dict[str, dict[str, Any]], ip_key: str | None) -> str | None:
    """The IP's peripheral class ('uart', 'i2c', 'flash', …). This is what makes
    role candidates data-driven instead of guessed from instance names: every
    curated register file declares one."""
    if not ip_key:
        return None
    return registers.get(ip_key, {}).get("class")
