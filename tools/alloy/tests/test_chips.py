"""Unit tests for the chip catalogue + clean-board scaffolding."""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from alloy_cli import chips
from alloy_cli.emit.common import EmitError

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

_have_devices = (DEVICES_ROOT / "chips").is_dir()
skip_no_devices = pytest.mark.skipif(not _have_devices, reason="alloy-devices not beside the framework")


@skip_no_devices
def test_list_chips_finds_hundreds_across_vendors() -> None:
    rows = chips.list_chips(DEVICES_ROOT)
    assert len(rows) > 100
    vendors = {r["vendor"] for r in rows}
    assert {"st", "espressif"} <= vendors
    ids = {r["id"] for r in rows}
    assert "st/stm32g071rb" in ids
    for r in rows[:5]:
        assert set(r) >= {"id", "vendor", "chip", "family"}


@skip_no_devices
def test_chip_clock_returns_profiles_and_safe_default() -> None:
    profiles, default = chips.chip_clock(DEVICES_ROOT, "st/stm32g071rb")
    assert "hsi_16mhz" in profiles
    assert default == profiles[0]  # boot-safe profile is first


@skip_no_devices
def test_chip_clock_rejects_unknown_chip() -> None:
    with pytest.raises(EmitError):
        chips.chip_clock(DEVICES_ROOT, "st/nope-not-a-chip")


def test_clean_board_json_is_valid_and_minimal() -> None:
    board = json.loads(chips.clean_board_json("myboard", "st/stm32g071rb", "hsi_16mhz"))
    assert board["schema"] == "alloy.board.v1"
    assert board["id"] == "myboard"
    assert board["chip"] == "st/stm32g071rb"
    assert board["clock_profile"] == "hsi_16mhz"
    assert board["roles"] == {}  # clean: the user fills these in


@skip_no_devices
def test_chip_info_pins_carry_grid_position_and_functions() -> None:
    # A builder-generated chip has the full AF matrix — the CubeMX-style pin
    # picker's data. Every pin must place itself (port/index) and pa0 on the
    # G474 must offer its documented alternates (tim2 ch1 on AF1 among them).
    info = chips.chip_info(DEVICES_ROOT, "st/stm32g474re")
    assert info["schema"] == "alloy.chip_info.v1"
    pins = {p["name"]: p for p in info["pins"]}
    assert len(pins) >= 40
    pa0 = pins["pa0"]
    assert (pa0["port"], pa0["index"]) == ("a", 0)
    assert {"peripheral": "tim2", "signal": "ch1", "af": 1} in pa0["functions"]
    # functions sorted deterministically, and a pin with no routes still lists
    for p in info["pins"]:
        fns = p["functions"]
        assert fns == sorted(fns, key=lambda f: (f["peripheral"], f["signal"]))


@skip_no_devices
def test_chip_info_pins_exist_for_curated_chips_too() -> None:
    info = chips.chip_info(DEVICES_ROOT, "st/stm32g071rb")
    pins = {p["name"]: p for p in info["pins"]}
    assert "pa2" in pins  # curated route: usart2 tx
    assert any(f["peripheral"] == "usart2" and f["signal"] == "tx"
               for f in pins["pa2"]["functions"])


# ---------------------------------------------------------- role catalogue
# The catalogue is what drives the visual configurator's role panels. It must be
# derived from the IP CLASS in the register data, never guessed from instance
# names, and it must be honest about what a chip cannot do.


@skip_no_devices
def test_role_catalogue_covers_every_emitter_role() -> None:
    from alloy_cli.emit.board import PRESENCE_ROLES

    info = chips.chip_info(DEVICES_ROOT, "st/stm32g071rb")
    # Every role the emitter can validate must be offerable by the UI, or the
    # gap the configurator had (3 of 16 roles) silently comes back.
    assert set(info["roles"]) == set(PRESENCE_ROLES)


@skip_no_devices
def test_role_candidates_come_from_ip_class_not_instance_names() -> None:
    info = chips.chip_info(DEVICES_ROOT, "st/stm32g071rb")
    roles = info["roles"]
    assert [c["peripheral"] for c in roles["i2c"]["candidates"]] == ["i2c1"]
    assert [c["peripheral"] for c in roles["spi"]["candidates"]] == ["spi1"]
    # 'adc' and 'flash' are named nothing like their role; class matching finds
    # them anyway, which name-prefix matching could not.
    assert [c["peripheral"] for c in roles["adc"]["candidates"]] == ["adc"]
    assert [c["peripheral"] for c in roles["nvm"]["candidates"]] == ["flash"]
    assert [c["peripheral"] for c in roles["watchdog"]["candidates"]] == ["iwdg"]


@skip_no_devices
def test_role_candidates_list_all_pins_a_signal_can_route_to() -> None:
    """One pin per signal is what forced users into board.json by hand."""
    info = chips.chip_info(DEVICES_ROOT, "st/stm32f767zi")
    uart4 = next(c for c in info["roles"]["debug_uart"]["candidates"]
                 if c["peripheral"] == "uart4")
    assert len(uart4["signals"]["tx"]) > 1
    assert len(uart4["signals"]["rx"]) > 1


@skip_no_devices
def test_pwm_candidates_carry_their_channels() -> None:
    info = chips.chip_info(DEVICES_ROOT, "st/stm32g071rb")
    tim2 = next(c for c in info["roles"]["led_pwm"]["candidates"]
                if c["peripheral"] == "tim2")
    assert {"channel": 1, "pins": ["pa5"]} in tim2["channels"]


@skip_no_devices
def test_unsupported_roles_say_why() -> None:
    info = chips.chip_info(DEVICES_ROOT, "st/stm32g071rb")
    can = info["roles"]["can"]
    assert can["supported"] is False
    assert can["candidates"] == []
    assert "can" in can["reason"]


@skip_no_devices
def test_uncurated_peripherals_are_offered_as_unusable() -> None:
    """A builder-generated chip admits peripherals with no register file. The
    emitter refuses them, so the UI must show them as not pickable rather than
    let a user choose one and hit a generation error."""
    info = chips.chip_info(DEVICES_ROOT, "st/stm32f767zi")
    assert info["roles"]["ethernet"]["supported"] is False
    curated = chips.chip_info(DEVICES_ROOT, "st/stm32f767")
    assert curated["roles"]["ethernet"]["supported"] is True
    assert curated["roles"]["ethernet"]["candidates"][0]["ip"] == "st/eth_v1"


@skip_no_devices
def test_chip_info_stays_backward_compatible() -> None:
    """Additive by contract: an IDE built against the older envelope must keep
    working, so the schema string and every old key keep their shape."""
    info = chips.chip_info(DEVICES_ROOT, "st/stm32g071rb")
    assert info["schema"] == "alloy.chip_info.v1"
    assert set(info) >= {"chip", "family", "clock_profiles", "boot_profile",
                         "gpio_pins", "pins", "peripherals"}
    assert {"debug_uart", "i2c", "spi"} <= set(info["peripherals"])
    uart = info["peripherals"]["debug_uart"][0]
    assert set(uart) >= {"peripheral"}


@skip_no_devices
def test_chip_info_reports_memories_and_interrupts() -> None:
    info = chips.chip_info(DEVICES_ROOT, "st/stm32g071rb")
    kinds = {m["kind"]: m["size"] for m in info["memories"]}
    assert kinds == {"flash": 131072, "ram": 36864}
    assert info["interrupt_count"] > 0
