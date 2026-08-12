"""The board-file half of the timer's two personalities.

`tests/test_encoder.cpp` proves the RUNTIME half: two hand-written `bind<>`s
naming one timer trap, in both orders. This file proves the other half — the
one that fires earlier, on a board that states the conflict as data, before a
compiler is started.

It also pins the data-model change the encoder personality forced: an IP's
`class` stopped being a single value, and three places that compared it for
equality now ask for membership.
"""

from __future__ import annotations

from pathlib import Path

import pytest
import yaml

from alloy_cli.emit.board import ROLE_PERSONALITY, _check_role_personalities
from alloy_cli.emit.common import EmitError
from alloy_cli.roles import ROLES, ip_classes

# tools/alloy/tests/ -> tools/alloy -> tools -> alloy
ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"


# ── The conflict, stated in board.json ───────────────────────────────────

def test_one_timer_cannot_be_pwm_and_encoder_in_one_board() -> None:
    """THE case the personality rule exists for, in the exact pair the docs
    have used as its example since before either driver existed."""
    with pytest.raises(EmitError) as excinfo:
        _check_role_personalities({
            "id": "synth",
            "roles": {"led_pwm": {"peripheral": "tim3", "channel": 1, "pin": "pa6"},
                      "encoder": {"peripheral": "tim3", "a": "pa6", "b": "pa7"}},
        })
    msg = str(excinfo.value)
    assert "tim3" in msg, "the diagnostic must name the contested peripheral"
    assert "pwm" in msg and "encoder" in msg, "and both personalities"


def test_pwm_and_encoder_on_DIFFERENT_timers_are_fine() -> None:
    """The negative control. A rule that refused this would refuse the lead
    board, which drives an LED off TIM2 and reads a knob off TIM3."""
    _check_role_personalities({
        "id": "synth",
        "roles": {"led_pwm": {"peripheral": "tim2", "channel": 1, "pin": "pa5"},
                  "encoder": {"peripheral": "tim3", "a": "pa6", "b": "pa7"}},
    })


def test_encoder_is_a_personality_the_cpp_side_can_name() -> None:
    """The generator refusing a conflict the firmware cannot express would be
    a rule with a hole in it. test_emit.py checks the whole vocabulary; this
    checks the enumerator this work depends on, by name."""
    assert ROLE_PERSONALITY["encoder"] == "encoder"
    header = (ALLOY_ROOT / "src" / "alloy" / "core" / "claim.hpp").read_text()
    assert "encoder," in header.split("enum class personality", 1)[1]


# ── One IP, several classes ──────────────────────────────────────────────

def _chip(chip_id: str) -> dict:
    return yaml.safe_load((DEVICES_ROOT / "chips" / f"{chip_id}.yaml").read_text())


def _registers() -> dict[str, dict]:
    out: dict[str, dict] = {}
    for path in (DEVICES_ROOT / "registers").rglob("*.yaml"):
        doc = yaml.safe_load(path.read_text())
        out[f"{doc['vendor']}/{doc['ip']}"] = doc
    return out


@pytest.mark.skipif(not DEVICES_ROOT.exists(), reason="alloy-devices not checked out")
def test_a_timer_reports_both_of_its_classes() -> None:
    """`ip_classes` used to answer one string per instance, which quietly
    encoded 'a block has one job'. A general-purpose timer has two."""
    classes = ip_classes(_chip("st/stm32g0b1re"), _registers())
    assert classes["tim3"] == ("pwm", "encoder"), (
        "the PRIMARY class stays first — a consumer that wants 'what is this "
        "block called' takes element 0"
    )
    # And a block with one job still reports one, so nothing else moved.
    assert classes["usart2"] == ("uart",)
    # An uncurated peripheral has no register file to ask, and says so by
    # being empty rather than by claiming a class it cannot back up.
    assert classes["tim6"] == ()


@pytest.mark.skipif(not DEVICES_ROOT.exists(), reason="alloy-devices not checked out")
def test_the_encoder_role_can_only_land_on_an_encoder_capable_block() -> None:
    """The role matcher asks MEMBERSHIP now. Both questions must still get
    the right answer: tim3 can fill it, usart2 cannot."""
    classes = ip_classes(_chip("st/stm32g0b1re"), _registers())
    assert ROLES["encoder"].ip_class in classes["tim3"]
    assert ROLES["encoder"].ip_class not in classes["usart2"]
    # ...and the timer has not lost its first job in the process.
    assert ROLES["led_pwm"].ip_class in classes["tim3"]


@pytest.mark.skipif(not DEVICES_ROOT.exists(), reason="alloy-devices not checked out")
def test_both_personalities_drivers_are_included_for_one_ip() -> None:
    """Codegen included exactly one alloy/hal/<class>/<vendor>_<ip>.hpp. The
    encoder driver is a second one over the SAME ip, and if it is not
    included the facade compiles against an undefined primary template — a
    diagnostic pointing at encoder_impl.hpp rather than at the missing data.
    """
    doc = _registers()["st/tim_gp16"]
    assert doc["class"] == "pwm"
    assert doc.get("personalities") == ["encoder"]
    for cls in [doc["class"], *doc["personalities"]]:
        assert (ALLOY_ROOT / "src" / "alloy" / "hal" / cls / "st_tim_gp16.hpp").exists(), (
            f"registers/st/tim_gp16.yaml claims the class '{cls}' but the "
            f"framework ships no driver for it"
        )


# ── What the lead board now generates ────────────────────────────────────

@pytest.mark.skipif(not DEVICES_ROOT.exists(), reason="alloy-devices not checked out")
def test_the_lead_board_binds_its_encoder_to_a_different_timer_than_its_pwm() -> None:
    """Not decoration: if these two roles ever name one peripheral, the board
    stops generating — so this is the assertion that keeps the example
    buildable AND the assertion that the two roles stay honest."""
    board = yaml_or_json(ALLOY_ROOT / "boards" / "nucleo_g0b1re" / "board.json")
    roles = board["roles"]
    assert roles["encoder"]["peripheral"] != roles["led_pwm"]["peripheral"]
    assert roles["encoder"]["a"] != roles["encoder"]["b"], "A and B are two pins"
    _check_role_personalities(board)


def yaml_or_json(path: Path) -> dict:
    import json
    return json.loads(path.read_text())
