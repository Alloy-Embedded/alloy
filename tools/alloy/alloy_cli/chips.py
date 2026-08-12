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
from .roles import ROLES, ip_classes, routes_by_peripheral

_FAMILY = re.compile(r"^family:\s*(\S+)", re.M)
_CORE = re.compile(r"name:\s*(cm0plus|cm0|cm3|cm4|cm7|cm33|lx6|lx7)\b")
# Timer route signals are named ch1/ch2/… — a PWM role picks a CHANNEL, and the
# pins it can drive come from that channel's routes.
_CHANNEL_SIGNAL = re.compile(r"^ch(\d+)$")


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


def _role_catalogue(data: dict[str, Any], classes: dict[str, tuple[str, ...]],
                    routes: dict[str, dict[str, list[str]]]) -> dict[str, Any]:
    """Every role the framework knows, with this chip's candidates for it."""
    peripherals = data.get("peripherals") or {}
    has_pins = bool(data.get("pins"))
    catalogue: dict[str, Any] = {}

    for role, spec in ROLES.items():
        entry: dict[str, Any] = {
            "kind": spec.kind,
            "required": list(spec.required),
            "optional": list(spec.optional),
            # Fields a PROJECT may choose from alloy.toml without owning the
            # board. An editor needs this to keep them editable on a curated
            # board that is otherwise read-only.
            "project_fields": list(spec.project_fields),
            "candidates": [],
        }
        if spec.requires_role:
            entry["requires_role"] = spec.requires_role

        if spec.kind in ("pin", "pins"):
            entry["supported"] = has_pins
            entry["reason"] = None if has_pins else "this chip's data lists no pins yet"
        elif spec.kind == "external":
            entry["supported"] = True
            entry["reason"] = None
        else:
            for name in sorted(peripherals):
                # MEMBERSHIP, NOT EQUALITY — one block can fill roles of
                # several classes, one at a time (roles.ip_classes).
                if spec.ip_class not in classes.get(name, ()):
                    continue
                signals = routes.get(name, {})
                candidate: dict[str, Any] = {
                    "peripheral": name,
                    "ip": peripherals[name].get("ip"),
                    # An uncurated peripheral is admitted by the database but has
                    # no register file yet: the emitter refuses it, so the UI must
                    # show it as unavailable rather than let the user pick it.
                    "curated": not peripherals[name].get("uncurated", False),
                    "signals": {s: signals.get(s, []) for s in spec.signals},
                }
                channels = [
                    {"channel": int(m.group(1)), "pins": pins}
                    for sig, pins in sorted(signals.items())
                    if (m := _CHANNEL_SIGNAL.match(sig))
                ]
                if channels:
                    candidate["channels"] = channels
                if adc_channels := peripherals[name].get("channels"):
                    candidate["analog_channels"] = adc_channels
                entry["candidates"].append(candidate)
            usable = [c for c in entry["candidates"] if c["curated"]]
            entry["supported"] = bool(usable)
            entry["reason"] = None if usable else (
                f"no curated {spec.ip_class} peripheral in this chip's data"
                if not entry["candidates"] else
                f"this chip's {spec.ip_class} peripheral is not curated yet"
            )
        catalogue[role] = entry
    return catalogue


def chip_info(devices_root: Path, chip_id: str) -> dict[str, Any]:
    """Everything a visual board configurator needs for one chip: clock profiles,
    memories, the pin map with each pin's alternate functions, every peripheral
    with its IP class, and the ROLE CATALOGUE — which of the framework's 16 board
    roles this silicon can fill, and with what. Pure data; no board.json written.

    Additive by contract: the envelope stays alloy.chip_info.v1 and older fields
    keep their shape, so an IDE built against an earlier CLI still works and a
    newer one feature-detects by checking for a field.
    """
    from .devices import load_chip, load_registers  # noqa: PLC0415

    data = load_chip(devices_root, chip_id)
    registers = load_registers(devices_root)
    # A FOURTH COPY OF THIS RULE USED TO LIVE HERE, inline, and it is exactly
    # the drift roles.py's own docstring warns about: "a third copy was one
    # commit away from disagreeing with the emitter". It was — resolving an
    # IP to ONE class, while board_validate and the emitter had learned that a
    # block can have several. Ask the shared helper.
    classes = ip_classes(data, registers)

    clk = data.get("clock") or {}
    profiles = [
        {"name": n, "description": p.get("description", ""), "sysclk_hz": p.get("sysclk_hz")}
        for n, p in clk.get("profiles", {}).items()
    ]

    # Group the pin-route table by peripheral: {usart2: {tx: pa2, rx: pa3}, …}.
    by_periph: dict[str, dict[str, str]] = {}
    for r in data.get("routes") or []:
        if "peripheral" in r and "signal" in r and "pin" in r:
            by_periph.setdefault(r["peripheral"], {})[r["signal"]] = r["pin"]

    def instances(prefixes: tuple[str, ...], signals: list[str]) -> list[dict[str, Any]]:
        out = []
        for periph in sorted(by_periph):
            if periph.startswith(prefixes):
                sigs = by_periph[periph]
                entry: dict[str, Any] = {"peripheral": periph}
                for s in signals:
                    if s in sigs:
                        entry[s] = sigs[s]
                out.append(entry)
        return out

    # Per-PIN view for the CubeMX-style configurator: every GPIO with its port/
    # index (grid placement) and every alternate function the route table offers
    # on it. Builder-generated chips carry the full AF matrix (~150 routes);
    # hand-curated ones only their curated subset — the UI shows what the data
    # honestly knows, never a guessed pinout.
    funcs_by_pin: dict[str, list[dict[str, Any]]] = {}
    for r in data.get("routes") or []:
        if "pin" in r and "peripheral" in r and "signal" in r:
            entry: dict[str, Any] = {"peripheral": r["peripheral"], "signal": r["signal"]}
            if "af" in r:
                entry["af"] = r["af"]
            funcs_by_pin.setdefault(r["pin"], []).append(entry)
    pins = [
        {
            "name": name,
            "port": meta.get("port"),
            "index": meta.get("index"),
            "functions": sorted(funcs_by_pin.get(name, []),
                                key=lambda f: (f["peripheral"], f["signal"])),
        }
        for name, meta in sorted((data.get("pins") or {}).items())
    ]

    routes = routes_by_peripheral(data)
    return {
        "schema": "alloy.chip_info.v1",
        "chip": chip_id,
        "family": data.get("family"),
        "part": data.get("part"),
        "clock_profiles": profiles,
        "boot_profile": profiles[0]["name"] if profiles else None,
        "gpio_pins": sorted((data.get("pins") or {}).keys()),
        "pins": pins,
        # Pins the DATA knows, which is not the package's pin count: a curated
        # chip carries only its curated subset. Named honestly so the UI never
        # draws a package it cannot back up (a real `package` fact is not in the
        # chip schema yet).
        "pin_count": len(pins),
        "package": data.get("package"),
        "memories": [
            {"name": m.get("name"), "kind": m.get("kind"),
             "base": m.get("base"), "size": m.get("size")}
            for m in data.get("memories") or []
        ],
        "interrupt_count": len(data.get("interrupts") or []),
        "peripherals": {
            "debug_uart": instances(("usart", "uart", "lpuart"), ["tx", "rx"]),
            "i2c": instances(("i2c",), ["scl", "sda"]),
            "spi": instances(("spi",), ["sck", "mosi", "miso"]),
            "all": [
                {"name": name, "ip": spec.get("ip"),
                 # `class` stays the PRIMARY one so this field keeps its
                 # meaning for every existing consumer; `classes` is the full
                 # set, and the IDE needs it to know a timer can be an encoder.
                 "class": (classes.get(name) or (None,))[0],
                 "classes": list(classes.get(name) or ()),
                 "curated": not spec.get("uncurated", False)}
                for name, spec in sorted((data.get("peripherals") or {}).items())
            ],
        },
        "roles": _role_catalogue(data, classes, routes),
    }


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
