"""Build one source tree for every board — the framework's claim, as a command.

alloy's whole promise is that the same `main.cpp` recompiles for another MCU by
changing one line. Until now the only place that was ever demonstrated was CI.
`alloy matrix` runs it locally: same sources, every board, one table of what
built and what it costs.

It never stops at the first failure. A missing toolchain for one family is an
ordinary state (few people have all four installed), and a sweep that aborted
there would hide the seven boards that do work.
"""

from __future__ import annotations

import os
import time
from pathlib import Path
from typing import Any

from .emit import generate
from .emit.common import EmitError
from .project import Project, load_project


def known_boards(alloy_root: Path, project_root: Path | None) -> list[str]:
    """Every board id this project could target: the framework's, plus any the
    project defines locally (which shadow same-named framework ones)."""
    ids: set[str] = set()
    for root in (alloy_root, project_root):
        boards = root / "boards" if root else None
        if boards and boards.is_dir():
            ids |= {b.name for b in boards.iterdir() if (b / "board.json").exists()}
    return sorted(ids)


def _reason(exc: BaseException) -> str:
    """One actionable line out of a build failure. A cmake log in a table cell
    helps nobody; the terminal already has the full output."""
    import subprocess  # noqa: PLC0415

    if isinstance(exc, subprocess.CalledProcessError):
        return "compilation failed — see the build output above"
    text = " ".join(str(exc).split())
    return text[:200] if text else exc.__class__.__name__


def build_matrix(project_root: Path, board_ids: list[str],
                 quiet: bool = False, db: Any = None) -> dict[str, Any]:
    """Build every board in turn. Returns the alloy.matrix.v1 envelope.

    `db` lets a caller that already validated the device database pass it in —
    loading it costs more than several of the builds it precedes.
    """
    from alloy_devices.loader import load_database  # noqa: PLC0415

    from .build import build  # noqa: PLC0415
    from .sizes import size_report  # noqa: PLC0415

    rows: list[dict[str, Any]] = []
    for board_id in board_ids:
        started = time.monotonic()
        row: dict[str, Any] = {
            "board": board_id, "chip": None, "ok": False, "seconds": 0.0,
            "flash": None, "ram": None, "error": None,
        }
        project: Project | None = None
        try:
            project = load_project(project_root, board_override=board_id)
            if db is None:
                db = load_database(project.devices_root)
            board = project.load_board()
            row["chip"] = board["chip"]
            generate(project, db)
            elf = build(project, db.chips[board["chip"]])
            report = size_report(project, db.chips[board["chip"]], elf)
            row["flash"] = report["flash"]
            row["ram"] = report["ram"]
            row["ok"] = True
        except (EmitError, OSError, KeyError, ValueError, RuntimeError) as exc:
            row["error"] = _reason(exc)
        except Exception as exc:  # noqa: BLE001 - a failing board must not end the sweep
            row["error"] = _reason(exc)
        row["seconds"] = round(time.monotonic() - started, 2)
        rows.append(row)
        if not quiet:
            mark = "ok  " if row["ok"] else "FAIL"
            print(f"  {mark} {board_id}", flush=True)

    return {
        "schema": "alloy.matrix.v1",
        "boards": rows,
        "built": sum(1 for r in rows if r["ok"]),
        "failed": sum(1 for r in rows if not r["ok"]),
        "ok": all(r["ok"] for r in rows),
    }


def run_matrix(project_root: Path, board_ids: list[str],
               as_json: bool, db: Any = None) -> dict[str, Any]:
    """build_matrix with the child-process output kept off stdout when the
    caller wants a machine-readable envelope (cmake and ninja write to fd 1
    directly, where a Python-level redirect cannot reach)."""
    if not as_json:
        return build_matrix(project_root, board_ids, db=db)
    saved = os.dup(1)
    try:
        os.dup2(2, 1)
        return build_matrix(project_root, board_ids, quiet=True, db=db)
    finally:
        os.dup2(saved, 1)
        os.close(saved)


def _kb(value: int | None) -> str:
    return "—" if value is None else f"{value / 1024:.1f}K"


def format_table(report: dict[str, Any]) -> str:
    """The sweep as a table: the point is comparing boards, so align them."""
    width = max((len(r["board"]) for r in report["boards"]), default=5)
    lines = [f"{'board'.ljust(width)}  {'flash':>16}  {'ram':>16}  time"]
    for row in report["boards"]:
        if not row["ok"]:
            lines.append(f"{row['board'].ljust(width)}  {row['error']}")
            continue
        flash, ram = row["flash"], row["ram"]
        lines.append(
            f"{row['board'].ljust(width)}  "
            f"{_kb(flash['used']) + ' / ' + _kb(flash['total']):>16}  "
            f"{_kb(ram['used']) + ' / ' + _kb(ram['total']):>16}  "
            f"{row['seconds']:.1f}s")
    lines.append("")
    lines.append(f"{report['built']} of {len(report['boards'])} boards — "
                 f"the same src/, no #ifdef")
    return "\n".join(lines)
