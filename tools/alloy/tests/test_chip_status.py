"""Unit tests for the `alloy chip-status` coverage scoreboard.

The point of the verb is that nothing in it is hand-maintained, so the tests are
built on a FAKE tree whose answer is known by construction: four peripherals, one
register file, one HAL header, one board. If the derivation ever starts guessing
instead of reading, these numbers move.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from alloy_cli import chip_status as cs
from alloy_cli.emit import renode
from alloy_cli.emit.common import EmitError

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"
_have_devices = (DEVICES_ROOT / "chips").is_dir()
skip_no_devices = pytest.mark.skipif(
    not _have_devices, reason="alloy-devices not beside the framework")

# One curated IP that a HAL driver AND a Renode model exist for (st/usart_v4 is
# in RENODE_UART), one curated IP with neither, one uncurated stub, one instance
# whose `ip` names a register file the database does NOT ship, plus a GPIO port
# reached only through a role's pin.
_CHIP = """\
schema: alloy.chip.v1
vendor: fake
part: FAKE123
family: fakefam
cores:
  - name: cm0plus
    arch: armv6m
peripherals:
  usart1:
    ip: st/usart_v4
    base: '0x40004400'
  usart2:
    ip: st/usart_v4
    base: '0x40004800'
  crc:
    ip: fake/crc_v1
    base: '0x40023000'
  wwdg:
    base: '0x40002c00'
    uncurated: true
  ghost:
    ip: fake/not_shipped
    base: '0x40009000'
  gpioa:
    ip: fake/gpio_v9
    base: '0x50000000'
pins:
  pa5:
    port: a
    index: 5
  pa2:
    port: a
    index: 2
"""

_BOARD = {
    "schema": "alloy.board.v1",
    "id": "fakeboard",
    "name": "Fake",
    "chip": "fake/fake123",
    "clock_profile": "hsi",
    "roles": {
        "led": {"pin": "pa5", "active": "high"},
        "debug_uart": {"peripheral": "usart2", "tx": "pa2", "rx": "pa5"},
    },
}

# A board on a DIFFERENT chip must not contribute a single binding.
_OTHER_BOARD = dict(_BOARD, id="otherboard", chip="fake/somethingelse")


def _tree(tmp_path: Path) -> tuple[Path, Path]:
    devices = tmp_path / "devices"
    (devices / "chips" / "fake").mkdir(parents=True)
    (devices / "chips" / "fake" / "fake123.yaml").write_text(_CHIP)
    regs = devices / "registers"
    (regs / "st").mkdir(parents=True)
    (regs / "st" / "usart_v4.yaml").write_text(
        "schema: alloy.ip.v1\nvendor: st\nip: usart_v4\nclass: uart\nregisters: []\n")
    (regs / "fake").mkdir(parents=True)
    (regs / "fake" / "crc_v1.yaml").write_text(
        "schema: alloy.ip.v1\nvendor: fake\nip: crc_v1\nclass: crc\nregisters: []\n")
    (regs / "fake" / "gpio_v9.yaml").write_text(
        "schema: alloy.ip.v1\nvendor: fake\nip: gpio_v9\nclass: gpio\nregisters: []\n")

    alloy = tmp_path / "alloy"
    (alloy / "src" / "alloy" / "hal" / "uart").mkdir(parents=True)
    (alloy / "src" / "alloy" / "hal" / "uart" / "st_usart_v4.hpp").write_text("// stub\n")
    (alloy / "boards" / "fakeboard").mkdir(parents=True)
    (alloy / "boards" / "fakeboard" / "board.json").write_text(json.dumps(_BOARD))
    (alloy / "boards" / "otherboard").mkdir(parents=True)
    (alloy / "boards" / "otherboard" / "board.json").write_text(json.dumps(_OTHER_BOARD))
    return alloy, devices


def _rows(status: dict) -> dict[str, dict]:
    return {r["name"]: r for r in status["peripherals"]}


def test_fake_tree_has_the_answer_it_was_built_to_have(tmp_path: Path) -> None:
    status = cs.chip_status(*_tree(tmp_path), "fake/fake123")
    assert status["schema"] == "alloy.chip_status.v1"
    rows = _rows(status)

    # curated + driver + Renode model, all three derived from real trees.
    assert rows["usart1"]["curated"] is True
    assert rows["usart1"]["class"] == "uart"
    assert rows["usart1"]["driver"] == "alloy/hal/uart/st_usart_v4.hpp"
    assert rows["usart1"]["renode_model"] == renode.RENODE_UART["st/usart_v4"]

    # curated, but the framework ships no alloy/hal/crc/fake_crc_v1.hpp.
    assert (rows["crc"]["curated"], rows["crc"]["driver"]) == (True, None)
    assert rows["crc"]["renode_model"] is None

    # the data's own `uncurated: true`.
    assert rows["wwdg"]["curated"] is False

    # an `ip` the database does not ship is NOT allowed to flatter the count.
    assert rows["ghost"]["ip"] == "fake/not_shipped"
    assert (rows["ghost"]["curated"], rows["ghost"]["driver"]) == (False, None)


def test_role_reachability_covers_peripherals_and_pins(tmp_path: Path) -> None:
    status = cs.chip_status(*_tree(tmp_path), "fake/fake123")
    rows = _rows(status)
    assert rows["usart2"]["roles"] == [{"board": "fakeboard", "role": "debug_uart"}]
    assert rows["usart1"]["roles"] == []  # same IP, but no board binds it
    # gpioa is named by NO role; it is reached because pa5/pa2 are.
    assert {b["role"] for b in rows["gpioa"]["roles"]} == {"led", "debug_uart"}
    assert status["boards"] == ["fakeboard"]  # the other board is a different chip


def test_summary_counts_instances_and_ips(tmp_path: Path) -> None:
    s = cs.chip_status(*_tree(tmp_path), "fake/fake123")["summary"]
    assert (s["peripherals"], s["curated"], s["uncurated"]) == (6, 4, 2)
    assert s["with_driver"] == 2          # usart1 + usart2 share one header
    assert s["with_renode_model"] == 2
    assert s["board_reachable"] == 2      # usart2 + gpioa
    # Two instances of one IP is ONE curation job — that is why both are shown.
    assert (s["ips_curated"], s["ips_with_driver"]) == (3, 1)
    assert s["headline"] == "4 of 6 peripherals curated, 2 with drivers"


def test_project_boards_are_seen_and_shadow_framework_ones(tmp_path: Path) -> None:
    alloy, devices = _tree(tmp_path)
    project = tmp_path / "proj"
    (project / "boards" / "myboard").mkdir(parents=True)
    (project / "boards" / "myboard" / "board.json").write_text(json.dumps(
        dict(_BOARD, id="myboard", roles={"i2c": {"peripheral": "crc"}})))
    rows = _rows(cs.chip_status(alloy, devices, "fake/fake123", project))
    assert rows["crc"]["roles"] == [{"board": "myboard", "role": "i2c"}]


def test_format_states_the_headline_and_what_it_does_not_count(tmp_path: Path) -> None:
    text = cs.format_status(cs.chip_status(*_tree(tmp_path), "fake/fake123"))
    assert "4 of 6 peripherals curated, 2 with drivers" in text
    assert "None of these is evidence from silicon." in text
    assert "usart1" in text and "st/usart_v4" in text


def test_unknown_chip_names_the_near_misses(tmp_path: Path) -> None:
    _, devices = _tree(tmp_path)
    with pytest.raises(EmitError, match="fake/fake123"):
        cs.resolve_chip_id(devices, "fake/fake1")
    with pytest.raises(EmitError, match="alloy chips"):
        cs.resolve_chip_id(devices, "nosuchvendor/x")


def test_renode_models_unions_every_ip_table() -> None:
    """The anti-rot guard for the one hand-written input: a new RENODE_* table
    that `renode_models()` forgets would make chip-status quietly under-report."""
    models = renode.renode_models()
    tables = {n: t for n, t in vars(renode).items()
              if n.startswith("RENODE_") and isinstance(t, dict)}
    assert "RENODE_UART" in tables and "RENODE_DMA" in tables
    for name, table in tables.items():
        if name == "RENODE_CPU":
            continue  # keyed by core arch, not by IP — deliberately not a model
        missing = {k: v for k, v in table.items() if models.get(k) != v}
        assert not missing, f"{name} entries missing from renode_models(): {missing}"


@skip_no_devices
def test_real_g0b1re_scoreboard_is_derived_not_asserted() -> None:
    status = cs.chip_status(ALLOY_ROOT, DEVICES_ROOT, "st/stm32g0b1re")
    rows = _rows(status)
    # The board's own roles must be visible, or "reachable" means nothing.
    assert "nucleo_g0b1re" in status["boards"]
    assert {b["role"] for b in rows["usart2"]["roles"]} == {"debug_uart"}
    assert rows["usart2"]["driver"] == "alloy/hal/uart/st_usart_v4.hpp"
    # The 100% push's gap: USB is real silicon with no register data.
    assert rows["usb"]["curated"] is False
    s = status["summary"]
    assert s["peripherals"] == s["curated"] + s["uncurated"] > 60
    assert 0 < s["curated"] < s["peripherals"]  # neither empty nor a fake 100%
