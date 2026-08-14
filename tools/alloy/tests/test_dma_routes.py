"""DMA route emission + board.json `dma:` assignment legality.

The contract (docs/design/dma-streams.md §1): the BOARD states which
controller+channel serves which role signal; the REQUEST id stays the chip's
fact and never appears in board.json. `dma_assignment_problems` is the one
expression of the legality rules — the emitter refuses the first problem, the
validator reports all of them — so these tests exercise the shared function
directly and then pin both consumers to it.

Phase-1 shape only: DMAMUX / free-router families (G0/G4), where any dma-class
channel may serve any request and the only inter-assignment rule is collision.
The F4/F7 triple check is a marked extension point, deliberately absent here.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import pytest

from alloy_cli.emit.board import dma_assignment_problems, dma_signal_candidates

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


# ------------------------------------------------------------ synthetic die

CHIP: dict[str, Any] = {
    "vendor": "st", "part": "SYNTH",
    "peripherals": {
        "usart2": {"ip": "st/usart_v4", "base": "0x40004400",
                   "dma_requests": {"rx": 52, "tx": 53}},
        # `channels` here is the ADC's vref/temp map — the SAME key a DMA
        # controller uses for its geometry. The rules must never read one as
        # the other.
        "adc": {"ip": "st/adc_v2", "base": "0x40012400",
                "dma_requests": {"conv": 5},
                "channels": {"vref": 13, "temp": 12}},
        "i2c1": {"ip": "st/i2c_v2", "base": "0x40005400"},
        "tim2": {"ip": "st/tim_gp16", "base": "0x40000000",
                 "dma_requests": {"ch1": 26, "up": 31}},
        "dma1": {"ip": "st/dma_v1", "base": "0x40020000",
                 "channels": {"count": 7}},
        "dmamux": {"ip": "st/dmamux_v1", "base": "0x40020800"},
        "uncur": {"uncurated": True, "base": "0x40001000"},
    },
}

REGISTERS: dict[str, dict[str, Any]] = {
    "st/dma_v1": {"class": "dma"},
    "st/dmamux_v1": {"class": "dmamux"},
    "st/usart_v4": {"class": "uart"},
    "st/adc_v2": {"class": "adc"},
    "st/i2c_v2": {"class": "i2c"},
    "st/tim_gp16": {"class": "timer"},
}


def _board(dma: dict[str, Any] | None) -> dict[str, Any]:
    board: dict[str, Any] = {
        "id": "synth",
        "roles": {
            "debug_uart": {"peripheral": "usart2", "tx": "pa2", "rx": "pa3"},
            "adc": {"peripheral": "adc"},
            "i2c": {"peripheral": "i2c1", "scl": "pb8", "sda": "pb9"},
        },
    }
    if dma is not None:
        board["dma"] = dma
    return board


def _problems(dma: dict[str, Any] | None) -> list[dict[str, Any]]:
    return dma_assignment_problems(_board(dma), CHIP, REGISTERS)


# ----------------------------------------------------------------- legality

def test_no_dma_map_is_no_problem() -> None:
    assert _problems(None) == []
    assert _problems({}) == []


def test_a_legal_assignment_is_clean() -> None:
    assert _problems({"adc.conv": {"controller": "dma1", "channel": 1},
                      "debug_uart.rx": {"controller": "dma1", "channel": 2}}) == []


def test_candidates_are_the_declared_roles_request_signals() -> None:
    """i2c1 advertises no dma_requests, so no i2c.* candidate may appear —
    suggesting an assignment the same rules would then refuse is worse than
    suggesting nothing."""
    assert dma_signal_candidates(_board(None), CHIP) == [
        "adc.conv", "debug_uart.rx", "debug_uart.tx"]


def test_a_key_that_is_not_role_dot_signal_is_named() -> None:
    for key in ("adcconv", "adc.conv.extra", ".conv", "adc.", "adc.Conv!"):
        problems = _problems({key: {"controller": "dma1", "channel": 1}})
        assert len(problems) == 1 and problems[0]["key"] == key
        assert "role.signal" in problems[0]["message"]
        assert "adc.conv" in problems[0]["suggestions"]


def test_an_undeclared_role_is_refused_with_candidates() -> None:
    problems = _problems({"spi.rx": {"controller": "dma1", "channel": 1}})
    assert len(problems) == 1
    assert "'spi' is not a peripheral-bearing role" in problems[0]["message"]
    assert "adc.conv" in problems[0]["suggestions"]


def test_a_signal_the_chip_states_no_request_for_is_refused() -> None:
    problems = _problems({"debug_uart.conv": {"controller": "dma1", "channel": 1}})
    assert len(problems) == 1
    assert "no DMA request for usart2 'conv'" in problems[0]["message"]
    assert problems[0]["suggestions"] == ["debug_uart.rx", "debug_uart.tx"]


def test_a_peripheral_with_no_requests_at_all_says_so() -> None:
    """i2c1 is declared and curated but carries no dma_requests (the exact
    shape the g071rb hand file had before this work): the message must blame
    the DATA, and the suggestions fall back to what IS assignable."""
    problems = _problems({"i2c.rx": {"controller": "dma1", "channel": 1}})
    assert len(problems) == 1
    assert "advertises no DMA requests at all" in problems[0]["message"]
    assert problems[0]["suggestions"] == ["adc.conv", "debug_uart.rx", "debug_uart.tx"]


def test_a_controller_that_is_not_dma_class_is_refused() -> None:
    for controller in ("tim2", "dmamux", "uncur", "dma9", None):
        problems = _problems({"adc.conv": {"controller": controller, "channel": 1}})
        assert len(problems) == 1
        assert "not a DMA controller" in problems[0]["message"]
        assert problems[0]["suggestions"] == ["dma1"]


def test_a_channel_outside_the_controllers_geometry_is_refused() -> None:
    """dma_v1 channels are 1-based: 0 is as illegal as count+1."""
    for channel in (0, 8, -1):
        problems = _problems({"adc.conv": {"controller": "dma1", "channel": channel}})
        assert len(problems) == 1
        assert "channels 1..7" in problems[0]["message"]
        assert problems[0]["suggestions"][0] == "1"


def test_a_channel_that_is_not_an_integer_is_refused() -> None:
    for channel in ("1", 1.0, True, None):
        problems = _problems({"adc.conv": {"controller": "dma1", "channel": channel}})
        assert len(problems) == 1
        assert "integer" in problems[0]["message"]


def test_a_malformed_assignment_value_is_refused() -> None:
    problems = _problems({"adc.conv": "dma1 ch1"})
    assert len(problems) == 1
    assert '{"controller"' in problems[0]["message"]


def test_two_assignments_on_one_channel_collide() -> None:
    """One channel moves one stream. The collision names the earlier claimant
    and the suggestions are channels still free on that controller."""
    problems = _problems({"adc.conv": {"controller": "dma1", "channel": 1},
                          "debug_uart.rx": {"controller": "dma1", "channel": 1}})
    assert len(problems) == 1
    assert problems[0]["key"] == "debug_uart.rx"
    assert "already serves 'adc.conv'" in problems[0]["message"]
    assert "1" not in problems[0]["suggestions"]
    assert "2" in problems[0]["suggestions"]


def test_every_problem_is_reported_not_just_the_first() -> None:
    problems = _problems({
        "adc.conv": {"controller": "dma1", "channel": 1},
        "debug_uart.rx": {"controller": "dma1", "channel": 1},   # collision
        "debug_uart.tx": {"controller": "tim2", "channel": 2},   # not a DMA
        "spi.rx": {"controller": "dma1", "channel": 3},          # no such role
    })
    assert {p["key"] for p in problems} == {"debug_uart.rx", "debug_uart.tx", "spi.rx"}


# ----------------------------------------------- the emitter and the verb
# consume the same rules

@skip_no_devices
def test_the_emitter_refuses_what_the_rules_refuse() -> None:
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header
    from alloy_cli.emit.common import EmitError

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    board["dma"] = {"adc.conv": {"controller": "dma1", "channel": 1},
                    "debug_uart.rx": {"controller": "dma1", "channel": 1}}
    chip = load_chip(DEVICES_ROOT, board["chip"])
    registers = load_registers(DEVICES_ROOT)
    with pytest.raises(EmitError) as excinfo:
        emit_board_header(board, chip, registers)
    message = str(excinfo.value)
    assert "dma1 channel 1" in message and "adc.conv" in message


@skip_no_devices
def test_the_validator_reports_the_same_problems_with_locations() -> None:
    from alloy_cli.board_validate import validate_board
    from alloy_cli.devices import load_chip, load_registers

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    board["dma"] = {"adc.conv": {"controller": "dma9", "channel": 1},
                    "debug_uart.rx": {"controller": "dma1", "channel": 99}}
    chip = load_chip(DEVICES_ROOT, board["chip"])
    registers = load_registers(DEVICES_ROOT)
    issues = [i for i in validate_board(board, chip, registers)
              if i["level"] == "error" and (i["field"] or "").startswith("dma.")]
    assert {i["field"] for i in issues} == {"dma.adc.conv", "dma.debug_uart.rx"}
    by_field = {i["field"]: i for i in issues}
    assert "dma1" in by_field["dma.adc.conv"]["suggestions"]
    assert "1" in by_field["dma.debug_uart.rx"]["suggestions"]


# ------------------------------------------------------------------ emission

@skip_no_devices
def test_a_legal_map_emits_typed_routes_under_board_dma() -> None:
    """The doc's §1 example, verbatim shape: the request id (5, the chip's
    fact) appears in the emitted constant and NOWHERE in the board file."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    board["dma"] = {"adc.conv": {"controller": "dma1", "channel": 1},
                    "debug_uart.rx": {"controller": "dma1", "channel": 2}}
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 1, "
            "/*request=*/5> adc_conv{};") in out
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 2, "
            "/*request=*/52> debug_uart_rx{};") in out
    assert '#include "alloy/dma.hpp"' in out


@skip_no_devices
def test_a_board_that_assigns_nothing_still_gets_the_namespace() -> None:
    """`namespace board::dma` must exist on EVERY board so a requires-probe
    for a route is well-formed everywhere; only the constants are conditional
    (and so is the alloy/dma.hpp include)."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_f767zi" / "board.json").read_text())
    assert "dma" not in board, "fixture assumption: f767zi assigns no routes yet"
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    assert "namespace dma {\n}  // namespace dma" in out
    assert "alloy::dma::route" not in out
    assert '#include "alloy/dma.hpp"' not in out
