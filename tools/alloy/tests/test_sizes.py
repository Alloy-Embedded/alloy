"""Unit tests for the firmware memory report (`alloy size`, `alloy build --json`).

The parser is where this can silently lie, so it is tested directly: reporting
flash as `text` alone under-reports by exactly the size of .data, which is the
kind of error nobody notices until an update image does not fit.
"""

from __future__ import annotations

from pathlib import Path

import pytest

from alloy_cli import sizes

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

_have_devices = (DEVICES_ROOT / "chips").is_dir()
skip_no_devices = pytest.mark.skipif(
    not _have_devices, reason="alloy-devices not beside the framework")

_BERKELEY = """\
   text\t   data\t    bss\t    dec\t    hex\tfilename
  21400\t    108\t   2320\t  23828\t   5d14\tblink.elf
"""


def test_parse_size_splits_flash_and_ram_correctly() -> None:
    got = sizes.parse_size(_BERKELEY)
    assert got == {"text": 21400, "data": 108, "bss": 2320,
                   # .data ships in flash AND occupies RAM — it counts twice.
                   "flash": 21400 + 108, "ram": 108 + 2320}


def test_parse_size_ignores_the_header_row() -> None:
    assert sizes.parse_size("   text\t   data\t    bss\t    dec\t    hex\tfilename\n") is None


def test_parse_size_survives_unexpected_output() -> None:
    assert sizes.parse_size("") is None
    assert sizes.parse_size("arm-none-eabi-size: 'x.elf': No such file\n") is None


class _FakeProject:
    """Just the attributes size_report touches."""

    def __init__(self, root: Path, board: str = "test_board") -> None:
        self.name = "app"
        self.board_id = board
        self.build_dir = root / ".alloy" / "build-tree" / board


def _chip(devices_root: Path, chip_id: str) -> dict:
    from alloy_cli.devices import load_chip

    return load_chip(devices_root, chip_id)


@skip_no_devices
def test_missing_build_is_a_state_not_an_error(tmp_path: Path) -> None:
    report = sizes.size_report(_FakeProject(tmp_path),
                               _chip(DEVICES_ROOT, "st/stm32g071rb"))
    assert report["schema"] == "alloy.size.v1"
    assert report["available"] is False
    assert "alloy build" in report["reason"]
    # The chip's memories are known even with nothing built, so the UI can still
    # draw an empty bar of the right size.
    assert report["flash"]["total"] == 131072
    assert report["ram"]["total"] == 36864
    assert report["flash"]["used"] is None


@skip_no_devices
def test_slot_regions_come_from_the_slot_emitter(tmp_path: Path) -> None:
    report = sizes.size_report(_FakeProject(tmp_path),
                               _chip(DEVICES_ROOT, "st/stm32g071rb"))
    names = [r["name"] for r in report["slots"]["regions"]]
    assert names == ["bootloader", "slot_a", "slot_b", "store", "provision"]
    # Nothing built yet, so fitting is unknown rather than falsely true.
    assert all(r["fits"] is None for r in report["slots"]["regions"])


@skip_no_devices
def test_chip_without_slot_support_reports_no_slots(tmp_path: Path) -> None:
    """RP2040 has no flash-controller IP the slot layout understands; the report
    must omit slots rather than invent a partitioning."""
    report = sizes.size_report(_FakeProject(tmp_path),
                               _chip(DEVICES_ROOT, "raspberrypi/rp2040"))
    assert report["slots"] is None


# ------------------------------------------- which memory the numbers refer to

@skip_no_devices
def test_regions_are_the_ones_the_linker_used() -> None:
    """A percentage is meaningless unless it names the right region. The Xtensa
    script puts .text in IROM and .data/.bss in DRAM; taking "the first ram"
    would report the 2 KiB vector window as the whole of RAM (it did, until
    `alloy matrix` put nine boards side by side and made it obvious).
    """
    esp32 = _chip(DEVICES_ROOT, "espressif/esp32")
    assert sizes._memory(esp32, "data") == {
        "name": "dram", "base": 0x3FFB0000, "size": 278528}
    assert (sizes._memory(esp32, "code") or {})["name"] == "irom"

    # Cortex-M: first flash / first ram, exactly as emit/linker.py does it.
    g071 = _chip(DEVICES_ROOT, "st/stm32g071rb")
    assert (sizes._memory(g071, "code") or {})["name"] == "flash"
    assert (sizes._memory(g071, "data") or {})["name"] == "sram"


@skip_no_devices
def test_the_report_names_its_region(tmp_path: Path) -> None:
    """So a reader can tell "1984K of flash" from "1984K of IROM"."""
    report = sizes.size_report(_FakeProject(tmp_path),
                               _chip(DEVICES_ROOT, "st/stm32g071rb"))
    assert report["flash"]["region"] == "flash"
    assert report["ram"]["region"] == "sram"
