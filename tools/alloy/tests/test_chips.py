"""Unit tests for the chip catalogue + clean-board scaffolding."""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from alloy_cli import chips
from alloy_cli.emit.common import EmitError

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

_have_devices = (DEVICES_ROOT / "chips").is_dir()
skip_no_devices = pytest.mark.skipif(not _have_devices, reason="alloy-devices not beside the framework")


@skip_no_devices
def test_list_chips_finds_hundreds_across_vendors() -> None:
    rows = chips.list_chips(DEVICES_ROOT)
    assert len(rows) > 100
    vendors = {r["vendor"] for r in rows}
    assert {"st", "espressif"} <= vendors
    ids = {r["id"] for r in rows}
    assert "st/stm32g071rb" in ids
    for r in rows[:5]:
        assert set(r) >= {"id", "vendor", "chip", "family"}


@skip_no_devices
def test_chip_clock_returns_profiles_and_safe_default() -> None:
    profiles, default = chips.chip_clock(DEVICES_ROOT, "st/stm32g071rb")
    assert "hsi_16mhz" in profiles
    assert default == profiles[0]  # boot-safe profile is first


@skip_no_devices
def test_chip_clock_rejects_unknown_chip() -> None:
    with pytest.raises(EmitError):
        chips.chip_clock(DEVICES_ROOT, "st/nope-not-a-chip")


def test_clean_board_json_is_valid_and_minimal() -> None:
    board = json.loads(chips.clean_board_json("myboard", "st/stm32g071rb", "hsi_16mhz"))
    assert board["schema"] == "alloy.board.v1"
    assert board["id"] == "myboard"
    assert board["chip"] == "st/stm32g071rb"
    assert board["clock_profile"] == "hsi_16mhz"
    assert board["roles"] == {}  # clean: the user fills these in


@skip_no_devices
def test_chip_info_pins_carry_grid_position_and_functions() -> None:
    # A builder-generated chip has the full AF matrix — the CubeMX-style pin
    # picker's data. Every pin must place itself (port/index) and pa0 on the
    # G474 must offer its documented alternates (tim2 ch1 on AF1 among them).
    info = chips.chip_info(DEVICES_ROOT, "st/stm32g474re")
    assert info["schema"] == "alloy.chip_info.v1"
    pins = {p["name"]: p for p in info["pins"]}
    assert len(pins) >= 40
    pa0 = pins["pa0"]
    assert (pa0["port"], pa0["index"]) == ("a", 0)
    assert {"peripheral": "tim2", "signal": "ch1", "af": 1} in pa0["functions"]
    # functions sorted deterministically, and a pin with no routes still lists
    for p in info["pins"]:
        fns = p["functions"]
        assert fns == sorted(fns, key=lambda f: (f["peripheral"], f["signal"]))


@skip_no_devices
def test_chip_info_pins_exist_for_curated_chips_too() -> None:
    info = chips.chip_info(DEVICES_ROOT, "st/stm32g071rb")
    pins = {p["name"]: p for p in info["pins"]}
    assert "pa2" in pins  # curated route: usart2 tx
    assert any(f["peripheral"] == "usart2" and f["signal"] == "tx"
               for f in pins["pa2"]["functions"])
