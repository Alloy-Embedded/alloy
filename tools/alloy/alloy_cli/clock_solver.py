"""Parametric PLL clock solver for STM32 families.

The device database ships a few hand-verified clock *profiles* per chip. This
solver goes further: given a target system frequency it searches the family's
PLL (source/M ×N /divider), respecting the datasheet limits (VCO window, divider
ranges, max sysclk), picks the flash wait states, and emits the same register
`program` the profiles use — so `alloy clock --chip X --sysclk N` yields a
buildable custom clock for (almost) any frequency.

Correctness anchor: on STM32G0 the solver reproduces the silicon-validated
pll_64mhz recipe exactly (M÷1 N×8 R÷2, PLLM=0/PLLN=8/PLLR=1, 2 wait states).

Safety: solutions are correct-by-construction against the datasheet limits, but
only frequencies within a family's `validated_max_hz` have an in-repo silicon
reference — anything above is computed, not board-tested. Families that need a
power boost / overdrive above a certain frequency are capped below it here.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable

from .emit.common import EmitError


def _poll(reg: str, field: str, us: int) -> dict[str, Any]:
    return {"op": "poll", "peripheral": "rcc", "register": reg, "field": field,
            "equals": 1, "timeout_us": us}


def _hsi_on() -> list[dict[str, Any]]:
    return [
        {"op": "rmw", "peripheral": "rcc", "register": "CR", "fields": {"HSION": 1}},
        _poll("CR", "HSIRDY", 2000),
    ]


def _pll_on_and_switch() -> list[dict[str, Any]]:
    return [
        {"op": "rmw", "peripheral": "rcc", "register": "CR", "fields": {"PLLON": 1}},
        _poll("CR", "PLLRDY", 10000),
        {"op": "rmw", "peripheral": "rcc", "register": "CFGR", "fields": {"SW": 2}},
        {"op": "poll", "peripheral": "rcc", "register": "CFGR", "field": "SWS",
         "equals": 2, "timeout_us": 10000},
    ]


def _g0g4_program(pllr_field: Callable[[int], int]) -> Callable[..., list[dict[str, Any]]]:
    # G0/G4: HSI16 -> PLLCFGR{PLLM=M-1, PLLN=N, PLLR=<enc>} -> sysclk on PLLR.
    def build(ws: int, m: int, n: int, div: int) -> list[dict[str, Any]]:
        return [
            *_hsi_on(),
            {"op": "rmw", "peripheral": "flash", "register": "ACR", "fields": {"LATENCY": ws}},
            {"op": "rmw", "peripheral": "rcc", "register": "PLLCFGR",
             "fields": {"PLLSRC": 2, "PLLM": m - 1, "PLLN": n, "PLLR": pllr_field(div), "PLLREN": 1}},
            *_pll_on_and_switch(),
        ]
    return build


def _f4f7_program(ws: int, m: int, n: int, div: int) -> list[dict[str, Any]]:
    # F4/F7: HSI16 as PLL source (PLLSRC=0), /M to the 1–2 MHz VCO input, ×N,
    # /P (2,4,6,8) to sysclk. PLLP field = P/2 - 1.
    return [
        *_hsi_on(),
        {"op": "rmw", "peripheral": "flash", "register": "ACR", "fields": {"LATENCY": ws}},
        {"op": "rmw", "peripheral": "rcc", "register": "PLLCFGR",
         "fields": {"PLLSRC": 0, "PLLM": m, "PLLN": n, "PLLP": div // 2 - 1, "PLLQ": 4}},
        *_pll_on_and_switch(),
    ]


@dataclass(frozen=True)
class ClockModel:
    family: str
    source_hz: int
    m_range: tuple[int, int]
    n_range: tuple[int, int]
    div_values: tuple[int, ...]          # PLLR (G0/G4) or PLLP (F4/F7) options
    vco_out_hz: tuple[int, int]
    sysclk_max_hz: int
    flash_ws: tuple[tuple[int, int], ...]  # ascending (max_hz, wait_states)
    program: Callable[..., list[dict[str, Any]]]
    vco_in_hz: tuple[int, int] | None = None   # F4/F7 need the /M output in [1,2] MHz
    validated_max_hz: int | None = None        # up to here has an in-repo silicon reference


MODELS: dict[str, ClockModel] = {
    # STM32G0 — HSI16, VCO 64–344, sysclk ≤ 64, WS 0/1/2. pll_64mhz validated.
    "stm32g0": ClockModel(
        family="stm32g0", source_hz=16_000_000,
        m_range=(1, 8), n_range=(8, 86), div_values=tuple(range(2, 9)),
        vco_out_hz=(64_000_000, 344_000_000), sysclk_max_hz=64_000_000,
        flash_ws=((24_000_000, 0), (48_000_000, 1), (64_000_000, 2)),
        program=_g0g4_program(lambda r: r - 1), validated_max_hz=64_000_000,
    ),
    # STM32G4 — VOS range 1 (no boost): sysclk ≤ 150, VCO 96–344, PLLR in {2,4,6,8}.
    "stm32g4": ClockModel(
        family="stm32g4", source_hz=16_000_000,
        m_range=(1, 16), n_range=(8, 127), div_values=(2, 4, 6, 8),
        vco_out_hz=(96_000_000, 344_000_000), sysclk_max_hz=150_000_000,
        flash_ws=((30_000_000, 0), (60_000_000, 1), (90_000_000, 2),
                  (120_000_000, 3), (150_000_000, 4)),
        program=_g0g4_program(lambda r: {2: 0, 4: 1, 6: 2, 8: 3}[r]),
    ),
    # STM32F4 — VCO input 1–2 MHz, VCO out 100–432, sysclk ≤ 168 (F407 class),
    # PLLP in {2,4,6,8}. (F401/F411 top out lower; capped conservatively.)
    "stm32f4": ClockModel(
        family="stm32f4", source_hz=16_000_000,
        m_range=(2, 63), n_range=(50, 432), div_values=(2, 4, 6, 8),
        vco_in_hz=(1_000_000, 2_000_000), vco_out_hz=(100_000_000, 432_000_000),
        sysclk_max_hz=168_000_000,
        flash_ws=((30_000_000, 0), (60_000_000, 1), (90_000_000, 2), (120_000_000, 3),
                  (150_000_000, 4), (168_000_000, 5)),
        program=_f4f7_program,
    ),
    # STM32F7 — like F4; sysclk ≤ 180 without overdrive (216 needs it — capped).
    "stm32f7": ClockModel(
        family="stm32f7", source_hz=16_000_000,
        m_range=(2, 63), n_range=(50, 432), div_values=(2, 4, 6, 8),
        vco_in_hz=(1_000_000, 2_000_000), vco_out_hz=(100_000_000, 432_000_000),
        sysclk_max_hz=180_000_000,
        flash_ws=((30_000_000, 0), (60_000_000, 1), (90_000_000, 2), (120_000_000, 3),
                  (150_000_000, 4), (180_000_000, 5)),
        program=_f4f7_program,
    ),
}


def _wait_states(table: tuple[tuple[int, int], ...], sysclk: int) -> int:
    for max_hz, ws in table:
        if sysclk <= max_hz:
            return ws
    return table[-1][1]


def solve(family: str, target_hz: int) -> dict[str, Any]:
    """Best PLL setup for `target_hz` on `family` — the computed clock profile."""
    model = MODELS.get(family)
    if model is None:
        raise EmitError(
            f"no clock model for family '{family}' — supported: {', '.join(sorted(MODELS))}")
    if target_hz > model.sysclk_max_hz:
        raise EmitError(
            f"{target_hz/1e6:.0f} MHz exceeds {family}'s max {model.sysclk_max_hz/1e6:.0f} MHz "
            "(higher needs a power boost/overdrive not modelled yet)")

    best: tuple[float, int, int, int, int] | None = None
    for m in range(model.m_range[0], model.m_range[1] + 1):
        vco_in = model.source_hz / m
        if model.vco_in_hz and not (model.vco_in_hz[0] <= vco_in <= model.vco_in_hz[1]):
            continue
        for n in range(model.n_range[0], model.n_range[1] + 1):
            vco = vco_in * n
            if not (model.vco_out_hz[0] <= vco <= model.vco_out_hz[1]):
                continue
            for div in model.div_values:
                sysclk = vco / div
                if sysclk > model.sysclk_max_hz:
                    continue
                err = abs(sysclk - target_hz)
                cand = (err, int(round(sysclk)), m, n, div)
                # Tie-break toward the lower VCO (less power) via smaller n.
                if best is None or cand < best:
                    best = cand
    if best is None:
        raise EmitError(f"no PLL solution for {target_hz/1e6:.1f} MHz on {family}")

    _, sysclk, m, n, div = best
    ws = _wait_states(model.flash_ws, sysclk)
    validated = model.validated_max_hz is not None and sysclk <= model.validated_max_hz
    return {
        "schema": "alloy.clock.v1",
        "family": family,
        "requested_hz": target_hz,
        "sysclk_hz": sysclk,
        "ahb_hz": sysclk,
        "apb_hz": sysclk,
        "wait_states": ws,
        "pll": {"m": m, "n": n, "div": div, "vco_hz": int(round(model.source_hz / m * n))},
        "silicon_validated": validated,
        "profile": {
            "description": (
                f"PLL {int(model.source_hz/1e6)}MHz /{m} x{n} /{div} = "
                f"{sysclk/1e6:.0f} MHz, {ws} wait states"
                + ("" if validated else " (computed — not silicon-validated)")
            ),
            "sysclk_hz": sysclk,
            "ahb_hz": sysclk,
            "apb_hz": sysclk,
            "program": model.program(ws, m, n, div),
        },
    }
