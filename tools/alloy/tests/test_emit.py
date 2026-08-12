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

# tools/alloy/tests/ -> tools/alloy -> tools -> alloy
ALLOY_ROOT = Path(__file__).resolve().parents[3]


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


# ------------------------------------------- configuration inside the image


def _chip_with_option_bytes() -> dict:
    return {
        "vendor": "renesas", "part": "R5F104BD", "family": "rl78_g14",
        "cores": [{"name": "rl78s3", "arch": "rl78_s3"}],
        "memories": [], "peripherals": {},
        "image_config": [
            {"name": "option_bytes", "base": "0x000C0", "size": 4,
             "erased_value": "0xFF",
             "description": "watchdog, LVD level, HOCO frequency, debug enable"},
        ],
    }


def test_unset_image_config_warns_rather_than_passing_quietly() -> None:
    """The CPU reads these before any code runs. Booting on the erased value is
    legal and almost never what someone meant, so it must be said out loud —
    but it is not an error, because a part with no watchdog still runs."""
    from alloy_cli.board_validate import _check_image_config

    issues = _check_image_config({"id": "x"}, _chip_with_option_bytes())
    assert len(issues) == 1
    assert issues[0]["level"] == "warning"
    assert "option_bytes" in issues[0]["message"]
    # And it must say what was left to chance, not just that something is unset.
    assert "watchdog" in issues[0]["message"]


def test_a_wrong_length_image_config_is_an_error() -> None:
    """Too few bytes leaves the rest erased; too many writes past the region.
    Either way the CPU has already read it by the time anything could complain."""
    from alloy_cli.board_validate import _check_image_config

    board = {"id": "x", "image_config": {"option_bytes": ["0xEF", "0xFF"]}}
    issues = _check_image_config(board, _chip_with_option_bytes())
    assert [i["level"] for i in issues] == ["error"]
    assert "exactly 4 byte" in issues[0]["message"]


def test_a_byte_that_is_not_a_byte_is_an_error() -> None:
    from alloy_cli.board_validate import _check_image_config

    board = {"id": "x",
             "image_config": {"option_bytes": ["0xEF", "0x100", "0xFF", "0x85"]}}
    issues = _check_image_config(board, _chip_with_option_bytes())
    assert [i["level"] for i in issues] == ["error"]
    assert "does not fit a byte" in issues[0]["message"]


def test_a_complete_image_config_passes() -> None:
    from alloy_cli.board_validate import _check_image_config

    board = {"id": "x",
             "image_config": {"option_bytes": ["0xEF", "0xFF", "0xE8", "0x85"]}}
    assert _check_image_config(board, _chip_with_option_bytes()) == []


def test_a_chip_with_no_image_config_is_unaffected() -> None:
    """Every existing chip: the check must be invisible to them."""
    from alloy_cli.board_validate import _check_image_config

    assert _check_image_config({"id": "x"}, {"vendor": "st"}) == []


# --------------------------------------------------- routes the framework
# does not model yet


def _routes_chip(routes: list[dict]) -> dict:
    return {
        "vendor": "st", "part": "SYNTH",
        "peripherals": {"tim2": {"ip": "st/tim_gp16", "base": "0x40000000"},
                        "lptim1": {"uncurated": True, "ip_hint": "lptim:v1b",
                                   "base": "0x40007C00"}},
        "routes": routes,
    }


def test_route_with_a_modelled_signal_is_emitted() -> None:
    from alloy_cli.emit.device import emit_routes_header
    out = emit_routes_header(_routes_chip(
        [{"pin": "pa5", "peripheral": "tim2", "signal": "ch1",
          "kind": "af_fixed", "af": 2}]))
    assert "alloy::signal::ch1" in out
    assert "static constexpr std::uint8_t af = 2u;" in out


def test_route_with_an_unmodelled_signal_is_skipped_and_named() -> None:
    """A full AF table names signals alloy::signal has no enumerator for. That
    must not fail the build (it would forbid the chip database from carrying
    the silicon's real routing), and it must not vanish silently either."""
    from alloy_cli.emit.device import emit_routes_header
    out = emit_routes_header(_routes_chip(
        [{"pin": "pa0", "peripheral": "tim2", "signal": "etr",
          "kind": "af_fixed", "af": 2},
         {"pin": "pa5", "peripheral": "tim2", "signal": "ch1",
          "kind": "af_fixed", "af": 2}]))
    assert "alloy::signal::etr" not in out
    assert "alloy::signal::ch1" in out
    assert "pa0 -> tim2.etr" in out, "the skipped route must be named in the header"


def test_route_into_an_uncurated_peripheral_is_not_listed_as_a_signal_gap() -> None:
    """Those routes are skipped for a different reason and were already
    skipped; listing them would drown the signal-gap list."""
    from alloy_cli.emit.device import emit_routes_header
    out = emit_routes_header(_routes_chip(
        [{"pin": "pa0", "peripheral": "lptim1", "signal": "out",
          "kind": "af_fixed", "af": 5}]))
    assert "pa0 -> lptim1" not in out
    assert "chip declares no routes" in out


# ── Instance ownership, build-time half ─────────────────────────────────
#
# A peripheral runs in ONE personality at a time. The runtime half of this
# rule lives in src/alloy/core/claim.hpp and catches hand-written binds; this
# half catches the board file, before a compiler is ever started.


def test_two_roles_may_share_a_peripheral_within_one_personality() -> None:
    """nvm and fs on one flash controller is the shape a real board ships
    (nucleo_g0b1re): one driver, two regions, not two personalities."""
    from alloy_cli.emit.board import _check_role_personalities
    _check_role_personalities({
        "id": "synth",
        "roles": {"nvm": {"peripheral": "flash"}, "fs": {"peripheral": "flash"}},
    })


def test_one_peripheral_in_two_personalities_fails_generation() -> None:
    from alloy_cli.emit.board import _check_role_personalities
    from alloy_cli.emit.common import EmitError
    with pytest.raises(EmitError) as excinfo:
        _check_role_personalities({
            "id": "synth",
            "roles": {"led_pwm": {"peripheral": "tim3"},
                      "rtc": {"peripheral": "tim3"}},
        })
    msg = str(excinfo.value)
    assert "tim3" in msg, "the diagnostic must name the contested peripheral"
    assert "pwm" in msg and "rtc" in msg, "and both personalities"


def test_roles_without_a_peripheral_are_not_claimants() -> None:
    """led/button name PINS, not peripherals; they must not be dragged in."""
    from alloy_cli.emit.board import _check_role_personalities
    _check_role_personalities({
        "id": "synth",
        "roles": {"led": {"pin": "pa5"}, "button": {"pin": "pc13"}},
    })


def test_every_generator_personality_exists_in_the_cpp_vocabulary() -> None:
    """The two halves of one rule, and nothing until now stopped them drifting.

    `ROLE_PERSONALITY` (Python, generation time) and `claim::personality`
    (C++, run time) are the SAME closed vocabulary written twice. A name that
    exists in only one of them is a rule with a hole in it: the generator
    would refuse a board conflict the firmware cannot express, or worse, a
    facade would have to spell its personality `user_a` and stop being
    distinguishable. `ethernet` was exactly that gap when this test was
    written; `dma` was a facade with no enumerator at all.
    """
    import re

    from alloy_cli.emit.board import ROLE_PERSONALITY

    header = (ALLOY_ROOT / "src" / "alloy" / "core" / "claim.hpp").read_text()
    body = header.split("enum class personality", 1)[1].split("};", 1)[0]
    cpp_names = set(re.findall(r"^\s*([a-z_][a-z_0-9]*)\s*(?:=\s*\d+\s*)?,", body, re.M))
    assert "uart" in cpp_names, "the enum body did not parse — fix this test, not the header"
    missing = sorted(set(ROLE_PERSONALITY.values()) - cpp_names)
    assert not missing, (
        f"emit/board.py names personalities the C++ enum does not have: {missing}"
    )


# ── DEGREE has two homes, and the emitter merges them ───────────────────────
#
# A UART FIFO depth differs between two instances on one die and belongs to the
# chip file; an ADC's analog-watchdog count is fixed by the IP version and
# belongs to the IP register file, or it is one integer copied into every chip
# that names the IP. Both must land in the same `struct feat`, and an instance
# must not be able to contradict its own IP quietly.


def _feat_chip() -> tuple[dict, dict]:
    chip = {
        "vendor": "st",
        "part": "TEST",
        "cores": [{"name": "cm0plus", "arch": "armv6m"}],
        "peripherals": {"adc1": {"ip": "st/adc_x", "base": "0x40012400"}},
        "interrupts": [],
    }
    registers = {
        "st/adc_x": {
            "vendor": "st", "ip": "adc_x", "class": "adc",
            "feat": {"analog_watchdogs": 3},
            "registers": [{"name": "ISR", "offset": "0x00", "access": "rw"}],
        }
    }
    return chip, registers


def test_ip_level_degree_lands_on_the_instance() -> None:
    from alloy_cli.emit.device import emit_device_header

    chip, registers = _feat_chip()
    out = emit_device_header(chip, registers, [])
    assert "struct feat {" in out
    assert "static constexpr std::uint32_t analog_watchdogs = 3u;" in out


def test_instance_and_ip_degree_merge_into_one_block() -> None:
    from alloy_cli.emit.device import emit_device_header

    chip, registers = _feat_chip()
    chip["peripherals"]["adc1"]["feat"] = {"rx_fifo_depth": 8}
    out = emit_device_header(chip, registers, [])
    assert "static constexpr std::uint32_t analog_watchdogs = 3u;" in out
    assert "static constexpr std::uint32_t rx_fifo_depth = 8u;" in out


def test_an_instance_may_not_quietly_contradict_its_own_ip() -> None:
    from alloy_cli.emit.common import EmitError
    from alloy_cli.emit.device import emit_device_header

    chip, registers = _feat_chip()
    chip["peripherals"]["adc1"]["feat"] = {"analog_watchdogs": 1}
    with pytest.raises(EmitError, match="one of the two"):
        emit_device_header(chip, registers, [])


def test_the_lead_boards_adc_reports_three_watchdogs_from_real_data() -> None:
    """The number a driver's static_assert compares against, read end to end
    from the shipped database rather than from a fixture."""
    from alloy_cli.devices import load_chip, load_registers

    devices_root = ALLOY_ROOT.parent / "alloy-devices"
    chip = load_chip(devices_root, "st/stm32g0b1re")
    registers = load_registers(devices_root)
    assert registers[chip["peripherals"]["adc1"]["ip"]]["feat"]["analog_watchdogs"] == 3


# ── CURATION IS A CLOSURE OVER COMPANIONS ───────────────────────────────────
#
# A feature can live in two blocks: an FDCAN acceptance filter's ELEMENTS are
# words in the companion message RAM and its LIST SIZE is a controller
# register. So "is this peripheral curated" is the wrong question — the right
# one is asked of every block the feature touches. An uncurated companion used
# to surface as `companion cycle among peripherals: ['fdcan1']`, which is a
# true sentence about the wrong thing: the cycle detector was reporting a
# dangling edge because the target had already been dropped as uncurated.
#
# Rule v3, question 0: docs/reference/peripheral-surface.md#question-0-what-does-the-database-already-know


def _companion_chip(ram_uncurated: bool) -> tuple[dict, dict]:
    ram: dict = ({"uncurated": True, "ip_hint": "fdcanram:v1"}
                 if ram_uncurated else {"ip": "st/fdcanram_v1"})
    ram["base"] = "0x4000B400"
    chip = {
        "vendor": "st",
        "part": "TEST",
        "cores": [{"name": "cm0plus", "arch": "armv6m"}],
        "peripherals": {
            "fdcan1": {"ip": "st/fdcan_x", "base": "0x40006400",
                       "companions": {"ram": "fdcanram1"}},
            "fdcanram1": ram,
        },
        "interrupts": [],
    }
    registers = {
        "st/fdcan_x": {"vendor": "st", "ip": "fdcan_x", "class": "can",
                       "registers": [{"name": "CCCR", "offset": "0x18",
                                      "access": "rw"}]},
        "st/fdcanram_v1": {"vendor": "st", "ip": "fdcanram_v1", "class": "canram",
                           "registers": [{"name": "FLSSA", "offset": "0x00",
                                          "access": "rw"}]},
    }
    return chip, registers


def test_a_curated_controller_with_an_uncurated_companion_names_the_companion() -> None:
    """The diagnostic must name the block that is actually missing, and must
    NOT be the cycle message — which is what a reader used to get."""
    from alloy_cli.emit.common import EmitError
    from alloy_cli.emit.device import emit_device_header

    chip, registers = _companion_chip(ram_uncurated=True)
    with pytest.raises(EmitError) as e:
        emit_device_header(chip, registers, [])
    msg = str(e.value)
    assert "fdcanram1" in msg, "the message must name the companion, not the owner"
    assert "only as curated as its companions" in msg
    assert "cycle" not in msg, "the old message blamed the dependency loop"


def test_a_curated_pair_emits_the_companion_alias() -> None:
    """The negative control, without which 'refuse everything' would pass: the
    same chip with the companion curated generates, and the controller reaches
    its RAM through the generated alias rather than through a literal base."""
    from alloy_cli.emit.device import emit_device_header

    chip, registers = _companion_chip(ram_uncurated=False)
    out = emit_device_header(chip, registers, [])
    assert "using ram_t = alloy::dev::fdcanram1_t;" in out
    assert out.index("struct fdcanram1_t {") < out.index("struct fdcan1_t {"), \
        "the companion has to be declared before the block that aliases it"
