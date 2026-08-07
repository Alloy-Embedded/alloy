"""Unit tests for `alloy matrix`.

Building nine boards for real belongs in CI, not in a unit test, so the compile
step is substituted here. What is checked is the behaviour that makes the verb
trustworthy: a board that fails does not end the sweep, its reason is one usable
line, and the table reports what was actually attempted.
"""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

from alloy_cli import matrix
from alloy_cli.emit.common import EmitError

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


class _FakeDb:
    """Just `.chips` — loading and validating all 412 costs more than the whole
    sweep this test stands in for."""

    def __init__(self, chips: dict[str, dict]) -> None:
        self.chips = chips


def _fake_db(*chip_ids: str) -> _FakeDb:
    from alloy_cli.devices import load_chip

    return _FakeDb({c: load_chip(DEVICES_ROOT, c) for c in chip_ids})


def _project(tmp_path: Path, board: str = "nucleo_g071rb") -> Path:
    root = tmp_path / "proj"
    (root / "src").mkdir(parents=True)
    (root / "src" / "main.cpp").write_text("int main() { return 0; }\n")
    (root / "alloy.toml").write_text(
        f'[project]\nname = "proj"\n\n[board]\nid = "{board}"\n\n'
        f'[alloy]\nroot = "{ALLOY_ROOT}"\n')
    return root


# ------------------------------------------------------------- board discovery

@skip_no_devices
def test_known_boards_includes_the_frameworks_own(tmp_path: Path) -> None:
    ids = matrix.known_boards(ALLOY_ROOT, _project(tmp_path))
    assert "nucleo_g071rb" in ids and "same70_xplained" in ids


@skip_no_devices
def test_a_project_board_joins_the_sweep(tmp_path: Path) -> None:
    """A custom board is exactly the thing whose portability you want checked."""
    root = _project(tmp_path)
    (root / "boards" / "my_widget").mkdir(parents=True)
    (root / "boards" / "my_widget" / "board.json").write_text("{}")
    assert "my_widget" in matrix.known_boards(ALLOY_ROOT, root)


# ---------------------------------------------------------------- the sweep

@skip_no_devices
def test_one_failing_board_does_not_end_the_sweep(tmp_path: Path, monkeypatch) -> None:
    """Few people have all four toolchains installed. A sweep that stopped at
    the first missing one would hide every board that does work."""
    attempted: list[str] = []

    def fake_build(project, _chip):
        attempted.append(project.board_id)
        if project.board_id == "same70_xplained":
            raise EmitError("toolchain not found")
        return Path("nonexistent.elf")

    monkeypatch.setattr("alloy_cli.build.build", fake_build)
    # matrix.py binds `generate` at import time, so patch the name IT uses —
    # patching alloy_cli.emit.generate would silently run the real codegen.
    monkeypatch.setattr("alloy_cli.matrix.generate", lambda *_a, **_k: [])

    report = matrix.build_matrix(
        _project(tmp_path),
        ["nucleo_g071rb", "same70_xplained", "nucleo_f722ze"], quiet=True,
        db=_fake_db("st/stm32g071rb", "microchip/atsame70q21", "st/stm32f722"))

    assert attempted == ["nucleo_g071rb", "same70_xplained", "nucleo_f722ze"], \
        "every board must be attempted, in order"
    assert report["schema"] == "alloy.matrix.v1"
    assert report["ok"] is False and report["failed"] == 1
    failed = next(r for r in report["boards"] if not r["ok"])
    assert failed["board"] == "same70_xplained"
    assert "toolchain not found" in failed["error"]
    # The others still carry their chip and a duration.
    ok_rows = [r for r in report["boards"] if r["ok"]]
    assert len(ok_rows) == 2
    assert all(r["chip"] and r["seconds"] >= 0 for r in ok_rows)


@skip_no_devices
def test_an_unknown_board_is_a_row_not_a_crash(tmp_path: Path) -> None:
    report = matrix.build_matrix(_project(tmp_path), ["not_a_board"],
                                 quiet=True, db=_FakeDb({}))
    assert report["failed"] == 1
    assert "not_a_board" in report["boards"][0]["error"]


def test_a_compiler_failure_reads_as_one_line() -> None:
    """A cmake log in a table cell helps nobody; the terminal already has it."""
    reason = matrix._reason(
        subprocess.CalledProcessError(1, ["cmake", "--build", "."]))
    assert reason == "compilation failed — see the build output above"
    assert "\n" not in matrix._reason(EmitError("a\nmultiline\nmessage"))


# ----------------------------------------------------------------- the table

def test_the_table_shows_failures_and_totals() -> None:
    report = {
        "schema": "alloy.matrix.v1",
        "boards": [
            {"board": "nucleo_g071rb", "chip": "st/stm32g071rb", "ok": True,
             "seconds": 2.1, "error": None,
             "flash": {"used": 2355, "total": 131072},
             "ram": {"used": 2150, "total": 36864}},
            {"board": "esp32_devkit", "chip": "espressif/esp32", "ok": False,
             "seconds": 0.3, "flash": None, "ram": None,
             "error": "xtensa toolchain not installed"},
        ],
        "built": 1, "failed": 1, "ok": False,
    }
    table = matrix.format_table(report)
    assert "2.3K / 128.0K" in table
    assert "xtensa toolchain not installed" in table
    assert "1 of 2 boards" in table
    # The claim being demonstrated is worth stating.
    assert "no #ifdef" in table
