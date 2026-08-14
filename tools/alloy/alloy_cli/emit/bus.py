"""Emit the bus wire contract: alloy/bus_messages.hpp + the manifest envelope.

Pure functions over the resolved model from bus.resolve() — the emitter
carries no policy (validate-before-emit already decided what is legal).

THE LOAD-BEARING PROMISE: the emitted binding is byte-for-byte the shape a
hand-written WireBinding (libs/bus bus/wire.hpp) has — same members, same
byteorder calls, same field order. Codegen replaces AUTHORSHIP, not shape,
so migrating a hand binding into bus.toml keeps every frame on the wire
byte-identical; examples/bus_bridge's Renode leg is the witness.

The generated header is deliberately standalone: it includes alloy's
byteorder (and <bit> for f32) but NOT libs/bus — the structs are plain data
usable anywhere, and the WireBinding concept is checked where a binding is
consumed (bridge_route<B>), not where it is declared.
"""

from __future__ import annotations

import json
from typing import Any

from .common import BANNER

_CPP_TYPE = {
    "u8": "std::uint8_t", "i8": "std::int8_t",
    "u16": "std::uint16_t", "i16": "std::int16_t",
    "u32": "std::uint32_t", "i32": "std::int32_t",
    "f32": "float", "bool": "bool",
}


def _encode_line(f: dict[str, Any]) -> str:
    o, n, t = f["offset"], f["name"], f["type"]
    if t == "u8":
        return f"        out[{o}] = m.{n};\n"
    if t == "i8":
        return f"        out[{o}] = static_cast<std::uint8_t>(m.{n});\n"
    if t == "u16":
        return f"        alloy::byteorder::store_le16(&out[{o}], m.{n});\n"
    if t == "i16":
        return (f"        alloy::byteorder::store_le16(&out[{o}], "
                f"static_cast<std::uint16_t>(m.{n}));\n")
    if t == "u32":
        return f"        alloy::byteorder::store_le32(&out[{o}], m.{n});\n"
    if t == "i32":
        return (f"        alloy::byteorder::store_le32(&out[{o}], "
                f"static_cast<std::uint32_t>(m.{n}));\n")
    if t == "f32":
        return (f"        alloy::byteorder::store_le32(&out[{o}], "
                f"std::bit_cast<std::uint32_t>(m.{n}));\n")
    return f"        out[{o}] = m.{n} ? 1 : 0;\n"  # bool


def _decode_expr(f: dict[str, Any]) -> str:
    o, t = f["offset"], f["type"]
    if t == "u8":
        return f"in[{o}]"
    if t == "i8":
        return f"static_cast<std::int8_t>(in[{o}])"
    if t == "u16":
        return f"alloy::byteorder::load_le16(&in[{o}])"
    if t == "i16":
        return f"static_cast<std::int16_t>(alloy::byteorder::load_le16(&in[{o}]))"
    if t == "u32":
        return f"alloy::byteorder::load_le32(&in[{o}])"
    if t == "i32":
        return f"static_cast<std::int32_t>(alloy::byteorder::load_le32(&in[{o}]))"
    if t == "f32":
        return f"std::bit_cast<float>(alloy::byteorder::load_le32(&in[{o}]))"
    return f"in[{o}] != 0"  # bool


def emit_bus_header(model: dict[str, Any]) -> str:
    """gen/alloy/bus_messages.hpp — structs + WireBinding-shaped bindings."""
    messages = model["messages"]
    active = [m for m in messages if not m["retired"]]
    retired = [m for m in messages if m["retired"]]

    out: list[str] = [BANNER, "#pragma once\n\n"]
    if any(f["type"] == "f32" for m in active for f in m["fields"]):
        out.append("#include <bit>\n")
    out.append("#include <cstddef>\n#include <cstdint>\n\n")
    out.append('#include "alloy/util/byteorder.hpp"\n\n')
    out.append("namespace messages {\n\n")

    if retired:
        out.append("// Retired ids — reserved forever, layouts gone. Reusing one would\n"
                   "// re-point whatever in the field still speaks it.\n")
        for m in retired:
            out.append(f"//   0x{m['id']:04X}  {m['name']}\n")
        out.append("\n")

    for m in active:
        out.append(f"struct {m['name']} {{\n")
        for f in m["fields"]:
            out.append(f"    {_CPP_TYPE[f['type']]} {f['name']};\n")
        out.append("};\n\n")

        out.append(f"struct {m['name']}_wire {{\n")
        out.append(f"    using message = {m['name']};\n")
        out.append(f"    static constexpr std::uint16_t id = 0x{m['id']:04X};\n")
        out.append(f"    static constexpr std::uint8_t ver = {m['version']};\n")
        out.append(f"    static constexpr std::size_t size = {m['size']};\n")
        out.append(f"    static void encode(const {m['name']}& m, "
                   "std::uint8_t* out) noexcept {\n")
        for f in m["fields"]:
            out.append(_encode_line(f))
        out.append("    }\n")
        out.append(f"    static {m['name']} decode(const std::uint8_t* in) noexcept {{\n")
        out.append("        return {")
        out.append(", ".join(_decode_expr(f) for f in m["fields"]))
        out.append("};\n    }\n};\n\n")

    out.append("}  // namespace messages\n")
    return "".join(out)


def emit_bus_manifest(model: dict[str, Any]) -> str:
    """The alloy.bus_manifest.v1 envelope — what the IDE's monitor decode and
    `alloy bus manifest` hand to machines. Same model, JSON shape."""
    return json.dumps({"schema": "alloy.bus_manifest.v1",
                       "messages": model["messages"]}, indent=2)
