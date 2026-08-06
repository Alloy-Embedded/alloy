"""Unit tests for the board.json "pins" section -> board::pins codegen.

The visual configurator stores {pin: {function, label}}; the emitter must turn
labels into real aliases (gpio handles / pin types), validate AF assignments
against the chip's route table (wrong function = generation error NAMING the
pin), and reject labels that aren't identifiers — the pin map must never emit
code that silently does the wrong thing.
"""

from __future__ import annotations

import pytest

from alloy_cli.emit.board import emit_board_header
from alloy_cli.emit.common import EmitError


def _chip() -> dict:
    return {
        "vendor": "st",
        "part": "TESTG0",
        "family": "stm32g0",
        "cores": [{"arch": "armv6m"}],
        "memories": [
            {"name": "flash", "kind": "flash", "base": "0x08000000", "size": 131072},
            {"name": "sram", "kind": "ram", "base": "0x20000000", "size": 36864},
        ],
        "peripherals": {},
        "pins": {"pa5": {"port": "a", "index": 5}, "pb8": {"port": "b", "index": 8}},
        "routes": [
            {"pin": "pb8", "peripheral": "i2c1", "signal": "scl", "af": 6},
        ],
        "clock": {"profiles": {"hsi": {"description": "HSI", "sysclk_hz": 16000000,
                                        "ahb_hz": 16000000, "apb_hz": 16000000,
                                        "program": []}}},
        "interrupts": [],
    }


def _board(pins: dict) -> dict:
    return {"schema": "alloy.board.v1", "id": "testboard", "chip": "st/testg0",
            "clock_profile": "hsi", "roles": {}, "pins": pins}


def test_gpio_label_emits_a_handle_alias() -> None:
    hpp = emit_board_header(_board({"pa5": {"function": "gpio_out", "label": "led_status"}}),
                            _chip(), {})
    assert "namespace pins {" in hpp
    assert ("inline constexpr alloy::gpio::output<alloy::dev::pa5_t, "
            "alloy::gpio::active_high_t> led_status{};") in hpp


def test_af_assignment_validated_and_aliased() -> None:
    hpp = emit_board_header(_board({"pb8": {"function": "i2c1:scl", "label": "sensor_scl"}}),
                            _chip(), {})
    assert "using sensor_scl = alloy::dev::pb8_t;" in hpp


def test_af_the_pin_cannot_do_is_a_generation_error_naming_the_pin() -> None:
    with pytest.raises(EmitError, match="pa5.*no route to.*i2c1 scl"):
        emit_board_header(_board({"pa5": {"function": "i2c1:scl", "label": "x"}}),
                          _chip(), {})


def test_bad_label_and_unknown_pin_and_function_rejected() -> None:
    with pytest.raises(EmitError, match="not a valid identifier"):
        emit_board_header(_board({"pa5": {"function": "gpio_out", "label": "1bad"}}),
                          _chip(), {})
    with pytest.raises(EmitError, match="not in chip data"):
        emit_board_header(_board({"pz9": {"function": "gpio_out", "label": "x"}}),
                          _chip(), {})
    with pytest.raises(EmitError, match="unknown\\s+function"):
        emit_board_header(_board({"pa5": {"function": "sideways", "label": "x"}}),
                          _chip(), {})


def test_unlabelled_pins_validate_but_emit_nothing() -> None:
    hpp = emit_board_header(_board({"pb8": {"function": "i2c1:scl"}}), _chip(), {})
    assert "namespace pins {" not in hpp
