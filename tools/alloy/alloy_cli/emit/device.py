"""Emit per-chip instance descriptors (alloy/device.hpp) and the typed route
table (alloy/routes_gen.hpp).

Every silicon fact here — bases, gate addresses, IRQ numbers, AF numbers —
is resolved from the database at generation time. Wrong data fails
GENERATION (or an offsetof/static_assert), never a quiet runtime.
"""

from __future__ import annotations

from typing import Any

from .common import (
    BANNER,
    CLOCK_NODES,
    SIGNALS,
    EmitError,
    cpp_ip_namespace,
    hex32,
    register_by_name,
)

_KIND_CPP = {
    "af_fixed": "alloy::routes::kind::af_fixed",
    "funcsel": "alloy::routes::kind::funcsel",
    "full_matrix": "alloy::routes::kind::full_matrix",
    "psel": "alloy::routes::kind::psel",
}


def _gate_args(chip: dict[str, Any], registers: dict[str, dict[str, Any]],
               periph_name: str, periph: dict[str, Any]) -> str | None:
    gate = periph.get("gate")
    if gate is None:
        return None
    owner = chip["peripherals"].get(gate["peripheral"])
    if owner is None:
        raise EmitError(f"{periph_name}: gate peripheral {gate['peripheral']} missing")
    ip_doc = registers[owner["ip"]]
    reg = register_by_name(ip_doc, gate["register"])
    addr = int(owner["base"], 16) + int(reg["offset"], 16)
    args = f"{hex32(addr)}, 1u << {gate['bit']}u"
    if gate.get("style", "rmw") == "write_set":
        args += ", alloy::clock_gate::style::write_set"
    return args


def emit_device_header(chip: dict[str, Any], registers: dict[str, dict[str, Any]],
                       driver_includes: list[str]) -> str:
    ips_used = sorted({p["ip"] for p in chip["peripherals"].values()})
    includes = "\n".join(
        f'#include "alloy/ip/{vendor}/{ip}.hpp"'
        for vendor, ip in (cpp_ip_namespace(k) for k in ips_used)
    )
    if driver_includes:
        includes += "\n\n// HAL drivers matching this chip's IP versions (data-driven selection).\n"
        includes += "\n".join(f'#include "{inc}"' for inc in sorted(driver_includes))

    irq_numbers = {i["name"]: i["number"] for i in chip["interrupts"]}

    blocks: list[str] = []
    for name in sorted(chip["peripherals"]):
        periph = chip["peripherals"][name]
        vendor, ip = cpp_ip_namespace(periph["ip"])
        lines = [
            f"struct {name}_t {{",
            f"    using ip = alloy::ip::{vendor}::{ip};",
            f"    static constexpr std::uintptr_t base = {hex32(int(periph['base'], 16))};",
        ]
        gate_args = _gate_args(chip, registers, name, periph)
        if gate_args:
            lines.append(f"    static constexpr alloy::clock_gate gate{{{gate_args}}};")
        if "irq" in periph:
            lines.append(
                f"    static constexpr alloy::irq_line irq{{{irq_numbers[periph['irq']]}}};"
            )
        if "kernel_clock" in periph:
            node = periph["kernel_clock"]
            if node not in CLOCK_NODES:
                raise EmitError(f"{name}: kernel_clock '{node}' not representable (skeleton supports {sorted(CLOCK_NODES)})")
            lines.append(f"    static constexpr alloy::clock_node kernel = alloy::clock_node::{node};")
        lines.append("};")
        lines.append(f"inline constexpr {name}_t {name}{{}};")
        blocks.append("\n".join(lines))

    pin_blocks: list[str] = []
    for pname in sorted(chip.get("pins", {})):
        pin = chip["pins"][pname]
        port_periph = f"gpio{pin['port']}"
        if port_periph not in chip["peripherals"]:
            raise EmitError(f"pin {pname}: no peripheral {port_periph} in chip data")
        pin_blocks.append(
            f"struct {pname}_t {{\n"
            f"    using port_t = {port_periph}_t;\n"
            f"    static constexpr unsigned index = {pin['index']}u;\n"
            f"}};"
        )

    body = "\n\n".join(blocks + pin_blocks)
    return f"""{BANNER}// Chip: {chip['vendor']}/{chip['part']} (alloy.chip.v1)
#pragma once

#include <cstdint>

#include "alloy/core/types.hpp"
{includes}

namespace alloy::dev {{

{body}

}}  // namespace alloy::dev
"""


def emit_routes_header(chip: dict[str, Any]) -> str:
    specs: list[str] = []
    for route in sorted(
        chip.get("routes", []),
        key=lambda r: (r["pin"], r["peripheral"], r["signal"]),
    ):
        if route["signal"] not in SIGNALS:
            raise EmitError(f"route {route['pin']}->{route['peripheral']}: unknown signal '{route['signal']}'")
        payload = [f"    static constexpr alloy::routes::kind k = {_KIND_CPP[route['kind']]};"]
        if route["kind"] == "af_fixed":
            payload.append(f"    static constexpr std::uint8_t af = {route['af']}u;")
        elif route["kind"] == "funcsel":
            payload.append(f"    static constexpr std::uint8_t funcsel = {route['funcsel']}u;")
        elif route["kind"] == "full_matrix":
            payload.append(f"    static constexpr std::uint16_t matrix_signal = {route['matrix_signal']}u;")
        specs.append(
            "template <>\n"
            f"struct route<alloy::dev::{route['pin']}_t, alloy::dev::{route['peripheral']}_t, "
            f"alloy::signal::{route['signal']}> {{\n" + "\n".join(payload) + "\n};"
        )

    body = "\n\n".join(specs) if specs else "// (chip declares no routes)"
    return f"""{BANNER}// Route table: {chip['vendor']}/{chip['part']}
#pragma once

#include <cstdint>

#include "alloy/core/routes.hpp"
#include "alloy/device.hpp"

namespace alloy::routes {{

{body}

}}  // namespace alloy::routes
"""
