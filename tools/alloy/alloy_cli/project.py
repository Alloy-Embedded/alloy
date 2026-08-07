"""Project, framework and database discovery.

A project is a directory with an ``alloy.toml``. The framework root (this
repo) is found by walking up from the project (in-repo examples) or via
``ALLOY_ROOT``. The device database defaults to the sibling ``alloy-devices``
checkout or ``ALLOY_DEVICES_ROOT``. Wheel-based distribution replaces this
discovery later without changing callers.
"""

from __future__ import annotations

import json
import os
import tomllib
from dataclasses import dataclass
from pathlib import Path
from typing import Any


class ProjectError(RuntimeError):
    pass


@dataclass(frozen=True)
class Project:
    root: Path
    name: str
    board_id: str
    alloy_root: Path
    devices_root: Path

    # Per-board trees: switching boards must never reuse another board's
    # CMake cache (cached CPU flags) or stale generated headers.
    @property
    def gen_dir(self) -> Path:
        return self.root / ".alloy" / "generated" / self.board_id

    @property
    def build_dir(self) -> Path:
        return self.root / ".alloy" / "build-tree" / self.board_id

    @property
    def board_json(self) -> Path:
        # A project may carry its OWN board (a custom/clean board scaffolded from
        # a chosen MCU): ./boards/<id>/board.json wins over a framework board of
        # the same id, so users can build for any chip without touching the
        # framework. Falls back to the curated framework board.
        local = self.root / "boards" / self.board_id / "board.json"
        return local if local.exists() else self.alloy_root / "boards" / self.board_id / "board.json"

    def lib_includes(self) -> list[Path]:
        """Include dirs of libraries vendored under ./libs and listed in [libs].

        `alloy lib add` copies a library into ./libs/<name> and records it in
        alloy.toml; the build adds each one's include/ dir so `#include <name.hpp>`
        resolves. Missing/renamed entries are skipped (a stale toml never breaks
        the build).
        """
        toml_path = self.root / "alloy.toml"
        if not toml_path.exists():
            return []
        data = tomllib.loads(toml_path.read_text())
        out: list[Path] = []
        for name in data.get("libs", {}):
            inc = self.root / "libs" / name / "include"
            if inc.is_dir():
                out.append(inc)
        return out

    def ota_options(self) -> dict[str, Any]:
        """The optional ``[ota]`` table from alloy.toml. `public_key` (a path to
        an `alloy keygen` .pub, or 64 hex chars inline) turns on signed-image
        verification: codegen bakes the key into alloy/ota_key.hpp and the build
        pulls in the Ed25519 verifier. Absent -> integrity-only v1 behaviour."""
        toml_path = self.root / "alloy.toml"
        if not toml_path.exists():
            return {}
        return tomllib.loads(toml_path.read_text()).get("ota", {})

    def net_options(self) -> dict[str, Any]:
        """The optional ``[net]`` table from alloy.toml (lwIP feature/pool policy).

        Drives the generated ``lwipopts.h`` — features (dhcp/dns/udp), static pool
        sizes, MTU, checksum offload. Empty when absent, in which case the
        generator falls back to defaults that match the v1 hand-written config, so
        a project that never mentions [net] is byte-for-byte unchanged.
        """
        toml_path = self.root / "alloy.toml"
        if not toml_path.exists():
            return {}
        return tomllib.loads(toml_path.read_text()).get("net", {})

    def load_board(self) -> dict[str, Any]:
        if not self.board_json.exists():
            boards_dir = self.alloy_root / "boards"
            known = sorted(p.name for p in boards_dir.iterdir() if (p / "board.json").exists()) \
                if boards_dir.exists() else []
            raise ProjectError(
                f"unknown board '{self.board_id}' — known boards: {', '.join(known) or '(none)'}"
            )
        board = json.loads(self.board_json.read_text())
        if board.get("schema") != "alloy.board.v1":
            raise ProjectError(f"{self.board_json}: expected schema alloy.board.v1")
        if board.get("id") != self.board_id:
            raise ProjectError(f"{self.board_json}: id '{board.get('id')}' != directory '{self.board_id}'")
        return self.apply_overrides(board)

    def project_settings(self) -> dict[str, Any]:
        """`[roles.*]` and `[clock]` from alloy.toml — what THIS project chose."""
        return read_project_settings(self.root)

    def apply_overrides(self, board: dict[str, Any]) -> dict[str, Any]:
        return apply_project_overrides(board, self.project_settings(),
                                       self.devices_root)


def apply_project_overrides(board: dict[str, Any], settings: dict[str, Any],
                            devices_root: Path) -> dict[str, Any]:
    """The board as a project uses it.

    A board.json value is a DEFAULT for the fields a project may choose — a baud
    rate, a watchdog timeout, how much flash to reserve, the clock. Overriding
    one from alloy.toml means a framework board keeps receiving upstream fixes
    instead of being forked by `board-clone` just to change a number.

    A module function, not a method, because every consumer must apply it: the
    emitter reads the board through Project, but `board-info` resolves the file
    itself, and the first version of this let those two disagree — the header
    came out at 48 MHz while the panel still said 64.

    What may be overridden is declared in roles.py. Anything else is a property
    of the hardware, and board-validate refuses it with that reason.
    """
    from .roles import ROLES  # noqa: PLC0415

    roles = settings.get("roles") or {}
    clock = settings.get("clock") or {}
    if not roles and not clock:
        return board

    board = json.loads(json.dumps(board))  # never mutate the caller's dict
    for role, overrides in roles.items():
        spec = ROLES.get(role)
        if spec is None or role not in board.get("roles", {}):
            continue
        for key, value in (overrides or {}).items():
            if key in spec.project_fields:
                board["roles"][role][key] = value

    if clock.get("profile"):
        board.pop("clock", None)
        board["clock_profile"] = clock["profile"]
    elif clock.get("mhz"):
        from .clock_solver import solve  # noqa: PLC0415
        from .devices import load_chip  # noqa: PLC0415

        chip = load_chip(devices_root, board["chip"])
        hse = board.get("hse") or {}
        solved = solve(chip.get("family"),
                       int(round(float(clock["mhz"]) * 1_000_000)),
                       external_hz=hse.get("hz") if clock.get("hse", bool(hse)) else None)
        board["clock"] = solved["profile"]
        board["clock_profile"] = "custom"
    return board


def read_project_settings(project_root: Path | None) -> dict[str, Any]:
    """`[roles.*]` and `[clock]` from a project's alloy.toml, or nothing."""
    if project_root is None:
        return {}
    toml_path = project_root / "alloy.toml"
    if not toml_path.exists():
        return {}
    data = tomllib.loads(toml_path.read_text())
    return {"roles": data.get("roles", {}), "clock": data.get("clock", {})}


def packaged_alloy_root() -> Path | None:
    """Framework payload embedded in the installed wheel, if any."""
    payload = Path(__file__).resolve().parent / "payload"
    if (payload / "boards").is_dir() and (payload / "src" / "alloy").is_dir():
        return payload
    return None


def _packaged_devices_root() -> Path | None:
    try:
        from alloy_devices.loader import data_root
    except ImportError:
        return None
    root = data_root()
    return root if (root / "chips").is_dir() else None


def _find_alloy_root(start: Path) -> Path:
    if env := os.environ.get("ALLOY_ROOT"):
        return Path(env).resolve()
    for candidate in [start, *start.parents]:
        if (candidate / "NORTH_STAR.md").exists() and (candidate / "boards").is_dir():
            return candidate
    if packaged := packaged_alloy_root():
        return packaged
    raise ProjectError(
        "could not find the alloy framework — run inside the repo, set ALLOY_ROOT, "
        "or install the alloy package (the wheel embeds the framework)"
    )


def _find_devices_root(alloy_root: Path) -> Path:
    if env := os.environ.get("ALLOY_DEVICES_ROOT"):
        return Path(env).resolve()
    sibling = alloy_root.parent / "alloy-devices"
    if (sibling / "chips").is_dir():
        return sibling
    if packaged := _packaged_devices_root():
        return packaged
    raise ProjectError(
        "could not find the alloy-devices database — set ALLOY_DEVICES_ROOT, "
        f"clone it next to the framework ({sibling}), or install the "
        "alloy-devices package"
    )


def load_project(project_dir: Path, board_override: str | None = None) -> Project:
    root = project_dir.resolve()
    toml_path = root / "alloy.toml"
    if not toml_path.exists():
        raise ProjectError(f"{root} is not an alloy project (no alloy.toml)")
    data = tomllib.loads(toml_path.read_text())
    try:
        name = data["project"]["name"]
        board_id = board_override or data["board"]["id"]
    except KeyError as exc:
        raise ProjectError(f"{toml_path}: missing required key {exc}") from exc

    # Out-of-repo projects: `alloy new` records the framework root it was
    # scaffolded against; in-repo examples keep discovery by walking up.
    if recorded_root := data.get("alloy", {}).get("root"):
        alloy_root = Path(recorded_root).expanduser().resolve()
        if not (alloy_root / "boards").is_dir():
            raise ProjectError(
                f"{toml_path}: [alloy] root = {recorded_root} does not look like "
                "an alloy checkout (no boards/ dir)"
            )
    else:
        alloy_root = _find_alloy_root(root)
    return Project(
        root=root,
        name=name,
        board_id=board_id,
        alloy_root=alloy_root,
        devices_root=_find_devices_root(alloy_root),
    )
