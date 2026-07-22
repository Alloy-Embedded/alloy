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
        return self.alloy_root / "boards" / self.board_id / "board.json"

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
        return board


def _find_alloy_root(start: Path) -> Path:
    if env := os.environ.get("ALLOY_ROOT"):
        return Path(env).resolve()
    for candidate in [start, *start.parents]:
        if (candidate / "NORTH_STAR.md").exists() and (candidate / "boards").is_dir():
            return candidate
    raise ProjectError(
        "could not find the alloy framework root — run inside the repo or set ALLOY_ROOT"
    )


def _find_devices_root(alloy_root: Path) -> Path:
    if env := os.environ.get("ALLOY_DEVICES_ROOT"):
        return Path(env).resolve()
    sibling = alloy_root.parent / "alloy-devices"
    if (sibling / "chips").is_dir():
        return sibling
    raise ProjectError(
        "could not find the alloy-devices database — set ALLOY_DEVICES_ROOT or "
        f"clone it next to the framework ({sibling})"
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

    alloy_root = _find_alloy_root(root)
    return Project(
        root=root,
        name=name,
        board_id=board_id,
        alloy_root=alloy_root,
        devices_root=_find_devices_root(alloy_root),
    )
