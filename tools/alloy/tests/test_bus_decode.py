"""Host-side frame decode: the monitor's half of bus observability.

The frames here are built by an INDEPENDENT implementation — a bitwise
CRC-32 and a hand-assembled frame, the same construction the Renode leg's
monitor-Python peer uses — so decoder and oracle cannot share a bug. (The
decoder itself spells the CRC as zlib.crc32.)

The properties that matter are the stream ones: a monitor carries log text
AND frames on one wire, and it must never trade one for the other.
"""

from __future__ import annotations

from alloy_cli.bus import BUS_SCHEMA, resolve
from alloy_cli.bus_decode import format_message, frame_scanner

MODEL = resolve({
    "schema": BUS_SCHEMA,
    "messages": {
        "ping": {"id": 0x0301, "version": 1,
                 "fields": [{"name": "token", "type": "u32"}]},
        "reading": {"id": 0x0101, "version": 2,
                    "fields": [{"name": "centi_c", "type": "i16"},
                               {"name": "ok", "type": "bool"}]},
        "gone": {"id": 0x0100, "retired": True},
    },
})


def _crc32(data: bytes) -> int:
    """Independent bitwise CRC-32/ISO-HDLC — not zlib, on purpose."""
    c = 0xFFFFFFFF
    for b in data:
        c ^= b
        for _ in range(8):
            c = (c >> 1) ^ 0xEDB88320 if c & 1 else c >> 1
    return c ^ 0xFFFFFFFF


def frame(seq: int, msg_id: int, ver: int, body: bytes,
          type_byte: int = 0x01) -> bytes:
    payload = bytes([msg_id & 0xFF, msg_id >> 8, ver]) + body
    inner = bytes([type_byte, seq, len(payload) & 0xFF, len(payload) >> 8]) + payload
    return b"\x7e" + inner + _crc32(inner).to_bytes(4, "little")


PING = frame(3, 0x0301, 1, (0xCAFE0001).to_bytes(4, "little"))

#: THE CROSS-LANGUAGE ANCHOR. The same 16 bytes are pinned in
#: libs/bus/tests/test_bus_wire.cpp, where the FIRMWARE encoder produces
#: them. Firmware and host decoder cannot drift apart silently: change
#: either side's bytes and the other side's suite fails.
GOLDEN_PING = bytes.fromhex("7e010307000103010100fecab468ee71")


def test_the_golden_frame_matches_the_firmware_encoder() -> None:
    assert PING == GOLDEN_PING, "independent build disagrees with the C++ golden"
    _, msgs = frame_scanner(MODEL).feed(GOLDEN_PING)
    assert msgs == [{"id": 0x0301, "ver": 1, "seq": 3, "name": "ping",
                     "fields": {"token": 0xCAFE0001}}]


def test_decodes_a_frame_into_named_fields() -> None:
    text, msgs = frame_scanner(MODEL).feed(PING)
    assert text == b""
    assert msgs == [{"id": 0x0301, "ver": 1, "seq": 3, "name": "ping",
                     "fields": {"token": 0xCAFE0001}}]


def test_signed_and_bool_fields_round_trip() -> None:
    body = (-1234 & 0xFFFF).to_bytes(2, "little") + b"\x01"
    _, msgs = frame_scanner(MODEL).feed(frame(0, 0x0101, 2, body))
    assert msgs[0]["fields"] == {"centi_c": -1234, "ok": True}


def test_log_text_passes_through_untouched() -> None:
    text, msgs = frame_scanner(MODEL).feed(b"alloy bus_bridge ready\r\n")
    assert text == b"alloy bus_bridge ready\r\n"
    assert msgs == []


def test_text_and_frames_interleave_without_loss() -> None:
    scanner = frame_scanner(MODEL)
    text, msgs = scanner.feed(b"before\n" + PING + b"after\n")
    assert text == b"before\nafter\n"
    assert len(msgs) == 1 and msgs[0]["name"] == "ping"


def test_a_frame_split_across_reads_still_decodes() -> None:
    """The stream property: serial hands you arbitrary chunk boundaries."""
    scanner = frame_scanner(MODEL)
    seen: list[dict] = []
    text = bytearray()
    for byte in PING:  # one byte per read, the worst case
        t, msgs = scanner.feed(bytes([byte]))
        text.extend(t)
        seen.extend(msgs)
    assert bytes(text) == b""  # nothing leaked to the log while assembling
    assert len(seen) == 1 and seen[0]["fields"]["token"] == 0xCAFE0001


def test_a_bad_crc_is_text_not_a_claimed_corrupt_frame() -> None:
    """A monitor carrying text must not invent damaged datagrams: text with
    a 0x7E in it is common, and the device's own counters witness corruption."""
    corrupt = bytearray(PING)
    corrupt[8] ^= 0xFF
    text, msgs = frame_scanner(MODEL).feed(bytes(corrupt))
    assert msgs == []
    assert text == bytes(corrupt)  # every byte handed back, none swallowed


def test_a_tilde_in_a_log_line_does_not_eat_the_line() -> None:
    scanner = frame_scanner(MODEL)
    text, msgs = scanner.feed(b"path ~/alloy ok\n")
    assert msgs == []
    assert text + scanner.flush() == b"path ~/alloy ok\n"


def test_a_trailing_partial_frame_is_released_by_flush() -> None:
    scanner = frame_scanner(MODEL)
    text, msgs = scanner.feed(PING[:6])  # held: might still become a frame
    assert (text, msgs) == (b"", [])
    assert scanner.flush() == PING[:6]


def test_unknown_id_degrades_to_hex() -> None:
    _, msgs = frame_scanner(MODEL).feed(frame(9, 0x0999, 1, b"\xaa\xbb"))
    assert msgs[0] == {"id": 0x0999, "ver": 1, "seq": 9, "raw": "aabb"}


def test_a_retired_id_is_not_decoded_by_name() -> None:
    _, msgs = frame_scanner(MODEL).feed(frame(0, 0x0100, 1, b"\x01"))
    assert "name" not in msgs[0] and msgs[0]["raw"] == "01"


def test_version_and_layout_mismatches_say_what_the_manifest_expects() -> None:
    _, msgs = frame_scanner(MODEL).feed(frame(0, 0x0301, 7, b"\x01\x02\x03\x04"))
    assert msgs[0]["name"] == "ping" and "v1" in msgs[0]["note"]

    _, msgs = frame_scanner(MODEL).feed(frame(0, 0x0301, 1, b"\x01\x02"))
    assert msgs[0]["raw"] == "0102" and "4 B" in msgs[0]["note"]


def test_a_non_datagram_frame_type_is_left_as_text() -> None:
    """0x02+ is reserved; a future frame type must not decode as a datagram."""
    other = frame(0, 0x0301, 1, b"\x01\x02\x03\x04", type_byte=0x02)
    text, msgs = frame_scanner(MODEL).feed(other)
    assert msgs == [] and text == other


def test_scanner_without_a_manifest_still_finds_frames() -> None:
    """No bus.toml is not "no frames" — ids just have no names yet."""
    _, msgs = frame_scanner(None).feed(PING)
    assert msgs[0]["id"] == 0x0301 and msgs[0]["raw"] == "0100feca"


def test_human_line_shape() -> None:
    """`name=value` in decimal — the same shape the monitor panel already
    mines for sparklines, so a telemetry message charts itself."""
    body = (-1234 & 0xFFFF).to_bytes(2, "little") + b"\x01"
    _, msgs = frame_scanner(MODEL).feed(frame(4, 0x0101, 2, body))
    assert format_message(msgs[0]) == "[bus] reading  seq=4  centi_c=-1234  ok=true"

    _, msgs = frame_scanner(MODEL).feed(frame(1, 0x0999, 1, b"\xaa"))
    assert format_message(msgs[0]) == "[bus] 0x0999  seq=1  raw=aa"
