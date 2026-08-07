"""Unit tests for the parametric PLL clock solver."""

from __future__ import annotations

from pathlib import Path

import pytest

from alloy_cli.clock_solver import MODELS, solve
from alloy_cli.emit.common import EmitError

_DEVICES_ROOT = Path(__file__).resolve().parents[3].parent / "alloy-devices"
_have_devices = (_DEVICES_ROOT / "chips").is_dir()


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


def _reg(result: dict, register: str) -> dict:
    step = next(s for s in result["profile"]["program"] if s.get("register") == register)
    return step.get("fields", step.get("value"))


def test_rp2040_125mhz_reproduces_the_validated_recipe() -> None:
    r = solve("rp2040", 125_000_000)
    assert r["sysclk_hz"] == 125_000_000 and r["silicon_validated"] is True
    assert _reg(r, "FBDIV_INT") == {"FBDIV": 125}
    assert _reg(r, "PRIM") == {"POSTDIV1": 6, "POSTDIV2": 2}


def test_same70_150mhz_reproduces_the_validated_recipe() -> None:
    r = solve("same70", 150_000_000)
    assert r["sysclk_hz"] == 150_000_000 and r["silicon_validated"] is True
    assert _reg(r, "CKGR_PLLAR") == "0x20183F01"  # MULA=24 (x25), DIVA=1
    assert _reg(r, "FMR") == {"FWS": 6}


def test_esp32_clock_is_not_solvable() -> None:
    with pytest.raises(EmitError, match="fixed"):
        solve("esp32", 160_000_000)


@pytest.mark.skipif(not _have_devices, reason="alloy-devices not beside the framework")
def test_same70_prologue_matches_chip_data() -> None:
    # The SAME70 solver copies a fixed WDT/CKGR_MOR init prologue into every program
    # (see clock_solver._same70_program). Those magic writes also live, as the single
    # source of truth, in the validated plla profile in the chip yaml. Pin them together
    # so the duplication can never drift silently.
    import yaml  # noqa: PLC0415

    chip = yaml.safe_load((_DEVICES_ROOT / "chips/microchip/atsame70q21.yaml").read_text())
    profiles = chip["clock"]["profiles"]
    validated = next(p for name, p in profiles.items() if name.startswith("plla_"))

    def fixed_writes(program: list[dict]) -> list[tuple]:
        return [(s["peripheral"], s["register"], s["value"]) for s in program
                if s.get("op") == "write" and s["register"] in ("MR", "CKGR_MOR")]

    solved = solve("same70", 150_000_000)["profile"]["program"]
    assert fixed_writes(solved) == fixed_writes(validated["program"]), \
        "SAME70 solver prologue drifted from the validated atsame70q21.yaml profile"


def test_every_model_with_flash_ws_covers_its_max() -> None:
    for family, m in MODELS.items():
        if not m.flash_ws:
            continue  # RP2040 runs from external XIP — no core flash wait states
        assert m.flash_ws[-1][0] >= m.sysclk_max_hz, f"{family}: WS table below max"


# --- external oscillator (crystal) ------------------------------------------
#
# A product runs the PLL off a crystal, not the internal RC: USB, CAN, high baud
# rates and RTC accuracy all need one. The crystal is a BOARD fact, so these
# pin the shape of the programs the solver builds from it.

def _ops(result: dict, register: str) -> list[dict]:
    return [s for s in result["profile"]["program"] if s.get("register") == register]


def test_hse_start_sequence_replaces_the_hsi_one() -> None:
    r = solve("stm32g0", 64_000_000, external_hz=8_000_000)
    fields = [f for op in _ops(r, "CR") for f in (op.get("fields") or {})]
    assert "HSEON" in fields and "HSION" not in fields
    ready = next(s for s in r["profile"]["program"] if s.get("field", "").endswith("RDY"))
    assert ready["field"] == "HSERDY"
    # A crystal needs milliseconds to start; the RC needs microseconds. Getting
    # this wrong is a board that boots on the wrong clock one time in ten.
    assert ready["timeout_us"] >= 100_000


def test_hse_bypass_sets_hsebyp_before_hseon() -> None:
    # HSEBYP is writable only while HSEON is clear — the order IS the contract.
    r = solve("stm32g0", 64_000_000, external_hz=8_000_000, bypass=True)
    cr_writes = [op for op in _ops(r, "CR") if op["op"] == "rmw"]
    assert list((cr_writes[0].get("fields") or {})) == ["HSEBYP"]
    assert "HSEON" in (cr_writes[1].get("fields") or {})


def test_hse_selects_the_external_pll_source() -> None:
    # G0/G4 PLLSRC 11 = HSE (10 = HSI16); F4/F7 PLLSRC is one bit, 1 = HSE.
    assert _pll_fields(solve("stm32g0", 64_000_000, external_hz=8_000_000))["PLLSRC"] == 3
    assert _pll_fields(solve("stm32g4", 96_000_000, external_hz=8_000_000))["PLLSRC"] == 3
    assert _pll_fields(solve("stm32f4", 96_000_000, external_hz=8_000_000))["PLLSRC"] == 1
    assert _pll_fields(solve("stm32f7", 96_000_000, external_hz=8_000_000))["PLLSRC"] == 0 + 1


def test_f4_168mhz_from_a_25mhz_crystal_is_sts_own_recipe() -> None:
    # ST's reference configuration for the F4 Discovery is PLLM=25, PLLN=336,
    # PLLP=2. An independent search landing on it is the strongest evidence the
    # datasheet limits in the model are right.
    f = _pll_fields(solve("stm32f4", 168_000_000, external_hz=25_000_000))
    assert (f["PLLM"], f["PLLN"], f["PLLP"]) == (25, 336, 0)  # PLLP encodes /2 as 0


def test_hsi_solutions_are_unchanged_by_the_external_support() -> None:
    hsi = solve("stm32g0", 64_000_000)
    assert hsi["source"]["external"] is False
    assert _pll_fields(hsi)["PLLSRC"] == 2


def test_a_crystal_outside_the_family_range_is_rejected() -> None:
    with pytest.raises(EmitError, match="outside stm32g0's HSE range"):
        solve("stm32g0", 64_000_000, external_hz=100_000_000)


def test_families_with_a_hardcoded_crystal_refuse_another_one() -> None:
    # RP2040/SAME70 bake 12 MHz into their bring-up prologue: silently accepting
    # a different crystal would produce a program that cannot work.
    for family, mhz in (("rp2040", 125_000_000), ("same70", 150_000_000)):
        with pytest.raises(EmitError, match="not supported for this family"):
            solve(family, mhz, external_hz=8_000_000)


def test_bypass_without_a_frequency_is_rejected() -> None:
    with pytest.raises(EmitError, match="pass the frequency too"):
        solve("stm32g0", 64_000_000, bypass=True)


def test_external_solutions_are_never_marked_silicon_validated() -> None:
    # The in-repo silicon reference ran on the internal RC. Inheriting its
    # confidence for a different clock path would be a false claim.
    assert solve("stm32g0", 64_000_000, external_hz=8_000_000)["silicon_validated"] is False


def test_pll_input_window_is_respected_for_odd_crystals() -> None:
    # 24 MHz / M must land in 2.66–16 MHz on the G0; M=1 (24 MHz) is illegal.
    r = solve("stm32g0", 64_000_000, external_hz=24_000_000)
    m = r["pll"]["m"]
    assert 2_660_000 <= 24_000_000 / m <= 16_000_000
    assert r["sysclk_hz"] == 64_000_000
