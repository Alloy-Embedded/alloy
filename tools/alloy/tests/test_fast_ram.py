"""Coupled RAM (DTCM / CCM) in the linker script, and the RL78 region fix.

`fast: true` on a chip's memory means a region the core reaches with no wait
states — and that the peripheral DMA cannot reach at all. Both halves are load
bearing, and the second is why `.data`/`.bss` must NOT be swept in there.
"""

from __future__ import annotations

import pytest

from alloy_cli.emit.linker import app_regions, emit_linker_script, fast_ram

_G474 = {
    "vendor": "st", "part": "stm32g474re",
    "memories": [
        {"name": "flash", "kind": "flash", "base": "0x08000000", "size": 524288},
        {"name": "sram", "kind": "ram", "base": "0x20000000", "size": 81920},
        {"name": "ccmram_icode", "kind": "ram", "base": "0x10000000",
         "size": 32768, "fast": True, "dma_reachable": False},
    ],
}
_G0 = {
    "vendor": "st", "part": "stm32g0b1re",
    "memories": [
        {"name": "flash", "kind": "flash", "base": "0x08000000", "size": 524288},
        {"name": "sram", "kind": "ram", "base": "0x20000000", "size": 147456},
    ],
}
_RL78 = {
    "vendor": "renesas", "part": "r5f104bd",
    "memories": [
        {"name": "flash", "kind": "flash", "base": "0x00000000", "size": 65536},
        {"name": "ram", "kind": "ram", "base": "0x000FF300", "size": 4096},
    ],
}


def test_fast_ram_finds_coupled_memory() -> None:
    assert fast_ram(_G474)["name"] == "ccmram_icode"
    assert fast_ram(_G474)["base"] == "0x10000000"


def test_fast_ram_is_none_when_the_part_has_none() -> None:
    assert fast_ram(_G0) is None


def test_fast_ram_ignores_a_ram_region_not_marked_fast() -> None:
    """A second plain SRAM bank is not coupled memory. Only the flag decides."""
    chip = {"memories": [
        {"name": "sram", "kind": "ram", "base": "0x20000000", "size": 1024},
        {"name": "sram2", "kind": "ram", "base": "0x20001000", "size": 1024},
    ]}
    assert fast_ram(chip) is None


def test_a_part_with_coupled_ram_gets_a_fastram_region() -> None:
    script = emit_linker_script(_G474, "cortex_m")
    assert "FASTRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 32768" in script
    assert "} > FASTRAM AT> FLASH" in script   # .fastdata: loaded and copied
    assert "} > FASTRAM\n" in script            # .fastbss: NOLOAD


def test_the_sections_exist_even_without_coupled_ram() -> None:
    """The startup's copy loop is the same code on every part.

    On a chip with no coupled memory the sections target ordinary RAM and stay
    empty, so the loop runs zero iterations. Emitting them conditionally would
    mean two startup paths, and the one nobody builds is the one that breaks.
    """
    script = emit_linker_script(_G0, "cortex_m")
    assert "FASTRAM" not in script
    for symbol in ("_sifastdata", "_sfastdata", "_efastdata",
                   "_sfastbss", "_efastbss"):
        assert symbol in script, f"{symbol} must be defined on every part"


def test_data_and_bss_never_go_to_coupled_ram() -> None:
    """The safety property, asserted rather than trusted to a comment.

    Coupled memory is invisible to the peripheral DMA. Sweeping .data or .bss
    into it — which is what the firmware that prompted this feature did — puts
    every DMA buffer in the program somewhere the DMA cannot write.
    """
    script = emit_linker_script(_G474, "cortex_m")
    # Find the region each section is placed into.
    for section in (".data", ".bss"):
        start = script.index(f"    {section} ")
        placement = script[start:start + 800]
        end = placement.index("} > ")
        region = placement[end + 4:].split()[0].rstrip(";")
        assert region == "RAM", f"{section} was placed in {region}, not RAM"


def test_fastcode_stays_in_ordinary_ram() -> None:
    """.fastcode is about flash wait states, not about the DMA.

    It belongs in whatever RAM executes code. On an M7 the instruction-side TCM
    would be better still, but ITCM is not carried as a `ram` memory precisely
    because it cannot hold data — mixing the two ideas would put .fastdata
    somewhere it cannot live.
    """
    script = emit_linker_script(_G474, "cortex_m")
    start = script.index("    .fastcode :")
    placement = script[start:start + 400]
    assert "} > RAM AT> FLASH" in placement


@pytest.mark.parametrize("arch,chip", [("cortex_m", _G0), ("cortex_m", _G474)])
def test_the_script_still_links_its_own_regions(arch: str, chip: dict) -> None:
    script = emit_linker_script(chip, arch)
    # Every region named in SECTIONS must be declared in MEMORY.
    memory = script[script.index("MEMORY"):script.index("SECTIONS")]
    for line in script.splitlines():
        if "} > " in line:
            region = line.split("} > ")[1].split()[0].rstrip(";")
            assert f"{region} " in memory or f"{region}(" in memory, \
                f"section placed in undeclared region {region}"


def test_rl78_app_regions_no_longer_raise() -> None:
    """THE BUG THIS FOUND.

    `app_regions` carried a paste from `emit_linker_script`: it referenced two
    names that do not exist in its scope and returned the linker script STRING
    where every caller expects a dict of regions. `alloy size` on an RL78 board
    raised NameError. Nothing had exercised the path.
    """
    got = app_regions(_RL78, "rl78")
    assert got["data"]["name"] == "ram"
    # Code starts ABOVE the vector table, CALLT table and option bytes — 0xD8.
    assert got["code"]["base"] == "0x000000D8"
    assert got["code"]["size"] == 65536 - 0xD8


def test_rl78_app_regions_match_the_script_it_describes() -> None:
    """The two used to be able to drift; the table sizes are now shared."""
    script = emit_linker_script(_RL78, "rl78")
    regions = app_regions(_RL78, "rl78")
    origin = f"ORIGIN = {int(regions['code']['base'], 16):#07x}"
    assert f"FLASH   (rx) : {origin}" in script
