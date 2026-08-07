"""Host side of serial firmware update: image packing + the update client.

Mirrors the device formats bit-for-bit:
  - alloy/ota/image.hpp     -> make_image()  (32-byte header, CRC-32/ISO-HDLC —
                               which is exactly Python's zlib.crc32)
  - alloy/ota/uart_transport.hpp -> encode_frame()/read_frame() + update()

The client is deliberately dumb (stop-and-wait, resend on NAK or timeout): all
sequencing intelligence lives here, the device is a pure responder — the right
shape when the device end must fit in a 16 KB bootloader.
"""

from __future__ import annotations

import struct
import zlib
from dataclasses import dataclass
from typing import Protocol

MAGIC = int.from_bytes(b"ALOY", "little")  # as image.hpp composes it
HEADER_SIZE = 32
APP_OFFSET_DEFAULT = 0x200

SOF = 0x7E
T_HELLO, T_DATA, T_FINISH = 0x01, 0x02, 0x03
T_INFO, T_ACK, T_NAK = 0x81, 0x82, 0x83


SIGNATURE_BYTES = 64


def generate_keypair() -> tuple[bytes, bytes]:
    """(private_seed_32, public_32) for Ed25519 image signing."""
    from cryptography.hazmat.primitives.asymmetric.ed25519 import (  # noqa: PLC0415
        Ed25519PrivateKey,
    )
    from cryptography.hazmat.primitives.serialization import (  # noqa: PLC0415
        Encoding, NoEncryption, PrivateFormat, PublicFormat,
    )

    priv = Ed25519PrivateKey.generate()
    return (priv.private_bytes(Encoding.Raw, PrivateFormat.Raw, NoEncryption()),
            priv.public_key().public_bytes(Encoding.Raw, PublicFormat.Raw))


def sign_image(image: bytes, private_seed: bytes) -> bytes:
    """Append the Ed25519 signature TRAILER over [header|payload].

    The signature covers the header too, so image_version can't be relabelled by
    an attacker — and it lives AFTER payload_length bytes, which is why nothing
    about the v1 header/CRC changes: the device's `covered` span (what
    verify_slot already computes) IS the signed message."""
    from cryptography.hazmat.primitives.asymmetric.ed25519 import (  # noqa: PLC0415
        Ed25519PrivateKey,
    )

    if len(private_seed) != 32:
        raise ValueError("Ed25519 private key must be 32 raw bytes")
    priv = Ed25519PrivateKey.from_private_bytes(private_seed)
    return image + priv.sign(image)


def elf_to_bin(elf: bytes) -> bytes:
    """Flatten an ELF32-LE firmware image to the raw bytes objcopy -O binary
    produces. Pure python so `alloy image` and `alloy update` work on machines
    with no cross toolchain (a field tech's laptop).

    objcopy works on SECTIONS, not segments — and that difference bit for real:
    GCC page-aligns the first PT_LOAD so it maps from file offset 0, ELF header
    included; flattening whole segments put those header bytes where the vector
    table belongs and the bootloader jumped into them. So: take each allocated,
    non-NOBITS section, compute its LMA through its containing PT_LOAD
    (lma = p_paddr + (sh_offset - p_offset)), and lay sections out by LMA with
    0xFF (erased flash) in the gaps."""
    if elf[:4] != b"\x7fELF":
        raise ValueError("not an ELF file")
    if elf[4] != 1 or elf[5] != 1:
        raise ValueError("only ELF32 little-endian firmware is supported")
    e_phoff, = struct.unpack_from("<I", elf, 28)
    e_shoff, = struct.unpack_from("<I", elf, 32)
    e_phentsize, e_phnum = struct.unpack_from("<HH", elf, 42)
    e_shentsize, e_shnum = struct.unpack_from("<HH", elf, 46)

    loads = []
    for i in range(e_phnum):
        off = e_phoff + i * e_phentsize
        p_type, p_offset, _va, p_paddr, p_filesz = struct.unpack_from("<IIIII", elf, off)
        if p_type == 1:  # PT_LOAD
            loads.append((p_offset, p_filesz, p_paddr))

    pieces = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        (sh_type, sh_flags, _addr, sh_offset,
         sh_size) = struct.unpack_from("<IIIII", elf, off + 4)
        if not (sh_flags & 0x2) or sh_type == 8 or sh_size == 0:  # !ALLOC | NOBITS
            continue
        for p_offset, p_filesz, p_paddr in loads:
            if p_offset <= sh_offset < p_offset + p_filesz:
                pieces.append((p_paddr + (sh_offset - p_offset),
                               elf[sh_offset:sh_offset + sh_size]))
                break
    if not pieces:
        raise ValueError("ELF has no loadable sections")
    pieces.sort()
    base = pieces[0][0]
    out = bytearray()
    for lma, data in pieces:
        gap = lma - base - len(out)
        if gap < 0:
            raise ValueError("overlapping sections")
        out += b"\xff" * gap + data
    return bytes(out)


def make_image(app: bytes, version: int, app_offset: int = APP_OFFSET_DEFAULT) -> bytes:
    """[image_header | 0xFF pad | app] — the payload starts at slot offset 32 and
    pads up to app_offset so the app's vector table lands where it was linked
    (slot_base + app_offset; see emit/slots.py)."""
    if app_offset < HEADER_SIZE:
        raise ValueError(f"app_offset must be >= {HEADER_SIZE}")
    payload = b"\xff" * (app_offset - HEADER_SIZE) + app
    head = struct.pack(
        "<IHHIII H H I",
        MAGIC, 1, HEADER_SIZE, version, len(payload), zlib.crc32(payload),
        0, 0, 0)
    assert len(head) == 28
    return head + struct.pack("<I", zlib.crc32(head)) + payload


def encode_frame(ftype: int, seq: int, payload: bytes = b"") -> bytes:
    body = struct.pack("<BBH", ftype, seq, len(payload)) + payload
    return bytes([SOF]) + body + struct.pack("<I", zlib.crc32(body))


class Link(Protocol):
    """What update() needs from a serial port; pyserial's Serial satisfies it."""

    def read(self, size: int) -> bytes: ...   # honors the port's timeout
    def write(self, data: bytes) -> int: ...


@dataclass
class Frame:
    ftype: int
    seq: int
    payload: bytes


class UpdateError(RuntimeError):
    pass


def read_frame(link: Link) -> Frame | None:
    """One frame off the wire, or None on timeout. Skips noise before the SOF."""
    while True:
        b = link.read(1)
        if not b:
            return None
        if b[0] == SOF:
            break
    hdr = link.read(4)
    if len(hdr) < 4:
        return None
    ftype, seq, length = struct.unpack("<BBH", hdr)
    payload = link.read(length) if length else b""
    if len(payload) < length:
        return None
    crc_raw = link.read(4)
    if len(crc_raw) < 4:
        return None
    if zlib.crc32(hdr + payload) != struct.unpack("<I", crc_raw)[0]:
        return None  # corrupt: caller treats like a timeout (retry)
    return Frame(ftype, seq, payload)


def _exchange(link: Link, out: bytes, retries: int) -> Frame:
    """Send `out`, wait for a reply frame; resend on timeout/corruption/NAK-0.
    A NAK carrying a device error code aborts (retrying won't fix bad flash)."""
    for _ in range(retries):
        link.write(out)
        reply = read_frame(link)
        if reply is None:
            continue
        if reply.ftype == T_NAK:
            err = reply.payload[0] if reply.payload else 0
            if err:
                raise UpdateError(f"device rejected update (ota_error {err})")
            continue  # framing/sequence noise: resend
        return reply
    raise UpdateError("device not responding (retries exhausted)")


def update(link: Link, image: bytes | dict[int, bytes], retries: int = 8,
           progress=None) -> dict:
    """Push a packed image. Returns the INFO facts (slot/version) on success.

    A/B-correct mode: pass {0: slot_a_image, 1: slot_b_image} and the right one
    is chosen AFTER the device reports its target slot in INFO — an app image is
    linked for a specific slot, so sending the wrong one would trial-boot into a
    crash (the rollback saves the device, but the update wastes a cycle)."""
    info = _exchange(link, encode_frame(T_HELLO, 0), retries)
    if info.ftype != T_INFO or len(info.payload) < 8:
        raise UpdateError(f"unexpected reply to HELLO (type 0x{info.ftype:02x})")
    proto, slot = info.payload[0], info.payload[1]
    running = struct.unpack("<I", info.payload[2:6])[0]
    max_chunk = struct.unpack("<H", info.payload[6:8])[0]
    if proto != 1:
        raise UpdateError(f"device speaks protocol {proto}, host speaks 1")
    if isinstance(image, dict):
        if slot not in image:
            raise UpdateError(f"device targets slot {'B' if slot else 'A'} but no "
                              f"image for it was provided")
        image = image[slot]

    seq = 0
    for off in range(0, len(image), max_chunk):
        chunk = image[off:off + max_chunk]
        reply = _exchange(link, encode_frame(T_DATA, seq & 0xFF, chunk), retries)
        if reply.ftype != T_ACK:
            raise UpdateError(f"unexpected reply to DATA (type 0x{reply.ftype:02x})")
        seq += 1
        if progress:
            progress(min(off + max_chunk, len(image)), len(image))

    reply = _exchange(link, encode_frame(T_FINISH, 0), retries)
    if reply.ftype != T_ACK:
        raise UpdateError(f"unexpected reply to FINISH (type 0x{reply.ftype:02x})")
    return {"target_slot": slot, "running_version": running}
