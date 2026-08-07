"""The whole clock, not just the PLL — where it comes from and where it goes.

The configurator drew a linear chain (source → M → N → VCO → divider → SYSCLK),
which is the interesting half of a PLL and none of what a user needs next: the
bus frequencies, and the clock each peripheral is actually fed. Both are curated
facts — profiles carry `ahb_hz`/`apb_hz`, and every peripheral instance carries
`kernel_clock` — so the tree can be built without guessing.

Deriving the CONSEQUENCES matters more than drawing the boxes. A UART's baud
error and a timer's reachable period are the numbers that decide whether a
configuration works, and both fall out of the kernel clock. They are computed
here with the same arithmetic the drivers use, so the panel cannot disagree with
the firmware: `st_usart_v2::baud_div` rounds, so the reported error is the error
you will measure.
"""

from __future__ import annotations

from typing import Any

from .emit.common import EmitError

# Bus nodes a peripheral's `kernel_clock` can name, in the order they hang off
# each other. Mirrors alloy::clock_node in src/alloy/core/types.hpp.
_NODES = ("sysclk", "ahb", "apb", "apb2")
_NODE_LABEL = {
    "sysclk": "SYSCLK", "ahb": "AHB · HCLK", "apb": "APB · PCLK", "apb2": "APB2 · PCLK2",
}
_PARENT = {"ahb": "sysclk", "apb": "ahb", "apb2": "ahb"}

# What a peripheral's class implies for the reader. Anything not listed simply
# reports its frequency.
_UART = "uart"
_TIMER = "pwm"


def _profile(chip: dict[str, Any], name: str | None) -> tuple[str, dict[str, Any]]:
    profiles = (chip.get("clock") or {}).get("profiles") or {}
    if not profiles:
        raise EmitError("this chip's data declares no clock profiles")
    if name is None:
        # By database convention the FIRST profile is the boot-safe one, and
        # without a board to ask that is the honest default.
        name = str(next(iter(profiles)))
    if name not in profiles:
        raise EmitError(f"clock profile '{name}' is not one this chip offers "
                        f"(has: {', '.join(profiles)})")
    return name, profiles[name]


def _node_hz(profile: dict[str, Any]) -> dict[str, int]:
    sysclk = int(profile["sysclk_hz"])
    ahb = int(profile.get("ahb_hz", sysclk))
    apb = int(profile.get("apb_hz", ahb))
    return {
        "sysclk": sysclk, "ahb": ahb, "apb": apb,
        "apb2": int(profile.get("apb2_hz", apb)),
    }


def _baud_note(kernel_hz: int, baud: int) -> dict[str, Any]:
    """The error the hardware will actually show.

    Same rounding as st_usart_v2::baud_div — a panel that computed it any other
    way would report a number the board does not produce.
    """
    if kernel_hz <= 0 or baud <= 0:
        return {"level": "warning", "text": "cannot compute baud error"}
    brr = (kernel_hz + baud // 2) // baud
    if brr == 0:
        return {"level": "error",
                "text": f"{baud} baud is faster than this kernel clock allows"}
    achieved = kernel_hz // brr
    error = abs(achieved - baud) / baud * 100.0
    level = "error" if error > 3.0 else "warning" if error > 2.0 else "info"
    return {
        "level": level,
        "text": f"{baud} baud → {achieved} ({error:.2f}% error)"
                + (" — most receivers need under 2%" if error > 2.0 else ""),
    }


def _timer_note(kernel_hz: int, counter_bits: int = 16) -> dict[str, Any]:
    """The PWM frequencies a 16-bit counter can reach from this clock."""
    top = (1 << counter_bits) - 1
    slowest = kernel_hz / (top + 1)
    return {
        "level": "info",
        "text": f"{counter_bits}-bit counter: {slowest:.0f} Hz – {kernel_hz / 2:.0f} Hz "
                f"without a prescaler",
    }


def clock_graph(chip: dict[str, Any], registers: dict[str, dict[str, Any]],
                chip_id: str = "", profile_name: str | None = None,
                solved: dict[str, Any] | None = None) -> dict[str, Any]:
    """The alloy.clock_graph.v1 envelope for one chip and one clock setup.

    `solved` is a result from `clock_solver.solve` — the graph for a frequency
    the user typed, before it is written to a board.
    """
    if solved is not None:
        name = "custom"
        profile = solved["profile"]
        pll = solved.get("pll")
        wait_states = solved.get("wait_states")
        validated = solved.get("silicon_validated", False)
    else:
        name, profile = _profile(chip, profile_name)
        pll = None  # a named profile carries its PLL inside the register program
        wait_states = None
        validated = True  # it is in the database because it was verified there

    hz = _node_hz(profile)
    # Which oscillator actually feeds the PLL. A SOLVED clock knows (the solver
    # reports it, and `--hse` makes it the board's crystal); a named profile does
    # not say, so the chip's boot source is the best the data offers.
    #
    # Getting this from boot_source unconditionally drew a chain that contradicted
    # its own arithmetic: HSI16 at 16 MHz into ÷4 ×180 ÷2 is 360 MHz, not the
    # 180 the same graph reported as SYSCLK.
    chosen = (solved or {}).get("source") or {}
    boot = (chip.get("clock") or {}).get("boot_source")
    declared = sorted(((chip.get("clock") or {}).get("sources") or {}).items())
    sources = [
        {"name": key, "hz": int(spec.get("hz", 0)),
         "selected": (int(spec.get("hz", 0)) == int(chosen["hz"])) if chosen.get("hz")
                     else key == boot}
        for key, spec in declared
    ]
    # A crystal the board fits is not always one the chip data lists.
    if chosen.get("hz") and not any(s["selected"] for s in sources):
        sources.append({"name": str(chosen.get("name", "external")),
                        "hz": int(chosen["hz"]), "selected": True})

    nodes = [{"name": "sysclk", "label": _NODE_LABEL["sysclk"], "hz": hz["sysclk"],
              "parent": None, "divider": None}]
    for node in ("ahb", "apb", "apb2"):
        parent = _PARENT[node]
        parent_hz = hz[parent]
        if node == "apb2" and hz["apb2"] == hz["apb"] and "apb2_hz" not in profile:
            continue  # this chip declares one APB; a second box would be a fiction
        nodes.append({
            "name": node, "label": _NODE_LABEL[node], "hz": hz[node],
            "parent": parent,
            "divider": (parent_hz // hz[node]) if hz[node] else None,
        })

    # Consumers: every curated peripheral that declares which node feeds it.
    consumers: list[dict[str, Any]] = []
    for periph, spec in sorted((chip.get("peripherals") or {}).items()):
        node = spec.get("kernel_clock")
        if node not in _NODES:
            continue
        ip_class = (registers.get(spec.get("ip") or "") or {}).get("class")
        kernel_hz = hz[node]
        notes: list[dict[str, Any]] = []
        if ip_class == _UART:
            notes.append(_baud_note(kernel_hz, 115200))
        elif ip_class == _TIMER:
            notes.append(_timer_note(kernel_hz))
        consumers.append({
            "peripheral": periph, "class": ip_class, "node": node,
            "hz": kernel_hz, "notes": notes,
        })

    # Peripherals whose feed this chip's data does not state. Some genuinely run
    # off their own oscillator (an independent watchdog on the LSI); others —
    # GPIO ports, RCC itself — simply never needed the fact because no driver
    # asks for it. Either way the tree cannot place them, and listing them is
    # more honest than a reader assuming they were forgotten.
    unstated = sorted(
        p for p, spec in (chip.get("peripherals") or {}).items()
        if "kernel_clock" not in spec and not spec.get("uncurated", False)
    )

    issues = [
        {"level": note["level"], "peripheral": c["peripheral"], "text": note["text"]}
        for c in consumers for note in c["notes"] if note["level"] != "info"
    ]

    return {
        "schema": "alloy.clock_graph.v1",
        "chip": chip_id or str(chip.get("part", "")),
        "profile": name,
        "description": profile.get("description", ""),
        "silicon_validated": validated,
        "sources": sources,
        "pll": pll,
        "wait_states": wait_states,
        "nodes": nodes,
        "consumers": consumers,
        "unstated": unstated,
        # With --mhz, the profile that would be written into board.json. Carried
        # here so an editor gets the graph AND the thing to save in one call.
        "solved_profile": profile if solved is not None else None,
        "issues": issues,
    }
