"""The coverage scoreboard: what a chip's silicon offers vs what alloy supports.

`alloy chip-status <chip>` answers four questions per peripheral instance, and
answers all four by DERIVING them from the trees that already decide them — so
the scoreboard cannot rot the way a hand-maintained checklist does:

  curated  — the chip yaml binds this instance to an `ip`, and alloy-devices
             ships ``registers/<vendor>/<ip>.yaml`` for it. This is exactly the
             gate ``emit.device.curated_peripherals`` applies, so "curated" here
             means "codegen emits a descriptor for it", not "someone looked at
             it once".
  driver   — the framework ships ``src/alloy/hal/<class>/<vendor>_<ip>.hpp``.
             That is the file ``emit/__init__._emit_chip_sources`` includes when
             it exists; same path, same rule, one source of truth.
  renode   — ``emit/renode.renode_models()`` names a Renode model for the IP.
  roles    — some board in the search path binds this instance to a board role
             (or, for a GPIO port, names one of its pins).

WHAT THE HEADLINE COUNTS, stated because a percentage invites misreading:

  * The denominator is peripheral INSTANCES of this one part, as the chip yaml
    lists them. usart1..usart6 count six times; they share one IP, one register
    file and one driver. The per-IP counts are reported alongside for that
    reason — six instances of a curated IP is one curation job done, not six.
  * "curated" is about REGISTER DATA, not about a working peripheral. An
    instance can be curated, have a driver, and still be unproven on silicon.
  * "driver" is about a HAL header EXISTING for the IP. It says nothing about
    how much of the peripheral that driver covers (a CAN driver that only does
    classic frames still counts here), and nothing about it being tested.
  * Nothing in this report is evidence from hardware. `renode` means a model
    exists for the IP, not that this chip's platform instantiates it (that
    additionally depends on the board's roles) and not that a leg exercises it.

So "100%" on this scoreboard is the floor of the 100% push, not the ceiling.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from .board_info import BOARD_SCHEMA, board_dirs
from .devices import load_chip, load_registers
from .emit.common import EmitError
from .emit.device import pin_port_peripheral
from .emit.renode import renode_models
from .roles import ROLES, role_pin_fields

SCHEMA = "alloy.chip_status.v1"


def _driver_path(registers: dict[str, dict[str, Any]], ip_key: str) -> str | None:
    """The HAL header for an IP, spelled exactly as codegen would include it."""
    doc = registers.get(ip_key)
    if doc is None or not doc.get("class"):
        return None
    vendor, _, ip = ip_key.partition("/")
    return f"alloy/hal/{doc['class']}/{vendor}_{ip}.hpp"


def _board_bindings(alloy_root: Path, project_root: Path | None,
                    chip_id: str, chip: dict[str, Any]) -> dict[str, list[dict[str, str]]]:
    """{peripheral: [{board, role}, …]} for every board that uses this chip.

    Two ways a role reaches a peripheral, both of them how the emitter reads a
    board: a `peripheral` field names an instance outright, and any field
    naming a PIN reaches that pin's GPIO-port peripheral. The second is what
    stops `gpioa` from looking unused on a board whose LED is on PA5.
    """
    import json  # noqa: PLC0415

    out: dict[str, list[dict[str, str]]] = {}
    seen: set[tuple[str, str, str]] = set()

    def bind(periph: str, board_id: str, role: str) -> None:
        if periph not in chip.get("peripherals", {}):
            return  # a board naming something this chip lacks: board-validate's job
        key = (periph, board_id, role)
        if key in seen:
            return
        seen.add(key)
        out.setdefault(periph, []).append({"board": board_id, "role": role})

    board_ids: set[str] = set()
    for directory, _source in board_dirs(alloy_root, project_root):
        if not directory.is_dir():
            continue
        for entry in sorted(directory.iterdir()):
            path = entry / "board.json"
            if not path.exists() or entry.name in board_ids:
                continue  # first hit wins: a project board shadows a framework one
            try:
                board = json.loads(path.read_text())
            except (OSError, ValueError):
                continue
            if board.get("schema") != BOARD_SCHEMA or board.get("chip") != chip_id:
                continue
            board_ids.add(entry.name)
            for role, cfg in (board.get("roles") or {}).items():
                spec = ROLES.get(role)
                if spec is None or not isinstance(cfg, dict):
                    continue
                if isinstance(cfg.get("peripheral"), str):
                    bind(cfg["peripheral"], entry.name, role)
                pins = [cfg[f] for f in role_pin_fields(spec)
                        if isinstance(cfg.get(f), str)]
                if spec.pin_list_field:
                    pins += [p for p in (cfg.get(spec.pin_list_field) or [])
                             if isinstance(p, str)]
                for pin in pins:
                    if pin in (chip.get("pins") or {}):
                        bind(pin_port_peripheral(chip, pin), entry.name, role)
    return out


def chip_status(alloy_root: Path, devices_root: Path, chip_id: str,
                project_root: Path | None = None) -> dict[str, Any]:
    """The scoreboard for one chip. Pure derivation; writes nothing."""
    chip = load_chip(devices_root, chip_id)
    registers = load_registers(devices_root)
    models = renode_models()
    bindings = _board_bindings(alloy_root, project_root, chip_id, chip)

    rows: list[dict[str, Any]] = []
    for name, spec in sorted((chip.get("peripherals") or {}).items()):
        ip_key = spec.get("ip")
        # `uncurated` is the data's own word for "admitted, but no register file
        # bound". An `ip` naming a register document the database does not ship
        # would be a broken chip file, so it is reported as uncurated too rather
        # than counted — the number must never be flattered by a dangling key.
        has_regs = bool(ip_key) and ip_key in registers
        curated = has_regs and not spec.get("uncurated", False)
        driver = _driver_path(registers, ip_key) if curated else None
        rows.append({
            "name": name,
            "ip": ip_key,
            "class": registers.get(ip_key or "", {}).get("class"),
            "curated": curated,
            "driver": driver if driver and (alloy_root / "src" / driver).exists() else None,
            "renode_model": models.get(ip_key) if curated else None,
            "roles": bindings.get(name, []),
        })

    ips = sorted({r["ip"] for r in rows if r["curated"]})
    total = len(rows)
    curated_n = sum(1 for r in rows if r["curated"])
    driver_n = sum(1 for r in rows if r["driver"])
    return {
        "schema": SCHEMA,
        "chip": chip_id,
        "family": chip.get("family"),
        "part": chip.get("part"),
        "boards": sorted({b["board"] for v in bindings.values() for b in v}),
        "peripherals": rows,
        "summary": {
            "peripherals": total,
            "curated": curated_n,
            "uncurated": total - curated_n,
            "with_driver": driver_n,
            "with_renode_model": sum(1 for r in rows if r["renode_model"]),
            "board_reachable": sum(1 for r in rows if r["roles"]),
            # Per-IP, because six USARTs are one curation job, not six.
            "ips_curated": len(ips),
            "ips_with_driver": len({r["ip"] for r in rows if r["driver"]}),
            "headline": (f"{curated_n} of {total} peripherals curated, "
                         f"{driver_n} with drivers"),
        },
        "counts": {
            "unit": "peripheral instances as the chip yaml lists them",
            "curated": "an `ip` bound to a registers/<vendor>/<ip>.yaml the "
                       "database ships — i.e. codegen emits a descriptor",
            "driver": "src/alloy/hal/<class>/<vendor>_<ip>.hpp exists; says "
                      "nothing about how much of the peripheral it covers",
            "renode_model": "a Renode model is known for the IP; not that this "
                            "board's platform instantiates it or that a leg "
                            "exercises it",
            "not_counted": "nothing here is evidence from silicon",
        },
    }


def format_status(status: dict[str, Any]) -> str:
    """The human table. One line per peripheral, then the headline."""
    rows = status["peripherals"]
    width = max((len(r["name"]) for r in rows), default=4)
    ipw = max((len(r["ip"] or "-") for r in rows), default=2)
    lines = [
        f"{status['chip']}  ({status.get('part') or '?'}, {status.get('family') or '?'})",
        "",
        f"{'PERIPHERAL':<{width}}  {'IP':<{ipw}}  REG  DRV  REN  ROLE",
    ]
    for r in rows:
        role = ", ".join(sorted({b["role"] for b in r["roles"]})) or "-"
        lines.append(
            f"{r['name']:<{width}}  {r['ip'] or '-':<{ipw}}  "
            f"{'yes' if r['curated'] else ' - ':<3}  "
            f"{'yes' if r['driver'] else ' - ':<3}  "
            f"{'yes' if r['renode_model'] else ' - ':<3}  {role}"
        )
    s = status["summary"]
    lines += [
        "",
        s["headline"] + f", {s['with_renode_model']} with a Renode model, "
        f"{s['board_reachable']} reachable from a board role"
        + (f" ({', '.join(status['boards'])})" if status["boards"] else ""),
        f"by IP: {s['ips_curated']} curated IPs, {s['ips_with_driver']} with drivers",
        "",
        "REG = curated register data (codegen emits a descriptor).  "
        "DRV = a HAL driver header exists for the IP.",
        "REN = a Renode model is known for the IP.  ROLE = bound by a board "
        "using this chip.",
        "None of these is evidence from silicon.",
    ]
    return "\n".join(lines)


def resolve_chip_id(devices_root: Path, chip_id: str) -> str:
    """Accept a chip id, and say so helpfully when only near ones exist."""
    vendor, _, part = chip_id.partition("/")
    if (devices_root / "chips" / vendor / f"{part}.yaml").exists():
        return chip_id
    near = sorted(p.stem for p in (devices_root / "chips" / vendor).glob(f"{part}*.yaml")) \
        if (devices_root / "chips" / vendor).is_dir() else []
    hint = f" — did you mean {', '.join(f'{vendor}/{n}' for n in near[:6])}?" if near else \
        " — try `alloy chips`"
    raise EmitError(f"unknown chip '{chip_id}'{hint}")
