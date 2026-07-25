"""Unit tests for the parametric PLL clock solver."""

from __future__ import annotations

import pytest

from alloy_cli.clock_solver import MODELS, solve
from alloy_cli.emit.common import EmitError


def _pll_fields(result: dict) -> dict:
    return next(s for s in result["profile"]["program"] if s.get("register") == "PLLCFGR")["fields"]


def _latency(result: dict) -> int:
    return next(s for s in result["profile"]["program"] if s.get("register") == "ACR")["fields"]["LATENCY"]


def test_g0_64mhz_reproduces_the_validated_recipe() -> None:
    # The silicon-validated pll_64mhz: HSI16 /1 x8 /2, PLLM=0/PLLN=8/PLLR=1, 2 WS.
    r = solve("stm32g0", 64_000_000)
    assert r["sysclk_hz"] == 64_000_000
    assert r["silicon_validated"] is True
    f = _pll_fields(r)
    assert (f["PLLM"], f["PLLN"], f["PLLR"], f["PLLSRC"]) == (0, 8, 1, 2)
    assert _latency(r) == 2


@pytest.mark.parametrize("mhz,ws", [(16, 0), (32, 1), (48, 1), (56, 2)])
def test_g0_hits_common_frequencies_with_right_wait_states(mhz: int, ws: int) -> None:
    r = solve("stm32g0", mhz * 1_000_000)
    assert r["sysclk_hz"] == mhz * 1_000_000
    assert _latency(r) == ws
    # VCO stays inside the datasheet window.
    assert 64_000_000 <= r["pll"]["vco_hz"] <= 344_000_000


def test_over_the_family_max_is_rejected() -> None:
    with pytest.raises(EmitError):
        solve("stm32g0", 100_000_000)  # G0 caps at 64 MHz


def test_unknown_family_is_rejected() -> None:
    with pytest.raises(EmitError):
        solve("stm32z9", 48_000_000)


@pytest.mark.parametrize("family,mhz", [("stm32g4", 150), ("stm32f4", 84), ("stm32f7", 180)])
def test_other_families_solve_but_are_flagged_unvalidated(family: str, mhz: int) -> None:
    r = solve(family, mhz * 1_000_000)
    assert r["sysclk_hz"] == mhz * 1_000_000
    assert r["silicon_validated"] is False  # no in-repo silicon reference
    assert "program" in r["profile"] and r["profile"]["sysclk_hz"] == mhz * 1_000_000


def test_every_model_has_a_flash_table_that_covers_its_max() -> None:
    for family, m in MODELS.items():
        assert m.flash_ws[-1][0] >= m.sysclk_max_hz, f"{family}: WS table below max"
