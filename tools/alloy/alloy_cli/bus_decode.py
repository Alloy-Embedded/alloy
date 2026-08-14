"""Host-side decode of libs/bus datagrams sniffed on a serial link.

The device side of observability needed no new code: a `bridge_route` on the
debug uart already taps whichever topics a project chooses to forward. What
was missing is this half — turning those bytes back into named messages
with named fields, against the same `bus.toml` the firmware compiled from.

The decode lives HERE, in the CLI, not in the IDE: the extension's first
guardrail is that no domain logic lives in TypeScript, so the panel is
handed decoded messages and renders them. The same decode serves the
interactive monitor, so a human running `alloy monitor` sees

    [bus] reading  seq=3  centi_c=2543  ok=true

where they used to see binary confetti between their log lines.

Mirrors bus/wire.hpp exactly (SOF 0x7E, type, seq, len u16 LE, payload,
crc32 LE over type..payload; payload = msg_id u16 LE, ver u8, body). The
CRC is CRC-32/ISO-HDLC — spelled here as zlib.crc32, which is bit-for-bit
the firmware's bytewise implementation (the OTA host tooling leans on the
same identity).

ONE deliberate refusal: a candidate frame whose CRC does not check is
handed BACK to the text stream, not reported as a corrupt frame. A monitor
carries log text as well as frames, and text containing 0x7E ('~') is
common — claiming those as damaged datagrams would invent evidence. The
device's own rx counters (bad_frames/lost) are where corruption is
witnessed, because that side knows a frame was meant.
"""

from __future__ import annotations

import struct
import zlib
from typing import Any

SOF = 0x7E
TYPE_DATAGRAM = 0x01
FRAME_OVERHEAD = 9  # SOF + type + seq + len16 + crc32
MSG_HEADER = 3      # msg_id u16 LE + ver u8
MAX_PAYLOAD = MSG_HEADER + 128  # wire_max_payload
MAX_FRAME = FRAME_OVERHEAD + MAX_PAYLOAD

_FMT = {"u8": "<B", "i8": "<b", "u16": "<H", "i16": "<h",
        "u32": "<I", "i32": "<i", "f32": "<f", "bool": "<?"}


def decode_body(msg: dict[str, Any], body: bytes) -> dict[str, Any] | None:
    """Body -> {field: value}, or None when the body does not match the
    declared layout (a peer built from a different bus.toml)."""
    if len(body) != msg["size"]:
        return None
    out: dict[str, Any] = {}
    for f in msg["fields"]:
        out[f["name"]] = struct.unpack_from(_FMT[f["type"]], body, f["offset"])[0]
    return out


class frame_scanner:
    """Split a byte stream into log text and decoded bus messages.

    feed() returns (text_bytes, messages). Bytes that are not part of a
    complete, CRC-valid frame come back as text, so a monitor never loses a
    log line to a false frame. A partial frame at the end of a chunk is held
    for the next one; flush() releases whatever is still held at close.
    """

    def __init__(self, model: dict[str, Any] | None = None) -> None:
        by_id: dict[int, dict[str, Any]] = {}
        for m in (model or {}).get("messages", []):
            if not m.get("retired"):
                by_id[m["id"]] = m
        self._by_id = by_id
        self._buf = bytearray()

    def feed(self, data: bytes) -> tuple[bytes, list[dict[str, Any]]]:
        self._buf.extend(data)
        buf = self._buf
        text = bytearray()
        msgs: list[dict[str, Any]] = []
        i = 0
        n = len(buf)
        while i < n:
            if buf[i] != SOF:
                text.append(buf[i])
                i += 1
                continue
            if n - i < FRAME_OVERHEAD:
                break  # too short to judge yet — hold it
            length = buf[i + 3] | (buf[i + 4] << 8)
            if length > MAX_PAYLOAD:
                text.append(buf[i])  # cannot be one of ours
                i += 1
                continue
            total = FRAME_OVERHEAD + length
            if n - i < total:
                break  # incomplete — hold for the next chunk
            frame = bytes(buf[i:i + total])
            want = int.from_bytes(frame[5 + length:9 + length], "little")
            if zlib.crc32(frame[1:5 + length]) != want:
                text.append(buf[i])  # not a frame (or damaged) — it is text
                i += 1
                continue
            decoded = self._decode(frame, length)
            if decoded is None:
                text.append(buf[i])
                i += 1
                continue
            msgs.append(decoded)
            i += total
        del buf[:i]
        return bytes(text), msgs

    def flush(self) -> bytes:
        held = bytes(self._buf)
        self._buf.clear()
        return held

    def _decode(self, frame: bytes, length: int) -> dict[str, Any] | None:
        if frame[1] != TYPE_DATAGRAM or length < MSG_HEADER:
            return None
        payload = frame[5:5 + length]
        msg_id = payload[0] | (payload[1] << 8)
        ver = payload[2]
        body = payload[MSG_HEADER:]
        out: dict[str, Any] = {"id": msg_id, "ver": ver, "seq": frame[2]}
        known = self._by_id.get(msg_id)
        # An id the manifest does not name, or a version/layout it does not
        # match, degrades to hex — a monitor that refused would go blank
        # exactly when the two ends disagree, which is when you need it.
        if known is None:
            out["raw"] = body.hex()
            return out
        out["name"] = known["name"]
        if ver != known["version"]:
            out["raw"] = body.hex()
            out["note"] = f"manifest declares v{known['version']}"
            return out
        fields = decode_body(known, body)
        if fields is None:
            out["raw"] = body.hex()
            out["note"] = f"manifest declares {known['size']} B body"
            return out
        out["fields"] = fields
        return out


def format_message(m: dict[str, Any]) -> str:
    """One human line for the interactive monitor.

    Fields are rendered as `name=value` in DECIMAL — the same shape the
    monitor panel already mines for sparklines, so a telemetry message
    charts itself with no extra plumbing. (Hex would read nicer for a token
    and worse for everything the bus actually carries.)
    """
    head = m.get("name") or f"0x{m['id']:04X}"
    parts = [f"[bus] {head}", f"seq={m['seq']}"]
    if "fields" in m:
        parts += [f"{k}={_fmt_value(v)}" for k, v in m["fields"].items()]
    else:
        parts.append(f"raw={m.get('raw', '')}")
    if "note" in m:
        parts.append(f"({m['note']})")
    return "  ".join(parts)


def _fmt_value(v: Any) -> str:
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, float):
        return f"{v:g}"
    return str(v)
