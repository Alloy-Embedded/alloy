"""The pin-interrupt (EXTI) half of the emitted Renode platform.

What these tests defend is the claim in the emitter's comment: the EXTI block is
DATA-DRIVEN. Not one of the numbers Renode reads — the NVIC vectors, the port
routing values, the port aperture — may be written down in the generator. So
each test perturbs a chip FACT and asserts the emitted platform moves with it; a
hardcoded number would keep the assertions passing with the fact changed, which
is exactly the failure these are shaped to catch.

The other half of the contract is that EXTI is OPTIONAL: a chip without the data
gets no block at all, like every other optional peripheral in the platform.
"""

from __future__ import annotations

import copy
import json
from pathlib import Path

import pytest

from alloy_cli.devices import load_chip
from alloy_cli.emit.renode import emit_renode_platform

ALLOY_ROOT = Path(__file__).resolve().parents[3]
DEVICES_ROOT = ALLOY_ROOT.parent / "alloy-devices"

skip_no_devices = pytest.mark.skipif(
    not (DEVICES_ROOT / "chips").is_dir(),
    reason="alloy-devices not beside the framework")


def _board(board_id: str) -> dict:
    return json.loads((ALLOY_ROOT / "boards" / board_id / "board.json").read_text())


def _g071() -> tuple[dict, dict]:
    return load_chip(DEVICES_ROOT, "st/stm32g071rb"), _board("nucleo_g071rb")


@skip_no_devices
def test_the_g0_platform_carries_an_exti_and_its_ports() -> None:
    chip, board = _g071()
    repl = emit_renode_platform(chip, board)
    # Bounded, not bare: see test_the_exti_window_stops_before_its_neighbour.
    assert ("exti: IRQControllers.STM32WBA_EXTI @ sysbus "
            "<0x40021800, +0x800>") in repl
    # Every curated port is present, and each reaches EXTI through the
    # connection group named by its port_index.
    for name, index in (("gpioa", 0), ("gpiob", 1), ("gpioc", 2),
                        ("gpiod", 3), ("gpiof", 5)):
        assert f"{name}: GPIOPort.STM32_GPIOPort @ sysbus" in repl
        assert f"[0-15] -> exti#{index}@[0-15]" in repl


@skip_no_devices
def test_the_port_group_is_the_port_index_not_a_dense_counter() -> None:
    """The G071RB has NO port E. gpiof is the FIFTH curated port but the SIXTH
    EXTICR code, so a dense counter over the ports this die has would route it
    to exti#4 and silently drop every edge on port F."""
    chip, board = _g071()
    repl = emit_renode_platform(chip, board)
    assert "exti#5@[0-15]" in repl
    assert "exti#4@[0-15]" not in repl


@skip_no_devices
def test_the_nvic_vectors_come_from_irq_lines() -> None:
    chip, board = _g071()
    repl = emit_renode_platform(chip, board)
    assert "    [4-15] -> nvicInput7@[0-11]" in repl
    assert "    -> nvic@7" in repl

    # Move the fact: lines 4..15 now raise EXTI2_3. The emitted wiring must
    # follow, and nothing may still point at vector 7.
    moved = copy.deepcopy(chip)
    moved["peripherals"]["exti"]["irq_lines"] = [
        {"irq": "EXTI0_1", "first": 0, "last": 1},
        {"irq": "EXTI2_3", "first": 2, "last": 15},
    ]
    repl2 = emit_renode_platform(moved, board)
    assert "    [2-15] -> nvicInput6@[0-13]" in repl2
    assert "nvicInput7" not in repl2


@skip_no_devices
def test_a_single_line_vector_skips_the_or_gate() -> None:
    """One line on its own vector needs no CombinedInput — Renode only needs an
    OR gate where several lines share an NVIC input."""
    chip, board = _g071()
    solo = copy.deepcopy(chip)
    solo["peripherals"]["exti"]["irq_lines"] = [
        {"irq": "EXTI0_1", "first": 0, "last": 0},
        {"irq": "EXTI4_15", "first": 4, "last": 15},
    ]
    repl = emit_renode_platform(solo, board)
    assert "    0 -> nvic@5" in repl
    assert "nvicInput5" not in repl


@skip_no_devices
def test_without_irq_lines_there_is_no_exti_block() -> None:
    """EXTI is an ADDITION to the platform: data too thin to wire the vectors
    means no block, not a wrong one."""
    chip, board = _g071()
    thin = copy.deepcopy(chip)
    del thin["peripherals"]["exti"]["irq_lines"]
    repl = emit_renode_platform(thin, board)
    assert "STM32WBA_EXTI" not in repl
    assert "GPIOPort.STM32_GPIOPort" not in repl


@skip_no_devices
def test_a_chip_with_no_curated_exti_is_unaffected() -> None:
    """The F7 EXTI is a different IP with the port select in SYSCFG; no Renode
    model is claimed for it here, so that platform must be untouched."""
    chip = load_chip(DEVICES_ROOT, "st/stm32f722")
    repl = emit_renode_platform(chip, _board("nucleo_f722ze"))
    assert "STM32WBA_EXTI" not in repl
    assert "GPIOPort.STM32_GPIOPort" not in repl


@skip_no_devices
def test_the_exti_window_stops_before_its_neighbour() -> None:
    """A peripheral line with no window takes its size from the MODEL, and a
    model written for another die can claim far more than the real block.

    STM32WBA_EXTI claims at least 4K. Unbounded at the G0's 0x40021800 it
    swallowed the flash controller at 0x40022000, so every FLASH_SR/FLASH_CR
    access landed inside EXTI and was logged as an unhandled offset 0x810/0x814
    — the emulation looked noisy rather than wrong, which is the worse failure.
    """
    chip, board = _g071()
    platform = emit_renode_platform(chip, board)

    line = next(ln for ln in platform.splitlines() if ln.startswith("exti:"))
    assert "<" in line, f"exti must carry an explicit window, got: {line}"

    base, size = _window_of(line)
    flash = int(chip["peripherals"]["flash"]["base"], 16)
    assert base + size <= flash, (
        f"exti reaches {base + size:#x}, into the flash controller at {flash:#x}")
    # And it is the gap, not a constant: 0x40022000 - 0x40021800.
    assert size == flash - base


@skip_no_devices
def test_the_window_is_the_gap_not_a_hardcoded_number() -> None:
    """Move the neighbour and the window must move with it — the check that a
    constant would survive."""
    chip, board = _g071()
    moved = copy.deepcopy(chip)
    moved["peripherals"]["flash"]["base"] = "0x40021c00"   # 0x400 above exti

    line = next(ln for ln in emit_renode_platform(moved, board).splitlines()
                if ln.startswith("exti:"))
    _, size = _window_of(line)
    assert size == 0x400, f"window did not follow the neighbour: {size:#x}"


@skip_no_devices
def test_no_emitted_window_contains_another_peripherals_base() -> None:
    """The general invariant. Only windows can be checked — a bare line's size
    lives inside Renode — so this is a floor, not a proof, and every peripheral
    alloy bounds itself has to clear it."""
    chip, board = _g071()
    bases = {n: int(p["base"], 16)
             for n, p in chip["peripherals"].items() if p.get("base")}

    for line in emit_renode_platform(chip, board).splitlines():
        if "@ sysbus <" not in line:
            continue
        name = line.split(":", 1)[0].strip()
        base, size = _window_of(line)
        for other, addr in bases.items():
            if other != name and base < addr < base + size:
                pytest.fail(f"{name} <{base:#x}, +{size:#x}> contains "
                            f"{other} at {addr:#x}")


def _window_of(line: str) -> tuple[int, int]:
    """(base, size) from `name: Model @ sysbus <0xBASE, +0xSIZE>`."""
    inside = line.split("<", 1)[1].split(">", 1)[0]
    base, size = (part.strip().lstrip("+") for part in inside.split(","))
    return int(base, 16), int(size, 16)
