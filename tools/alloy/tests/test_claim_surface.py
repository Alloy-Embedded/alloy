"""The claim inventory, checked against the code instead of asserted in prose.

Hole (A) — two binders naming one peripheral instance, both opening, the second
silently reprogramming the port under the first handle — was declared closed
twice, and both times the declaration was a paragraph. The second review found
the identical defect still live in shipped code simply by asking the facades,
one at a time, whether they actually had the claim the page said they had: five
of twelve did.

So the page's inventory table is load-bearing now. These tests parse
`docs/reference/peripheral-surface.md`'s "Which facade claims what" table and
check it against `src/alloy/`. Two ways to fail, and both are the failure that
happened:

  * a facade that claims LESS than its row says      -> the (A2) defect,
  * a peripheral class with NO row at all            -> a facade nobody asked
                                                        the question about.

They are grep-shaped on purpose. A test that instantiated the facades would
need a chip, a clock tree and an MMIO window, which is why nothing checked this
for two rounds.
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest

ALLOY_ROOT = Path(__file__).resolve().parents[3]
SURFACE_DOC = ALLOY_ROOT / "docs" / "reference" / "peripheral-surface.md"

# Which file backs each row of the table. Most rows are a Tier-1 facade header;
# `exti` is the exception and the reason this mapping is explicit rather than
# derived from the row name — an EXTI LINE is claimed by the driver, because the
# line number only exists down there (see src/alloy/hal/exti/exti_impl.hpp).
ROW_SOURCE = {
    "uart": "src/alloy/uart.hpp",
    "i2c": "src/alloy/i2c.hpp",
    "spi": "src/alloy/spi.hpp",
    "adc": "src/alloy/adc.hpp",
    "pwm": "src/alloy/pwm.hpp",
    "dma": "src/alloy/dma.hpp",
    "wdt": "src/alloy/wdt.hpp",
    "wwdt": "src/alloy/wwdt.hpp",
    "dac": "src/alloy/dac.hpp",
    "can": "src/alloy/can.hpp",
    "rtc": "src/alloy/rtc.hpp",
    "flash": "src/alloy/flash.hpp",
    "gpio": "src/alloy/gpio.hpp",
    "encoder": "src/alloy/encoder.hpp",
    "exti": "src/alloy/hal/exti/exti_impl.hpp",
    "ethernet": None,  # a role and a generated board::eth, no portable facade
}

# Every peripheral CLASS alloy has a driver for owes the table a row. This is
# the half that catches a new facade arriving with no claim and no argument for
# not having one — the directory has to be created either way.
HAL_DIR_TO_ROW = {
    "adc": "adc",
    "can": "can",
    "dac": "dac",
    "dma": "dma",
    "encoder": "encoder",
    "exti": "exti",
    "flash": "flash",
    "gpio": "gpio",
    "i2c": "i2c",
    "net": "ethernet",
    "pwm": "pwm",
    "rtc": "rtc",
    "spi": "spi",
    "tick": "tick",
    "uart": "uart",
    "watchdog": "wdt",
    "window_watchdog": "wwdt",
}

# Column text -> the call that must appear in the source. `—` and *none* mean
# the opposite assertion: the file must contain no claim of that shape at all.
SHAPE_CALL = {
    "exclusive": "claim::exclusive<",
    "shared": "claim::shared<",
    "sub_exclusive": "claim::sub_exclusive<",
    "sub_shared": "claim::sub_shared<",
}
EMPTY = {"—", "-", "*none*", "none", ""}


def _table_rows() -> dict[str, tuple[str, str]]:
    """Row name -> (instance scope cell, sub-resource scope cell)."""
    text = SURFACE_DOC.read_text()
    section = text.split("Which facade claims what", 1)[1]
    rows: dict[str, tuple[str, str]] = {}
    for line in section.splitlines():
        if not line.startswith("|"):
            if rows:
                break  # the table ended; anything after it is a different table
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 4 or cells[0].startswith("---") or cells[0] == "Facade":
            continue
        name = cells[0].strip("`*").split()[0].strip("`")
        rows[name] = (cells[2], cells[3])
    return rows


def _shape_of(cell: str) -> str | None:
    """The claim shape a table cell names, or None for a cell that names none."""
    if cell.replace("*", "").strip() in EMPTY:
        return None
    for shape in ("sub_exclusive", "sub_shared", "exclusive", "shared"):
        if cell.startswith(shape) or cell.startswith(f"`{shape}"):
            return shape
    raise AssertionError(f"unrecognised claim shape in the surface table: {cell!r}")


def test_the_table_parses_at_all() -> None:
    rows = _table_rows()
    assert "uart" in rows and "pwm" in rows, (
        "the 'Which facade claims what' table did not parse — fix this test, "
        "not the docs, if the table moved"
    )


@pytest.mark.parametrize("row", sorted(ROW_SOURCE))
def test_each_row_matches_the_code_it_describes(row: str) -> None:
    """The (A2) failure, mechanised: a facade claiming less than its row says."""
    rows = _table_rows()
    assert row in rows, f"{row} lost its row in the surface table"
    source = ROW_SOURCE[row]
    if source is None:
        return
    path = ALLOY_ROOT / source
    if not path.exists():
        # A row whose file does not exist is a facade the page describes and the
        # tree does not have yet. Harmless in that direction — the check that
        # cannot be skipped is the other one, which walks the DRIVER
        # directories, so a facade that ships without a row still fails.
        pytest.skip(f"{source} is not in this tree")
    text = path.read_text()
    for cell, scope in zip(rows[row], ("instance", "sub-resource")):
        shape = _shape_of(cell)
        if shape is None:
            # An empty cell is checked in ONE direction only, deliberately. A
            # facade that grows a claim this page has not caught up with is a
            # documentation lag; a facade that LOST a claim this page promises
            # is hole (A2). The two facades that promise nothing outright are
            # asserted by name below, where the promise is the whole point.
            continue
        call = SHAPE_CALL[shape]
        assert call in text, (
            f"the surface table says {row} claims {shape} at {scope} scope, and "
            f"{source} does not call {call} — this is exactly how the (A2) "
            f"defect survived the first repair"
        )


def test_the_two_facades_that_promise_nothing_still_promise_nothing() -> None:
    """`flash` and `gpio` claim nothing, and that is an argued decision.

    `flash` has no configuring entry point at all — the array is on at reset and
    `erase_page`/`program` only move data, so a claim there would sit in the
    inner loop of every settings write and buy nothing. A pin is a third
    ownership scope this mechanism does not model, and boards alloy ships
    deliberately route two roles to one pin. A claim appearing in either file is
    that argument being reversed by accident rather than on purpose — the EXTI
    line's claim lives in the driver, not in `gpio.hpp`, for exactly this reason.
    """
    for name in ("flash", "gpio"):
        text = (ALLOY_ROOT / "src" / "alloy" / f"{name}.hpp").read_text()
        for shape, call in SHAPE_CALL.items():
            assert call not in text, (
                f"src/alloy/{name}.hpp now makes a {shape} claim; the surface "
                f"table says it makes none, and the reason is argued there"
            )


def test_every_peripheral_class_owes_the_table_a_row() -> None:
    """A driver directory is the earliest moment a new peripheral class exists.

    Adding one without adding a row is how a facade ships with the ownership
    question never asked. `net` maps to the `ethernet` row, which says out loud
    that it has no portable facade to hang a runtime claim on.
    """
    hal = ALLOY_ROOT / "src" / "alloy" / "hal"
    classes = sorted(p.name for p in hal.iterdir() if p.is_dir())
    unknown = [c for c in classes if c not in HAL_DIR_TO_ROW]
    assert not unknown, (
        f"peripheral classes with no entry in this test's map: {unknown}. Add "
        f"the row to docs/reference/peripheral-surface.md first, then map it."
    )
    rows = _table_rows()
    missing = sorted({HAL_DIR_TO_ROW[c] for c in classes} - set(rows))
    assert not missing, f"peripheral classes with no row in the surface table: {missing}"


def test_the_release_is_offered_at_one_scope_only() -> None:
    """`sub_release` exists; `release` deliberately does not.

    Nothing in alloy gives a whole peripheral back — there is no close() and no
    lifetime story for one. The EXTI line is the single resource with a shipped
    disarm, so the release lives at the sub scope, alone. A `release<Inst, P>()`
    appearing here would mean that decision was reversed by accident.
    """
    text = (ALLOY_ROOT / "src" / "alloy" / "core" / "claim.hpp").read_text()
    assert "inline void sub_release(" in text
    assert "inline void release(" not in text
