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
    assert (lay.bootloader.base, lay.bootloader.size) == (0x08000000, 32 * 1024)
    # 128K - 16K bootloader - 4K store = 108K -> 54K per slot
    assert lay.slot_a.base == 0x08008000
    assert lay.slot_a.size == 46 * 1024
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
    assert "slot_a_base = 0x08008000u" in text
    assert "bootloader_base = 0x08000000u" in text
    assert f"app_offset = {APP_OFFSET:#x}u" in text
    assert "store_base = 0x0801f000u" in text


def test_linker_window_places_app_inside_slot() -> None:
    chip = _g0_chip()
    lay = slot_layout(chip)
    text = emit_linker_script(
        chip, "cortex_m", "flash", 0,
        (lay.slot_a.base + APP_OFFSET, lay.slot_a.size - APP_OFFSET))
    assert "ORIGIN = 0x08008200" in text
    assert f"LENGTH = {46 * 1024 - APP_OFFSET}" in text


def test_slot_build_refuses_boards_with_flash_reserved() -> None:
    chip = _g0_chip()
    lay = slot_layout(chip)
    with pytest.raises(EmitError, match="nvm/fs"):
        emit_linker_script(chip, "cortex_m", "flash", 4096,
                          (lay.slot_a.base + APP_OFFSET, lay.slot_a.size - APP_OFFSET))


def _f7_chip(flash_size: int = 512 * 1024) -> dict:
    c = _g0_chip(flash_size)
    c["part"] = "STM32F722ZE"
    c["memories"][0]["base"] = "0x08000000"
    c["peripherals"]["flash"]["ip"] = "st/flash_f7"
    return c


def test_f7_512k_sector_layout_uses_natural_boundaries() -> None:
    lay = slot_layout(_f7_chip())
    assert lay.page_size == 128 * 1024              # updater stride = one big sector
    assert (lay.bootloader.base, lay.bootloader.size) == (0x08000000, 32 * 1024)
    assert (lay.store.base, lay.store.size) == (0x08008000, 32 * 1024)
    assert lay.store_page_b == 0x0800c000           # sector 3, independently erasable
    assert (lay.slot_a.base, lay.slot_a.size) == (0x08020000, 128 * 1024)
    assert (lay.slot_b.base, lay.slot_b.size) == (0x08040000, 128 * 1024)


def test_f7_1m_layout_scales_the_sector_pattern() -> None:
    lay = slot_layout(_f7_chip(1024 * 1024))
    assert lay.page_size == 256 * 1024
    assert lay.bootloader.size == 64 * 1024
    assert lay.slot_a.base == 0x08040000            # after 4x32K + 128K
    assert lay.slot_b.base == 0x08080000


def test_g0_store_page_b_is_one_page_after_store_base() -> None:
    lay = slot_layout(_g0_chip())
    assert lay.store_page_b == lay.store.base + lay.page_size


def test_same70_efc_uniform_layout() -> None:
    c = _g0_chip(2 * 1024 * 1024)
    c["part"] = "ATSAME70Q21B"
    c["memories"][0]["base"] = "0x00400000"
    c["peripherals"]["flash"]["ip"] = "microchip/efc_v1"
    lay = slot_layout(c)
    assert lay.page_size == 8192                    # 16-page EPA erase block
    assert (lay.bootloader.base, lay.bootloader.size) == (0x00400000, 32 * 1024)
    assert lay.store_page_b == lay.store.base + 8192
    assert lay.slot_a.base % 8192 == 0 and lay.slot_a.size % 8192 == 0


# ---------------------------------------------------- the architecture gate


def test_an_unknown_architecture_is_refused_not_approximated() -> None:
    """The emitter used to treat "not xtensa" as "Cortex-M".

    So `emit_linker_script(chip, "rl78")` returned a full ARM script —
    ENTRY(Reset_Handler), an ARM exception-index discard, an ARM vector table —
    and it would have LINKED. The image just could not boot. A framework whose
    rule is that facts are generated cannot silently generate the wrong ones.
    """
    with pytest.raises(EmitError) as caught:
        emit_linker_script(_g0_chip(), "rl78")

    message = str(caught.value)
    assert "rl78" in message, "the error must name the architecture asked for"
    assert "cortex_m" in message and "xtensa" in message, \
        "and the ones that do exist, so the reader knows the shape of the fix"


def test_the_two_real_backends_still_emit() -> None:
    """The gate must not become the thing that breaks the working paths."""
    arm = emit_linker_script(_g0_chip(), "cortex_m")
    assert "ENTRY(Reset_Handler)" in arm

    # The real chip, not a hand-built stub: the xtensa backend reads more of the
    # memory map than a plausible-looking fake carries, and a fake that drifts
    # from the database would make this test pass on something nobody ships.
    from pathlib import Path

    from alloy_cli.devices import load_chip

    devices = Path(__file__).resolve().parents[3].parent / "alloy-devices"
    if (devices / "chips").is_dir():
        esp = load_chip(devices, "espressif/esp32")
        assert emit_linker_script(esp, "xtensa"), "xtensa must still emit"


def test_the_default_argument_is_one_of_the_backends() -> None:
    """`arch_ns` defaults to cortex_m. If a rename ever left the default outside
    _BACKENDS, every caller that omits it would start raising."""
    import inspect

    from alloy_cli.emit.linker import _BACKENDS

    default = inspect.signature(emit_linker_script).parameters["arch_ns"].default
    assert default in _BACKENDS
