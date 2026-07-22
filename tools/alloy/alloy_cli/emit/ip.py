"""Emit one C++ header per peripheral IP version.

Output shape (consumed by hand-written drivers via the IP tag type):

    namespace alloy::ip::st {
    struct gpio_v2 {
        struct regs { rw32 MODER; ... };
        template <unsigned I> requires (I < 16) static constexpr auto moder = ...;
        static constexpr auto pllon = alloy::field<&regs::CR, 24u, 1>;
    };
    }

Layout is self-verified with offsetof static_asserts so a data error can
never silently ship a wrong overlay.
"""

from __future__ import annotations

from typing import Any

from .common import BANNER, EmitError

_ACCESS_TYPE = {"rw": "rw32", "ro": "ro32", "wo": "wo32"}


def emit_ip_header(doc: dict[str, Any]) -> str:
    vendor, ip = doc["vendor"], doc["ip"]
    regs = sorted(doc["registers"], key=lambda r: int(r["offset"], 16))

    members: list[str] = []
    asserts: list[str] = []
    array_consts: list[str] = []
    # Register arrays live entirely outside the struct (accessed via
    # alloy::reg_at with offset/stride/count constants) — interleaved
    # arrays (LEDC channel banks) cannot be expressed as struct members.
    for reg in regs:
        arr = reg.get("array")
        if arr:
            array_consts.append(
                f"    static constexpr std::uintptr_t {reg['name']}_offset = {reg['offset']};\n"
                f"    static constexpr unsigned {reg['name']}_stride = {arr['stride']}u;\n"
                f"    static constexpr unsigned {reg['name']}_count = {arr['count']}u;"
            )

    cursor = 0
    pad = 0
    for reg in regs:
        if reg.get("array"):
            continue
        offset = int(reg["offset"], 16)
        if reg.get("size", 32) != 32:
            raise EmitError(f"{vendor}/{ip}: only 32-bit registers supported yet ({reg['name']})")
        if offset < cursor:
            raise EmitError(f"{vendor}/{ip}: register {reg['name']} overlaps previous register")
        if offset > cursor:
            gap = offset - cursor
            if gap % 4 != 0:
                raise EmitError(f"{vendor}/{ip}: unaligned gap before {reg['name']}")
            members.append(f"        std::uint32_t _reserved{pad}[{gap // 4}];")
            pad += 1
        members.append(f"        {_ACCESS_TYPE[reg['access']]} {reg['name']};")
        asserts.append(
            f"    static_assert(offsetof(regs, {reg['name']}) == {reg['offset']});"
        )
        cursor = offset + 4

    accessors: list[str] = []
    seen: dict[str, str] = {}
    for reg in regs:
        for f in reg.get("fields", []):
            name = f["name"].lower()
            if name in seen:
                raise EmitError(
                    f"{vendor}/{ip}: field accessor '{name}' from {reg['name']} collides with "
                    f"{seen[name]} — rename one field in the data (accessor names are per-IP)"
                )
            seen[name] = reg["name"]
            width = f.get("width", 1)
            rep = f.get("repeat")
            if reg.get("array"):
                # Array registers have no struct member — fields become
                # raw_field constants used with alloy::reg_at.
                if rep:
                    raise EmitError(f"{vendor}/{ip}: repeat fields inside array register {reg['name']} unsupported")
                accessors.append(
                    f"    static constexpr alloy::raw_field {name}{{{f['bit']}u, {width}u}};"
                )
            elif rep:
                accessors.append(
                    f"    template <unsigned I>\n"
                    f"        requires (I < {rep['count']}u)\n"
                    f"    static constexpr auto {name} =\n"
                    f"        alloy::field<&regs::{reg['name']}, {f['bit']}u + I * {rep['stride']}u, {width}>;"
                )
            else:
                accessors.append(
                    f"    static constexpr auto {name} = "
                    f"alloy::field<&regs::{reg['name']}, {f['bit']}u, {width}>;"
                )

    body = "\n".join(members)
    assert_block = "\n".join(asserts)
    accessor_block = "\n\n".join(array_consts + accessors)
    return f"""{BANNER}// IP: {vendor}/{ip} (alloy.registers.v1)
#pragma once

#include <cstddef>
#include <cstdint>

#include "alloy/core/mmio.hpp"

namespace alloy::ip::{vendor} {{

struct {ip} {{
    struct regs {{
{body}
    }};

{assert_block}

{accessor_block}
}};

}}  // namespace alloy::ip::{vendor}
"""
