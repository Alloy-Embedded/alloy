"""bus.toml -> model -> generated header/manifest: the resolve and emit half.

The one promise that outranks the rest: the emitted binding shape is EXACTLY
the hand-written WireBinding shape, byteorder calls and all — codegen
replaces authorship, not shape. The line-level assertions here pin that;
examples/bus_bridge's Renode leg is the byte-on-the-wire witness.
"""

from __future__ import annotations

import pytest
from alloy_cli.bus import BUS_SCHEMA, MAX_BODY, message_issues, resolve
from alloy_cli.emit.bus import emit_bus_header, emit_bus_manifest
from alloy_cli.emit.common import EmitError

VALID = {
    "schema": BUS_SCHEMA,
    "messages": {
        "ping": {"id": 0x0301, "version": 1,
                 "fields": [{"name": "token", "type": "u32"}]},
        "pong": {"id": 0x0302,
                 "fields": [{"name": "token", "type": "u32"},
                            {"name": "count", "type": "u32"}]},
        "kitchen": {"id": 0x0101, "version": 3,
                    "fields": [{"name": "a", "type": "u8"},
                               {"name": "b", "type": "i8"},
                               {"name": "c", "type": "u16"},
                               {"name": "d", "type": "i16"},
                               {"name": "e", "type": "u32"},
                               {"name": "f", "type": "i32"},
                               {"name": "g", "type": "f32"},
                               {"name": "h", "type": "bool"}]},
        "legacy": {"id": 0x0300, "retired": True},
    },
}


def test_resolve_lays_out_fields_in_declared_order() -> None:
    model = resolve(VALID)
    kitchen = next(m for m in model["messages"] if m["name"] == "kitchen")
    assert kitchen["size"] == 19
    assert [f["offset"] for f in kitchen["fields"]] == [0, 1, 2, 4, 6, 10, 14, 18]
    assert kitchen["version"] == 3
    # version defaults to 1 when absent — ids never default, versions may.
    pong = next(m for m in model["messages"] if m["name"] == "pong")
    assert pong["version"] == 1


def test_resolve_orders_by_id_and_keeps_tombstones() -> None:
    model = resolve(VALID)
    assert [m["id"] for m in model["messages"]] == [0x0101, 0x0300, 0x0301, 0x0302]
    legacy = next(m for m in model["messages"] if m["name"] == "legacy")
    assert legacy["retired"] is True


def test_emitted_binding_is_the_hand_written_shape() -> None:
    """Byte-for-byte the shape examples wrote by hand in F1/F2."""
    text = emit_bus_header(resolve(VALID))
    for line in (
        "struct ping {\n    std::uint32_t token;\n};",
        "struct ping_wire {",
        "    using message = ping;",
        "    static constexpr std::uint16_t id = 0x0301;",
        "    static constexpr std::uint8_t ver = 1;",
        "    static constexpr std::size_t size = 4;",
        "        alloy::byteorder::store_le32(&out[0], m.token);",
        "        return {alloy::byteorder::load_le32(&in[0])};",
    ):
        assert line in text, f"missing emitted line: {line!r}"


def test_emitted_kitchen_covers_every_type() -> None:
    text = emit_bus_header(resolve(VALID))
    for line in (
        "        out[0] = m.a;",
        "        out[1] = static_cast<std::uint8_t>(m.b);",
        "        alloy::byteorder::store_le16(&out[2], m.c);",
        "        alloy::byteorder::store_le16(&out[4], "
        "static_cast<std::uint16_t>(m.d));",
        "        alloy::byteorder::store_le32(&out[6], m.e);",
        "        alloy::byteorder::store_le32(&out[10], "
        "static_cast<std::uint32_t>(m.f));",
        "        alloy::byteorder::store_le32(&out[14], "
        "std::bit_cast<std::uint32_t>(m.g));",
        "        out[18] = m.h ? 1 : 0;",
    ):
        assert line in text, f"missing emitted line: {line!r}"
    assert "#include <bit>" in text  # f32 pulls it in
    assert "in[18] != 0" in text     # bool decode


def test_retired_ids_are_documented_not_emitted() -> None:
    text = emit_bus_header(resolve(VALID))
    assert "//   0x0300  legacy" in text
    assert "struct legacy" not in text


def test_header_without_f32_skips_bit_include() -> None:
    data = {"schema": BUS_SCHEMA,
            "messages": {"m": {"id": 1, "fields": [{"name": "x", "type": "u8"}]}}}
    assert "#include <bit>" not in emit_bus_header(resolve(data))


def test_manifest_envelope() -> None:
    import json

    manifest = json.loads(emit_bus_manifest(resolve(VALID)))
    assert manifest["schema"] == "alloy.bus_manifest.v1"
    ping = next(m for m in manifest["messages"] if m["name"] == "ping")
    assert ping["id"] == 0x0301
    assert ping["fields"][0] == {"name": "token", "type": "u32",
                                 "size": 4, "offset": 0}


def test_empty_registry_is_legal() -> None:
    model = resolve({"schema": BUS_SCHEMA, "messages": {}})
    assert model["messages"] == []
    assert "namespace messages" in emit_bus_header(model)


@pytest.mark.parametrize("broken, needle", [
    ({"messages": {"m": {"fields": [{"name": "x", "type": "u8"}]}}},
     "never auto-assigned"),
    ({"messages": {"m": {"id": 0, "fields": [{"name": "x", "type": "u8"}]}}},
     "sentinel"),
    ({"messages": {"m": {"id": 0xFF01, "fields": [{"name": "x", "type": "u8"}]}}},
     "reserved for framework"),
    ({"messages": {"m": {"id": 0x1_0000, "fields": [{"name": "x", "type": "u8"}]}}},
     "u16"),
    ({"messages": {"a": {"id": 5, "fields": [{"name": "x", "type": "u8"}]},
                   "b": {"id": 5, "fields": [{"name": "x", "type": "u8"}]}}},
     "duplicate id"),
    ({"messages": {"m": {"id": 1, "retired": True,
                         "fields": [{"name": "x", "type": "u8"}]}}},
     "tombstone"),
    ({"messages": {"m": {"id": 1, "fields": [{"name": "x", "type": "u64"}]}}},
     "unknown type"),
    ({"messages": {"m": {"id": 1, "fields": [{"name": "x", "type": "u8"},
                                             {"name": "x", "type": "u8"}]}}},
     "duplicate field"),
    ({"messages": {"m": {"id": 1}}}, "declares its fields"),
    ({"messages": {"m": {"id": 1, "flavor": "spicy",
                         "fields": [{"name": "x", "type": "u8"}]}}},
     "unknown key"),
    ({"typo_section": {}, "messages": {}}, "unknown key"),
    ({"messages": {"m": {"id": 1, "version": 0,
                         "fields": [{"name": "x", "type": "u8"}]}}},
     "u8 >= 1"),
    ({"messages": {"m": {"id": 1,
                         "fields": [{"name": "x", "type": "u32"}] * 33}}},
     "slow-plane cap"),
])
def test_each_rule_refuses_and_names_the_problem(broken, needle) -> None:
    data = {"schema": BUS_SCHEMA, **broken}
    issues = message_issues(data)
    assert issues, f"oracle accepted: {broken}"
    assert any(needle in msg for _, msg in issues), (
        f"no issue mentions {needle!r}: {issues}")
    with pytest.raises(EmitError):
        resolve(data)


def test_body_cap_is_the_wire_cap() -> None:
    """MAX_BODY here and wire_max_body in bus/wire.hpp are one fact."""
    wire = (__import__("pathlib").Path(__file__).resolve().parents[3]
            / "libs" / "bus" / "include" / "bus" / "wire.hpp").read_text()
    assert f"wire_max_body = {MAX_BODY};" in wire
