"""The two chip facts a pin-interrupt driver needs, on their way into C++.

st/exti_g0's driver never writes down which vector a line raises or which value
routes a port — it reads both off the emitted instance descriptor. So the fact
has to survive the trip from chip data into device.hpp EXACTLY, and it has to
disappear entirely for a chip that does not declare it (or a driver would gate
on a constant that is always there).

Each test perturbs a fact and asserts the header moves with it. A constant
hardcoded in the emitter would keep these green with the data changed, which is
precisely the failure they are shaped to catch.
"""

from __future__ import annotations

import copy
import json
from pathlib import Path

import pytest

from alloy_cli.devices import load_chip, load_registers
from alloy_cli.emit.common import EmitError
from alloy_cli.emit.device import emit_device_header

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _emit(chip: dict) -> str:
    return emit_device_header(chip, load_registers(DEVICES_ROOT), [])


def _g071() -> dict:
    return load_chip(DEVICES_ROOT, "st/stm32g071rb")


@skip_no_devices
def test_the_exti_carries_its_vector_groups_as_a_table() -> None:
    out = _emit(_g071())
    assert "static constexpr unsigned irq_line_count = 3u;" in out
    assert "static constexpr alloy::irq_line_range irq_lines[3] = {" in out
    # The G0 groups 16 pin lines onto three vectors. Numbers come from the
    # chip's interrupts table (EXTI0_1=5, EXTI2_3=6, EXTI4_15=7).
    assert "{0u, 1u, alloy::irq_line{5}}," in out
    assert "{2u, 3u, alloy::irq_line{6}}," in out
    assert "{4u, 15u, alloy::irq_line{7}}," in out


@skip_no_devices
def test_moving_a_group_moves_the_emitted_vector() -> None:
    chip = copy.deepcopy(_g071())
    # Same three vectors, different split: 0-3 on EXTI0_1, 4-5 on EXTI2_3.
    chip["peripherals"]["exti"]["irq_lines"] = [
        {"irq": "EXTI0_1", "first": 0, "last": 3},
        {"irq": "EXTI2_3", "first": 4, "last": 5},
        {"irq": "EXTI4_15", "first": 6, "last": 15},
    ]
    out = _emit(chip)
    assert "{0u, 3u, alloy::irq_line{5}}," in out
    assert "{4u, 5u, alloy::irq_line{6}}," in out
    assert "{0u, 1u, alloy::irq_line{5}}," not in out


@skip_no_devices
def test_groups_are_emitted_lowest_line_first_whatever_the_data_order() -> None:
    chip = copy.deepcopy(_g071())
    chip["peripherals"]["exti"]["irq_lines"] = list(
        reversed(chip["peripherals"]["exti"]["irq_lines"]))
    out = _emit(chip)
    body = out[out.index("irq_lines[3]"):]
    assert body.index("{0u, 1u,") < body.index("{2u, 3u,") < body.index("{4u, 15u,")


@skip_no_devices
def test_a_vector_name_that_is_not_an_interrupt_fails_generation() -> None:
    chip = copy.deepcopy(_g071())
    chip["peripherals"]["exti"]["irq_lines"][0]["irq"] = "EXTI0_9"
    with pytest.raises(EmitError, match="EXTI0_9"):
        _emit(chip)


@skip_no_devices
def test_an_inverted_range_fails_generation() -> None:
    chip = copy.deepcopy(_g071())
    chip["peripherals"]["exti"]["irq_lines"][0] = {
        "irq": "EXTI0_1", "first": 1, "last": 0}
    with pytest.raises(EmitError, match="inverts"):
        _emit(chip)


@skip_no_devices
def test_port_index_is_the_silicon_code_not_the_alphabetical_position() -> None:
    """The G071RB has NO port E. gpiof is the fifth curated port but EXTICR
    code 5, so an emitter that counted ports would emit 4 and every edge on
    port F would be routed to a port that does not exist."""
    out = _emit(_g071())
    assert "struct gpiof_t {" in out
    tail = out[out.index("struct gpiof_t {"):]
    assert "static constexpr unsigned port_index = 5u;" in tail[:tail.index("\n};")]


@skip_no_devices
def test_a_port_names_the_controller_that_serves_it() -> None:
    """The companion is how a pin driver finds its interrupt controller —
    without it the driver would have to assume there is exactly one."""
    out = _emit(_g071())
    tail = out[out.index("struct gpioc_t {"):]
    assert "using exti_t = alloy::dev::exti_t;" in tail[:tail.index("\n};")]


@skip_no_devices
def test_a_chip_without_the_data_gets_neither_constant() -> None:
    chip = copy.deepcopy(_g071())
    del chip["peripherals"]["exti"]
    for name in ("gpioa", "gpiob", "gpioc", "gpiod", "gpiof"):
        chip["peripherals"][name].pop("port_index", None)
        chip["peripherals"][name].pop("companions", None)
    out = _emit(chip)
    assert "irq_line_range" not in out
    assert "port_index" not in out
    assert "exti_t" not in out


@skip_no_devices
def test_a_chip_that_never_had_an_exti_is_unaffected() -> None:
    """Every other ST chip in the database must be byte-identical: the two keys
    are optional, and nothing else in the descriptor may have moved."""
    chip = load_chip(DEVICES_ROOT, "st/stm32f767zi")
    out = _emit(chip)
    assert "irq_line_range" not in out
    assert "port_index" not in out


def test_board_json_declares_the_pin_irq_example_board() -> None:
    """The emulation leg presses PC13 on the Nucleo-G071RB; if the board ever
    stopped declaring that pin as its button the leg would pass by printing the
    'not available' line instead."""
    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    assert board["roles"]["button"]["pin"] == "pc13"
    assert board["roles"]["button"]["active"] == "low"
