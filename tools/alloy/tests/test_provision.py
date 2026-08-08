"""`alloy provision` — the identity codec, its refusals, and the probe plan.

The codec half is pure, so everything interesting is testable with no hardware:
the exact bytes, the CRC, and every refusal a factory can trip. The probe half is
tested through an injected runner that plays openocd, including the two failure
modes that matter — a write the hardware silently dropped (write-protected page)
and a readback that does not match what was sent.

kGOLDEN is pinned on BOTH sides of the language boundary: tests/test_provision.cpp
carries the same 64 bytes as a C array. Nothing else stops the host encoder and
the firmware parser from drifting apart, because they never run together.
"""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest
from alloy_cli import provision
from alloy_cli.emit.common import EmitError
from alloy_cli.emit.slots import Region, SlotLayout

# alloy provision write --serial ALY-0001-A7 --mac 02:1a:2b:3c:4d:5e
#                       --hw-rev 3 --batch 42 -o id.bin
GOLDEN = bytes.fromhex(
    "41505256" "0100" "4000"
    "414c592d303030312d413700000000 00".replace(" ", "")
    + "021a2b3c4d5e" "0300"
    + "2a000000"
    + "00" * 24
    + "408613df")

SAMPLE = provision.Identity(serial="ALY-0001-A7",
                            mac=bytes.fromhex("021a2b3c4d5e"),
                            hw_revision=3, batch=42)


# ── the bytes ───────────────────────────────────────────────────────────────


def test_golden_record_is_exactly_64_bytes() -> None:
    assert len(GOLDEN) == provision.RECORD_SIZE == 64


def test_encode_matches_the_golden_the_firmware_test_also_pins() -> None:
    """If this fails, so does provision_serializes_byte_for_byte_like_the_host_verb
    in tests/test_provision.cpp — and that pair is the only thing keeping the
    host writer and the device parser agreeing about a device's serial number."""
    assert provision.encode(SAMPLE) == GOLDEN


def test_encode_decode_round_trip() -> None:
    assert provision.decode(provision.encode(SAMPLE)) == SAMPLE


def test_crc_covers_the_reserved_bytes() -> None:
    raw = bytearray(GOLDEN)
    raw[40] ^= 0xFF  # inside the reserved block
    with pytest.raises(EmitError, match="CRC mismatch"):
        provision.decode(bytes(raw))


def test_crc_agrees_with_zlib() -> None:
    """The hand-written CRC-32 must be CRC-32/ISO-HDLC and nothing else — the
    firmware's own bytewise implementation is the third party to this agreement."""
    import zlib

    for sample in (b"", b"123456789", GOLDEN[:60]):
        assert provision.crc32(sample) == zlib.crc32(sample)


def test_a_full_16_byte_serial_survives_the_round_trip() -> None:
    ident = provision.make_identity("0123456789ABCDEF")
    back = provision.decode(provision.encode(ident))
    assert back.serial == "0123456789ABCDEF"


# ── decode refusals ─────────────────────────────────────────────────────────


def test_erased_page_says_never_provisioned() -> None:
    with pytest.raises(EmitError, match="NEVER been provisioned"):
        provision.decode(b"\xff" * 64)


def test_foreign_bytes_say_no_record_here() -> None:
    with pytest.raises(EmitError, match="no identity record here"):
        provision.decode(b"\x00" * 64)


def test_a_short_buffer_is_refused() -> None:
    with pytest.raises(EmitError, match="need 64"):
        provision.decode(GOLDEN[:32])


def test_a_newer_format_version_is_refused() -> None:
    raw = bytearray(GOLDEN)
    raw[4] = 2
    raw[60:64] = provision.crc32(bytes(raw[:60])).to_bytes(4, "little")
    with pytest.raises(EmitError, match="format_version 2"):
        provision.decode(bytes(raw))


# ── make_identity refusals: the ones a factory actually trips ───────────────


def test_serial_longer_than_the_field_is_refused_not_truncated() -> None:
    with pytest.raises(EmitError, match="refusal, not a warning"):
        provision.make_identity("THIS-SERIAL-IS-FAR-TOO-LONG")


def test_serial_with_stray_whitespace_is_refused() -> None:
    with pytest.raises(EmitError, match="whitespace"):
        provision.make_identity(" SN-1 ")


def test_empty_serial_is_refused() -> None:
    with pytest.raises(EmitError, match="not provisioned"):
        provision.make_identity("")


def test_non_ascii_serial_is_refused() -> None:
    with pytest.raises(EmitError, match="not ASCII"):
        provision.make_identity("SN-é")


def test_control_characters_in_a_serial_are_refused() -> None:
    with pytest.raises(EmitError, match="control characters"):
        provision.make_identity("SN\t1")


def test_out_of_range_hw_rev_and_batch_are_refused() -> None:
    with pytest.raises(EmitError, match="hw-rev"):
        provision.make_identity("SN1", hw_revision=70000)
    with pytest.raises(EmitError, match="batch"):
        provision.make_identity("SN1", batch=-1)


@pytest.mark.parametrize("text", ["aa:bb:cc:dd:ee:f", "zz:bb:cc:dd:ee:ff", "",
                                  "aa:bb:cc:dd:ee:ff:00"])
def test_malformed_mac_is_refused(text: str) -> None:
    with pytest.raises(EmitError, match="not a MAC address"):
        provision.parse_mac(text)


def test_multicast_mac_is_refused() -> None:
    """The bit that makes a MAC a group address is bit 0 of the FIRST octet, and
    a device that uses one as its own source address is dropped by switches. It
    is a one-nibble typo away from a valid unicast address, which is exactly why
    it is refused rather than warned about."""
    with pytest.raises(EmitError, match="MULTICAST"):
        provision.parse_mac("01:00:5e:00:00:01")


def test_broadcast_mac_is_refused() -> None:
    with pytest.raises(EmitError, match="broadcast"):
        provision.parse_mac("ff:ff:ff:ff:ff:ff")


def test_locally_administered_mac_is_accepted() -> None:
    """02:… is the locally administered range, which is what a project without a
    purchased OUI should use. Refusing it would have been the wrong guard."""
    assert provision.parse_mac("02:1a:2b:3c:4d:5e") == bytes.fromhex("021a2b3c4d5e")


@pytest.mark.parametrize("text", ["02:1a:2b:3c:4d:5e", "02-1a-2b-3c-4d-5e",
                                  "021a2b3c4d5e", " 02:1A:2B:3C:4D:5E "])
def test_mac_spellings_all_parse_the_same(text: str) -> None:
    assert provision.parse_mac(text) == bytes.fromhex("021a2b3c4d5e")


# ── the probe plan ──────────────────────────────────────────────────────────


def _layout() -> SlotLayout:
    return SlotLayout(page_size=2048,
                      bootloader=Region(0x08000000, 32768),
                      slot_a=Region(0x08008000, 46 * 1024),
                      slot_b=Region(0x08013800, 46 * 1024),
                      store=Region(0x0801F000, 4096),
                      store_page_b=0x0801F800,
                      provision=Region(0x08007800, 2048))


_BOARD = {"id": "nucleo_g071rb", "probe": {"kind": "stlink"}}
_CHIP = {"family": "stm32g0", "part": "STM32G071RB", "vendor": "st"}


def test_region_comes_from_the_layout_never_typed() -> None:
    assert provision.provision_region(_layout()) == (0x08007800, 2048)


def test_write_commands_erase_program_and_dump_back(tmp_path: Path) -> None:
    cmds = provision.write_commands(0x08007800, tmp_path / "in.bin",
                                    tmp_path / "out.bin")
    assert cmds[0] == "init"
    assert cmds[-1] == "shutdown"
    assert any("flash write_image erase" in c and "0x08007800" in c for c in cmds)
    # The dump is the verification. If it is ever dropped, a write the hardware
    # refused would report success.
    assert any(c.startswith("dump_image") and c.endswith(" 64") for c in cmds)


def _runner(dump: Path, content: bytes | None, rc: int = 0):
    def run(_argv: list[str]) -> subprocess.CompletedProcess:
        if content is not None:
            dump.write_bytes(content)
        return subprocess.CompletedProcess([], rc, "", "")
    return run


def test_run_write_reports_what_the_DEVICE_says(tmp_path: Path) -> None:
    dump = tmp_path / "alloy-identity-readback.bin"
    got = provision.run_write(_BOARD, _CHIP, _layout(), SAMPLE, tmp_path,
                              runner=_runner(dump, GOLDEN))
    assert got == SAMPLE


def test_run_write_catches_a_write_that_silently_did_nothing(tmp_path: Path) -> None:
    """The write-protect trap: `alloy secure apply --wrp-bootloader` was run
    first, so the identity page (the last page of the bootloader region on
    uniform-page flash) is frozen. openocd exits 0 and the page is still erased.
    Only the readback catches it — and the message has to name the cause,
    because the operator's next move is either 'clear WRP' or 'scrap the batch
    order'."""
    dump = tmp_path / "alloy-identity-readback.bin"
    with pytest.raises(EmitError, match="still ERASED"):
        provision.run_write(_BOARD, _CHIP, _layout(), SAMPLE, tmp_path,
                            runner=_runner(dump, b"\xff" * 64))


def test_run_write_catches_a_corrupted_readback(tmp_path: Path) -> None:
    dump = tmp_path / "alloy-identity-readback.bin"
    wrong = bytearray(GOLDEN)
    wrong[9] ^= 0x20
    with pytest.raises(EmitError, match="readback mismatch"):
        provision.run_write(_BOARD, _CHIP, _layout(), SAMPLE, tmp_path,
                            runner=_runner(dump, bytes(wrong)))


def test_a_failed_probe_session_names_the_likely_causes(tmp_path: Path) -> None:
    dump = tmp_path / "alloy-identity-readback.bin"
    with pytest.raises(EmitError, match="RDP level 2"):
        provision.run_read(_BOARD, _CHIP, _layout(), tmp_path,
                           runner=_runner(dump, None, rc=1))


def test_run_read_decodes_what_the_probe_dumped(tmp_path: Path) -> None:
    dump = tmp_path / "alloy-identity-readback.bin"
    got = provision.run_read(_BOARD, _CHIP, _layout(), tmp_path,
                             runner=_runner(dump, GOLDEN))
    assert got.serial == "ALY-0001-A7"
    assert got.mac_str == "02:1a:2b:3c:4d:5e"


def test_a_board_without_a_probe_is_refused_before_anything_runs(
        tmp_path: Path) -> None:
    with pytest.raises(EmitError, match="declares no probe"):
        provision.run_read({"id": "pico"}, _CHIP, _layout(), tmp_path,
                           runner=_runner(tmp_path / "x", None))
