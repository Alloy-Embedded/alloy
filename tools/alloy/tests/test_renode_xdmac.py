"""The SAME70 XDMAC block in the emitted Renode platform.

Phase 5a of docs/design/dma-streams.md. Renode 1.16.1 ships NO XDMAC model and
no Microchip DMA model of any kind (RENODE_XDMAC records the three independent
ways that was measured), so the platform emits a generated C# one — the third
time this project writes a model, after the SAME70 EFC and the ST stream
engine, and the only tier that can serve: the cheaper Python-peripheral tier
has no IRQ property, and the NVIC line is the entire phase-5 assertion.

WHAT THESE TESTS COVER AND WHAT THEY DO NOT. Here: that the block is emitted at
all, that every number in it is DERIVED from chip data rather than typed (each
one is perturbed and the wire must move), and that the .cs is compiled before
the platform description names its type. NOT here: that the model behaves — the
behaviour is witnessed by the emulation leg (tests/emulation/dma_uart.robot on
same70_xplained, green in ~5 s where the same firmware against the same
platform without the model fails at "dma via DMA" after 31 s).

And one thing NOTHING witnesses, stated because the honesty label is part of
the deliverable: PERID request routing. The model stores it, logs it and never
acts on it, because nothing can pace a transfer here — UART.SAM_USART exposes
no DMA-request output. On the G0 and the F7 the request half is unwitnessed
because the platform wire and the firmware route descend from the same
board.json statement; on this family it is unwitnessed BY CONSTRUCTION, and
only silicon will ever prove it.
"""

from __future__ import annotations

import copy
import json
from pathlib import Path

import pytest

from alloy_cli.devices import load_chip
from alloy_cli.emit.renode import (
    XDMAC_CS_NAME,
    _resolve_xdmac,
    emit_renode_platform,
    emit_renode_script,
    renode_support_files,
)

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _board(board_id: str) -> dict:
    return json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())


@skip_no_devices
def test_the_same70_platform_instantiates_the_xdmac_on_its_one_nvic_line() -> None:
    """The block itself: the generated model at the chip's base, wired to the
    chip's single XDMAC vector. ONE line for 24 channels is the shape that makes
    the driver's per-channel GIS guard load-bearing, so there is deliberately no
    CombinedInput OR-gate here — unlike the G0's shared channel lines."""
    repl = emit_renode_platform(load_chip(DEVICES_ROOT, "microchip/atsame70q21"),
                                _board("same70_xplained"))
    assert "xdmac: DMA.SAME70_XDMAC @ sysbus 0x40078000" in repl
    assert "IRQ -> nvic@58" in repl
    # One controller, one wire: no OR gate, and nothing per-channel.
    assert repl.count("DMA.SAME70_XDMAC") == 1


@skip_no_devices
def test_the_vector_comes_from_the_chip_not_from_this_emitter() -> None:
    """Anti-drift, the shape test_renode_exti.py states for its numbers: move
    the chip's XDMAC interrupt and the platform's wire must follow. A hardcoded
    58 passes the test above and fails this one."""
    chip = copy.deepcopy(load_chip(DEVICES_ROOT, "microchip/atsame70q21"))
    for entry in chip["interrupts"]:
        if entry["name"] == "XDMAC":
            entry["number"] = 41
    repl = emit_renode_platform(chip, _board("same70_xplained"))
    assert "IRQ -> nvic@41" in repl
    assert "nvic@58" not in repl


@skip_no_devices
def test_the_base_address_comes_from_the_chip_too() -> None:
    chip = copy.deepcopy(load_chip(DEVICES_ROOT, "microchip/atsame70q21"))
    chip["peripherals"]["xdmac"]["base"] = "0x40099000"
    repl = emit_renode_platform(chip, _board("same70_xplained"))
    assert "DMA.SAME70_XDMAC @ sysbus 0x40099000" in repl


@skip_no_devices
def test_a_vector_the_chip_does_not_name_emits_nothing() -> None:
    """Data too thin to wire an interrupt: emit no controller rather than one
    that cannot deliver. This model's entire purpose IS the interrupt, so a
    silent no-vector instantiation would be worse than absence — the leg would
    move bytes and never fire, which reads like a driver bug."""
    chip = copy.deepcopy(load_chip(DEVICES_ROOT, "microchip/atsame70q21"))
    chip["interrupts"] = [i for i in chip["interrupts"] if i["name"] != "XDMAC"]
    assert _resolve_xdmac(chip) is None
    assert "SAME70_XDMAC" not in emit_renode_platform(chip, _board("same70_xplained"))


@skip_no_devices
def test_an_uncurated_controller_emits_nothing() -> None:
    chip = copy.deepcopy(load_chip(DEVICES_ROOT, "microchip/atsame70q21"))
    chip["peripherals"]["xdmac"]["uncurated"] = True
    assert _resolve_xdmac(chip) is None


@skip_no_devices
def test_the_st_boards_get_no_xdmac_and_no_model_file() -> None:
    """The table is keyed on the IP, not on "is this a DMA controller": an ST
    board must be untouched by any of this."""
    chip = load_chip(DEVICES_ROOT, "st/stm32g071rb")
    board = _board("nucleo_g071rb")
    assert _resolve_xdmac(chip) is None
    assert "SAME70_XDMAC" not in emit_renode_platform(chip, board)
    assert XDMAC_CS_NAME not in renode_support_files(chip, board)


@skip_no_devices
def test_the_model_source_is_written_and_compiled_before_the_platform() -> None:
    """Ordering is not cosmetic: `machine LoadPlatformDescription` resolves the
    type name, so an `include` of the .cs that came after it would fail with
    "Error E04: Could not resolve type" instead of loading."""
    chip = load_chip(DEVICES_ROOT, "microchip/atsame70q21")
    board = _board("same70_xplained")
    files = renode_support_files(chip, board)
    assert XDMAC_CS_NAME in files
    assert "class SAME70_XDMAC" in files[XDMAC_CS_NAME]

    resc = emit_renode_script(chip, board, "/tmp/s70/same70_xplained.repl",
                              "/tmp/s70/app.elf")
    include_at = resc.index(f"include @/tmp/s70/{XDMAC_CS_NAME}")
    load_at = resc.index("machine LoadPlatformDescription")
    assert include_at < load_at


@skip_no_devices
def test_the_model_refuses_linked_list_mode_out_loud() -> None:
    """The 5b ring has NO witness and the model is the place that must not
    pretend otherwise. A view-0 descriptor's memory layout is curated in no file
    in either repo, so a model that chased the list would encode the same
    unverified reading microchip_xdmac_v1_body.hpp does, and a green ring leg
    would prove only that the two agree with each other. So a GE on a channel
    with CNDC.NDE set logs a warning and moves nothing.

    This asserts the REFUSAL, not any ring behaviour — there is none to assert.
    It exists so that "make the ring leg pass" cannot be answered by quietly
    teaching the model a descriptor layout nobody has checked.

    It asserts the GUARD EXPRESSION rather than a keyword, and that is
    deliberate: the first version of this test looked for "NdeBit" and passed
    happily with the conditional replaced by `if(false)`, because the constant's
    own declaration still matched. The model is a generated blob with no host
    executor, so its source text is the only instrument available; the
    instrument has to point at the line that does the work."""
    files = renode_support_files(load_chip(DEVICES_ROOT, "microchip/atsame70q21"),
                                 _board("same70_xplained"))
    model = files[XDMAC_CS_NAME]
    assert "if((cndc[ch] & NdeBit) != 0)" in model
    assert "LogLevel.Warning" in model
    assert "is NOT modelled" in model
    assert "Nothing was transferred." in model
