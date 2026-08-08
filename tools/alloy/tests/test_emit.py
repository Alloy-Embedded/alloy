"""Unit tests for the host-side codegen/flash helpers.

The board×example CI matrix only proves emitted output *compiles*; it cannot
catch a helper that emits a subtly-wrong-but-valid value (an off-by-one in a
repeat-expanded field mask, a mis-encoded UF2/partition byte). These exercise
those pure functions directly.
"""

from __future__ import annotations

import struct
from pathlib import Path

import pytest

from alloy_cli.emit.board import _eth_mac_names, _polarity, _region_bytes_ok
from alloy_cli.emit.common import (
    EmitError,
    cpp_ip_namespace,
    field_lookup,
    hex32,
    register_by_name,
)
from alloy_cli.flash import _generate_partition_table


def test_hex32_is_zero_padded_and_unsigned() -> None:
    assert hex32(0) == "0x00000000u"
    assert hex32(0x40021000) == "0x40021000u"
    assert hex32(0xFFFFFFFF) == "0xFFFFFFFFu"


def test_cpp_ip_namespace_splits_vendor_and_ip() -> None:
    assert cpp_ip_namespace("st/gpio_v2") == ("st", "gpio_v2")
    assert cpp_ip_namespace("espressif/uart_v1") == ("espressif", "uart_v1")


def test_register_by_name_found_and_missing() -> None:
    ip = {"vendor": "st", "ip": "rcc_g0", "registers": [{"name": "CR", "offset": "0x00"}]}
    assert register_by_name(ip, "CR")["offset"] == "0x00"
    with pytest.raises(EmitError, match="no register IOPENR"):
        register_by_name(ip, "IOPENR")


def test_field_lookup_plain_field() -> None:
    reg = {"name": "CR", "fields": [{"name": "PLLON", "bit": 24},
                                    {"name": "SW", "bit": 0, "width": 3}]}
    assert field_lookup(reg, "PLLON") == (24, 1)
    assert field_lookup(reg, "SW") == (0, 3)


def test_field_lookup_repeat_expansion_is_off_by_one_safe() -> None:
    # MODER is a 2-bit field repeated 16× at stride 2: MODER5 -> bit 10, width 2.
    reg = {"name": "MODER", "fields": [
        {"name": "MODER", "bit": 0, "width": 2, "repeat": {"count": 16, "stride": 2}}]}
    assert field_lookup(reg, "MODER0") == (0, 2)
    assert field_lookup(reg, "MODER5") == (10, 2)
    assert field_lookup(reg, "MODER15") == (30, 2)


def test_field_lookup_rejects_out_of_range_and_unknown() -> None:
    reg = {"name": "MODER", "fields": [
        {"name": "MODER", "bit": 0, "width": 2, "repeat": {"count": 16, "stride": 2}}]}
    with pytest.raises(EmitError, match="no field MODER16"):
        field_lookup(reg, "MODER16")  # count is 16 -> indices 0..15
    with pytest.raises(EmitError, match="no field NOPE"):
        field_lookup(reg, "NOPE")


def test_eth_mac_names_are_ip_version_driven() -> None:
    # The MAC HAL is selected by IP version, so a 2nd MAC family plugs in with data
    # only — no GMAC hardcoding in the emitter.
    assert _eth_mac_names("microchip/gmac_v1") == ("microchip_gmac_v1", "gmac")
    assert _eth_mac_names("st/eth_v1") == ("st_eth_v1", "eth")
    assert _eth_mac_names("st/eth_v2b") == ("st_eth_v2b", "eth")


def test_polarity_maps_high_and_treats_anything_else_as_low() -> None:
    assert _polarity("high") == "alloy::gpio::active_high_t"
    assert _polarity("low") == "alloy::gpio::active_low_t"
    # Documents the current fallback: any non-"high" string (incl. a typo) is
    # active_low. board.json schema validation is what should catch typos.
    assert _polarity("High") == "alloy::gpio::active_low_t"


def test_region_bytes_must_be_positive_page_multiple() -> None:
    # nvm/fs regions carve whole flash pages; a sub-page or non-multiple `bytes`
    # would erase across the region boundary — must fail generation.
    assert _region_bytes_ok(2048, 2048)  # exactly one page
    assert _region_bytes_ok(32768, 2048)  # 16 pages (the fs default)
    assert not _region_bytes_ok(5000, 2048)  # not a page multiple
    assert not _region_bytes_ok(1024, 2048)  # smaller than a page
    assert not _region_bytes_ok(0, 2048)  # zero -> no blocks
    # erase_size 0 = page size not curated for this chip yet -> unchecked.
    assert _region_bytes_ok(5000, 0)


def test_partition_table_structure(tmp_path: Path) -> None:
    out = tmp_path / "partitions.bin"
    _generate_partition_table(
        [["nvs", "data", "nvs", "0x9000", "0x6000"],
         ["factory", "app", "factory", "0x10000", "0x100000"]],
        out,
    )
    data = out.read_bytes()
    assert len(data) == 0xC00  # padded
    assert data[:2] == b"\xaa\x50"  # first entry magic
    assert data.count(b"\xaa\x50") == 2  # one per partition
    # MD5 marker precedes the padding.
    assert b"\xeb\xeb" in data
    # First entry: type=data(1), subtype=nvs(2), offset=0x9000, size=0x6000.
    t, s, offset, size = struct.unpack("<BBII", data[2:12])
    assert (t, s, offset, size) == (0x01, 0x02, 0x9000, 0x6000)


def test_partition_table_rejects_unknown_type(tmp_path: Path) -> None:
    with pytest.raises(EmitError, match="unknown type/subtype"):
        _generate_partition_table([["x", "bogus", "nvs", "0x9000", "0x1000"]], tmp_path / "p.bin")


def test_cmake_lfs_defines_do_not_leak_onto_other_vendored_packages(tmp_path):
    """The LFS_* strip-config used to ride on EVERY vendored TU — monocypher
    included. Package config must be scoped to the package that owns it."""
    from pathlib import Path

    from alloy_cli.build import _cmakelists
    from alloy_cli.project import Project

    proj = Project(root=tmp_path, name="x", board_id="nucleo_g071rb",
                   alloy_root=tmp_path, devices_root=tmp_path)
    chip = {"cores": [{"arch": "armv6m"}]}
    mono = [tmp_path / "monocypher.c", tmp_path / "monocypher-ed25519.c"]
    lfs = [tmp_path / "lfs.c", tmp_path / "lfs_util.c"]
    text = _cmakelists(proj, chip, sources=[tmp_path / "main.cpp"],
                       runtime_sources=[], vendor_sources=[*mono, *lfs],
                       lfs_sources=lfs)

    blocks = [b for b in text.split("set_source_files_properties")[1:]]
    lfs_blocks = [b for b in blocks if "LFS_NO_MALLOC" in b]
    assert len(lfs_blocks) == 1
    assert "lfs.c" in lfs_blocks[0]
    assert "monocypher" not in lfs_blocks[0]
    # monocypher still gets its warnings silenced, in its own block.
    mono_blocks = [b for b in blocks if "monocypher.c" in b]
    assert len(mono_blocks) == 1
    assert "LFS_" not in mono_blocks[0]


# ------------------------------------------------ registers in two windows


def _rl78_adc() -> dict:
    """The RL78/G14 ADC as it really is: ADM0/ADS in the SFR area, ADM2 in the
    2nd SFR area about 64 KB below. One peripheral, two address windows."""
    return {
        "vendor": "renesas", "ip": "adc_g14", "class": "adc",
        "registers": [
            {"name": "ADM0", "offset": "0x00", "size": 8, "access": "rw",
             "fields": [{"name": "adcs", "bit": 7, "width": 1}]},
            {"name": "ADS", "offset": "0x01", "size": 8, "access": "rw", "fields": []},
            {"name": "ADM2", "offset": "0x00", "size": 8, "access": "rw",
             "window": "sfr2", "fields": [{"name": "adtyp", "bit": 0, "width": 1}]},
        ],
    }


def test_a_window_gets_its_own_struct() -> None:
    """Laid out as one struct these would span ~64 KB, on a part with 4 KB of
    RAM. Each window is its own overlay and the chip data binds a base to it."""
    from alloy_cli.emit.ip import emit_ip_header

    out = emit_ip_header(_rl78_adc())
    assert "struct regs {" in out
    assert "struct regs_sfr2 {" in out
    # Offsets are relative to the window's OWN base, so both start at zero.
    assert "static_assert(offsetof(regs, ADM0) == 0x00);" in out
    assert "static_assert(offsetof(regs_sfr2, ADM2) == 0x00);" in out


def test_a_field_names_the_struct_its_register_landed_in() -> None:
    """The accessor is the only thing tying a field to its window; pointing it
    at `regs` for a register in `regs_sfr2` would not compile, which is the
    good case — but it would be an emitter bug either way."""
    from alloy_cli.emit.ip import emit_ip_header

    out = emit_ip_header(_rl78_adc())
    assert "alloy::field<&regs::ADM0, 7u, 1>" in out
    assert "alloy::field<&regs_sfr2::ADM2, 0u, 1>" in out


def test_a_window_name_must_be_usable_as_a_struct_name() -> None:
    from alloy_cli.emit.common import EmitError
    from alloy_cli.emit.ip import emit_ip_header

    doc = _rl78_adc()
    doc["registers"][2]["window"] = "2nd SFR"
    with pytest.raises(EmitError, match="lowercase identifier"):
        emit_ip_header(doc)


def test_eight_and_sixteen_bit_registers_emit_their_own_width() -> None:
    from alloy_cli.emit.ip import emit_ip_header

    doc = {
        "vendor": "renesas", "ip": "mixed", "class": "gpio",
        "registers": [
            {"name": "P", "offset": "0x00", "size": 8, "access": "rw", "fields": []},
            {"name": "CNT", "offset": "0x02", "size": 16, "access": "rw", "fields": []},
            {"name": "W", "offset": "0x04", "size": 32, "access": "ro", "fields": []},
        ],
    }
    out = emit_ip_header(doc)
    assert "rw8 P;" in out and "rw16 CNT;" in out and "ro32 W;" in out
    # A one-byte gap: the old word-granular padding rejected this outright.
    assert "std::uint8_t _reserved0[1];" in out


def test_a_register_must_be_aligned_to_its_own_width() -> None:
    from alloy_cli.emit.common import EmitError
    from alloy_cli.emit.ip import emit_ip_header

    doc = {
        "vendor": "renesas", "ip": "bad", "class": "gpio",
        "registers": [
            {"name": "A", "offset": "0x00", "size": 8, "access": "rw", "fields": []},
            {"name": "B", "offset": "0x01", "size": 16, "access": "rw", "fields": []},
        ],
    }
    with pytest.raises(EmitError, match="2-byte aligned"):
        emit_ip_header(doc)
