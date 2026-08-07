"""Board introspection — everything the IDE needs to SHOW a board.

The visual configurator used to work only on project-local boards, so a user who
started from a curated board (the recommended path) could not even look at their
own pinout. This module resolves any board — framework or project-local — and
reports it as one versioned envelope, including the capabilities the generated
``board::caps`` will carry and whatever the emitter would reject.

Facts still come from one place: capabilities from ``emit.board.role_caps`` and
validation from the real emitters, so this verb can never claim a board is
healthy when generation would fail.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .devices import load_chip, load_registers
from .emit.board import role_caps
from .emit.common import EmitError
from .roles import ROLES, role_pin_fields

BOARD_SCHEMA = "alloy.board.v1"



def board_dirs(alloy_root: Path, project_root: Path | None) -> list[tuple[Path, str]]:
    """Search order: a project's own boards/ shadows a framework board of the
    same id — the same precedence ``Project.board_json`` uses to build."""
    dirs: list[tuple[Path, str]] = []
    if project_root is not None:
        dirs.append((project_root / "boards", "project"))
    dirs.append((alloy_root / "boards", "framework"))
    return dirs


def resolve_board(alloy_root: Path, project_root: Path | None,
                  board_id: str) -> tuple[Path, str]:
    """(board.json path, 'project' | 'framework') for a board id."""
    for directory, source in board_dirs(alloy_root, project_root):
        candidate = directory / board_id / "board.json"
        if candidate.exists():
            return candidate, source
    known = ", ".join(list_board_ids(alloy_root, project_root)) or "(none)"
    raise EmitError(f"unknown board '{board_id}' — known boards: {known}")


def list_board_ids(alloy_root: Path, project_root: Path | None) -> list[str]:
    ids: list[str] = []
    for directory, _ in board_dirs(alloy_root, project_root):
        if directory.is_dir():
            ids.extend(p.name for p in sorted(directory.iterdir())
                       if (p / "board.json").exists())
    return sorted(dict.fromkeys(ids))


def _load(path: Path) -> dict[str, Any]:
    board = json.loads(path.read_text())
    if board.get("schema") != BOARD_SCHEMA:
        raise EmitError(f"{path}: expected schema {BOARD_SCHEMA}, "
                        f"got '{board.get('schema')}'")
    return board


def _clock_summary(board: dict[str, Any], chip: dict[str, Any]) -> dict[str, Any]:
    """The board's clock as one shape, whether it names a curated profile or
    carries an inline custom PLL from `alloy clock` / the visual editor."""
    inline = board.get("clock")
    if isinstance(inline, dict):
        return {
            "mode": "inline",
            "profile": "custom",
            "sysclk_hz": inline.get("sysclk_hz"),
            "description": inline.get("description", ""),
            "silicon_validated": bool(inline.get("silicon_validated", False)),
        }
    name = board.get("clock_profile")
    profile = (chip.get("clock", {}).get("profiles") or {}).get(name, {})
    return {
        "mode": "profile",
        "profile": name,
        "sysclk_hz": profile.get("sysclk_hz"),
        "description": profile.get("description", ""),
        "silicon_validated": bool(profile.get("silicon_validated", False)),
    }


def _pins_used(board: dict[str, Any], chip: dict[str, Any]) -> list[dict[str, str]]:
    """Pins a role has already claimed. The RMII bank is a CHIP fact (the MAC's
    fixed pin bank), so an Ethernet board reports those pins too even though
    board.json never names them."""
    roles = board.get("roles", {})
    used: list[dict[str, str]] = []
    for role, spec in ROLES.items():
        cfg = roles.get(role)
        if not cfg:
            continue
        for field in role_pin_fields(spec):
            pin = cfg.get(field)
            if isinstance(pin, str):
                used.append({"pin": pin, "owner": role, "signal": f"{role} {field}"})
        if spec.pin_list_field:
            for pin in cfg.get(spec.pin_list_field) or []:
                used.append({"pin": pin, "owner": role, "signal": f"{role} bus"})
    eth = roles.get("ethernet")
    if eth and eth.get("peripheral"):
        rmii = chip.get("peripherals", {}).get(eth["peripheral"], {}).get("rmii") or {}
        for pin in rmii.get("pins") or []:
            used.append({"pin": pin, "owner": "ethernet", "signal": "rmii"})
    used.sort(key=lambda e: (e["pin"], e["owner"]))
    return used


def _validate(board: dict[str, Any], chip: dict[str, Any],
              registers: dict[str, dict[str, Any]]) -> list[dict[str, Any]]:
    """Everything wrong with this board: the located, all-at-once rules from
    `board_validate`, plus the real emitters as a backstop.

    The rules are a second expression of the emitter's contract, so they could
    drift. Running generation afterwards makes that drift visible instead of
    silent — if the rules say "clean" and generation still fails, the failure is
    reported rather than swallowed. (`test_board_validate` asserts the two agree,
    so this backstop should never fire.)
    """
    from .board_validate import validate_board  # noqa: PLC0415
    from .emit import _arch_ns  # noqa: PLC0415 - avoids an import cycle
    from .emit.board import emit_board_header, emit_board_source  # noqa: PLC0415

    issues: list[dict[str, Any]] = [
        {**i, "stage": "roles"} for i in validate_board(board, chip, registers)
    ]
    for stage, run in (
        ("roles", lambda: emit_board_header(board, chip, registers)),
        ("clock", lambda: emit_board_source(board, chip, registers, _arch_ns(chip))),
    ):
        try:
            run()
        except (EmitError, KeyError, TypeError, ValueError) as exc:
            message = str(exc).strip("'") or exc.__class__.__name__
            if not any(i["level"] == "error" for i in issues):
                issues.append({"level": "error", "stage": stage, "message": message,
                               "role": None, "field": None, "pin": None,
                               "suggestions": []})
            break  # a broken header makes the source failure noise, not news
    return issues


def board_info(devices_root: Path, alloy_root: Path, project_root: Path | None,
               board_id: str) -> dict[str, Any]:
    path, source = resolve_board(alloy_root, project_root, board_id)
    board = _load(path)
    chip = load_chip(devices_root, board["chip"])
    registers = load_registers(devices_root)

    caps = role_caps(board, chip, registers)
    caps["net"] = caps["ethernet"] or caps["wifi"]

    return {
        "schema": "alloy.board_info.v1",
        "id": board["id"],
        "name": board.get("name", ""),
        "chip": board["chip"],
        "family": chip.get("family"),
        "part": chip.get("part"),
        "source": source,
        # Only a project-local board may be edited in place: rewriting a
        # framework board would change every project that uses it.
        "editable": source == "project",
        "path": str(path),
        "clock": _clock_summary(board, chip),
        "probe": board.get("probe"),
        "roles": board.get("roles", {}),
        "caps": dict(sorted(caps.items())),
        "pins_used": _pins_used(board, chip),
        "named_pins": [
            {"pin": pin, "function": spec.get("function", ""), "label": spec.get("label")}
            for pin, spec in sorted((board.get("pins") or {}).items())
        ],
        "issues": _validate(board, chip, registers),
    }


def clone_board(alloy_root: Path, project_root: Path, source_id: str,
                new_id: str) -> Path:
    """Copy a board into the project as an editable one. This is how a user
    starts from a real, working board instead of an empty custom one — the
    curated pin choices are the point."""
    if not (project_root / "alloy.toml").exists():
        raise EmitError(f"{project_root} is not an alloy project (no alloy.toml)")
    if not new_id.replace("_", "").isalnum() or not new_id[:1].isalpha():
        raise EmitError(f"board id '{new_id}' must start with a letter and contain "
                        "only letters, digits and underscores")
    src_path, _ = resolve_board(alloy_root, project_root, source_id)
    dest = project_root / "boards" / new_id / "board.json"
    if dest.exists():
        raise EmitError(f"board '{new_id}' already exists at {dest}")

    board = _load(src_path)
    board["id"] = new_id
    name = board.get("name") or source_id
    board["name"] = f"{name} (custom)"
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(json.dumps(board, indent=2) + "\n")
    return dest
