"""examples/factory/line.py — the mass-programming driver.

The script itself has never driven a probe (no board was on hand), so what is
tested here is everything that is pure and everything that is a REFUSAL: the
serial ledger, the work-order parser, the step order, and the exact argv of each
step. Those are the parts that brick a batch when they are wrong.

The most important assertion in this file is the order one: `secure` must be
last. A board locked at RDP 1 before it was provisioned is scrap — the probe can
no longer reach flash, and getting back to level 0 mass-erases it.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path
from types import ModuleType

import pytest

_LINE_PY = (Path(__file__).resolve().parents[3] / "examples" / "factory" / "line.py")


def _load() -> ModuleType:
    spec = importlib.util.spec_from_file_location("alloy_factory_line", _LINE_PY)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


line = _load()


def _row(**kw):
    return line.WorkOrderRow(**{"serial": "ALY-0001", **kw})


def _cmd(step: str, row=None, **over) -> list[list[str]]:
    kwargs = {
        "board": "nucleo_g071rb",
        "row": row or _row(),
        "bootloader_project": Path("examples/bootloader_uart"),
        "linetest_project": Path("examples/factory"),
        "product_image": Path("/tmp/product.img"),
        "linetest_image": Path("/tmp/linetest.img"),
        "port": "/dev/ttyACM0",
    }
    kwargs.update(over)
    return line.step_commands(step, **kwargs)


# ── the order ───────────────────────────────────────────────────────────────


def test_secure_is_the_last_step() -> None:
    """The whole reason this script exists. `alloy secure apply` sets RDP 1
    (probe loses flash access) and write-protects the bootloader region, which
    on uniform-page flash contains the identity page. Anything after it is
    impossible; anything skipped before it is permanent."""
    assert line.STEP_ORDER[-1] == "secure"


def test_provisioning_comes_before_the_line_test_and_before_secure() -> None:
    order = list(line.STEP_ORDER)
    assert order.index("provision") < order.index("line-test")
    assert order.index("provision") < order.index("secure")


def test_the_bootloader_is_first() -> None:
    assert line.STEP_ORDER[0] == "flash-bootloader"


def test_an_unknown_step_is_refused_rather_than_skipped() -> None:
    with pytest.raises(line.LineError, match="unknown step"):
        _cmd("burn-in")


# ── the commands ────────────────────────────────────────────────────────────


def test_only_the_bootloader_needs_the_probe_to_load_code() -> None:
    bl = _cmd("flash-bootloader")[0]
    assert "flash" in bl and "--slot" in bl and "bl" in bl
    for step in ("load-line-test", "load-product"):
        argv = _cmd(step)[0]
        assert "update" in argv, f"{step} should go over the field path"
        assert "flash" not in argv


def test_provision_passes_every_optional_field_through() -> None:
    argv = _cmd("provision", _row(mac="02:1a:2b:3c:4d:5e", hw_rev=3, batch=42))[0]
    assert "--serial" in argv and "ALY-0001" in argv
    assert "--mac" in argv and "02:1a:2b:3c:4d:5e" in argv
    assert "--hw-rev" in argv and "3" in argv
    assert "--batch" in argv and "42" in argv


def test_provision_omits_fields_that_were_not_ordered() -> None:
    argv = _cmd("provision", _row())[0]
    assert "--mac" not in argv and "--hw-rev" not in argv and "--batch" not in argv


def test_the_line_test_is_not_a_subprocess() -> None:
    """It is a UART read with a verdict, not a command — a step that shells out
    to something that always exits 0 would be a step that never fails."""
    assert _cmd("line-test") == []


def test_secure_asks_for_rdp1_and_bootloader_wrp() -> None:
    argv = _cmd("secure")[0]
    assert "--rdp" in argv and "1" in argv
    assert "--wrp-bootloader" in argv


# ── the work order ──────────────────────────────────────────────────────────


def _write(tmp_path: Path, text: str) -> Path:
    path = tmp_path / "serials.csv"
    path.write_text(text)
    return path


def test_work_order_parses_optional_columns(tmp_path: Path) -> None:
    path = _write(tmp_path, "serial,mac,hw_rev,batch\n"
                            "ALY-0001,02:00:00:00:00:01,3,42\n"
                            "ALY-0002,,,\n")
    rows = line.read_work_order(path)
    assert [r.serial for r in rows] == ["ALY-0001", "ALY-0002"]
    assert rows[0].mac == "02:00:00:00:00:01"
    assert rows[0].hw_rev == 3 and rows[0].batch == 42
    assert rows[1].mac is None and rows[1].hw_rev == 0


def test_a_duplicate_serial_in_the_work_order_is_refused(tmp_path: Path) -> None:
    """The spreadsheet copy-paste. Two devices with one serial is discovered
    months later by a fleet server and cannot be fixed in the field."""
    path = _write(tmp_path, "serial\nALY-0001\nALY-0002\nALY-0001\n")
    with pytest.raises(line.LineError, match="already appears on line 2"):
        line.read_work_order(path)


def test_a_work_order_without_a_serial_column_is_refused(tmp_path: Path) -> None:
    path = _write(tmp_path, "sn,mac\nALY-0001,\n")
    with pytest.raises(line.LineError, match="serial. column"):
        line.read_work_order(path)


def test_an_empty_serial_cell_is_refused(tmp_path: Path) -> None:
    # A whitespace-only cell, not a blank line: csv skips blank lines, but a
    # row of spaces is exactly what a spreadsheet export produces.
    path = _write(tmp_path, "serial,mac\nALY-0001,\n  ,02:00:00:00:00:01\n")
    with pytest.raises(line.LineError, match="empty serial"):
        line.read_work_order(path)


# ── the ledger ──────────────────────────────────────────────────────────────


def test_the_ledger_resumes_at_the_next_unused_serial(tmp_path: Path) -> None:
    rows = [_row(serial="A"), _row(serial="B"), _row(serial="C")]
    path = tmp_path / "serials.done"
    ledger = line.Ledger(path)
    assert ledger.next_row(rows).serial == "A"
    ledger.consume("A")
    assert ledger.next_row(rows).serial == "B"
    # A crash here: a NEW Ledger over the same file must not hand out A again.
    assert line.Ledger(path).next_row(rows).serial == "B"


def test_the_ledger_is_flushed_after_every_board(tmp_path: Path) -> None:
    """The failure this exists for is the line PC losing power mid-batch, so the
    file has to be right after each board, not at the end of the run."""
    path = tmp_path / "serials.done"
    ledger = line.Ledger(path)
    ledger.consume("A")
    assert path.read_text().split() == ["A"]


def test_a_serial_can_never_be_consumed_twice(tmp_path: Path) -> None:
    ledger = line.Ledger(tmp_path / "serials.done")
    ledger.consume("A")
    with pytest.raises(line.LineError, match="already on a board"):
        ledger.consume("A")


def test_an_exhausted_work_order_stops_the_line(tmp_path: Path) -> None:
    rows = [_row(serial="A")]
    ledger = line.Ledger(tmp_path / "serials.done")
    ledger.consume("A")
    with pytest.raises(line.LineError, match="will not reuse a serial"):
        ledger.next_row(rows)
