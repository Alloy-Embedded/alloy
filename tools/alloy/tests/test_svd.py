"""Unit tests for `alloy svd`.

An SVD file is consumed by a debugger, not by a human, so "it looks right" is
not a check. These parse the output and assert the things a register viewer
actually reads — and the two honesty rules: no ARM, no file; no curated
registers, no peripheral pretending to have them.
"""

from __future__ import annotations

import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

from alloy_cli.emit.common import EmitError
from alloy_cli.emit.svd import emit_svd

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _emit(chip_id: str):
    from alloy_cli.devices import load_chip, load_registers

    document, skipped = emit_svd(load_chip(DEVICES_ROOT, chip_id),
                                 load_registers(DEVICES_ROOT))
    return ET.fromstring(document), skipped, document


@skip_no_devices
def test_device_header_describes_the_real_core() -> None:
    root, _, _ = _emit("st/stm32g071rb")
    assert root.findtext("name") == "STM32G071RB"
    assert root.findtext("cpu/name") == "CM0PLUS"
    assert root.findtext("cpu/fpuPresent") == "false"
    # nvicPrioBits is chip data, and a debugger uses it to decode priorities.
    assert root.findtext("cpu/nvicPrioBits") == "2"


@skip_no_devices
def test_registers_and_fields_survive_the_trip() -> None:
    root, _, _ = _emit("st/stm32g071rb")
    i2c = next(p for p in root.findall("peripherals/peripheral")
               if p.findtext("name") == "I2C1")
    assert i2c.findtext("baseAddress") == "0x40005400"
    cr2 = next(r for r in i2c.findall("registers/register")
               if r.findtext("name") == "CR2")
    assert cr2.findtext("addressOffset") == "0x04"
    assert cr2.findtext("access") == "read-write"
    nbytes = next(f for f in cr2.findall("fields/field")
                  if f.findtext("name") == "NBYTES")
    assert nbytes.findtext("bitOffset") == "16"
    assert nbytes.findtext("bitWidth") == "8"


@skip_no_devices
def test_a_peripherals_interrupt_number_comes_from_the_chip() -> None:
    """Without this a viewer cannot tie a pending IRQ to its peripheral."""
    root, _, _ = _emit("st/stm32g071rb")
    i2c = next(p for p in root.findall("peripherals/peripheral")
               if p.findtext("name") == "I2C1")
    assert i2c.findtext("interrupt/name") == "I2C1"
    assert int(i2c.findtext("interrupt/value") or -1) >= 0


@skip_no_devices
def test_named_field_values_become_enumerations() -> None:
    """The curated `values` are what make a mux field readable as LSE/LSI
    instead of 1/2."""
    root, _, _ = _emit("st/stm32g071rb")
    rtcsel = root.find('.//field[name="RTCSEL"]')
    assert rtcsel is not None
    names = {e.findtext("name") for e in rtcsel.findall(".//enumeratedValue")}
    assert {"LSE", "LSI"} <= names


@skip_no_devices
def test_repeated_instances_of_one_ip_derive_instead_of_duplicating() -> None:
    root, _, _ = _emit("st/stm32g071rb")
    peripherals = root.findall("peripherals/peripheral")
    derived = [p for p in peripherals if p.get("derivedFrom")]
    assert derived, "several USART/SPI/I2C instances share an IP — derive them"
    for p in derived:
        # A derived peripheral carries only what differs: address and IRQ.
        assert p.find("registers") is None
        assert p.findtext("baseAddress")


@skip_no_devices
def test_uncurated_peripherals_are_absent_and_named() -> None:
    """A viewer showing an empty peripheral looks broken; leaving it out
    silently looks incomplete. It is skipped AND reported."""
    root, skipped, _ = _emit("st/stm32f767zi")
    described = {p.findtext("name") for p in root.findall("peripherals/peripheral")}
    assert skipped, "this builder-generated chip admits uncurated peripherals"
    assert not ({s.upper() for s in skipped} & described)


@skip_no_devices
def test_a_non_arm_chip_is_refused_with_a_reason() -> None:
    """SVD is a CMSIS format. Emitting one for an Xtensa part would produce a
    file no debugger can use."""
    with pytest.raises(EmitError, match="ARM Cortex-M"):
        _emit("espressif/esp32")


@skip_no_devices
def test_descriptions_are_xml_safe_and_single_line() -> None:
    """Curated descriptions contain '&', '<' and hard-wrapped YAML text; either
    would corrupt the document or the viewer's tree."""
    _, _, document = _emit("st/stm32g071rb")
    body = document.split("<peripherals>", 1)[1]
    for line in body.splitlines():
        if "<description>" in line:
            assert line.strip().endswith("</description>"), \
                f"description spans lines: {line!r}"
    ET.fromstring(document)  # would raise on a stray '&'


def _board_chips() -> list[str]:
    """The chips actually behind the shipped boards — the ones a user debugs."""
    import json

    boards = ALLOY_ROOT / "boards"
    if not boards.is_dir():
        return []
    return sorted({json.loads((b / "board.json").read_text())["chip"]
                   for b in boards.iterdir() if (b / "board.json").exists()})


@skip_no_devices
@pytest.mark.parametrize("chip_id", _board_chips())
def test_every_board_chip_either_emits_or_says_why(chip_id: str) -> None:
    """No shipped board may fail in a confusing way: it emits a parseable
    device, or refuses with a reason a user can act on."""
    try:
        root, _, _ = _emit(chip_id)
    except EmitError as exc:
        assert "ARM Cortex-M" in str(exc) or "no curated peripheral" in str(exc), \
            f"{chip_id}: unhelpful refusal — {exc}"
        return
    assert root.findall("peripherals/peripheral"), f"{chip_id}: empty device"


@skip_no_devices
def test_address_block_covers_the_registers_it_describes() -> None:
    root, _, _ = _emit("st/stm32g071rb")
    for peripheral in root.findall("peripherals/peripheral"):
        block = peripheral.find("addressBlock")
        if block is None:
            continue  # derived: it inherits the origin's block
        size = int(block.findtext("size") or "0", 16)
        highest = max(
            int(r.findtext("addressOffset") or "0", 16)
            for r in peripheral.findall("registers/register"))
        assert size > highest, f"{peripheral.findtext('name')}: block too small"
