"""Unit tests for the A/B firmware-slot layout (emit/slots.py).

The layout math is what the bootloader, the updater and two linker scripts all
trust — an off-by-one page here bricks devices in the field, so pin it hard:
page alignment, region adjacency/disjointness, and honest rejection of chips
that can't support A/B.
"""

from __future__ import annotations

import pytest

from alloy_cli.emit.common import EmitError
from alloy_cli.emit.linker import emit_linker_script
from alloy_cli.emit.slots import (
    APP_OFFSET,
    emit_slots_header,
    has_slot_layout,
    slot_layout,
)


def _g0_chip(flash_size: int = 128 * 1024) -> dict:
    return {
        "vendor": "st",
        "part": "STM32G071RB",
        "memories": [
            {"name": "flash", "kind": "flash", "base": "0x08000000", "size": flash_size},
            {"name": "sram", "kind": "ram", "base": "0x20000000", "size": 36864},
        ],
        "peripherals": {"flash": {"ip": "st/flash_g0", "base": "0x40022000"}},
        "cores": [{"arch": "armv6m"}],
    }


def test_g0_layout_partitions_128k_correctly() -> None:
    lay = slot_layout(_g0_chip())
    assert lay.page_size == 2048
    assert (lay.bootloader.base, lay.bootloader.size) == (0x08000000, 16 * 1024)
    # 128K - 16K bootloader - 4K store = 108K -> 54K per slot
    assert lay.slot_a.base == 0x08004000
    assert lay.slot_a.size == 54 * 1024
    assert lay.slot_b.base == lay.slot_a.base + lay.slot_a.size
    assert lay.slot_b.size == lay.slot_a.size
    assert lay.store.base == 0x08000000 + 128 * 1024 - 2 * 2048
    assert lay.store.size == 2 * 2048


def test_every_region_is_page_aligned_and_disjoint() -> None:
    lay = slot_layout(_g0_chip())
    regions = [lay.bootloader, lay.slot_a, lay.slot_b, lay.store]
    for r in regions:
        assert r.base % lay.page_size == 0, f"{r} base not page-aligned"
        assert r.size % lay.page_size == 0, f"{r} size not page-aligned"
    # bootloader | slot A | slot B are adjacent; store never overlaps slot B
    assert lay.slot_a.base == lay.bootloader.base + lay.bootloader.size
    assert lay.slot_b.base + lay.slot_b.size <= lay.store.base


def test_unknown_flash_ip_is_rejected_quietly() -> None:
    chip = _g0_chip()
    chip["peripherals"]["flash"]["ip"] = "vendorx/flash_v9"
    assert not has_slot_layout(chip)
    with pytest.raises(EmitError, match="page-size"):
        slot_layout(chip)


def test_too_small_flash_is_rejected() -> None:
    chip = _g0_chip(flash_size=32 * 1024)  # 32K - 16K - 4K = 6K/slot: useless
    assert not has_slot_layout(chip)
    with pytest.raises(EmitError, match="too small"):
        slot_layout(chip)


def test_slots_header_carries_the_layout() -> None:
    text = emit_slots_header(_g0_chip())
    assert "slot_a_base = 0x08004000u" in text
    assert "bootloader_base = 0x08000000u" in text
    assert f"app_offset = {APP_OFFSET:#x}u" in text
    assert "store_base = 0x0801f000u" in text


def test_linker_window_places_app_inside_slot() -> None:
    chip = _g0_chip()
    lay = slot_layout(chip)
    text = emit_linker_script(
        chip, "cortex_m", "flash", 0,
        (lay.slot_a.base + APP_OFFSET, lay.slot_a.size - APP_OFFSET))
    assert "ORIGIN = 0x08004200" in text
    assert f"LENGTH = {54 * 1024 - APP_OFFSET}" in text


def test_slot_build_refuses_boards_with_flash_reserved() -> None:
    chip = _g0_chip()
    lay = slot_layout(chip)
    with pytest.raises(EmitError, match="nvm/fs"):
        emit_linker_script(chip, "cortex_m", "flash", 4096,
                          (lay.slot_a.base + APP_OFFSET, lay.slot_a.size - APP_OFFSET))
