"""Per-device factory identity, written through the debug probe.

`alloy provision` is the production line's half of src/alloy/provision — it
encodes a 64-byte identity record (serial, MAC, hw revision, batch) and programs
it into ``alloy::slots::provision_base``, the page the slot layout carves out
for exactly this and which no firmware update ever touches.

Shape, deliberately the same as `alloy secure`:

  * a PURE codec (encode/decode/validate) that needs no hardware and carries
    every refusal, so the interesting behaviour is unit-testable;
  * a thin openocd runner around it;
  * verify-after-write, always — the record is read BACK off the device and
    compared byte for byte before the command reports success.

THE REFUSALS ARE THE PRODUCT. A factory types these values once per board, at
speed, from a work order. A multicast MAC, a duplicated serial, a serial with a
stray trailing space — each is a silent disaster discovered months later in the
field, and each is a one-line mistake. They are refused here, loudly, by name.

WRITE ORDER MATTERS AND IS NOT RECOVERABLE. On uniform-page flash the identity
page is the last page of the bootloader region, which is precisely the range
``alloy secure apply --wrp-bootloader`` write-protects. Provision BEFORE you
secure. If you get it backwards the write silently does nothing and the readback
below is what tells you — see examples/factory/line.py for the whole order.

HONESTY: nothing here is silicon-witnessed — no board was on hand. What IS
witnessed is the record format (host round-trip tests plus a Renode leg that
LoadBinary's an encoded page at provision_base and reads the firmware print it
back). The openocd command list is reasoned from openocd's documented
`flash write_image` / `dump_image` and has never been run against a probe.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .emit.common import EmitError
from .emit.slots import SlotLayout
from .secure import Runner, openocd_argv


def default_runner(argv: list[str]) -> subprocess.CompletedProcess:
    """`alloy provision` is openocd-only in v1, the same call `alloy secure`
    makes and for the same reason: probe-rs's CLI has no verb that programs a
    bare address range and dumps it back in one session, and guessing at one
    would put a wrong address between a factory and a batch of boards."""
    if not shutil.which("openocd"):
        raise EmitError("openocd not on PATH — `alloy provision` is openocd-only "
                        "in v1 (install openocd, or run `alloy setup`)")
    return subprocess.run(argv, capture_output=True, text=True, check=False)

# ── record format (mirror of src/alloy/provision/identity.hpp) ──────────────

MAGIC = b"APRV"
FORMAT_VERSION = 1
RECORD_SIZE = 64
SERIAL_CAPACITY = 16
MAC_SIZE = 6

_CRC_POLY = (0xEDB8 << 16) | 0x8320  # CRC-32/ISO-HDLC, reflected


def crc32(data: bytes) -> int:
    """CRC-32/ISO-HDLC — the same function as src/alloy/ota/crc32.hpp.

    Written out rather than using zlib.crc32 so this file states the polynomial
    it agrees with the firmware on; the tests assert the two agree on real
    records via the C++ side's own encoder."""
    c = 0xFFFFFFFF
    for byte in data:
        c ^= byte
        for _ in range(8):
            c = (c >> 1) ^ (_CRC_POLY & -(c & 1))
    return c ^ 0xFFFFFFFF


@dataclass(frozen=True)
class Identity:
    """One device's factory identity. Construct through `make_identity`, which
    is where every refusal lives — this dataclass holds validated values."""

    serial: str
    mac: bytes = b"\x00" * MAC_SIZE
    hw_revision: int = 0
    batch: int = 0

    @property
    def mac_str(self) -> str:
        return ":".join(f"{b:02x}" for b in self.mac)

    def describe(self) -> str:
        mac = self.mac_str if any(self.mac) else "(none)"
        return (f"serial {self.serial!r}  mac {mac}  "
                f"hw_rev {self.hw_revision}  batch {self.batch}")


def parse_mac(text: str) -> bytes:
    """Accept aa:bb:cc:dd:ee:ff, aa-bb-..., or 12 bare hex digits.

    Refuses the two encodings that are valid hex and invalid as a device
    address: the MULTICAST/group bit (bit 0 of the first octet) set — a station
    with a multicast source address is dropped by switches and by lwIP itself,
    and the symptom is "the board is on the network but nobody answers" — and
    the all-ones broadcast address."""
    cleaned = re.sub(r"[:\-.\s]", "", text.strip())
    if len(cleaned) != 2 * MAC_SIZE or not re.fullmatch(r"[0-9a-fA-F]+", cleaned):
        raise EmitError(
            f"--mac {text!r} is not a MAC address — expected 6 hex octets "
            f"(aa:bb:cc:dd:ee:ff, aa-bb-cc-dd-ee-ff, or aabbccddeeff)")
    mac = bytes.fromhex(cleaned)
    # Broadcast FIRST: it also has the multicast bit set, and "that is the
    # broadcast address" is the more useful sentence than "bit 0 is odd".
    if mac == b"\xff" * MAC_SIZE:
        raise EmitError("--mac ff:ff:ff:ff:ff:ff is the broadcast address, not a "
                        "device address")
    if mac[0] & 0x01:
        raise EmitError(
            f"--mac {text!r} has the MULTICAST bit set (first octet {mac[0]:#04x} "
            f"is odd). A device may not use a group address as its own; frames "
            f"it sends will be dropped. Did you mean {mac[0] & ~1:#04x}...?")
    return mac


def make_identity(serial: str, mac: str | None = None, hw_revision: int = 0,
                  batch: int = 0) -> Identity:
    """Validate and build. Every refusal a factory can trip is here."""
    if serial != serial.strip():
        raise EmitError(
            f"--serial {serial!r} has leading/trailing whitespace. It is stored "
            f"verbatim, so this device would not match its own work order — "
            f"strip it rather than let the mismatch ship")
    if not serial:
        raise EmitError("--serial is empty — a device with no serial number is "
                        "not provisioned, it is just written to")
    try:
        raw = serial.encode("ascii")
    except UnicodeEncodeError as exc:
        raise EmitError(
            f"--serial {serial!r} is not ASCII. The record stores 16 raw bytes "
            f"and firmware prints them over a debug UART; keep serials to "
            f"printable ASCII") from exc
    if any(b < 0x20 or b == 0x7F for b in raw):
        raise EmitError(f"--serial {serial!r} contains control characters")
    if len(raw) > SERIAL_CAPACITY:
        raise EmitError(
            f"--serial {serial!r} is {len(raw)} bytes; the record holds "
            f"{SERIAL_CAPACITY}. Truncating a serial silently is how two devices "
            f"end up with the same one, so this is a refusal, not a warning")
    if not 0 <= hw_revision <= 0xFFFF:
        raise EmitError(f"--hw-rev must be 0..65535 (got {hw_revision})")
    if not 0 <= batch <= 0xFFFFFFFF:
        raise EmitError(f"--batch must be 0..4294967295 (got {batch})")
    return Identity(serial=serial,
                    mac=parse_mac(mac) if mac else b"\x00" * MAC_SIZE,
                    hw_revision=hw_revision, batch=batch)


def encode(identity: Identity) -> bytes:
    """The 64 bytes that go into flash. Byte-for-byte what
    alloy::provision::identity::serialize writes."""
    body = bytearray(RECORD_SIZE - 4)
    body[0:4] = MAGIC
    body[4:6] = FORMAT_VERSION.to_bytes(2, "little")
    body[6:8] = RECORD_SIZE.to_bytes(2, "little")
    body[8:8 + SERIAL_CAPACITY] = identity.serial.encode("ascii").ljust(
        SERIAL_CAPACITY, b"\x00")
    body[24:24 + MAC_SIZE] = identity.mac
    body[30:32] = identity.hw_revision.to_bytes(2, "little")
    body[32:36] = identity.batch.to_bytes(4, "little")
    # bytes 36..60 stay zero (reserved)
    return bytes(body) + crc32(bytes(body)).to_bytes(4, "little")


def decode(raw: bytes) -> Identity:
    """Parse a record, refusing exactly what the firmware refuses (and saying
    which of the two 'nothing there' cases it is — never provisioned vs
    provisioned with garbage — because they mean different things on a line)."""
    if len(raw) < RECORD_SIZE:
        raise EmitError(f"identity record is {len(raw)} bytes, need {RECORD_SIZE}")
    raw = raw[:RECORD_SIZE]
    if raw[0:4] != MAGIC:
        if raw[0:4] == b"\xff\xff\xff\xff":
            raise EmitError("this device has NEVER been provisioned — the "
                            "identity page is erased")
        raise EmitError(
            f"no identity record here: expected magic {MAGIC!r}, found "
            f"{raw[0:4]!r}. Either the wrong address was read or something else "
            f"was written over the identity page")
    if crc32(raw[:60]) != int.from_bytes(raw[60:64], "little"):
        raise EmitError("identity record CRC mismatch — the record is corrupt "
                        "(torn write, or a partially erased page). Re-provision")
    version = int.from_bytes(raw[4:6], "little")
    if version != FORMAT_VERSION:
        raise EmitError(f"identity record format_version {version}; this alloy "
                        f"writes/reads {FORMAT_VERSION}")
    serial = raw[8:8 + SERIAL_CAPACITY].split(b"\x00")[0].decode("ascii", "replace")
    return Identity(serial=serial, mac=bytes(raw[24:24 + MAC_SIZE]),
                    hw_revision=int.from_bytes(raw[30:32], "little"),
                    batch=int.from_bytes(raw[32:36], "little"))


# ── probe plan ──────────────────────────────────────────────────────────────


def provision_region(layout: SlotLayout) -> tuple[int, int]:
    """(base, size) of the identity page, straight from the SAME layout the
    linker and the bootloader use. Never hand-typed, for the same reason the
    WRP range in `alloy secure` is not."""
    return layout.provision.base, layout.provision.size


def write_commands(base: int, src: Path, dump: Path) -> list[str]:
    """openocd script that programs the record and dumps it straight back.

    `flash write_image erase` erases the sectors it touches — here exactly the
    identity page, which is why the layout gives it a page of its own. The dump
    in the SAME session is the verification: comparing bytes in Python beats
    trusting a return code, and it is the one check that catches a write the
    hardware refused (write-protected region) rather than failed."""
    return ["init", "halt",
            f"flash write_image erase {{{src}}} {base:#010x} bin",
            f"dump_image {{{dump}}} {base:#010x} {RECORD_SIZE}",
            "shutdown"]


def read_commands(base: int, dump: Path) -> list[str]:
    return ["init", "halt",
            f"dump_image {{{dump}}} {base:#010x} {RECORD_SIZE}",
            "shutdown"]


def _run(board: dict[str, Any], chip: dict[str, Any], cmds: list[str],
         dump: Path, runner: Runner | None) -> bytes:
    proc = (runner or default_runner)(openocd_argv(board, chip, cmds))
    output = (proc.stdout or "") + (proc.stderr or "")
    if proc.returncode != 0 or not dump.exists():
        raise EmitError(
            "the probe session failed. Causes, most likely first: no board "
            "attached / wrong probe; the part is at RDP level 2 (debug "
            "permanently disabled); openocd/probe mismatch. openocd said:\n"
            + output.strip()[-2000:])
    return dump.read_bytes()


def run_write(board: dict[str, Any], chip: dict[str, Any], layout: SlotLayout,
              identity: Identity, workdir: Path,
              runner: Runner | None = None) -> Identity:
    """Program the identity and prove it landed. Returns what the DEVICE says
    it is — decoded from the readback, never from the input."""
    base, _ = provision_region(layout)
    want = encode(identity)
    src = workdir / "alloy-identity.bin"
    dump = workdir / "alloy-identity-readback.bin"
    src.write_bytes(want)
    dump.unlink(missing_ok=True)
    got = _run(board, chip, write_commands(base, src, dump), dump, runner)
    if got[:RECORD_SIZE] != want:
        if got[:4] == b"\xff\xff\xff\xff":
            raise EmitError(
                f"the identity page at {base:#010x} is still ERASED after the "
                f"write — the device accepted the command and stored nothing. "
                f"On uniform-page flash this page is inside the bootloader "
                f"region, so the usual cause is that `alloy secure apply "
                f"--wrp-bootloader` has already write-protected it. Provisioning "
                f"comes BEFORE securing (examples/factory/line.py); clearing WRP "
                f"over the probe is the only way back.")
        raise EmitError(
            f"identity readback mismatch at {base:#010x} — wrote {want.hex()}, "
            f"read {got[:RECORD_SIZE].hex()}. Nothing about this device's "
            f"identity should be trusted; re-run before it leaves the line")
    return decode(got)


def run_read(board: dict[str, Any], chip: dict[str, Any], layout: SlotLayout,
             workdir: Path, runner: Runner | None = None) -> Identity:
    base, _ = provision_region(layout)
    dump = workdir / "alloy-identity-readback.bin"
    dump.unlink(missing_ok=True)
    return decode(_run(board, chip, read_commands(base, dump), dump, runner))


__all__ = [
    "Identity", "MAC_SIZE", "RECORD_SIZE", "SERIAL_CAPACITY", "crc32", "decode",
    "default_runner", "encode", "make_identity", "parse_mac", "provision_region",
    "read_commands", "run_read", "run_write", "write_commands",
]
