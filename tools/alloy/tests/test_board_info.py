"""Unit tests for board introspection (`alloy board-info` / `board-clone`).

These cover the promise the IDE relies on: any board can be READ, only a
project-local one is editable, the reported capabilities are the ones the
generated header will carry, and a board the emitter would reject says so
instead of looking healthy.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from alloy_cli import board_info
from alloy_cli.emit.board import role_caps
from alloy_cli.emit.common import EmitError

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

_have_devices = (DEVICES_ROOT / "chips").is_dir()
skip_no_devices = pytest.mark.skipif(
    not _have_devices, reason="alloy-devices not beside the framework")


def _project(tmp_path: Path) -> Path:
    root = tmp_path / "proj"
    (root / "src").mkdir(parents=True)
    (root / "alloy.toml").write_text(
        f'[project]\nname = "proj"\n\n[board]\nid = "nucleo_g071rb"\n\n'
        f'[alloy]\nroot = "{ALLOY_ROOT}"\n')
    return root


# ---------------------------------------------------------------- resolution

@skip_no_devices
def test_curated_board_is_readable_but_not_editable() -> None:
    info = board_info.board_info(DEVICES_ROOT, ALLOY_ROOT, None, "nucleo_g071rb")
    assert info["schema"] == "alloy.board_info.v1"
    assert info["source"] == "framework"
    # The whole point of the verb: a curated board can be inspected. Editing it
    # in place would change every project that uses it.
    assert info["editable"] is False
    # Warnings are allowed (this board deliberately shares PA5 between the LED
    # and its PWM); nothing may be an ERROR.
    assert [i for i in info["issues"] if i["level"] == "error"] == []


@skip_no_devices
def test_project_board_shadows_a_framework_board_of_the_same_id(tmp_path: Path) -> None:
    root = _project(tmp_path)
    board_info.clone_board(ALLOY_ROOT, root, "nucleo_g071rb", "nucleo_g071rb_local")
    # A project board with a FRAMEWORK board's id must win, exactly as it does
    # when building (Project.board_json).
    local = root / "boards" / "nucleo_g071rb"
    local.mkdir(parents=True)
    board = json.loads((ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    (local / "board.json").write_text(json.dumps(board))
    path, source = board_info.resolve_board(ALLOY_ROOT, root, "nucleo_g071rb")
    assert source == "project"
    assert path == local / "board.json"


def test_unknown_board_names_the_known_ones() -> None:
    with pytest.raises(EmitError) as exc:
        board_info.resolve_board(ALLOY_ROOT, None, "definitely_not_a_board")
    assert "nucleo_g071rb" in str(exc.value)


# ------------------------------------------------------------------ contents

@skip_no_devices
def test_caps_match_the_generated_header_exactly() -> None:
    """board-info must never disagree with board::caps — same function, and the
    only extra key is the derived `net`."""
    from alloy_cli.devices import load_chip, load_registers

    board = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    chip = load_chip(DEVICES_ROOT, board["chip"])
    expected = role_caps(board, chip, load_registers(DEVICES_ROOT))

    info = board_info.board_info(DEVICES_ROOT, ALLOY_ROOT, None, "nucleo_g071rb")
    reported = dict(info["caps"])
    assert reported.pop("net") == (expected["ethernet"] or expected["wifi"])
    assert reported == expected


@skip_no_devices
def test_pins_used_reports_every_role_pin() -> None:
    info = board_info.board_info(DEVICES_ROOT, ALLOY_ROOT, None, "nucleo_g071rb")
    owned = {(e["pin"], e["owner"]) for e in info["pins_used"]}
    assert ("pa2", "debug_uart") in owned      # tx
    assert ("pa3", "debug_uart") in owned      # rx
    assert ("pa5", "led") in owned
    assert ("pb8", "i2c") in owned and ("pb9", "i2c") in owned
    assert ("pb3", "spi") in owned             # sck
    assert ("pc13", "button") in owned


@skip_no_devices
def test_ethernet_board_reports_the_rmii_bank_from_chip_data() -> None:
    """The RMII pins live in CHIP data, not board.json — a pin map that only
    read board.json would show them as free."""
    info = board_info.board_info(DEVICES_ROOT, ALLOY_ROOT, None, "nucleo_f767zi")
    rmii = [e["pin"] for e in info["pins_used"] if e["signal"] == "rmii"]
    assert len(rmii) >= 7, f"expected the RMII bank, got {rmii}"
    assert info["caps"]["ethernet"] and info["caps"]["net"]


@skip_no_devices
def test_inline_custom_clock_is_reported_as_such(tmp_path: Path) -> None:
    root = _project(tmp_path)
    board_info.clone_board(ALLOY_ROOT, root, "nucleo_g071rb", "custom_clk")
    path = root / "boards" / "custom_clk" / "board.json"
    board = json.loads(path.read_text())
    board.pop("clock_profile", None)
    board["clock"] = {"sysclk_hz": 48_000_000, "description": "solved by alloy clock",
                      "program": [], "ahb_hz": 48_000_000, "apb_hz": 48_000_000}
    path.write_text(json.dumps(board))
    info = board_info.board_info(DEVICES_ROOT, ALLOY_ROOT, root, "custom_clk")
    assert info["clock"]["mode"] == "inline"
    assert info["clock"]["sysclk_hz"] == 48_000_000


# ---------------------------------------------------------------- validation

@skip_no_devices
def test_a_board_the_emitter_would_reject_reports_an_issue(tmp_path: Path) -> None:
    root = _project(tmp_path)
    board_info.clone_board(ALLOY_ROOT, root, "nucleo_g071rb", "broken")
    path = root / "boards" / "broken" / "board.json"
    board = json.loads(path.read_text())
    board["roles"]["led"]["pin"] = "pz99"        # no such pin on this chip
    path.write_text(json.dumps(board))

    info = board_info.board_info(DEVICES_ROOT, ALLOY_ROOT, root, "broken")
    errors = [i for i in info["issues"] if i["level"] == "error"]
    assert errors, "a board that cannot generate must not look healthy"
    # Located, not just a sentence: the UI can put the message on the field.
    assert errors[0]["role"] == "led" and errors[0]["field"] == "pin"
    assert "pz99" in errors[0]["message"]        # the message names the offender


# --------------------------------------------------------------------- clone

@skip_no_devices
def test_clone_makes_an_editable_copy(tmp_path: Path) -> None:
    root = _project(tmp_path)
    dest = board_info.clone_board(ALLOY_ROOT, root, "nucleo_g071rb", "my_widget")
    assert dest == root / "boards" / "my_widget" / "board.json"
    copy = json.loads(dest.read_text())
    assert copy["id"] == "my_widget"
    assert "(custom)" in copy["name"]
    # Roles carry over verbatim — starting from a working board is the point.
    original = json.loads(
        (ALLOY_ROOT / "boards" / "nucleo_g071rb" / "board.json").read_text())
    assert copy["roles"] == original["roles"]

    info = board_info.board_info(DEVICES_ROOT, ALLOY_ROOT, root, "my_widget")
    assert info["source"] == "project" and info["editable"] is True
    assert [i for i in info["issues"] if i["level"] == "error"] == []


@skip_no_devices
def test_clone_refuses_to_overwrite(tmp_path: Path) -> None:
    root = _project(tmp_path)
    board_info.clone_board(ALLOY_ROOT, root, "nucleo_g071rb", "twice")
    with pytest.raises(EmitError, match="already exists"):
        board_info.clone_board(ALLOY_ROOT, root, "nucleo_g071rb", "twice")


@skip_no_devices
@pytest.mark.parametrize("bad_id", ["9lives", "has-dash", "with space", "_leading"])
def test_clone_rejects_ids_that_are_not_identifiers(tmp_path: Path, bad_id: str) -> None:
    root = _project(tmp_path)
    with pytest.raises(EmitError):
        board_info.clone_board(ALLOY_ROOT, root, "nucleo_g071rb", bad_id)


def test_clone_outside_a_project_is_refused(tmp_path: Path) -> None:
    with pytest.raises(EmitError, match="not an alloy project"):
        board_info.clone_board(ALLOY_ROOT, tmp_path, "nucleo_g071rb", "nope")
