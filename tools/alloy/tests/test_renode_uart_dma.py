"""The UART RX-DMA request wire in the emitted Renode platform.

Phase 2 of docs/design/dma-streams.md: where a board assigns `debug_uart.rx`
(board.json "dma"), the platform wires the UART model's ReceiveDmaRequest
output straight to the assigned channel's input on the G0 DMA model — no C#
bridge, because STM32F7_USART (pinned 1.16.1) already carries the request
GPIO the ADC model lacked. The channel number is 1-based on both sides and
passes through untranslated; it must move with the board statement, so these
tests perturb the assignment and assert the wire moves — the same
data-driven defense test_renode_exti.py states for its numbers.

What none of this witnesses: the IDLE wake, which the pinned model does not
implement at all (see _uart_rx_dma_wire's docstring for the measured facts).

The byte-movement claim itself WAS witnessed locally (2026-08-14, pinned
Renode 1.16.1, macOS arm64) by a throwaway probe on nucleo_g071rb — not yet
by a CI leg. The probe: a ring on the generated debug_uart.rx route draining
RDR by DMA (CR3.DMAR poked directly; the firmware never reads RDR, never
arms RXNE), echoing delivered lines with a sequence number. Green run:
"HELLO" delivered as line #1; with CR3.DMAR cleared by the monitor, "DEAD"
never delivered (the model's request GPIO is DMAR-gated); after an RQR.RXFRQ
flush and DMAR restore, "WORLD" delivered as line #2 — the sequence number
proving DEAD's absence exactly, and the flush proving the documented
stuck-level recovery. Negative control on causation: the same firmware
against the same platform with only the ReceiveDmaRequest line deleted fails
on line #1 (10 s timeout where the good run passes in 1.5 s).
"""

from __future__ import annotations

import copy
import json
from pathlib import Path

import pytest

from alloy_cli.devices import load_chip
from alloy_cli.emit.renode import emit_renode_platform

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _board(board_id: str) -> dict:
    return json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())


@skip_no_devices
@pytest.mark.parametrize("board_id,chip_id", [
    ("nucleo_g0b1re", "st/stm32g0b1re"),
    ("nucleo_g071rb", "st/stm32g071rb"),
])
def test_the_shipped_g0_boards_wire_rx_requests_to_the_assigned_channel(
        board_id: str, chip_id: str) -> None:
    """Both boards assign debug_uart.rx to dma1 channel 2; the wire says so,
    inside the debug UART's block."""
    board = _board(board_id)
    assert board["dma"]["debug_uart.rx"] == {
        "controller": "dma1", "channel": 2}
    repl = emit_renode_platform(load_chip(DEVICES_ROOT, chip_id), board)
    assert "ReceiveDmaRequest -> dma1@2" in repl
    # ...and it is a line of the usart2 block, not a stray top-level entry.
    uart_block = repl.split("usart2: UART.STM32F7_USART")[1].split("\n\n")[0]
    assert "ReceiveDmaRequest -> dma1@2" in uart_block


@skip_no_devices
def test_the_wire_follows_the_board_assignment_not_a_constant() -> None:
    """Perturb the channel: the emitted wire must move with it. A hardcoded
    channel would keep the previous test green and silently misroute."""
    chip = load_chip(DEVICES_ROOT, "st/stm32g071rb")
    board = copy.deepcopy(_board("nucleo_g071rb"))
    board["dma"]["debug_uart.rx"] = {"controller": "dma1", "channel": 5}
    repl = emit_renode_platform(chip, board)
    assert "ReceiveDmaRequest -> dma1@5" in repl
    assert "ReceiveDmaRequest -> dma1@2" not in repl


@skip_no_devices
def test_no_assignment_no_wire() -> None:
    """A board that does not assign debug_uart.rx gets no wire at all — the
    platform must not invent a hookup the firmware has no route for."""
    chip = load_chip(DEVICES_ROOT, "st/stm32g071rb")
    board = copy.deepcopy(_board("nucleo_g071rb"))
    del board["dma"]["debug_uart.rx"]
    repl = emit_renode_platform(chip, board)
    assert "ReceiveDmaRequest" not in repl


@skip_no_devices
def test_an_assignment_on_an_unmodelled_controller_emits_no_wire() -> None:
    """Same rule as _adc_dma_wire: an assignment onto a controller this
    platform does not emit (g0b1re's dma2 has no modelled block) must not
    produce a dangling reference."""
    chip = load_chip(DEVICES_ROOT, "st/stm32g0b1re")
    board = copy.deepcopy(_board("nucleo_g0b1re"))
    board["dma"]["debug_uart.rx"] = {"controller": "dma2", "channel": 1}
    repl = emit_renode_platform(chip, board)
    assert "ReceiveDmaRequest" not in repl
