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
