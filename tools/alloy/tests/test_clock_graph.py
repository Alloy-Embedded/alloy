"""Unit tests for `alloy clock --graph`.

The graph's value is not the boxes, it is the consequences: a baud error and a
timer's reachable range are what decide whether a clock choice works. So most of
this file checks that those numbers match what the FIRMWARE will do — a panel
that computed them differently would be worse than one that showed nothing.
"""

from __future__ import annotations

from pathlib import Path

import pytest

from alloy_cli.clock_graph import _baud_note, _timer_note, clock_graph
from alloy_cli.emit.common import EmitError

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _graph(chip_id: str, profile: str | None = None, solved=None):
    from alloy_cli.devices import load_chip, load_registers

    return clock_graph(load_chip(DEVICES_ROOT, chip_id), load_registers(DEVICES_ROOT),
                       chip_id=chip_id, profile_name=profile, solved=solved)


def _node(graph, name):
    return next(n for n in graph["nodes"] if n["name"] == name)


def _consumer(graph, name):
    return next(c for c in graph["consumers"] if c["peripheral"] == name)


# --------------------------------------------------------------- the buses

@skip_no_devices
def test_bus_frequencies_and_dividers_come_from_the_profile() -> None:
    """The F767 is the interesting case: the buses are genuinely divided down,
    which is exactly what the old linear PLL diagram could not show."""
    graph = _graph("st/stm32f767", "pll_180mhz")
    assert _node(graph, "sysclk")["hz"] == 180_000_000
    assert _node(graph, "ahb")["hz"] == 180_000_000
    apb = _node(graph, "apb")
    assert (apb["hz"], apb["divider"], apb["parent"]) == (45_000_000, 4, "ahb")
    apb2 = _node(graph, "apb2")
    assert (apb2["hz"], apb2["divider"], apb2["parent"]) == (90_000_000, 2, "ahb")


@skip_no_devices
def test_a_second_apb_is_not_invented() -> None:
    """The G0 has one APB. Drawing a second box because the emitter mirrors the
    value would be a fiction the data does not support."""
    graph = _graph("st/stm32g071rb", "pll_64mhz")
    assert [n["name"] for n in graph["nodes"]] == ["sysclk", "ahb", "apb"]


# ----------------------------------------------------------- the consumers

@skip_no_devices
def test_each_peripheral_reports_the_clock_it_is_actually_fed() -> None:
    """`kernel_clock` is the same fact `alloy::uart::bind::kernel_hz()` reads,
    so the panel and the firmware cannot disagree about it."""
    graph = _graph("st/stm32f767", "pll_180mhz")
    assert _consumer(graph, "usart3")["hz"] == 45_000_000     # APB
    assert _consumer(graph, "eth")["hz"] == 180_000_000       # AHB
    assert _consumer(graph, "eth")["node"] == "ahb"


@skip_no_devices
def test_peripherals_with_no_declared_feed_are_listed_not_dropped() -> None:
    """An independent watchdog runs off its own oscillator; a GPIO port simply
    never needed the fact. Both are unplaceable, and silence would read as
    "forgotten"."""
    graph = _graph("st/stm32g071rb", "pll_64mhz")
    assert "iwdg" in graph["unstated"]
    assert "gpioa" in graph["unstated"]
    placed = {c["peripheral"] for c in graph["consumers"]}
    assert not (placed & set(graph["unstated"])), "a peripheral is placed or it is not"


# ------------------------------------------------------- the consequences

def test_baud_error_uses_the_drivers_own_rounding() -> None:
    """st_usart_v2 computes BRR = (kernel + baud/2) / baud and the hardware then
    divides by it. Anything else here would report an error nobody measures."""
    note = _baud_note(64_000_000, 115200)
    brr = (64_000_000 + 115200 // 2) // 115200          # 556
    assert f"{64_000_000 // brr}" in note["text"]        # 115107, not 115200
    assert note["level"] == "info"


@pytest.mark.parametrize("kernel_hz,level", [
    (64_000_000, "info"),      # 0.08% — fine
    (2_000_000, "warning"),    # 2.4% — marginal
    (1_000_000, "error"),      # 3.5% — most receivers will not lock
])
def test_baud_error_is_graded_by_what_receivers_tolerate(kernel_hz, level) -> None:
    assert _baud_note(kernel_hz, 115200)["level"] == level


def test_a_baud_faster_than_the_clock_is_an_error_not_a_division() -> None:
    note = _baud_note(50_000, 115200)
    assert note["level"] == "error" and "faster than" in note["text"]


def test_timer_range_reflects_the_counter_width() -> None:
    """977 Hz is where a 16-bit counter runs out at 64 MHz — the number that
    tells you whether you need a prescaler."""
    text = _timer_note(64_000_000)["text"]
    assert "977 Hz" in text and "16-bit" in text


@skip_no_devices
def test_problems_are_promoted_to_the_top_level() -> None:
    """A panel should not need to walk every consumer to find out something is
    wrong."""
    graph = _graph("st/stm32g071rb", "pll_64mhz")
    assert graph["issues"] == []
    # Every issue must be traceable back to the peripheral that raised it.
    for issue in graph["issues"]:
        assert issue["peripheral"] and issue["level"] in ("warning", "error")


# ------------------------------------------------------------- the profile

@skip_no_devices
def test_a_solved_clock_carries_its_pll_and_honesty_flag() -> None:
    from alloy_cli.clock_solver import solve

    solved = solve("stm32g0", 20_000_000)
    graph = _graph("st/stm32g071rb", solved=solved)
    assert graph["profile"] == "custom"
    assert graph["pll"]["n"] == solved["pll"]["n"]
    assert graph["wait_states"] == solved["wait_states"]
    assert graph["silicon_validated"] == solved["silicon_validated"]
    # And the buses follow the solved sysclk, not the chip's stored profile.
    assert _node(graph, "sysclk")["hz"] == solved["sysclk_hz"]


@skip_no_devices
def test_an_unknown_profile_names_the_real_ones() -> None:
    with pytest.raises(EmitError, match="pll_64mhz"):
        _graph("st/stm32g071rb", "turbo_mode")


@skip_no_devices
def test_the_default_profile_is_the_boot_safe_one() -> None:
    """Without a board to ask, defaulting to the fastest would describe a clock
    the chip is not in."""
    graph = _graph("st/stm32g071rb")
    assert graph["profile"] == "hsi_16mhz"
    assert _node(graph, "sysclk")["hz"] == 16_000_000


# ------------------------------------------------- the chain must add up

@skip_no_devices
@pytest.mark.parametrize("hse_hz", [None, 8_000_000])
def test_the_drawn_chain_is_arithmetically_consistent(hse_hz: int | None) -> None:
    """source / M x N / div MUST equal the SYSCLK the same graph reports.

    This is the invariant a picture of a clock has to satisfy, and it is what
    caught the graph marking the chip's boot source as selected while the solver
    was feeding the PLL from the board's crystal: HSI16 at 16 MHz into ÷4 ×180 ÷2
    is 360 MHz, next to a SYSCLK box reading 180.
    """
    from alloy_cli.clock_solver import solve

    solved = solve("stm32f7", 180_000_000, external_hz=hse_hz)
    graph = _graph("st/stm32f767", solved=solved)

    selected = [s for s in graph["sources"] if s["selected"]]
    assert len(selected) == 1, f"exactly one source drives the PLL, got {selected}"
    pll = graph["pll"]
    computed = selected[0]["hz"] / pll["m"] * pll["n"] / pll["div"]
    assert computed == _node(graph, "sysclk")["hz"]


@skip_no_devices
def test_a_crystal_the_chip_data_does_not_list_still_appears() -> None:
    """A board may fit an oscillator the chip file never mentions; leaving it out
    would draw a PLL with nothing feeding it."""
    from alloy_cli.clock_solver import solve

    solved = solve("stm32g0", 48_000_000, external_hz=12_000_000)
    graph = _graph("st/stm32g071rb", solved=solved)
    selected = next(s for s in graph["sources"] if s["selected"])
    assert selected["hz"] == 12_000_000
