#!/usr/bin/env python3
"""
modbus_discovery_client.py — Alloy FC 0x65 discovery probe.

Sends a vendor discovery request (FC 0x65, sub-function 0x01 or 0x02) to a
Modbus RTU slave and decodes the variable table response.

Usage:
    python modbus_discovery_client.py --port /dev/ttyUSB0 --baud 115200
    python modbus_discovery_client.py --port /dev/ttyUSB0 --sub rich
    python modbus_discovery_client.py --port /dev/ttyUSB0 --fc 0x66 --sub thin

Output (thin):
    Addr  Regs  Type    Access     Name
    0x00  2     Float   ReadOnly   temperature
    0x02  1     Uint16  ReadWrite  status_word
    ...

Output (rich, adds unit, description, range):
    0x00  2  Float  ReadOnly  temperature  degC  ambient temperature  -40.0..125.0

Requires:
    pip install pyserial
    (no pymodbus needed — raw RTU frame built here)

Protocol reference: alloy/docs/MODBUS.md — Discovery FC reference
"""

import argparse
import struct
import sys
import time

try:
    import serial
except ImportError:
    print("error: pyserial not installed — run: pip install pyserial", file=sys.stderr)
    sys.exit(1)


# ============================================================================
# Constants (must match discovery.hpp)
# ============================================================================

FC_DISCOVERY_DEFAULT = 0x65
SUB_THIN = 0x01
SUB_RICH = 0x02

VAR_TYPES = {0: "Bool", 1: "Int16", 2: "Uint16", 3: "Int32",
             4: "Uint32", 5: "Float", 6: "Double"}
ACCESS_MODES = {0: "ReadOnly", 1: "WriteOnly", 2: "ReadWrite"}


# ============================================================================
# RTU framing (CRC-16, polynomial 0xA001)
# ============================================================================

def _build_crc_table() -> list[int]:
    table = []
    for i in range(256):
        crc = i
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
        table.append(crc)
    return table


_CRC_TABLE = _build_crc_table()


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc = (crc >> 8) ^ _CRC_TABLE[(crc ^ b) & 0xFF]
    return crc


def build_rtu_frame(slave_id: int, pdu: bytes) -> bytes:
    raw = bytes([slave_id]) + pdu
    crc = crc16(raw)
    return raw + struct.pack("<H", crc)


def check_rtu_frame(frame: bytes) -> bytes | None:
    """Return PDU bytes if CRC is valid, else None."""
    if len(frame) < 4:
        return None
    crc_calc = crc16(frame[:-2])
    crc_recv = struct.unpack_from("<H", frame, len(frame) - 2)[0]
    if crc_calc != crc_recv:
        return None
    return frame[1:-2]  # strip slave_id and CRC


# ============================================================================
# Response decoder
# ============================================================================

class DecodeError(Exception):
    pass


def _read_byte(data: bytes, pos: int) -> tuple[int, int]:
    if pos >= len(data):
        raise DecodeError(f"truncated at byte {pos}")
    return data[pos], pos + 1


def _read_u16(data: bytes, pos: int) -> tuple[int, int]:
    if pos + 2 > len(data):
        raise DecodeError(f"truncated at u16 @{pos}")
    val = (data[pos] << 8) | data[pos + 1]
    return val, pos + 2


def _read_string(data: bytes, pos: int) -> tuple[str, int]:
    length, pos = _read_byte(data, pos)
    if pos + length > len(data):
        raise DecodeError(f"string truncated at {pos}")
    s = data[pos:pos + length].decode("utf-8", errors="replace")
    return s, pos + length


def _read_f32_be(data: bytes, pos: int) -> tuple[float, int]:
    if pos + 4 > len(data):
        raise DecodeError(f"f32 truncated at {pos}")
    val = struct.unpack_from(">f", data, pos)[0]
    return val, pos + 4


def decode_discovery_pdu(pdu: bytes) -> list[dict]:
    if len(pdu) < 4:
        raise DecodeError("PDU too short for discovery header")
    pos = 0
    _, pos = _read_byte(pdu, pos)  # FC byte — echoed from request, not needed here
    sub_fn, pos = _read_byte(pdu, pos)
    count, pos = _read_u16(pdu, pos)
    is_rich = (sub_fn == SUB_RICH)

    entries = []
    for _ in range(count):
        e: dict = {}
        e["address"], pos = _read_u16(pdu, pos)
        e["reg_count"], pos = _read_byte(pdu, pos)
        type_raw, pos = _read_byte(pdu, pos)
        access_raw, pos = _read_byte(pdu, pos)
        e["type"] = VAR_TYPES.get(type_raw, f"?{type_raw}")
        e["access"] = ACCESS_MODES.get(access_raw, f"?{access_raw}")
        e["name"], pos = _read_string(pdu, pos)
        e["unit"] = ""
        e["desc"] = ""
        e["range_min"] = 0.0
        e["range_max"] = 0.0
        if is_rich:
            e["unit"], pos = _read_string(pdu, pos)
            e["desc"], pos = _read_string(pdu, pos)
            e["range_min"], pos = _read_f32_be(pdu, pos)
            e["range_max"], pos = _read_f32_be(pdu, pos)
        entries.append(e)
    return entries


# ============================================================================
# Serial probe
# ============================================================================

def probe(port: str, baud: int, slave_id: int, fc: int, sub_fn: int,
          timeout: float = 1.0) -> list[dict]:
    pdu = bytes([fc, sub_fn])
    frame = build_rtu_frame(slave_id, pdu)

    with serial.Serial(port, baud, timeout=timeout) as ser:
        ser.reset_input_buffer()
        ser.write(frame)
        ser.flush()
        time.sleep(0.05)  # give slave time to respond

        # Read up to 512 bytes (max discovery response for ~50 vars)
        raw = ser.read(512)

    if not raw:
        raise DecodeError("no response from slave (timeout)")

    pdu_resp = check_rtu_frame(raw)
    if pdu_resp is None:
        raise DecodeError(f"CRC mismatch — raw bytes: {raw.hex()}")

    return decode_discovery_pdu(pdu_resp)


# ============================================================================
# Formatting
# ============================================================================

def print_table(entries: list[dict], rich: bool) -> None:
    if rich:
        hdr = f"{'Addr':<6} {'Regs':<5} {'Type':<8} {'Access':<11} {'Name':<24} {'Unit':<8} {'Description':<30} Range"
        print(hdr)
        print("-" * len(hdr))
        for e in entries:
            rng = f"{e['range_min']:.3g}..{e['range_max']:.3g}" if (e['range_min'] or e['range_max']) else ""
            print(f"0x{e['address']:04X}  {e['reg_count']:<4}  {e['type']:<8} {e['access']:<11} "
                  f"{e['name']:<24} {e['unit']:<8} {e['desc']:<30} {rng}")
    else:
        hdr = f"{'Addr':<6} {'Regs':<5} {'Type':<8} {'Access':<11} Name"
        print(hdr)
        print("-" * len(hdr))
        for e in entries:
            print(f"0x{e['address']:04X}  {e['reg_count']:<4}  {e['type']:<8} {e['access']:<11} {e['name']}")


# ============================================================================
# CLI
# ============================================================================

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Alloy FC 0x65 Modbus discovery probe",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--port", required=True, help="Serial port (e.g. /dev/ttyUSB0, COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    parser.add_argument("--slave", type=lambda x: int(x, 0), default=0x01,
                        help="Slave ID (default 0x01)")
    parser.add_argument("--fc", type=lambda x: int(x, 0), default=FC_DISCOVERY_DEFAULT,
                        help=f"Discovery function code (default 0x{FC_DISCOVERY_DEFAULT:02X})")
    parser.add_argument("--sub", choices=["thin", "rich"], default="thin",
                        help="Sub-function: thin (addr/type/name) or rich (+ unit/desc/range)")
    parser.add_argument("--timeout", type=float, default=1.0,
                        help="Response timeout in seconds (default 1.0)")
    parser.add_argument("--json", action="store_true",
                        help="Print output as JSON instead of a table")
    args = parser.parse_args()

    sub_fn = SUB_THIN if args.sub == "thin" else SUB_RICH

    try:
        entries = probe(args.port, args.baud, args.slave, args.fc, sub_fn, args.timeout)
    except DecodeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
    except serial.SerialException as exc:
        print(f"serial error: {exc}", file=sys.stderr)
        sys.exit(1)

    if args.json:
        import json
        print(json.dumps(entries, indent=2))
    else:
        print(f"slave 0x{args.slave:02X} — {len(entries)} variable(s) [{args.sub}]")
        print()
        print_table(entries, rich=(args.sub == "rich"))


if __name__ == "__main__":
    main()
