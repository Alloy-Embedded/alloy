"""Parametric PLL clock solver, per family.

The device database ships a few hand-verified clock *profiles* per chip. This
solver goes further: given a target system frequency it searches the family's
PLL — every supported family reduces to `source / M × N / divider` — respecting
the datasheet limits (VCO window, divider ranges, max sysclk), picks flash wait
states where the part has them, and emits the register `program`. So
`alloy clock --chip X --mhz N` yields a buildable custom clock.

Each family's program builder is its SILICON-VALIDATED profile with only the
frequency-dependent fields substituted — the fixed bring-up steps (oscillator
start, PLL lock polling, clock routing) are copied verbatim, so the solver
reproduces the validated recipe exactly at its anchor frequency:
  STM32G0  64 MHz  -> PLLM=0 PLLN=8 PLLR=1, 2 WS   (pll_64mhz)
  RP2040  125 MHz  -> REFDIV=1 FBDIV=125 PD1=6 PD2=2 (pll_125mhz)
  SAME70  150 MHz  -> DIVA=1 MULA=24 (x25) /2, 6 WS (plla_150mhz)

Safety: solutions are correct-by-construction against the datasheet limits, but
only families/frequencies with an in-repo silicon reference (validated_max_hz)
are board-tested. ESP32 v1 runs a fixed ROM/XTAL clock — not solvable here.

EXTERNAL SOURCES (crystals). A product almost never runs the PLL off the
internal RC: USB, CAN, high baud rates and RTC accuracy all need a crystal. The
crystal is a BOARD fact — the chip only says an external input exists and over
what range it is legal — so `solve(..., external_hz=…)` takes the frequency from
the board and the family model bounds it (`hse_hz`). Every generated program
keeps the same shape, so the fallback in the generated `board::init()` still
applies: if the crystal is absent or dead the HSERDY poll times out, the program
aborts, and the board runs on its boot source instead of hanging.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable

from .emit.common import EmitError


def _poll(periph: str, reg: str, fld: str, us: int) -> dict[str, Any]:
    return {"op": "poll", "peripheral": periph, "register": reg, "field": fld,
            "equals": 1, "timeout_us": us}


@dataclass(frozen=True)
class Source:
    """Where the PLL takes its reference.

    `external` means a crystal or an oscillator wired on the board, so `hz` came
    from board.json rather than from the chip data. `bypass` distinguishes a
    driven clock signal (an active oscillator, or the 8 MHz MCO an ST-Link feeds
    a Nucleo) from a passive crystal the MCU has to excite itself.
    """

    hz: int
    name: str
    external: bool = False
    bypass: bool = False


# A crystal needs milliseconds to start and stabilize; the internal RC needs
# microseconds. ST's own HAL waits 100 ms for HSE, and that is the number that
# decides whether a board with a marginal crystal boots or falls back.
_HSE_STARTUP_US = 100_000
_HSI_STARTUP_US = 2_000


# ---- STM32 program builders ------------------------------------------------

def _stm32_start(src: Source) -> list[dict[str, Any]]:
    if not src.external:
        return [
            {"op": "rmw", "peripheral": "rcc", "register": "CR", "fields": {"HSION": 1}},
            _poll("rcc", "CR", "HSIRDY", _HSI_STARTUP_US),
        ]
    ops: list[dict[str, Any]] = []
    if src.bypass:
        # HSEBYP is writable only while HSEON is clear — set it FIRST or the
        # write is dropped and the MCU tries to drive a crystal that isn't there.
        ops.append({"op": "rmw", "peripheral": "rcc", "register": "CR",
                    "fields": {"HSEBYP": 1}})
    ops.append({"op": "rmw", "peripheral": "rcc", "register": "CR",
                "fields": {"HSEON": 1}})
    ops.append(_poll("rcc", "CR", "HSERDY", _HSE_STARTUP_US))
    return ops


def _stm32_common(ws: int, pll_fields: dict[str, Any],
                  src: Source) -> list[dict[str, Any]]:
    return [
        *_stm32_start(src),
        {"op": "rmw", "peripheral": "flash", "register": "ACR", "fields": {"LATENCY": ws}},
        {"op": "rmw", "peripheral": "rcc", "register": "PLLCFGR", "fields": pll_fields},
        {"op": "rmw", "peripheral": "rcc", "register": "CR", "fields": {"PLLON": 1}},
        _poll("rcc", "CR", "PLLRDY", 10000),
        {"op": "rmw", "peripheral": "rcc", "register": "CFGR", "fields": {"SW": 2}},
        _poll("rcc", "CFGR", "SWS", 10000),
    ]


def _g0g4_program(r_field: Callable[[int], int]) -> Callable[..., list[dict[str, Any]]]:
    def build(ws: int, m: int, n: int, div: int, _tok: Any,
              src: Source) -> list[dict[str, Any]]:
        # RM0444/RM0440 PLLSRC[1:0]: 10 = HSI16, 11 = HSE.
        return _stm32_common(ws, {"PLLSRC": 3 if src.external else 2,
                                  "PLLM": m - 1, "PLLN": n,
                                  "PLLR": r_field(div), "PLLREN": 1}, src)
    return build


def _f4f7_program(ws: int, m: int, n: int, div: int, _tok: Any,
                  src: Source) -> list[dict[str, Any]]:
    # RM0090/RM0410 PLLCFGR.PLLSRC is one bit: 0 = HSI, 1 = HSE.
    return _stm32_common(ws, {"PLLSRC": 1 if src.external else 0,
                              "PLLM": m, "PLLN": n,
                              "PLLP": div // 2 - 1, "PLLQ": 4}, src)


# ---- RP2040 program builder (pll_125mhz template, FBDIV/POSTDIV substituted) --

def _rp2040_program(_ws: int, _m: int, n: int, _div: int, tok: Any,
                    _src: Source) -> list[dict[str, Any]]:
    pd1, pd2 = tok
    return [
        {"op": "rmw", "peripheral": "clocks", "register": "CLK_SYS_CTRL", "fields": {"SYS_SRC": 0}},
        {"op": "rmw", "peripheral": "xosc", "register": "STARTUP", "fields": {"DELAY": 47}},
        {"op": "rmw", "peripheral": "xosc", "register": "CTRL", "fields": {"FREQ_RANGE": 2720}},
        {"op": "rmw", "peripheral": "xosc", "register": "CTRL", "fields": {"ENABLE": 4011}},
        {"op": "rmw", "peripheral": "clocks", "register": "CLK_REF_CTRL", "fields": {"REF_SRC": 2}},
        {"op": "rmw", "peripheral": "resets", "register": "RESET", "fields": {"PLL_SYS": 0}},
        {"op": "rmw", "peripheral": "pll_sys", "register": "CS", "fields": {"REFDIV": 1}},
        {"op": "rmw", "peripheral": "pll_sys", "register": "FBDIV_INT", "fields": {"FBDIV": n}},
        {"op": "rmw", "peripheral": "pll_sys", "register": "PWR", "fields": {"PD": 0, "VCOPD": 0}},
        {"op": "rmw", "peripheral": "pll_sys", "register": "PRIM",
         "fields": {"POSTDIV1": pd1, "POSTDIV2": pd2}},
        {"op": "rmw", "peripheral": "pll_sys", "register": "PWR", "fields": {"POSTDIVPD": 0}},
        {"op": "rmw", "peripheral": "clocks", "register": "CLK_SYS_CTRL", "fields": {"SYS_AUXSRC": 0}},
        {"op": "rmw", "peripheral": "clocks", "register": "CLK_SYS_CTRL", "fields": {"SYS_SRC": 1}},
        {"op": "rmw", "peripheral": "clocks", "register": "CLK_PERI_CTRL",
         "fields": {"PERI_AUXSRC": 0, "PERI_ENABLE": 1}},
    ]


# ---- SAME70 program builder (plla_150mhz template, MULA/DIVA/PRES/FWS subst.) --

def _same70_program(ws: int, m: int, n: int, _div: int, tok: Any,
                    _src: Source) -> list[dict[str, Any]]:
    pres_field = tok
    mula = n - 1  # PLLA multiplies by MULA+1
    # CKGR_PLLAR = ONE(bit29) | MULA<<16 | PLLACOUNT(0x3F)<<8 | DIVA. 12 MHz xtal.
    pllar = 0x2000_0000 | (mula << 16) | (0x3F << 8) | m
    # The fixed prologue below is frequency-INDEPENDENT and is copied verbatim from
    # the silicon-validated `plla_150mhz` profile in alloy-devices
    # (chips/microchip/atsame70q21.yaml). These four writes MUST be full-register
    # writes, not decomposable `rmw` field ops: WDT/RSWDT `MR` are write-once, and
    # `CKGR_MOR` is KEY-protected (0x37) — a read-modify-write would be rejected by
    # the silicon. test_same70_prologue_matches_chip_data pins them to the yaml so
    # they can't drift from the single source of truth.
    return [
        {"op": "write", "peripheral": "wdt", "register": "MR", "value": "0x0FFF8000"},  # contract-ok: watchdog-disable, write-once MR; mirrors atsame70q21.yaml
        {"op": "write", "peripheral": "rswdt", "register": "MR", "value": "0x0FFF8000"},  # contract-ok: reinforced-watchdog-disable, write-once MR; mirrors atsame70q21.yaml
        {"op": "write", "peripheral": "pmc", "register": "CKGR_MOR", "value": "0x0037FF29"},  # contract-ok: enable 12 MHz crystal, KEY=0x37 full write; mirrors atsame70q21.yaml
        _poll("pmc", "SR", "MOSCXTS", 100000),
        {"op": "write", "peripheral": "pmc", "register": "CKGR_MOR", "value": "0x0137FF29"},  # contract-ok: select crystal as main clock, KEY=0x37 full write; mirrors atsame70q21.yaml
        _poll("pmc", "SR", "MOSCSELS", 100000),
        {"op": "rmw", "peripheral": "efc", "register": "FMR", "fields": {"FWS": ws}},
        {"op": "write", "peripheral": "pmc", "register": "CKGR_PLLAR",
         "value": f"0x{pllar:08X}"},
        _poll("pmc", "SR", "LOCKA", 20000),
        {"op": "rmw", "peripheral": "pmc", "register": "MCKR", "fields": {"CSS": 1}},
        _poll("pmc", "SR", "MCKRDY", 20000),
        {"op": "rmw", "peripheral": "pmc", "register": "MCKR", "fields": {"PRES": pres_field}},
        _poll("pmc", "SR", "MCKRDY", 20000),
        {"op": "rmw", "peripheral": "pmc", "register": "MCKR", "fields": {"MDIV": 0}},
        _poll("pmc", "SR", "MCKRDY", 20000),
        {"op": "rmw", "peripheral": "pmc", "register": "MCKR", "fields": {"CSS": 2}},
        _poll("pmc", "SR", "MCKRDY", 20000),
    ]


@dataclass(frozen=True)
class Divisor:
    value: int            # the overall division applied to the VCO
    token: Any            # what the program builder needs (e.g. (pd1, pd2) or a PRES field)


@dataclass(frozen=True)
class ClockModel:
    family: str
    source_hz: int
    m_range: tuple[int, int]
    n_range: tuple[int, int]
    divisors: tuple[Divisor, ...]
    vco_out_hz: tuple[int, int]
    sysclk_max_hz: int
    program: Callable[..., list[dict[str, Any]]]
    flash_ws: tuple[tuple[int, int], ...] = ()   # ascending (max_hz, wait_states); () = none
    vco_in_hz: tuple[int, int] | None = None
    validated_max_hz: int | None = None
    source_name: str = "internal oscillator"
    note: str | None = None
    # Legal frequency range of the family's EXTERNAL oscillator input, or None
    # when this family's source is fixed by the board designs alloy supports
    # (RP2040 and SAME70 hardcode a 12 MHz crystal in their bring-up prologue).
    external_hz: tuple[int, int] | None = None
    external_name: str = "HSE"


def _r_divs(rs: range) -> tuple[Divisor, ...]:
    return tuple(Divisor(r, r) for r in rs)


def _p_divs() -> tuple[Divisor, ...]:  # STM32 F4/F7 PLLP in {2,4,6,8}
    return tuple(Divisor(p, p) for p in (2, 4, 6, 8))


def _rp2040_divs() -> tuple[Divisor, ...]:
    # POSTDIV1 in 1..7, POSTDIV2 in 1..POSTDIV1 (datasheet convention pd1 >= pd2).
    seen: dict[int, tuple[int, int]] = {}
    for pd1 in range(7, 0, -1):
        for pd2 in range(pd1, 0, -1):
            seen.setdefault(pd1 * pd2, (pd1, pd2))
    return tuple(Divisor(v, t) for v, t in sorted(seen.items()))


def _same70_divs() -> tuple[Divisor, ...]:
    # PMC PRES field: 0=/1 1=/2 2=/4 3=/8 4=/16 5=/32 6=/64 7=/3.
    table = {1: 0, 2: 1, 4: 2, 8: 3, 16: 4, 32: 5, 64: 6, 3: 7}
    return tuple(Divisor(v, f) for v, f in table.items())


MODELS: dict[str, ClockModel] = {
    "stm32g0": ClockModel(
        family="stm32g0", source_hz=16_000_000, source_name="HSI16",
        m_range=(1, 8), n_range=(8, 86), divisors=_r_divs(range(2, 9)),
        # RM0444: the PLL input (source/M) must land in 2.66–16 MHz. With HSI16
        # this only excludes M=7,8; with an arbitrary crystal it is what keeps
        # the solver from proposing an out-of-spec divider.
        vco_in_hz=(2_660_000, 16_000_000),
        vco_out_hz=(64_000_000, 344_000_000), sysclk_max_hz=64_000_000,
        flash_ws=((24_000_000, 0), (48_000_000, 1), (64_000_000, 2)),
        program=_g0g4_program(lambda r: r - 1), validated_max_hz=64_000_000,
        external_hz=(4_000_000, 48_000_000),
    ),
    "stm32g4": ClockModel(
        family="stm32g4", source_hz=16_000_000, source_name="HSI16",
        m_range=(1, 16), n_range=(8, 127), divisors=_r_divs(range(2, 9, 2)),
        vco_in_hz=(2_660_000, 16_000_000),  # RM0440, same window as the G0
        vco_out_hz=(96_000_000, 344_000_000), sysclk_max_hz=150_000_000,
        flash_ws=((30_000_000, 0), (60_000_000, 1), (90_000_000, 2),
                  (120_000_000, 3), (150_000_000, 4)),
        program=_g0g4_program(lambda r: {2: 0, 4: 1, 6: 2, 8: 3}[r]),
        external_hz=(4_000_000, 48_000_000),
    ),
    "stm32f4": ClockModel(
        family="stm32f4", source_hz=16_000_000, source_name="HSI16",
        m_range=(2, 63), n_range=(50, 432), divisors=_p_divs(),
        vco_in_hz=(1_000_000, 2_000_000), vco_out_hz=(100_000_000, 432_000_000),
        sysclk_max_hz=168_000_000,
        flash_ws=((30_000_000, 0), (60_000_000, 1), (90_000_000, 2), (120_000_000, 3),
                  (150_000_000, 4), (168_000_000, 5)),
        program=_f4f7_program,
        external_hz=(4_000_000, 26_000_000),
    ),
    "stm32f7": ClockModel(
        family="stm32f7", source_hz=16_000_000, source_name="HSI16",
        m_range=(2, 63), n_range=(50, 432), divisors=_p_divs(),
        vco_in_hz=(1_000_000, 2_000_000), vco_out_hz=(100_000_000, 432_000_000),
        sysclk_max_hz=180_000_000,
        flash_ws=((30_000_000, 0), (60_000_000, 1), (90_000_000, 2), (120_000_000, 3),
                  (150_000_000, 4), (180_000_000, 5)),
        program=_f4f7_program,
        external_hz=(4_000_000, 26_000_000),
    ),
    # RP2040 — XOSC 12 MHz, PLL_SYS: /REFDIV ×FBDIV /(POSTDIV1×POSTDIV2). VCO
    # 400–1600 MHz, sysclk ≤ 133. Flash is external XIP (no core wait states here).
    "rp2040": ClockModel(
        family="rp2040", source_hz=12_000_000, source_name="XOSC 12 MHz crystal",
        m_range=(1, 1), n_range=(16, 320), divisors=_rp2040_divs(),
        vco_out_hz=(400_000_000, 1600_000_000), sysclk_max_hz=133_000_000,
        program=_rp2040_program, validated_max_hz=125_000_000,
    ),
    # SAME70 — XTAL 12 MHz, PLLA ×(MULA+1) /DIVA, MCK prescaler. PLLA out
    # 160–500 MHz. Capped at 150 MHz: that is the validated recipe AND the flash
    # (EEFC) limit — running the core above it needs code in ITCM/cache, which
    # this data-driven clock init does not set up.
    "same70": ClockModel(
        family="same70", source_hz=12_000_000, source_name="XTAL 12 MHz",
        m_range=(1, 1), n_range=(2, 63), divisors=_same70_divs(),
        vco_out_hz=(160_000_000, 500_000_000), sysclk_max_hz=150_000_000,
        flash_ws=((23_000_000, 0), (46_000_000, 1), (69_000_000, 2), (92_000_000, 3),
                  (115_000_000, 4), (138_000_000, 5), (150_000_000, 6)),
        program=_same70_program, validated_max_hz=150_000_000,
    ),
}

# Families whose clock is fixed / not solvable, with a helpful message.
_UNSOLVABLE = {
    "esp32": "ESP32 v1 runs a fixed ROM/XTAL clock — clock configuration is not "
             "available for this family yet.",
}


def _wait_states(table: tuple[tuple[int, int], ...], sysclk: int) -> int:
    for max_hz, ws in table:
        if sysclk <= max_hz:
            return ws
    return table[-1][1] if table else 0


def _source_for(model: ClockModel, external_hz: int | None, bypass: bool) -> Source:
    if external_hz is None:
        if bypass:
            raise EmitError("--hse-bypass only means something with an external clock "
                            f"— pass the frequency too (--hse <Hz>)")
        return Source(hz=model.source_hz, name=model.source_name)
    if model.external_hz is None:
        raise EmitError(
            f"{model.family} already runs from {model.source_name} and its bring-up "
            "sequence hardcodes that frequency — an arbitrary external clock is not "
            "supported for this family yet")
    lo, hi = model.external_hz
    if not (lo <= external_hz <= hi):
        raise EmitError(
            f"{external_hz/1e6:g} MHz is outside {model.family}'s {model.external_name} "
            f"range {lo/1e6:g}–{hi/1e6:g} MHz")
    kind = "external clock" if bypass else "crystal"
    return Source(hz=external_hz, name=f"{model.external_name} {external_hz/1e6:g} MHz {kind}",
                  external=True, bypass=bypass)


def solve(family: str, target_hz: int, external_hz: int | None = None,
          bypass: bool = False) -> dict[str, Any]:
    """Best PLL setup for `target_hz` on `family` — the computed clock profile.

    `external_hz` switches the PLL to the board's crystal/oscillator (a board
    fact); `bypass` says that input is a driven clock signal rather than a
    passive crystal.
    """
    if family in _UNSOLVABLE:
        raise EmitError(_UNSOLVABLE[family])
    model = MODELS.get(family)
    if model is None:
        raise EmitError(
            f"no clock model for family '{family}' — supported: {', '.join(sorted(MODELS))}")
    if target_hz > model.sysclk_max_hz:
        raise EmitError(
            f"{target_hz/1e6:.0f} MHz exceeds {family}'s max {model.sysclk_max_hz/1e6:.0f} MHz "
            "(higher needs a power boost/overdrive not modelled yet)")
    src = _source_for(model, external_hz, bypass)

    best: tuple[float, int, int, int, Divisor] | None = None
    for m in range(model.m_range[0], model.m_range[1] + 1):
        if model.vco_in_hz:
            vco_in = src.hz / m
            if not (model.vco_in_hz[0] <= vco_in <= model.vco_in_hz[1]):
                continue
        for n in range(model.n_range[0], model.n_range[1] + 1):
            vco = src.hz / m * n
            if not (model.vco_out_hz[0] <= vco <= model.vco_out_hz[1]):
                continue
            for d in model.divisors:
                sysclk = vco / d.value
                if sysclk > model.sysclk_max_hz:
                    continue
                err = abs(sysclk - target_hz)
                cand = (err, int(round(sysclk)), m, n, d)
                if best is None or cand[:4] < best[:4]:
                    best = cand
    if best is None:
        raise EmitError(f"no PLL solution for {target_hz/1e6:.1f} MHz on {family}")

    _, sysclk, m, n, d = best
    ws = _wait_states(model.flash_ws, sysclk)
    # An external source is never silicon-validated by the in-repo reference: the
    # anchor recipes all run from the internal RC. Say so rather than inherit a
    # confidence that was measured on a different clock path.
    validated = (model.validated_max_hz is not None
                 and sysclk <= model.validated_max_hz
                 and not src.external)
    return {
        "schema": "alloy.clock.v1",
        "family": family,
        "requested_hz": target_hz,
        "sysclk_hz": sysclk,
        "ahb_hz": sysclk,
        "apb_hz": sysclk,
        "wait_states": ws,
        "source": {"hz": src.hz, "name": src.name,
                   "external": src.external, "bypass": src.bypass},
        "pll": {"m": m, "n": n, "div": d.value, "vco_hz": int(round(src.hz / m * n))},
        "silicon_validated": validated,
        "profile": {
            "description": (
                f"PLL {src.hz/1e6:g}MHz /{m} x{n} /{d.value} = "
                f"{sysclk/1e6:.0f} MHz" + (f", {ws} wait states" if model.flash_ws else "")
                + (f" from {src.name}" if src.external else "")
                + ("" if validated else " (computed — not silicon-validated)")
            ),
            "sysclk_hz": sysclk,
            "ahb_hz": sysclk,
            "apb_hz": sysclk,
            "program": model.program(ws, m, n, d.value, d.token, src),
        },
    }
