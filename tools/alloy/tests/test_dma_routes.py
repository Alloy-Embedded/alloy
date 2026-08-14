"""DMA route emission + board.json `dma:` assignment legality.

The contract (docs/design/dma-streams.md §1): the BOARD states which
controller+channel serves which role signal; the REQUEST id stays the chip's
fact and never appears in board.json. `dma_assignment_problems` is the one
expression of the legality rules — the emitter refuses the first problem, the
validator reports all of them — so these tests exercise the shared function
directly and then pin both consumers to it.

Two silicon shapes, each with its own fixture die:

- DMAMUX / free-router (G0/G4, `dma_requests`): any dma-class channel may
  serve any request; the only inter-assignment rule is collision. Channels
  are 1-BASED (dma_v1).
- Stream engine (F4/F7, `dma_routes` triples — phase 3, filling the marked
  extension point): one signal reaches ONLY its triples, the matched triple
  supplies the request (CHSEL), refusal prints every legal
  "{controller, stream}" (design §1), and streams are 0-BASED per ST
  numbering — the tests pin the loudness of that distinction both ways.
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


def test_a_stream_key_on_a_free_router_chip_names_both_bases() -> None:
    """The reverse confusion must be as loud as the forward one: writing the
    stream engines' 0-based key on a DMAMUX board is refused naming BOTH
    numbering bases, never silently read as a channel (the off-by-one that
    would misroute every transfer by one channel)."""
    problems = _problems({"adc.conv": {"controller": "dma1", "stream": 1}})
    assert len(problems) == 1
    assert "'channel' (1-based" in problems[0]["message"]
    assert "0-based" in problems[0]["message"]


# ------------------------------------------- the stream-engine shape (F4/F7)
#
# A synthetic F7-shaped die: usart3's rx reaches exactly one triple, tx
# reaches two (the real RM0431 shape for USART3_TX: DMA1 stream 3 CHSEL 4 or
# stream 4 CHSEL 7 — different requests on purpose, so a test can PROVE the
# request follows the matched triple). `spare` overlaps rx's stream for the
# collision test.

STREAM_CHIP: dict[str, Any] = {
    "vendor": "st", "part": "SYNTH_F7",
    "peripherals": {
        "usart3": {"ip": "st/usart_v3", "base": "0x40004800",
                   "dma_routes": {
                       "rx": [{"controller": "dma1", "stream": 1, "request": 4}],
                       "tx": [{"controller": "dma1", "stream": 3, "request": 4},
                              {"controller": "dma1", "stream": 4, "request": 7}]}},
        "spare1": {"ip": "st/spi_v2", "base": "0x40013000",
                   "dma_routes": {
                       "rx": [{"controller": "dma1", "stream": 1, "request": 6}]}},
        "dma1": {"ip": "st/dma_v2", "base": "0x40026000"},
        "dma2": {"ip": "st/dma_v2", "base": "0x40026400"},
    },
}

STREAM_REGISTERS: dict[str, dict[str, Any]] = {
    "st/dma_v2": {"class": "dma"},
    "st/usart_v3": {"class": "uart"},
    "st/spi_v2": {"class": "spi"},
}


def _stream_board(dma: dict[str, Any] | None) -> dict[str, Any]:
    board: dict[str, Any] = {
        "id": "synth_f7",
        "roles": {
            "debug_uart": {"peripheral": "usart3", "tx": "pd8", "rx": "pd9"},
            "spi": {"peripheral": "spare1", "sck": "pa5", "miso": "pa6",
                    "mosi": "pa7"},
        },
    }
    if dma is not None:
        board["dma"] = dma
    return board


def _stream_problems(dma: dict[str, Any] | None) -> list[dict[str, Any]]:
    return dma_assignment_problems(_stream_board(dma), STREAM_CHIP,
                                   STREAM_REGISTERS)


def test_stream_candidates_come_from_the_routes() -> None:
    assert dma_signal_candidates(_stream_board(None), STREAM_CHIP) == [
        "debug_uart.rx", "debug_uart.tx", "spi.rx"]


def test_a_matching_triple_is_clean_and_so_is_the_alternative() -> None:
    assert _stream_problems({
        "debug_uart.rx": {"controller": "dma1", "stream": 1},
        "debug_uart.tx": {"controller": "dma1", "stream": 3}}) == []
    assert _stream_problems({
        "debug_uart.tx": {"controller": "dma1", "stream": 4}}) == []


def test_a_non_matching_stream_prints_every_legal_triple() -> None:
    """The design §1 promise, verbatim: refusal lists the legal
    "{controller, stream}" alternatives — in the MESSAGE (the emitter shows
    only the first problem's text) and in the suggestions (board_validate's
    structured field)."""
    problems = _stream_problems({"debug_uart.tx": {"controller": "dma1",
                                                   "stream": 5}})
    assert len(problems) == 1
    assert "{dma1, stream 5} does not reach usart3 'tx'" in problems[0]["message"]
    assert "{dma1, stream 3}" in problems[0]["message"]
    assert "{dma1, stream 4}" in problems[0]["message"]
    assert problems[0]["suggestions"] == ["{dma1, stream 3}", "{dma1, stream 4}"]


def test_the_wrong_controller_is_refused_the_same_way() -> None:
    """The doc's own example: a controller the triples never name, even a
    real dma-class one, is a generation error listing the legal options."""
    problems = _stream_problems({"debug_uart.rx": {"controller": "dma2",
                                                   "stream": 1}})
    assert len(problems) == 1
    assert "{dma2, stream 1} does not reach usart3 'rx'" in problems[0]["message"]
    assert problems[0]["suggestions"] == ["{dma1, stream 1}"]


def test_the_channel_key_on_a_stream_engine_names_both_bases() -> None:
    """dma_v1's 1-based `channel` on an F7 assignment is the off-by-one trap
    the phase-1 rename exists to kill: refused by NAME, stating both
    numbering bases, and still listing the legal triples."""
    problems = _stream_problems({"debug_uart.tx": {"controller": "dma1",
                                                   "channel": 3}})
    assert len(problems) == 1
    assert "'stream' (0-based" in problems[0]["message"]
    assert "'channel' (dma_v1's 1-based)" in problems[0]["message"]
    assert problems[0]["suggestions"] == ["{dma1, stream 3}", "{dma1, stream 4}"]


def test_a_stream_that_is_not_an_integer_is_refused() -> None:
    for stream in ("1", 1.0, True, None):
        problems = _stream_problems({"debug_uart.rx": {"controller": "dma1",
                                                       "stream": stream}})
        assert len(problems) == 1
        assert "'stream' must be an integer" in problems[0]["message"]


def test_a_malformed_stream_assignment_says_stream_not_channel() -> None:
    problems = _stream_problems({"debug_uart.rx": "dma1 s1"})
    assert len(problems) == 1
    assert '"stream"' in problems[0]["message"]


def test_two_signals_on_one_stream_collide() -> None:
    """Stream collisions are refused exactly like channel collisions; the
    free-alternatives list is honest — spare1's only triple is taken, so it
    suggests nothing rather than something illegal."""
    problems = _stream_problems({
        "debug_uart.rx": {"controller": "dma1", "stream": 1},
        "spi.rx": {"controller": "dma1", "stream": 1}})
    assert len(problems) == 1
    assert problems[0]["key"] == "spi.rx"
    assert "already serves 'debug_uart.rx'" in problems[0]["message"]
    assert problems[0]["suggestions"] == []


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
@pytest.mark.parametrize("board_id", ["nucleo_g0b1re", "nucleo_g071rb"])
def test_the_shipped_g0_boards_route_the_adc_ring(board_id: str) -> None:
    """The phase-1 anchor (doc §2.1) starts here: both curated G0 boards
    assign adc.conv to dma1 channel 1, and the emitted route carries request 5
    — the RM0444 DMAMUX id, read from the chip file, absent from board.json."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    assert board["dma"]["adc.conv"] == {"controller": "dma1", "channel": 1}
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 1, "
            "/*request=*/5> adc_conv{};") in out


@skip_no_devices
@pytest.mark.parametrize("board_id", ["nucleo_g0b1re", "nucleo_g071rb"])
def test_the_shipped_g0_boards_route_the_debug_uart(board_id: str) -> None:
    """The phase-2 anchors (doc §2.2/§2.3) start here: both curated G0 boards
    assign debug_uart.rx/tx to dma1 channels 2/3 (the doc §1 example split;
    adc.conv holds channel 1), and the emitted routes carry requests 52/53 —
    the RM0444 DMAMUX ids for USART2_RX/TX, stated by dmamux_v1.yaml and each
    chip's usart2 `dma_requests`, absent from board.json."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    assert board["dma"]["debug_uart.rx"] == {"controller": "dma1", "channel": 2}
    assert board["dma"]["debug_uart.tx"] == {"controller": "dma1", "channel": 3}
    assert board["roles"]["debug_uart"]["peripheral"] == "usart2"
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 2, "
            "/*request=*/52> debug_uart_rx{};") in out
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 3, "
            "/*request=*/53> debug_uart_tx{};") in out


@skip_no_devices
def test_a_board_that_assigns_nothing_still_gets_the_namespace() -> None:
    """`namespace board::dma` must exist on EVERY board so a requires-probe
    for a route is well-formed everywhere; only the constants are conditional
    (and so is the alloy/dma.hpp include). Fixture moved off f767zi when
    phase 3 gave the F7 boards their assignments; same70 still assigns none
    (its XDMAC backend has no circular mode, doc §3.4)."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / "same70_xplained" / "board.json").read_text())
    assert "dma" not in board, "fixture assumption: same70 assigns no routes"
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    assert "namespace dma {\n}  // namespace dma" in out
    assert "alloy::dma::route" not in out
    assert '#include "alloy/dma.hpp"' not in out


@skip_no_devices
@pytest.mark.parametrize("board_id", ["nucleo_g0b1re", "nucleo_g071rb"])
def test_the_adc_conv_route_rides_on_the_binder(board_id: str) -> None:
    """Design §1: the generator emits the constant AND 'attaches it to the
    role's binder'. The binder attachment is what makes the anchor spelling
    `adc.ring(samples)` real — the handle knows its route without the user
    naming one — and it must be the SAME type the board::dma constant has
    (one helper spells both, so they cannot drift)."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    adc_periph = board["roles"]["adc"]["peripheral"]
    assert (f"using adc = alloy::adc::bind<alloy::dev::{adc_periph}_t, "
            "clock_profile, alloy::dma::route<alloy::dev::dma1_t, 1, "
            "/*request=*/5>>;") in out


@skip_no_devices
def test_a_board_without_the_assignment_keeps_the_two_parameter_binder() -> None:
    """No route -> ConvRoute defaults to void inside the facade and ring() is
    constrained away; the emitted binder must stay the two-parameter spelling
    so that default actually applies."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    del board["dma"]
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    adc_periph = board["roles"]["adc"]["peripheral"]
    assert (f"using adc = alloy::adc::bind<alloy::dev::{adc_periph}_t, "
            "clock_profile>;") in out


@skip_no_devices
@pytest.mark.parametrize("board_id", ["nucleo_g0b1re", "nucleo_g071rb"])
def test_the_debug_uart_rx_route_rides_on_the_binder(board_id: str) -> None:
    """Design §1 for the phase-2 anchor (§2.2): the debug_uart.rx assignment
    is attached to the uart binder as an alloy::uart::rx_dma<> tag — the fact
    that makes `uart.rx_ring(rxbuf)` compile on the wired boards. Same type
    spelling as the board::dma constant (_dma_route_type emits both)."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    assert ("alloy::uart::rx_dma<alloy::dma::route<alloy::dev::dma1_t, 2, "
            "/*request=*/52>>") in out
    # The tag rides the debug_uart bind specifically, after clock_profile.
    bind = out.split("using debug_uart = ")[1].split(";")[0]
    assert "alloy::uart::rx_dma<" in bind


@skip_no_devices
def test_a_board_without_the_rx_assignment_keeps_the_untagged_uart_binder() -> None:
    """No debug_uart rx/tx assignments -> no dma tags -> the handle's RxRoute
    defaults to void and rx_ring(storage) is constrained away, tx_route is
    void and the route-claim branch folds."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    del board["dma"]["debug_uart.rx"]
    del board["dma"]["debug_uart.tx"]
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    bind = out.split("using debug_uart = ")[1].split(";")[0]
    assert "rx_dma" not in bind
    assert "tx_dma" not in bind
    assert bind.rstrip().endswith("clock_profile>")


@skip_no_devices
def test_the_rx_tag_moves_with_the_board_statement() -> None:
    """The channel in the tag is the board's statement, untranslated — perturb
    the assignment and the emitted tag must move with it (the same data-driven
    defense the renode wire tests state for their numbers).

    The perturbation must land on a channel this board leaves FREE, or the
    collision rule refuses it before the tag is ever emitted — which is why
    phase 4 kept dma1 channel 7 in reserve here (see
    test_g071rb_keeps_a_channel_in_reserve)."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    board["dma"]["debug_uart.rx"] = {"controller": "dma1", "channel": 7}
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    assert ("alloy::uart::rx_dma<alloy::dma::route<alloy::dev::dma1_t, 7, "
            "/*request=*/52>>") in out


@skip_no_devices
@pytest.mark.parametrize("board_id", ["nucleo_g0b1re", "nucleo_g071rb"])
def test_the_debug_uart_tx_route_rides_on_the_binder_too(board_id: str) -> None:
    """Anchor 2.3's portable gate: the debug_uart.tx assignment is attached
    as alloy::uart::tx_dma<> -> the binder's `tx_route` — the dependent name
    a requires-probe can fold on (the namespace-scope board::dma constant
    cannot). Same type spelling as board::dma::debug_uart_tx."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    bind = out.split("using debug_uart = ")[1].split(";")[0]
    assert ("alloy::uart::tx_dma<alloy::dma::route<alloy::dev::dma1_t, 3, "
            "/*request=*/53>>") in bind


@skip_no_devices
def test_a_tx_only_assignment_grows_only_the_tx_tag() -> None:
    """Each signal's tag is independent: dropping the rx assignment must drop
    rx_dma<> and keep tx_dma<>."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    del board["dma"]["debug_uart.rx"]
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    bind = out.split("using debug_uart = ")[1].split(";")[0]
    assert "rx_dma" not in bind
    assert "alloy::uart::tx_dma<" in bind


# ------------------------------------------- stream-engine emission (F4/F7)
#
# The phase-3 promise (doc §6): the SAME portable examples open on the F7
# Nucleos by adding ONLY board.json dma assignments. These tests pin the
# board half of that promise: the shipped assignments emit the same
# constant + binder-tag shape the G0 boards get, with the request read from
# the matched dma_routes triple.

def _emit_shipped(board_id: str, mutate=None) -> str:
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    if mutate is not None:
        mutate(board)
    return emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                             load_registers(DEVICES_ROOT))


@skip_no_devices
@pytest.mark.parametrize("board_id", ["nucleo_f722ze", "nucleo_f767zi"])
def test_the_shipped_f7_boards_route_the_debug_uart(board_id: str) -> None:
    """Both Nucleo-144s put the VCP on USART3 and assign {dma1, stream 1}
    (rx) / {dma1, stream 3} (tx). The emitted route number is the 0-BASED
    STREAM, and the request ids (4/4) are the triples' CHSEL — chip facts,
    absent from board.json."""
    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    assert board["dma"]["debug_uart.rx"] == {"controller": "dma1", "stream": 1}
    assert board["dma"]["debug_uart.tx"] == {"controller": "dma1", "stream": 3}
    assert board["roles"]["debug_uart"]["peripheral"] == "usart3"
    out = _emit_shipped(board_id)
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 1, "
            "/*request=*/4> debug_uart_rx{};") in out
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 3, "
            "/*request=*/4> debug_uart_tx{};") in out
    assert '#include "alloy/dma.hpp"' in out


@skip_no_devices
@pytest.mark.parametrize("board_id", ["nucleo_f722ze", "nucleo_f767zi"])
def test_the_f7_routes_ride_the_binder_like_the_g0_ones(board_id: str) -> None:
    """Same attachment shape as G0: rx_dma<>/tx_dma<> tags on the debug_uart
    bind, same type spelling as the board::dma constants (_dma_route_type
    emits both, so they cannot drift)."""
    out = _emit_shipped(board_id)
    bind = out.split("using debug_uart = ")[1].split(";")[0]
    assert ("alloy::uart::rx_dma<alloy::dma::route<alloy::dev::dma1_t, 1, "
            "/*request=*/4>>") in bind
    assert ("alloy::uart::tx_dma<alloy::dma::route<alloy::dev::dma1_t, 3, "
            "/*request=*/4>>") in bind


@skip_no_devices
def test_the_request_follows_the_matched_triple() -> None:
    """The anti-drift proof the G0 mutation tests cannot state: on a stream
    engine the request is NOT a per-signal constant — moving tx from stream 3
    to its other legal triple (stream 4) must move the request from 4 to 7,
    because the CHSEL rides the triple, not the signal."""
    def move_tx(board): board["dma"]["debug_uart.tx"] = {
        "controller": "dma1", "stream": 4}
    out = _emit_shipped("nucleo_f722ze", move_tx)
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 4, "
            "/*request=*/7> debug_uart_tx{};") in out
    bind = out.split("using debug_uart = ")[1].split(";")[0]
    assert ("alloy::uart::tx_dma<alloy::dma::route<alloy::dev::dma1_t, 4, "
            "/*request=*/7>>") in bind


@skip_no_devices
def test_the_emitter_refuses_an_off_triple_stream_printing_the_legal_ones() -> None:
    """Generation (not just validation) refuses a stream the triples never
    name, and the refusal text carries the legal alternatives — the §1
    promise at the emitter's door."""
    from alloy_cli.emit.common import EmitError

    def wreck_tx(board): board["dma"]["debug_uart.tx"] = {
        "controller": "dma1", "stream": 5}
    with pytest.raises(EmitError) as excinfo:
        _emit_shipped("nucleo_f722ze", wreck_tx)
    message = str(excinfo.value)
    assert "{dma1, stream 5} does not reach usart3 'tx'" in message
    assert "{dma1, stream 3}" in message and "{dma1, stream 4}" in message


@skip_no_devices
def test_the_validator_mirrors_the_stream_rules_with_suggestions() -> None:
    """board_validate reports the same triple check through the one shared
    function, suggestions capped like every other issue."""
    from alloy_cli.board_validate import MAX_SUGGESTIONS, validate_board
    from alloy_cli.devices import load_chip, load_registers

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_f767zi" / "board.json").read_text())
    board["dma"]["debug_uart.rx"] = {"controller": "dma2", "stream": 1}
    board["dma"]["debug_uart.tx"] = {"controller": "dma1", "channel": 3}
    chip = load_chip(DEVICES_ROOT, board["chip"])
    issues = [i for i in validate_board(board, chip, load_registers(DEVICES_ROOT))
              if i["level"] == "error" and (i["field"] or "").startswith("dma.")]
    by_field = {i["field"]: i for i in issues}
    assert set(by_field) == {"dma.debug_uart.rx", "dma.debug_uart.tx"}
    assert by_field["dma.debug_uart.rx"]["suggestions"] == ["{dma1, stream 1}"]
    assert "'stream' (0-based" in by_field["dma.debug_uart.tx"]["message"]
    for issue in issues:
        assert len(issue["suggestions"]) <= MAX_SUGGESTIONS


# ------------------------------------------- the phase-4 signals (SPI / I2C)
#
# Anchor 2.4 (doc §2.4) is a PAIR: `spi.transfer_dma()` claims both channels,
# RX first. The board half of that is two assignments on one role — the first
# role in the tree to carry more than one signal — plus i2c's two one-shot
# directions. These tests pin the shipped statements and the refusals that
# guard them; the C++ facades gate on the same routes by their binder alias.

_G0_BOARDS = ["nucleo_g0b1re", "nucleo_g071rb"]


@skip_no_devices
@pytest.mark.parametrize("board_id", _G0_BOARDS)
def test_the_g0_boards_route_the_spi_pair(board_id: str) -> None:
    """Both curated G0 boards serve spi1 from dma1 channels 4 (rx) and 5 (tx),
    after the three phase-1/2 assignments. The requests — 16/17 — are the
    RM0444 DMAMUX ids for SPI1_RX/SPI1_TX, read from the chip file and absent
    from board.json exactly like every route before them."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    assert board["dma"]["spi.rx"] == {"controller": "dma1", "channel": 4}
    assert board["dma"]["spi.tx"] == {"controller": "dma1", "channel": 5}
    assert board["roles"]["spi"]["peripheral"] == "spi1"
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 4, "
            "/*request=*/16> spi_rx{};") in out
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 5, "
            "/*request=*/17> spi_tx{};") in out


@skip_no_devices
@pytest.mark.parametrize("board_id", _G0_BOARDS)
def test_the_g0_boards_route_the_i2c_read(board_id: str) -> None:
    """i2c1's RX gets channel 6 and request 10 (the RM0444 DMAMUX id for
    I2C1_RX). Unlike SPI, I2C's two directions never run together — it is half
    duplex — so these are one-shots that share a role, NOT a pair, and the
    both-or-neither rule that governs `spi.rx`/`spi.tx` does not apply."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    assert board["dma"]["i2c.rx"] == {"controller": "dma1", "channel": 6}
    assert board["roles"]["i2c"]["peripheral"] == "i2c1"
    out = emit_board_header(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT))
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 6, "
            "/*request=*/10> i2c_rx{};") in out


@skip_no_devices
def test_only_the_roomy_g0_board_routes_the_i2c_write() -> None:
    """`i2c.tx` (channel 7, request 11) is stated on nucleo_g0b1re and
    deliberately NOT on nucleo_g071rb, and the asymmetry is the point:

    - g071rb's die has ONE dma controller with seven channels; phase 4 would
      spend its last one on the direction nothing can exercise there. Renode
      1.16.1's model of this IP drives a request line from CR1.RXDMAEN only —
      TXDMAEN is an inert tag — so an emulated i2c DMA WRITE leg cannot be
      witnessed on the board both i2c legs run on.
    - g0b1re has a whole spare dma2, so it can afford the route, and the
      facade's write path gets a shipped board to compile against.

    A portable program therefore meets both branches of its own
    `requires`-gate across the board matrix, which is worth more than a
    uniform board file."""
    from alloy_cli.devices import load_chip, load_registers
    from alloy_cli.emit.board import emit_board_header

    roomy = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g0b1re" / "board.json").read_text())
    tight = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    assert roomy["dma"]["i2c.tx"] == {"controller": "dma1", "channel": 7}
    assert "i2c.tx" not in tight["dma"]
    out = emit_board_header(roomy, load_chip(DEVICES_ROOT, roomy["chip"]),
                            load_registers(DEVICES_ROOT))
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma1_t, 7, "
            "/*request=*/11> i2c_tx{};") in out
    tight_out = emit_board_header(tight, load_chip(DEVICES_ROOT, tight["chip"]),
                                  load_registers(DEVICES_ROOT))
    assert "i2c_tx{}" not in tight_out


@skip_no_devices
def test_the_f722_board_routes_the_spi_pair_on_the_stream_engine() -> None:
    """The F7 leg of anchor 2.4, and the promise that a new family opens the
    same example by adding board.json lines: nucleo_f722ze already declares the
    spi role, so two `dma:` lines are the WHOLE board change. dma2 streams 0
    and 3 both come from stm32f722's curated spi1 triples; dma1 streams 1/3
    stay with usart3, a different controller entirely."""
    out = _emit_shipped("nucleo_f722ze")
    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_f722ze" / "board.json").read_text())
    assert board["dma"]["spi.rx"] == {"controller": "dma2", "stream": 0}
    assert board["dma"]["spi.tx"] == {"controller": "dma2", "stream": 3}
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma2_t, 0, "
            "/*request=*/3> spi_rx{};") in out
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma2_t, 3, "
            "/*request=*/3> spi_tx{};") in out


@skip_no_devices
def test_the_f722_spi_pair_may_move_to_its_other_legal_streams() -> None:
    """stm32f722's spi1 triples offer dma2 stream 2 for rx and stream 5 for tx.
    Both alternatives generate, and the emitted route number follows the board
    statement — the assignment is a choice the board is entitled to make, not a
    constant the emitter re-derives."""
    def move(board):
        board["dma"]["spi.rx"] = {"controller": "dma2", "stream": 2}
        board["dma"]["spi.tx"] = {"controller": "dma2", "stream": 5}
    out = _emit_shipped("nucleo_f722ze", move)
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma2_t, 2, "
            "/*request=*/3> spi_rx{};") in out
    assert ("inline constexpr alloy::dma::route<alloy::dev::dma2_t, 5, "
            "/*request=*/3> spi_tx{};") in out


@skip_no_devices
def test_the_f722_spi_refusal_prints_the_legal_streams_per_direction() -> None:
    """The §1 promise on the new signals: rx and tx have DIFFERENT legal
    triples, so the refusal must print the ones for the direction that is
    wrong — not a merged list, and never the other direction's."""
    from alloy_cli.emit.common import EmitError

    def wreck_rx(board): board["dma"]["spi.rx"] = {"controller": "dma2",
                                                   "stream": 3}
    with pytest.raises(EmitError) as excinfo:
        _emit_shipped("nucleo_f722ze", wreck_rx)
    message = str(excinfo.value)
    assert "{dma2, stream 3} does not reach spi1 'rx'" in message
    assert "{dma2, stream 0}" in message and "{dma2, stream 2}" in message
    assert "stream 5" not in message  # that is TX's alternative, not RX's


@skip_no_devices
def test_the_channel_key_on_the_f722_spi_pair_names_both_bases() -> None:
    """The 1-based/0-based trap, now on a role that carries two signals: each
    half is checked on its own and each refusal carries its own direction's
    legal triples."""
    from alloy_cli.board_validate import validate_board
    from alloy_cli.devices import load_chip, load_registers

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_f722ze" / "board.json").read_text())
    board["dma"]["spi.rx"] = {"controller": "dma2", "channel": 0}
    board["dma"]["spi.tx"] = {"controller": "dma2", "channel": 3}
    chip = load_chip(DEVICES_ROOT, board["chip"])
    issues = {i["field"]: i for i in
              validate_board(board, chip, load_registers(DEVICES_ROOT))
              if i["level"] == "error" and (i["field"] or "").startswith("dma.")}
    assert set(issues) == {"dma.spi.rx", "dma.spi.tx"}
    assert issues["dma.spi.rx"]["suggestions"] == ["{dma2, stream 0}",
                                                   "{dma2, stream 2}"]
    assert issues["dma.spi.tx"]["suggestions"] == ["{dma2, stream 3}",
                                                   "{dma2, stream 5}"]
    for issue in issues.values():
        assert "'stream' (0-based" in issue["message"]
        assert "'channel' (dma_v1's 1-based)" in issue["message"]


@skip_no_devices
def test_a_pair_pointed_at_one_channel_collides_like_any_other() -> None:
    """Half a duplex is not a thing, and neither is a duplex on one channel:
    the pair's two halves collide under the SAME rule two roles do. The
    refusal names the earlier claimant — rx, because the keys are walked in
    sorted order and 'spi.rx' < 'spi.tx' — which is also the order §1 makes
    the pair claim them in."""
    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    board["dma"]["spi.tx"] = {"controller": "dma1", "channel": 4}
    from alloy_cli.devices import load_chip, load_registers

    problems = dma_assignment_problems(board, load_chip(DEVICES_ROOT,
                                                        board["chip"]),
                                       load_registers(DEVICES_ROOT))
    assert [p["key"] for p in problems] == ["spi.tx"]
    assert "already serves 'spi.rx'" in problems[0]["message"]


@skip_no_devices
def test_g071rb_keeps_a_channel_in_reserve() -> None:
    """The scarcity this board lives under, pinned so nobody spends the last
    channel by accident: stm32g071rb has ONE dma-class controller with seven
    channels, phase 4 leaves exactly one of them free, and the refusal a
    seventh signal gets can still offer it. Fill channel 7 as well and the
    suggestion list goes EMPTY — the honest answer, and the moment the board
    file needs a human."""
    from alloy_cli.devices import load_chip, load_registers

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    chip = load_chip(DEVICES_ROOT, board["chip"])
    registers = load_registers(DEVICES_ROOT)
    assert dma_assignment_problems(board, chip, registers) == []
    assert set(board["dma"]) == {"adc.conv", "debug_uart.rx", "debug_uart.tx",
                                 "spi.rx", "spi.tx", "i2c.rx"}
    assert chip["peripherals"]["dma1"]["channels"]["count"] == 7
    assert [n for n, p in chip["peripherals"].items()
            if p.get("ip") == "st/dma_v1"] == ["dma1"]
    # The reserve is real, and it is visible in what a refusal can offer: a
    # misassigned spi.tx is told about channel 5 (the one it just vacated) AND
    # channel 7 (the reserve).
    def collide(b: dict[str, Any]) -> list[str]:
        b = {**b, "dma": {**b["dma"], "spi.tx": {"controller": "dma1",
                                                 "channel": 1}}}
        found = dma_assignment_problems(b, chip, registers)
        assert [p["key"] for p in found] == ["spi.tx"]
        return found[0]["suggestions"]

    assert collide(board) == ["5", "7"]
    # Spend channel 7 on the seventh signal — legal, generates, and the die
    # is then wired to its last channel: the same refusal has only the
    # vacated one left to offer.
    board["dma"]["i2c.tx"] = {"controller": "dma1", "channel": 7}
    assert dma_assignment_problems(board, chip, registers) == []
    assert collide(board) == ["5"]
    # And this die advertises MORE assignable signals than it has channels —
    # led_pwm's `up`/`ch1` can never be served here at all, whatever a board
    # file says. Scarcity, not oversight.
    assert len(dma_signal_candidates(board, chip)) > 7


@skip_no_devices
@pytest.mark.parametrize("board_id", _G0_BOARDS + ["nucleo_f722ze"])
def test_a_duplex_role_never_ships_half_assigned(board_id: str) -> None:
    """The board-file discipline behind the pair: `spi.rx` and `spi.tx` are
    both stated or neither is. A board with one of them builds, and then
    `transfer_dma` is constrained away with nothing saying why the OTHER half
    is missing — a half-configured board that looks configured."""
    board = json.loads(
        (ALLOY_ROOT / "boards" / board_id / "board.json").read_text())
    assigned = set(board.get("dma") or {})
    assert ("spi.rx" in assigned) == ("spi.tx" in assigned)


@skip_no_devices
def test_the_spi_and_i2c_signals_are_offered_as_candidates() -> None:
    """The suggestion machinery needed no new knowledge: `dma_signal_candidates`
    derives keys from any peripheral-bearing role's `dma_requests`, so the
    phase-4 signals became suggestible the moment the chips carried them."""
    from alloy_cli.devices import load_chip

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    candidates = dma_signal_candidates(board, load_chip(DEVICES_ROOT,
                                                        board["chip"]))
    assert {"spi.rx", "spi.tx", "i2c.rx", "i2c.tx"} <= set(candidates)
