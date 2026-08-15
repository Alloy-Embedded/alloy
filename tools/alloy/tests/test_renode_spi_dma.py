"""The SPI RX-DMA request wire in the emitted Renode platform.

Phase 4 of docs/design/dma-streams.md (anchor 2.4): where a board assigns
`spi.rx`, the platform wires SPI.STM32SPI's DMA-request output straight to the
assigned channel's input — no C# bridge and no access shim, because unlike the
ADC this model already carries the GPIO and already gates it on the CR2 enable
bit the driver writes. ONE emitter path serves both engines (the G0's 1-based
`channel` and the stream engine's 0-based `stream`), because the emitted line
is byte-identical either way and the index passes through untranslated on both.

The index must move with the board statement, so these tests perturb the
assignment and assert the wire follows — the same data-driven defense
test_renode_uart_dma.py and test_renode_exti.py state for their numbers.

WITNESSED BY THE LEG, not just by these string assertions (spi_read.robot,
pinned Renode 1.16.1, run locally on nucleo_g071rb / nucleo_g0b1re /
nucleo_f722ze): the four DMA'd bytes come back exactly as primed, and a peer
cursor five deep proves the transmit channel clocked four cycles. Negative
controls that took the leg RED, each measured: the wire moved by one index,
the pair's arming order swapped (transmit first), and CR2.RXDMAEN never
raised. NOT witnessed by anything here or there: the REQUEST id half of the
route — Renode 1.16.1 models no DMAMUX at all and CHSEL routes nothing in the
generated stream model, and the platform wire and the firmware route descend
from the same board.json statement, so they cannot disagree. Silicon only.
"""

from __future__ import annotations

import copy
import json
from pathlib import Path

import pytest

from alloy_cli.devices import load_chip
from alloy_cli.emit.renode import SPI_DMA_RX_GPIO, emit_renode_platform

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _board(board_id: str) -> dict:
    return json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())


def _spi_block(repl: str) -> str:
    return repl.split("spi1: SPI.STM32SPI")[1].split("\n\n")[0]


@skip_no_devices
@pytest.mark.parametrize("board_id,chip_id", [
    ("nucleo_g0b1re", "st/stm32g0b1re"),
    ("nucleo_g071rb", "st/stm32g071rb"),
])
def test_the_g0_boards_wire_the_spi_receive_request_to_the_assigned_channel(
        board_id: str, chip_id: str) -> None:
    """Both G0s assign spi.rx to dma1 channel 4 (1-based, free router); the
    wire says so, inside the SPI's own block and not as a stray entry."""
    board = _board(board_id)
    assert board["dma"]["spi.rx"] == {"controller": "dma1", "channel": 4}
    repl = emit_renode_platform(load_chip(DEVICES_ROOT, chip_id), board)
    assert f"{SPI_DMA_RX_GPIO} -> dma1@4" in _spi_block(repl)


@skip_no_devices
def test_the_f7_board_wires_the_zero_based_stream_untranslated() -> None:
    """The stream engine's index is 0-BASED on both sides — the board's
    `stream` key and the generated model's OnGPIO — so a board that says
    stream 0 gets @0, NOT @1. This is the off-by-one the phase-1 channel/
    stream rename exists to prevent, asserted on the shipped board."""
    board = _board("nucleo_f722ze")
    assert board["dma"]["spi.rx"] == {"controller": "dma2", "stream": 0}
    repl = emit_renode_platform(load_chip(DEVICES_ROOT, "st/stm32f722"), board)
    assert f"{SPI_DMA_RX_GPIO} -> dma2@0" in _spi_block(repl)


@skip_no_devices
@pytest.mark.parametrize("chip_id,board_id,key,value,want", [
    ("st/stm32g071rb", "nucleo_g071rb", "channel", 6, "dma1@6"),
    ("st/stm32f722", "nucleo_f722ze", "stream", 2, "dma2@2"),
])
def test_the_wire_follows_the_board_assignment_not_a_constant(
        chip_id: str, board_id: str, key: str, value: int, want: str) -> None:
    """Perturb the index on each engine: the emitted wire must move with it.
    A hardcoded index would keep the two tests above green and silently
    misroute — and misrouting is exactly what the leg's first negative
    control detects, so it must not be reachable from the data."""
    board = copy.deepcopy(_board(board_id))
    ctrl = board["dma"]["spi.rx"]["controller"]
    board["dma"]["spi.rx"] = {"controller": ctrl, key: value}
    repl = emit_renode_platform(load_chip(DEVICES_ROOT, chip_id), board)
    assert f"{SPI_DMA_RX_GPIO} -> {want}" in repl


@skip_no_devices
def test_no_assignment_no_wire() -> None:
    """A board that assigns no spi.rx gets no wire — the platform must not
    invent a hookup the firmware has no route for. The firmware says so too:
    the example's third leg prints "spi dma: not assigned" there."""
    board = copy.deepcopy(_board("nucleo_g071rb"))
    del board["dma"]["spi.rx"]
    repl = emit_renode_platform(load_chip(DEVICES_ROOT, "st/stm32g071rb"),
                                board)
    assert SPI_DMA_RX_GPIO not in repl


@skip_no_devices
def test_an_assignment_on_an_unmodelled_controller_emits_no_wire() -> None:
    """Same rule as the UART and ADC wires: an assignment onto a controller
    this platform does not emit (g0b1re's dma2 has no modelled block) must not
    produce a dangling reference that fails platform load."""
    board = copy.deepcopy(_board("nucleo_g0b1re"))
    board["dma"]["spi.rx"] = {"controller": "dma2", "channel": 1}
    repl = emit_renode_platform(load_chip(DEVICES_ROOT, "st/stm32g0b1re"),
                                board)
    assert SPI_DMA_RX_GPIO not in repl
    assert "dma2@" not in repl


@skip_no_devices
@pytest.mark.parametrize("chip_id,board_id,wrong", [
    # The free router's key on a stream engine, and the stream engine's key on
    # the free router: each is the OTHER engine's shape, and each must emit
    # nothing rather than guess a translation between 1-based and 0-based.
    ("st/stm32f722", "nucleo_f722ze", "channel"),
    ("st/stm32g071rb", "nucleo_g071rb", "stream"),
])
def test_the_wrong_engines_key_emits_no_wire(chip_id: str, board_id: str,
                                             wrong: str) -> None:
    board = copy.deepcopy(_board(board_id))
    ctrl = board["dma"]["spi.rx"]["controller"]
    board["dma"]["spi.rx"] = {"controller": ctrl, wrong: 1}
    repl = emit_renode_platform(load_chip(DEVICES_ROOT, chip_id), board)
    assert SPI_DMA_RX_GPIO not in repl


@skip_no_devices
@pytest.mark.parametrize("board_id,chip_id", [
    ("nucleo_g071rb", "st/stm32g071rb"),
    ("nucleo_f722ze", "st/stm32f722"),
])
def test_the_transmit_half_of_the_pair_gets_no_wire(board_id: str,
                                                    chip_id: str) -> None:
    """Both boards assign spi.tx, and the platform must stay silent about it.
    Measured model fact, not an oversight: an m2p transfer completes IN FULL
    at the write that sets EN in both engines' models, and the generated
    stream model's OnRequest returns early for any direction that is not
    peripheral-to-memory — so a transmit wire is inert at best. (The pinned
    SPI model has no send-request GPIO to wire to in the first place; upstream
    added one after v1.16.1.) Exactly one request wire per platform."""
    board = _board(board_id)
    assert "spi.tx" in board["dma"]
    repl = emit_renode_platform(load_chip(DEVICES_ROOT, chip_id), board)
    assert repl.count(SPI_DMA_RX_GPIO) == 1
    assert "DMASend" not in repl


def test_the_gpio_name_is_the_pinned_releases_misspelling() -> None:
    """A guard on a VERSION-PINNED STRING, not on style. SPI.STM32SPI's
    request output is spelled `DMARecieve` in Renode 1.16.1 — read out of the
    shipped assembly, where the backing field is `<DMARecieve>k__BackingField`
    — and upstream fixed the typo to `DMAReceive` AFTER that release. Emitting
    the corrected spelling against the pinned emulator fails platform load with
    `Error E13: Property 'DMAReceive' does not exist`. So a well-meaning fix
    here is a break, and this test is where it gets caught; the day ci.yml's
    Renode pin moves past that commit, this constant and this test flip
    together."""
    assert SPI_DMA_RX_GPIO == "DMARecieve"
