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

import re

from typing import Any

from .common import BANNER, EmitError

#: (access, width in bits) -> the volatile alias in alloy/core/mmio.hpp.
#: Width is a silicon fact: RL78, AVR and MSP430 declare 8- and 16-bit SFRs,
#: several of them explicitly refusing wider access, and a 32-bit read-modify-
#: write on an 8-bit register carries its three neighbours with it.
_ACCESS_TYPE = {
    ("rw", 32): "rw32", ("ro", 32): "ro32", ("wo", 32): "wo32",
    ("rw", 16): "rw16", ("ro", 16): "ro16", ("wo", 16): "wo16",
    ("rw", 8): "rw8", ("ro", 8): "ro8", ("wo", 8): "wo8",
}


def _layout(vendor: str, ip: str, regs: list[dict[str, Any]],
            sname: str) -> tuple[list[str], list[str]]:
    """The member list and offsetof asserts for one address window."""
    members: list[str] = []
    asserts: list[str] = []
    cursor = 0
    pad = 0
    # Offsets are relative to the window's OWN base, always — the struct starts
    # where the chip data says the window starts, not at its first register.
    # Rebasing to the first register looked tidier and moved every address: a
    # block whose first register is at 0xC0 lost the 192 bytes of padding that
    # put it there.
    for reg in regs:
        offset = int(reg["offset"], 16)
        size = int(reg.get("size", 32))
        if (reg["access"], size) not in _ACCESS_TYPE:
            raise EmitError(f"{vendor}/{ip}: {reg['name']} is {size}-bit {reg['access']}; "
                            f"registers may be 8, 16 or 32 bits")
        width = size // 8
        if offset % width:
            raise EmitError(f"{vendor}/{ip}: {reg['name']} at {reg['offset']} is not "
                            f"{width}-byte aligned, which its width requires")
        if offset < cursor:
            raise EmitError(f"{vendor}/{ip}: register {reg['name']} overlaps previous register")
        if offset > cursor:
            members.append(f"        std::uint8_t _reserved{pad}[{offset - cursor}];")
            pad += 1
        members.append(f"        {_ACCESS_TYPE[(reg['access'], size)]} {reg['name']};")
        asserts.append(
            f"    static_assert(offsetof({sname}, {reg['name']}) == {reg['offset']});"
        )
        cursor = offset + width
    return members, asserts


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

    # A peripheral is not always one contiguous block. RL78 splits several of
    # them across the SFR area and the 2nd SFR area, ~64 KB apart: one ADC has
    # ADM0 at 0xFFF30 and ADM2 at 0xF0010. Laid out as a single struct that is a
    # 65 KB object on a part with 4 KB of RAM, so each WINDOW gets its own
    # struct and the chip data binds a base to each. Registers with no window
    # named land in `regs`, exactly as before.
    windows: dict[str, list[dict[str, Any]]] = {}
    for reg in regs:
        if reg.get("array"):
            continue
        windows.setdefault(reg.get("window") or "", []).append(reg)

    struct_of: dict[str, str] = {}
    structs: list[str] = []
    assert_groups: list[str] = []
    if not windows:
        # An IP that is entirely register ARRAYS still declares the type: it is
        # named by drivers and by sizeof, and dropping it is a silent API break.
        structs.append("    struct regs {\n\n    };")
    for window, wregs in sorted(windows.items()):
        sname = "regs" if not window else f"regs_{window}"
        if window and not re.fullmatch(r"[a-z][a-z0-9_]*", window):
            raise EmitError(f"{vendor}/{ip}: window name '{window}' must be a "
                            f"lowercase identifier — it becomes a struct name")
        members, asserts = _layout(vendor, ip, wregs, sname)
        for reg in wregs:
            struct_of[reg["name"]] = sname
        structs.append("    struct " + sname + " {\n" + "\n".join(members) + "\n    };")
        assert_groups.extend(asserts)

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
                # …and their enumerated VALUES, which until now had nowhere to
                # go: the flags-enum block below skips array registers (there
                # is no struct member to name), so a `values:` map on an array
                # field was accepted by the schema and silently dropped, and
                # every driver spelled the encoding as a bare integer. Emitted
                # UNSHIFTED, because `raw_field::write` does the shifting —
                # the shifted spelling of the same fact would be a second,
                # disagreeable one.
                for vname, val in (f.get("values") or {}).items():
                    cname = f"{name}_{vname.lower()}"
                    if cname in seen:
                        raise EmitError(
                            f"{vendor}/{ip}: value constant '{cname}' from {reg['name']} "
                            f"collides with {seen[cname]} — rename one in the data")
                    seen[cname] = reg["name"]
                    accessors.append(
                        f"    static constexpr std::uint32_t {cname} = {val}u;")
            elif rep:
                accessors.append(
                    f"    template <unsigned I>\n"
                    f"        requires (I < {rep['count']}u)\n"
                    f"    static constexpr auto {name} =\n"
                    f"        alloy::field<&{struct_of[reg['name']]}::{reg['name']}, "
                    f"{f['bit']}u + I * {rep['stride']}u, {width}>;"
                )
            else:
                accessors.append(
                    f"    static constexpr auto {name} = "
                    f"alloy::field<&{struct_of[reg['name']]}::{reg['name']}, "
                    f"{f['bit']}u, {width}>;"
                )

    # Typed register-flags enums: one scoped enum per register, single-bit
    # fields as their bit and multi-bit fields' named `values` as shifted
    # constants. Drivers write `r().CR = cr::rxen | cr::txen;` (see mmio flags).
    enum_blocks: list[str] = []
    flag_specializations: list[str] = []
    for reg in regs:
        if reg.get("array"):
            continue
        enum_members: list[str] = []
        for f in reg.get("fields", []):
            fname = f["name"].lower()
            bit = f["bit"]
            if f.get("width", 1) == 1:
                enum_members.append(f"        {fname} = std::uint32_t{{1}} << {bit}u,")
            for vname, val in (f.get("values") or {}).items():
                enum_members.append(f"        {fname}_{vname.lower()} = {val}u << {bit}u,")
        if not enum_members:
            continue
        ename = reg["name"].lower()
        if ename in seen:
            raise EmitError(
                f"{vendor}/{ip}: register flags enum '{ename}' collides with field "
                f"accessor '{ename}' — rename the field in the data")
        enum_blocks.append(
            f"    enum class {ename} : std::uint32_t {{\n"
            + "\n".join(enum_members)
            + "\n    };")
        flag_specializations.append(
            f"template <>\ninline constexpr bool "
            f"reg_flags_enabled<ip::{vendor}::{ip}::{ename}> = true;")

    body = "\n\n".join(structs)
    assert_block = "\n".join(assert_groups)
    accessor_block = "\n\n".join(array_consts + accessors)
    enum_block = ("\n\n" + "\n\n".join(enum_blocks)) if enum_blocks else ""
    flags_block = (
        "\nnamespace alloy {\n" + "\n".join(flag_specializations) + "\n}  // namespace alloy\n"
        if flag_specializations else "")
    return f"""{BANNER}// IP: {vendor}/{ip} (alloy.registers.v1)
#pragma once

#include <cstddef>
#include <cstdint>

#include "alloy/core/mmio.hpp"

namespace alloy::ip::{vendor} {{

struct {ip} {{
{body}

{assert_block}

{accessor_block}{enum_block}
}};

}}  // namespace alloy::ip::{vendor}
{flags_block}"""
