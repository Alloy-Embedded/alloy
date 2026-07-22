"""Emit the board role layer: alloy/board.hpp + board.cpp.

Roles are resolved against the chip data — the emitter carries ZERO
addresses, AF numbers or clock constants of its own (the old ecosystem's
emit_board.py died of exactly that). Unknown pins/peripherals/profiles fail
generation with a message naming the board.json entry.
"""

from __future__ import annotations

from typing import Any

from .common import BANNER, EmitError, field_lookup, register_by_name


def _require(cond: bool, msg: str) -> None:
    if not cond:
        raise EmitError(msg)


def _polarity(active: str) -> str:
    return "alloy::gpio::active_high_t" if active == "high" else "alloy::gpio::active_low_t"


def emit_board_header(board: dict[str, Any], chip: dict[str, Any]) -> str:
    roles = board.get("roles", {})
    profile_name = board["clock_profile"]
    profile = chip["clock"]["profiles"].get(profile_name)
    _require(profile is not None, f"board {board['id']}: clock_profile '{profile_name}' not in chip data")

    caps: dict[str, bool] = {"led": False, "button": False, "debug_uart": False}
    decls: list[str] = []

    led = roles.get("led")
    if led:
        _require(led["pin"] in chip.get("pins", {}),
                 f"board {board['id']}: led pin '{led['pin']}' not in chip data")
        caps["led"] = True
        decls.append(
            f"inline constexpr alloy::gpio::output<alloy::dev::{led['pin']}_t, "
            f"{_polarity(led.get('active', 'high'))}> led{{}};"
        )

    button = roles.get("button")
    if button:
        _require(button["pin"] in chip.get("pins", {}),
                 f"board {board['id']}: button pin '{button['pin']}' not in chip data")
        caps["button"] = True
        decls.append(
            f"inline constexpr alloy::gpio::input<alloy::dev::{button['pin']}_t, "
            f"{_polarity(button.get('active', 'high'))}> button{{}};"
        )

    uart = roles.get("debug_uart")
    if uart:
        for key in ("peripheral", "tx", "rx"):
            _require(key in uart, f"board {board['id']}: debug_uart missing '{key}'")
        _require(uart["peripheral"] in chip["peripherals"],
                 f"board {board['id']}: debug_uart peripheral '{uart['peripheral']}' not in chip data")
        caps["debug_uart"] = True
        decls.append(
            f"using debug_uart = alloy::uart::bind<alloy::dev::{uart['peripheral']}_t,\n"
            f"                                     alloy::uart::tx<alloy::dev::{uart['tx']}_t>,\n"
            f"                                     alloy::uart::rx<alloy::dev::{uart['rx']}_t>,\n"
            f"                                     clock_profile>;\n"
            f"inline constexpr std::uint32_t debug_uart_baud = {uart.get('baud', 115200)}u;"
        )
    else:
        decls.append(
            "// This board declares no debug UART; the stub keeps\n"
            "// `if constexpr (board::caps::debug_uart)` code compiling everywhere.\n"
            "struct debug_uart {\n"
            "    struct null_handle {\n"
            "        void write(std::uint8_t) const {}\n"
            "        void write(const char*) const {}\n"
            "        bool read(std::uint8_t&) const { return false; }\n"
            "        void flush() const {}\n"
            "    };\n"
            "    static null_handle open(alloy::uart::config) { return {}; }\n"
            "};\n"
            "inline constexpr std::uint32_t debug_uart_baud = 0u;"
        )

    caps_body = "\n".join(
        f"inline constexpr bool {name} = {'true' if value else 'false'};"
        for name, value in sorted(caps.items())
    )
    decl_body = "\n\n".join(decls)

    return f"""{BANNER}// Board: {board['id']} ({board.get('name', '')})
#pragma once

#include <cstdint>

#include "alloy/device.hpp"
#include "alloy/gpio.hpp"
#include "alloy/routes_gen.hpp"
#include "alloy/time.hpp"
#include "alloy/uart.hpp"

namespace board {{

struct clock_profile {{
    static constexpr std::uint32_t sysclk_hz = {profile['sysclk_hz']}u;
    static constexpr std::uint32_t ahb_hz = {profile['ahb_hz']}u;
    static constexpr std::uint32_t apb_hz = {profile['apb_hz']}u;
}};
inline constexpr std::uint32_t system_clock_hz = clock_profile::sysclk_hz;

namespace caps {{
{caps_body}
}}  // namespace caps

{decl_body}

// Clocks + timebase + role pins; returns false if the clock program timed
// out and the board is running on the boot clock instead.
bool init();

}}  // namespace board
"""


def _resolve_step(chip: dict[str, Any], registers: dict[str, dict[str, Any]],
                  op: dict[str, Any]) -> str:
    if op["op"] == "delay":
        return (f"    alloy::clock_step{{alloy::clock_step::op::delay, 0u, 0u, "
                f"{op['us']}u, 0u}},")

    periph = chip["peripherals"][op["peripheral"]]
    ip_doc = registers[periph["ip"]]
    reg = register_by_name(ip_doc, op["register"])
    addr = int(periph["base"], 16) + int(reg["offset"], 16)

    if op["op"] == "write":
        return (f"    alloy::clock_step{{alloy::clock_step::op::write, 0x{addr:08X}u, "
                f"0xFFFFFFFFu, {op['value']}u, 0u}},")
    if op["op"] == "rmw":
        mask = 0
        value = 0
        for fname, fval in op["fields"].items():
            bit, width = field_lookup(reg, fname)
            fmask = ((1 << width) - 1) << bit
            mask |= fmask
            value |= (fval << bit) & fmask
        return (f"    alloy::clock_step{{alloy::clock_step::op::rmw, 0x{addr:08X}u, "
                f"0x{mask:08X}u, 0x{value:08X}u, 0u}},")
    if op["op"] == "poll":
        bit, width = field_lookup(reg, op["field"])
        mask = ((1 << width) - 1) << bit
        value = (op["equals"] << bit) & mask
        return (f"    alloy::clock_step{{alloy::clock_step::op::poll, 0x{addr:08X}u, "
                f"0x{mask:08X}u, 0x{value:08X}u, {op['timeout_us']}u}},")
    raise EmitError(f"unknown clock op {op['op']}")


def emit_board_source(board: dict[str, Any], chip: dict[str, Any],
                      registers: dict[str, dict[str, Any]], arch_ns: str) -> str:
    roles = board.get("roles", {})
    profile = chip["clock"]["profiles"][board["clock_profile"]]
    boot_hz = chip["clock"]["sources"][chip["clock"]["boot_source"]]["hz"]

    steps = "\n".join(_resolve_step(chip, registers, op) for op in profile["program"])

    role_init: list[str] = []
    if "led" in roles:
        role_init.append("    led.init();\n    led.off();")
    if "button" in roles:
        if roles["button"].get("pull") == "up":
            role_init.append("    button.init_pullup();")
        else:
            role_init.append("    button.init();")

    role_block = "\n".join(role_init)
    return f"""{BANNER}// Board: {board['id']} — role + clock bring-up
#include "alloy/board.hpp"

#include "alloy/arch/{arch_ns}/systick.hpp"
#include "alloy/hal/clock_program.hpp"

namespace board {{
namespace {{

// Clock profile '{board['clock_profile']}' resolved from chip data.
constexpr alloy::clock_step kClockProgram[] = {{
{steps}
}};

}}  // namespace

bool init() {{
    const bool clock_ok = alloy::hal::run_clock_program(kClockProgram);
    // SysTick counts the CPU clock (sysclk). On failure the chip is still on
    // its boot clock; keep the timebase honest.
    const std::uint32_t core_hz = clock_ok ? clock_profile::sysclk_hz : {boot_hz}u;
    alloy::arch::{arch_ns}::systick_init(core_hz);
{role_block}
    alloy::arch::{arch_ns}::enable_irq();
    return clock_ok;
}}

}}  // namespace board
"""
