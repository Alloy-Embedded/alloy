"""Unit tests for `alloy board-validate`.

`board_validate` is a SECOND expression of the emitter's contract, written to
report every problem at once with a location and a fix. Two descriptions of one
contract can drift, so most of this file is the property that keeps them tied:

    P1  validate finds no errors  =>  generation succeeds
    P2  generation fails          =>  validate found an error

The converse of P2 is deliberately NOT required: validate is allowed to be
STRICTER than generation, and it is — a signal pin with no route to its
peripheral generates fine and then fails to compile inside a template. Catching
that at config time is the whole point of the verb.
"""

from __future__ import annotations

import copy
import json
from pathlib import Path
from typing import Any

import pytest

from alloy_cli.board_validate import validate_board
from alloy_cli.emit.common import EmitError

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

_have_devices = (DEVICES_ROOT / "chips").is_dir()
skip_no_devices = pytest.mark.skipif(
    not _have_devices, reason="alloy-devices not beside the framework")

BOARD_IDS = sorted(p.name for p in (ALLOY_ROOT / "boards").iterdir()
                   if (p / "board.json").exists()) if (ALLOY_ROOT / "boards").is_dir() else []


def _load(board_id: str) -> dict[str, Any]:
    return json.loads((ALLOY_ROOT / "boards" / board_id / "board.json").read_text())


def _chip_and_registers(board: dict[str, Any]):
    from alloy_cli.devices import load_chip, load_registers

    return load_chip(DEVICES_ROOT, board["chip"]), load_registers(DEVICES_ROOT)


def _generates(board: dict[str, Any], chip, registers) -> bool:
    """Would `alloy gen` accept this board?"""
    from alloy_cli.emit import _arch_ns
    from alloy_cli.emit.board import emit_board_header, emit_board_source

    try:
        emit_board_header(board, chip, registers)
        emit_board_source(board, chip, registers, _arch_ns(chip))
    except (EmitError, KeyError, TypeError, ValueError):
        return False
    return True


def _errors(board: dict[str, Any], chip, registers) -> list[dict[str, Any]]:
    return [i for i in validate_board(board, chip, registers) if i["level"] == "error"]


# ------------------------------------------------------- the shipped boards

@skip_no_devices
@pytest.mark.parametrize("board_id", BOARD_IDS)
def test_every_shipped_board_is_clean(board_id: str) -> None:
    """A board in the repo builds in CI, so validation calling it broken would
    make the verb useless noise on day one."""
    board = _load(board_id)
    chip, registers = _chip_and_registers(board)
    assert _errors(board, chip, registers) == []


@skip_no_devices
def test_a_rom_configured_uart_needs_no_pins() -> None:
    """The classic ESP32 UART0 is set up by the boot ROM and routes nothing; the
    emitter waives tx/rx for it, so validation must too."""
    board = _load("esp32_devkit")
    assert board["roles"]["debug_uart"]["mode"] == "rom", "fixture assumption"
    chip, registers = _chip_and_registers(board)
    assert _errors(board, chip, registers) == []


# --------------------------------------------------------------- the rules

@skip_no_devices
def test_unroutable_signal_pin_is_caught_before_the_compiler() -> None:
    """THE rule. pb3 is a real pin and a real route — for spi1 sck, not i2c1 scl.
    Generation accepts it; the app only finds out when it opens the bus and hits
    a static_assert. Validation names it now and says which pin to use."""
    board = _load("nucleo_g071rb")
    board["roles"]["i2c"]["scl"] = "pb3"
    chip, registers = _chip_and_registers(board)

    errors = _errors(board, chip, registers)
    assert len(errors) == 1
    issue = errors[0]
    assert issue["role"] == "i2c" and issue["field"] == "scl" and issue["pin"] == "pb3"
    assert "pb8" in issue["suggestions"], "must point at the pin that DOES route"
    # And the gap is real: the emitter is perfectly happy with it.
    assert _generates(board, chip, registers)


@skip_no_devices
def test_pwm_pin_must_be_an_output_of_the_chosen_channel() -> None:
    board = _load("nucleo_g071rb")
    board["roles"]["led_pwm"]["pin"] = "pb3"      # a real pin; not a tim2 ch1 output
    chip, registers = _chip_and_registers(board)
    issue = next(i for i in _errors(board, chip, registers) if i["role"] == "led_pwm")
    assert issue["pin"] == "pb3"
    assert "pa5" in issue["suggestions"]


@skip_no_devices
def test_a_channel_with_no_route_data_is_a_warning_not_silence() -> None:
    """tim2 ch4 exists in silicon but not in this chip's curated routes. We
    cannot verify the pin, and saying nothing would look like approval."""
    board = _load("nucleo_g071rb")
    board["roles"]["led_pwm"]["channel"] = 4
    chip, registers = _chip_and_registers(board)
    issues = validate_board(board, chip, registers)
    warned = [i for i in issues if i["role"] == "led_pwm" and i["level"] == "warning"]
    assert warned and "cannot be checked" in warned[0]["message"]


@skip_no_devices
def test_uncurated_peripheral_says_so_and_offers_the_curated_ones() -> None:
    """A builder-generated chip admits peripherals with no register file. The
    emitter refuses them, so validation must name the curated alternatives."""
    from alloy_cli.devices import load_chip, load_registers

    chip = load_chip(DEVICES_ROOT, "st/stm32f767zi")
    registers = load_registers(DEVICES_ROOT)
    profile = next(iter((chip.get("clock") or {}).get("profiles") or {}), None)
    board = {
        "schema": "alloy.board.v1", "id": "t", "chip": "st/stm32f767zi",
        "clock_profile": profile,
        # iwdg is admitted by the database but carries no ip / register file.
        "roles": {"watchdog": {"peripheral": "iwdg"}},
    }
    assert chip["peripherals"]["iwdg"].get("uncurated") is True, "fixture assumption"
    issue = next(i for i in _errors(board, chip, registers) if i["field"] == "peripheral")
    assert "curated register file" in issue["message"]
    assert issue["suggestions"] == [], "this stub chip curates no watchdog at all"
    # And the emitter agrees — this is P2, not a rule invented here.
    assert not _generates(board, chip, registers)


@skip_no_devices
def test_named_pin_without_a_route_is_reported_with_alternatives() -> None:
    board = _load("nucleo_g071rb")
    board["pins"] = {"pb3": {"function": "i2c1:scl", "label": "SENSOR_SCL"}}
    chip, registers = _chip_and_registers(board)
    issue = next(i for i in _errors(board, chip, registers) if i["pin"] == "pb3")
    assert "no route" in issue["message"]
    assert "spi1:sck" in issue["suggestions"], "offer what pb3 CAN do"


@skip_no_devices
def test_shared_pin_is_a_warning_not_an_error() -> None:
    """Every shipped Nucleo exposes its LED as both GPIO and PWM on one pin.
    Calling that an error would flag boards that are correct and in CI."""
    board = _load("nucleo_g071rb")
    chip, registers = _chip_and_registers(board)
    issues = validate_board(board, chip, registers)
    shared = [i for i in issues if i["pin"] == "pa5"]
    assert shared and all(i["level"] == "warning" for i in shared)
    assert "led" in shared[0]["message"] and "led_pwm" in shared[0]["message"]


@skip_no_devices
def test_every_problem_is_reported_not_just_the_first() -> None:
    """The reason this exists as a separate rule set: a form needs all of them."""
    board = _load("nucleo_g071rb")
    board["roles"]["led"]["pin"] = "pz99"
    board["roles"]["i2c"]["scl"] = "pz98"
    board["clock_profile"] = "not_a_profile"
    chip, registers = _chip_and_registers(board)

    errors = _errors(board, chip, registers)
    assert len(errors) >= 3
    assert {i["role"] for i in errors} >= {"led", "i2c"}
    assert any(i["field"] == "clock_profile" for i in errors)


# ------------------------------------------------- the anti-drift property

_MUTATIONS: list[tuple[str, Any]] = [
    ("unknown led pin", lambda b: b["roles"]["led"].__setitem__("pin", "pz99")),
    ("unknown peripheral", lambda b: b["roles"]["i2c"].__setitem__("peripheral", "i2c9")),
    ("missing required signal", lambda b: b["roles"]["i2c"].pop("scl")),
    ("missing peripheral field", lambda b: b["roles"]["watchdog"].pop("peripheral")),
    ("unknown clock profile", lambda b: b.__setitem__("clock_profile", "nope")),
    ("empty gpio bus", lambda b: b["roles"].__setitem__("gpio_bus", {"pins": []})),
    ("unknown named pin", lambda b: b.__setitem__("pins", {"pz99": {"function": "gpio_out"}})),
    ("named pin without a route",
     lambda b: b.__setitem__("pins", {"pb3": {"function": "i2c1:scl"}})),
    ("unroutable signal pin", lambda b: b["roles"]["i2c"].__setitem__("scl", "pb3")),
    ("role the framework does not know",
     lambda b: b["roles"].__setitem__("teleporter", {"peripheral": "i2c1"})),
    ("dma channel collision",
     lambda b: b.__setitem__("dma", {
         "adc.conv": {"controller": "dma1", "channel": 1},
         "debug_uart.rx": {"controller": "dma1", "channel": 1}})),
    ("dma controller that is not one",
     lambda b: b.__setitem__("dma", {
         "adc.conv": {"controller": "tim2", "channel": 1}})),
    ("dma signal the chip has no request for",
     lambda b: b.__setitem__("dma", {
         "debug_uart.conv": {"controller": "dma1", "channel": 1}})),
    # Phase 4 put two signals on ONE role for the first time (the SPI pair).
    # Both halves are ordinary assignments under the same rules, and the two
    # ways to get a pair wrong must both reach the validator.
    ("dma pair with both halves on one channel",
     lambda b: b.__setitem__("dma", {
         "spi.rx": {"controller": "dma1", "channel": 4},
         "spi.tx": {"controller": "dma1", "channel": 4}})),
    ("dma pair half written with the stream engines' key",
     lambda b: b.__setitem__("dma", {
         "spi.rx": {"controller": "dma1", "channel": 4},
         "spi.tx": {"controller": "dma1", "stream": 5}})),
]


@skip_no_devices
@pytest.mark.parametrize("name,mutate", _MUTATIONS, ids=[m[0] for m in _MUTATIONS])
def test_generation_failure_is_always_reported(name: str, mutate) -> None:
    """P2: whatever makes generation fail must show up as an error here — so the
    UI never says "looks fine" about a board that cannot build."""
    board = _load("nucleo_g071rb")
    mutate(board)
    chip, registers = _chip_and_registers(board)
    if _generates(board, chip, registers):
        pytest.skip(f"{name} is accepted by the emitter (validate is stricter here)")
    assert _errors(board, chip, registers), \
        f"{name}: generation fails but validation reported nothing"


@skip_no_devices
@pytest.mark.parametrize("board_id", BOARD_IDS)
def test_clean_boards_generate(board_id: str) -> None:
    """P1: if validation is silent, generation must work — otherwise 'ok' lies."""
    board = _load(board_id)
    chip, registers = _chip_and_registers(board)
    assert not _errors(board, chip, registers)
    assert _generates(board, chip, registers)


@skip_no_devices
def test_a_clean_mutation_still_generates() -> None:
    """P1 with something actually changed: switching to another routable pin of
    the same signal must stay clean AND still generate."""
    board = _load("nucleo_f767zi")
    chip, registers = _chip_and_registers(board)
    from alloy_cli.roles import routes_by_peripheral

    routes = routes_by_peripheral(chip)
    tx_pins = routes["usart3"]["tx"]
    if len(tx_pins) < 2:
        pytest.skip("this chip's data offers a single tx pin")
    board["roles"]["debug_uart"]["tx"] = tx_pins[1]
    assert not _errors(board, chip, registers)
    assert _generates(board, chip, registers)


@skip_no_devices
def test_deep_copy_is_not_needed_for_isolation() -> None:
    """Guard against a test-authoring mistake: mutations must not leak between
    cases through a shared dict."""
    a = _load("nucleo_g071rb")
    a["roles"]["led"]["pin"] = "pz99"
    b = _load("nucleo_g071rb")
    assert b["roles"]["led"]["pin"] != "pz99"
    assert copy.deepcopy(b) == b


# --- pads a role drives without naming them ------------------------------
# The "pins claimed twice" warning only ever saw pins a role SPELLS OUT in
# board.json. Some peripherals carry a fixed pin bank in CHIP data instead —
# an RMII MAC picks its pads from the silicon, not from the board — and the
# emitter muxes that whole bank at open(). Those pads had no owner, so a second
# role could take one in silence. It happened: nucleo_f767zi's bridge took pb13
# as CH1N while the same board's ethernet muxes pb13 as RMII_TXD1.


@skip_no_devices
def test_an_implicit_pin_bank_owns_its_pads() -> None:
    """The regression, on the real board that had it."""
    board = _load("nucleo_f767zi")
    chip, registers = _chip_and_registers(board)
    rmii = set(chip["peripherals"]["eth"]["rmii"]["pins"])
    assert "pb13" in rmii, "this test's premise moved into chip data"
    assert board["roles"]["bridge"]["al"] == "pb13", \
        "the collision this test guards is no longer in the board"

    shared = [i for i in validate_board(board, chip, registers)
              if i["pin"] == "pb13"]
    assert shared, "pb13 is muxed by two roles and validate said nothing"
    # Naming the BANK is the difference between a puzzle and an answer: the
    # ethernet role never mentions pb13, so "ethernet uses pb13" would send a
    # reader looking through board.json for a line that is not there.
    assert "rmii" in shared[0]["message"]
    assert "bridge" in shared[0]["message"]


@skip_no_devices
def test_an_implicit_bank_is_a_warning_not_an_error() -> None:
    """One pad with two owners is only a DEFECT if one program opens both, and
    a board is allowed to describe both. P1 must still hold — this board has to
    keep generating — or the check would have made a true statement fatal."""
    board = _load("nucleo_f767zi")
    chip, registers = _chip_and_registers(board)
    assert not _errors(board, chip, registers)
    assert _generates(board, chip, registers)


@skip_no_devices
def test_the_bank_check_reads_data_not_a_hard_coded_name() -> None:
    """`rmii` is not spelled in the checker. A peripheral that arrives with its
    own fixed bank must be covered by the data it ships with."""
    board = _load("nucleo_f767zi")
    chip, registers = _chip_and_registers(board)
    chip = copy.deepcopy(chip)
    # A bank under a name nothing in the tree has ever used, on the peripheral
    # the adc role already names, colliding with the debug UART's tx pad.
    tx = board["roles"]["debug_uart"]["tx"]
    chip["peripherals"]["adc1"]["some_future_bank"] = {"af": 3, "pins": [tx]}

    shared = [i for i in validate_board(board, chip, registers) if i["pin"] == tx]
    assert shared, "a bank under an unknown name owned nothing"
    assert "some_future_bank" in shared[0]["message"]


@skip_no_devices
def test_a_bank_on_an_unopened_peripheral_owns_nothing() -> None:
    """Ownership follows the ROLE, not the silicon: a bank on a peripheral no
    role names is not muxed by anyone, and must not manufacture a warning."""
    board = _load("nucleo_f767zi")
    chip, registers = _chip_and_registers(board)
    chip = copy.deepcopy(chip)
    named = {cfg.get("peripheral") for cfg in board["roles"].values()
             if isinstance(cfg, dict)}
    spare = next(p for p in chip["peripherals"] if p not in named)
    led = board["roles"]["led"]["pin"]
    chip["peripherals"][spare]["idle_bank"] = {"pins": [led]}

    shared = [i for i in validate_board(board, chip, registers)
              if i["pin"] == led and "idle_bank" in i["message"]]
    assert not shared, f"{spare} is opened by no role but claimed {led}"
