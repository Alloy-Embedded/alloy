"""A board.json value is a DEFAULT; alloy.toml chooses.

The split: a board fact is fixed by the PCB, a project field is one the same
hardware supports many values of. Getting it wrong in either direction is bad —
forbid too much and changing a baud rate forks you off the framework's board;
allow too much and alloy.toml quietly redesigns hardware it cannot change.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from alloy_cli.board_validate import validate_board
from alloy_cli.project import apply_project_overrides, read_project_settings
from alloy_cli.roles import ROLES

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _board() -> dict:
    return json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())


def _project(tmp_path: Path, toml_extra: str = "") -> Path:
    root = tmp_path / "proj"
    (root / "src").mkdir(parents=True)
    (root / "alloy.toml").write_text(
        f'[project]\nname = "proj"\n\n[board]\nid = "nucleo_g071rb"\n\n'
        f'[alloy]\nroot = "{ALLOY_ROOT}"\n{toml_extra}')
    return root


# ------------------------------------------------------- what may be chosen

def test_the_split_is_declared_not_guessed() -> None:
    """Every project field must also be an optional field of its role — a
    project cannot choose something the board is required to state."""
    for name, spec in ROLES.items():
        for field in spec.project_fields:
            assert field in spec.optional, f"{name}.{field} is required, not a choice"
            assert field not in spec.signals and field not in spec.pin_fields, \
                f"{name}.{field} names a pin — that is hardware"


@skip_no_devices
def test_a_project_may_change_a_baud_without_touching_the_board(tmp_path: Path) -> None:
    root = _project(tmp_path, "\n[roles.debug_uart]\nbaud = 921600\n")
    effective = apply_project_overrides(_board(), read_project_settings(root),
                                        DEVICES_ROOT)
    assert effective["roles"]["debug_uart"]["baud"] == 921600
    # The pins next to it are untouched: this is a choice, not a redesign.
    assert effective["roles"]["debug_uart"]["tx"] == _board()["roles"]["debug_uart"]["tx"]
    # And the framework's file is not modified — that is the whole point.
    assert _board()["roles"]["debug_uart"]["baud"] == 115200


@skip_no_devices
def test_overriding_a_board_property_is_refused_with_the_way_out(tmp_path: Path) -> None:
    """Silently ignoring it would be the worst outcome: the user writes a pin,
    nothing happens, and the LED stays dark with no explanation."""
    from alloy_cli.devices import load_chip, load_registers

    root = _project(tmp_path, '\n[roles.led]\npin = "pb7"\n')
    settings = read_project_settings(root)
    board = apply_project_overrides(_board(), settings, DEVICES_ROOT)
    assert board["roles"]["led"]["pin"] == "pa5", "the override must not apply"

    issues = validate_board(board, load_chip(DEVICES_ROOT, board["chip"]),
                            load_registers(DEVICES_ROOT), settings)
    problem = next(i for i in issues if i["field"] == "pin")
    assert "property of the BOARD" in problem["message"]
    assert "board-clone" in problem["message"], "say how to actually do it"


@skip_no_devices
def test_a_role_that_does_not_exist_is_reported(tmp_path: Path) -> None:
    from alloy_cli.devices import load_chip, load_registers

    root = _project(tmp_path, "\n[roles.teleporter]\nbaud = 1\n")
    settings = read_project_settings(root)
    issues = validate_board(_board(), load_chip(DEVICES_ROOT, "st/stm32g071rb"),
                            load_registers(DEVICES_ROOT), settings)
    assert any("not a board role" in i["message"] for i in issues)


# ------------------------------------------------------------------ the clock

@skip_no_devices
def test_a_project_may_pick_a_named_profile(tmp_path: Path) -> None:
    root = _project(tmp_path, '\n[clock]\nprofile = "hsi_16mhz"\n')
    effective = apply_project_overrides(_board(), read_project_settings(root),
                                        DEVICES_ROOT)
    assert effective["clock_profile"] == "hsi_16mhz"
    assert "clock" not in effective, "a named profile must not leave an inline one"


@skip_no_devices
def test_a_project_may_ask_for_a_frequency(tmp_path: Path) -> None:
    root = _project(tmp_path, "\n[clock]\nmhz = 48\n")
    effective = apply_project_overrides(_board(), read_project_settings(root),
                                        DEVICES_ROOT)
    assert effective["clock_profile"] == "custom"
    assert effective["clock"]["sysclk_hz"] == 48_000_000
    assert effective["clock"]["program"], "must carry the register program to run it"


@skip_no_devices
def test_no_settings_returns_the_board_untouched(tmp_path: Path) -> None:
    root = _project(tmp_path)
    board = _board()
    assert apply_project_overrides(board, read_project_settings(root),
                                   DEVICES_ROOT) == board


# --------------------------------------------- the property the first cut broke

@skip_no_devices
def test_every_consumer_sees_the_same_effective_board(tmp_path: Path) -> None:
    """The first version applied overrides only in Project.load_board, so the
    firmware was generated at 48 MHz while `board-info` still reported 64. A
    panel that disagrees with the header is worse than one that shows nothing.
    """
    from alloy_cli import board_info as bi
    from alloy_cli.project import load_project

    root = _project(tmp_path, "\n[roles.debug_uart]\nbaud = 921600\n\n[clock]\nmhz = 48\n")
    (root / "src" / "main.cpp").write_text("int main() { return 0; }\n")

    via_project = load_project(root).load_board()
    via_info = bi.board_info(DEVICES_ROOT, ALLOY_ROOT, root, "nucleo_g071rb")

    assert via_project["roles"]["debug_uart"]["baud"] == 921600
    assert via_info["roles"]["debug_uart"]["baud"] == 921600
    assert via_project["clock"]["sysclk_hz"] == 48_000_000
    assert via_info["clock"]["sysclk_hz"] == 48_000_000


@skip_no_devices
def test_board_info_says_where_each_value_came_from(tmp_path: Path) -> None:
    """A number with no story is not much better than a wrong one."""
    from alloy_cli import board_info as bi

    root = _project(tmp_path, "\n[roles.debug_uart]\nbaud = 921600\n")
    info = bi.board_info(DEVICES_ROOT, ALLOY_ROOT, root, "nucleo_g071rb")
    baud = info["project_overrides"]["roles"]["debug_uart"]["baud"]
    assert baud == {"board": 115200, "project": 921600}


@skip_no_devices
def test_a_project_with_no_overrides_reports_none(tmp_path: Path) -> None:
    from alloy_cli import board_info as bi

    info = bi.board_info(DEVICES_ROOT, ALLOY_ROOT, _project(tmp_path),
                         "nucleo_g071rb")
    assert info["project_overrides"] == {"roles": {}, "clock": None}
