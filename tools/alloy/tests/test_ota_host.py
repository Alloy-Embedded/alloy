"""Unit tests for the host side of serial firmware update (ota_host.py).

The image bytes must match alloy/ota/image.hpp bit-for-bit and the frames must
match alloy/ota/uart_transport.hpp — both layouts are re-derived here from the
documented offsets (not from the code under test), and the update() client is
driven against a fake device implementing the receiver's exact semantics
(ACK/NAK/INFO, duplicate-seq tolerance) including a lossy link.
"""

from __future__ import annotations

import struct
import zlib

import pytest

from alloy_cli.ota_host import (
    HEADER_SIZE,
    SOF,
    T_ACK,
    T_DATA,
    T_FINISH,
    T_HELLO,
    T_INFO,
    T_NAK,
    UpdateError,
    encode_frame,
    make_image,
    update,
)


def test_image_header_matches_documented_layout() -> None:
    app = b"\x01\x02\x03\x04"
    img = make_image(app, version=42, app_offset=0x200)
    # header fields at their documented offsets (image.hpp)
    assert img[0:4] == b"ALOY"
    assert struct.unpack_from("<H", img, 4)[0] == 1          # header_version
    assert struct.unpack_from("<H", img, 6)[0] == HEADER_SIZE
    assert struct.unpack_from("<I", img, 8)[0] == 42          # image_version
    payload_len = struct.unpack_from("<I", img, 12)[0]
    assert payload_len == (0x200 - HEADER_SIZE) + len(app)
    payload = img[HEADER_SIZE:]
    assert len(payload) == payload_len
    assert struct.unpack_from("<I", img, 16)[0] == zlib.crc32(payload)
    assert struct.unpack_from("<I", img, 28)[0] == zlib.crc32(img[:28])
    # pad is erased-flash 0xFF and the app lands at app_offset
    assert set(img[HEADER_SIZE:0x200]) == {0xFF}
    assert img[0x200:] == app


def test_frame_encoding_matches_wire_format() -> None:
    f = encode_frame(T_DATA, 7, b"xyz")
    assert f[0] == SOF
    ftype, seq, ln = struct.unpack_from("<BBH", f, 1)
    assert (ftype, seq, ln) == (T_DATA, 7, 3)
    assert f[5:8] == b"xyz"
    assert struct.unpack_from("<I", f, 8)[0] == zlib.crc32(f[1:8])


class FakeDevice:
    """In-memory Link speaking the device side of the protocol, with optional
    fault injection. Parses frames exactly like uart_receiver dispatches them."""

    def __init__(self, max_chunk: int = 64, drop_first_ack: bool = False,
                 finish_error: int = 0):
        self.rx = bytearray()   # bytes the host wrote, pending device parse
        self.tx = bytearray()   # bytes the device replied, pending host read
        self.max_chunk = max_chunk
        self.drop_first_ack = drop_first_ack
        self.finish_error = finish_error
        self.written = bytearray()
        self.expected_seq = 0
        self.begins = 0

    # ── Link protocol (what update() calls) ────────────────────────────────
    def write(self, data: bytes) -> int:
        self.rx += data
        self._process()
        return len(data)

    def read(self, size: int) -> bytes:
        out, self.tx = bytes(self.tx[:size]), self.tx[size:]
        return out  # empty = timeout

    # ── Device behaviour ───────────────────────────────────────────────────
    def _reply(self, ftype: int, seq: int, payload: bytes = b"") -> None:
        self.tx += encode_frame(ftype, seq, payload)

    def _process(self) -> None:
        while True:
            i = self.rx.find(bytes([SOF]))
            if i < 0 or len(self.rx) < i + 9:
                return
            ftype, seq, ln = struct.unpack_from("<BBH", self.rx, i + 1)
            end = i + 5 + ln + 4
            if len(self.rx) < end:
                return
            payload = bytes(self.rx[i + 5:i + 5 + ln])
            crc = struct.unpack_from("<I", self.rx, i + 5 + ln)[0]
            self.rx = self.rx[end:]
            if zlib.crc32(struct.pack("<BBH", ftype, seq, ln) + payload) != crc:
                self._reply(T_NAK, 0, b"\x00")
                continue
            if ftype == T_HELLO:
                self.begins += 1
                self.expected_seq = 0
                self.written.clear()
                self._reply(T_INFO, 0,
                            struct.pack("<BBIH", 1, 1, 7, self.max_chunk))
            elif ftype == T_DATA:
                if seq == (self.expected_seq - 1) & 0xFF:
                    self._reply(T_ACK, seq)      # retransmit: ack, don't rewrite
                elif seq != self.expected_seq & 0xFF:
                    self._reply(T_NAK, 0, b"\x00")
                else:
                    self.written += payload
                    if self.drop_first_ack and self.expected_seq == 0:
                        self.drop_first_ack = False  # swallow the ACK once
                    else:
                        self._reply(T_ACK, seq)
                    self.expected_seq += 1
            elif ftype == T_FINISH:
                if self.finish_error:
                    self._reply(T_NAK, 0, bytes([self.finish_error]))
                else:
                    self._reply(T_ACK, seq)


def test_update_happy_path_streams_whole_image() -> None:
    dev = FakeDevice(max_chunk=64)
    image = make_image(b"A" * 300, version=5)
    info = update(dev, image)
    assert bytes(dev.written) == image
    assert info == {"target_slot": 1, "running_version": 7}


def test_update_survives_a_lost_ack_without_duplicating_bytes() -> None:
    dev = FakeDevice(max_chunk=32, drop_first_ack=True)
    image = make_image(b"B" * 100, version=6)
    update(dev, image)
    # the retransmitted chunk was ACKed but written once
    assert bytes(dev.written) == image


def test_update_surfaces_device_error_on_finish() -> None:
    dev = FakeDevice(finish_error=3)  # e.g. ota_error::bad_payload_crc
    image = make_image(b"C" * 10, version=7)
    with pytest.raises(UpdateError, match="ota_error 3"):
        update(dev, image)


def test_update_gives_up_on_a_dead_device() -> None:
    class Dead:
        def write(self, data: bytes) -> int: return len(data)
        def read(self, size: int) -> bytes: return b""

    with pytest.raises(UpdateError, match="not responding"):
        update(Dead(), make_image(b"D", version=1), retries=2)


def _mini_elf(segs: list[tuple[int, bytes]]) -> bytes:
    """A minimal ELF32-LE with the given (paddr, data) PT_LOAD segments."""
    ehsize, phentsize = 52, 32
    phoff = ehsize
    data_off = phoff + phentsize * len(segs)
    head = bytearray(b"\x7fELF" + bytes([1, 1, 1, 0]) + b"\x00" * 8)
    head += struct.pack("<HHIIIIIHHHHHH", 2, 40, 1, 0, phoff, 0, 0,
                        ehsize, phentsize, len(segs), 0, 0, 0)
    body = bytearray()
    phdrs = bytearray()
    for paddr, data in segs:
        off = data_off + len(body)
        phdrs += struct.pack("<IIIIIIII", 1, off, paddr, paddr,
                             len(data), len(data), 5, 4)
        body += data
    return bytes(head) + bytes(phdrs) + bytes(body)


def test_elf_to_bin_lays_out_segments_by_lma_with_ff_gaps() -> None:
    from alloy_cli.ota_host import elf_to_bin

    elf = _mini_elf([(0x08004200, b"AAAA"), (0x08004210, b"BB")])
    out = elf_to_bin(elf)
    assert out == b"AAAA" + b"\xff" * 12 + b"BB"


def test_elf_to_bin_rejects_non_elf_and_64bit() -> None:
    from alloy_cli.ota_host import elf_to_bin

    with pytest.raises(ValueError, match="not an ELF"):
        elf_to_bin(b"\x00\x01\x02\x03rubbish")
    bad = bytearray(_mini_elf([(0, b"x")]))
    bad[4] = 2  # ELFCLASS64
    with pytest.raises(ValueError, match="ELF32"):
        elf_to_bin(bytes(bad))


def test_update_dual_image_picks_the_devices_target_slot() -> None:
    img_a = make_image(b"A" * 40, version=9)
    img_b = make_image(b"B" * 40, version=9)
    dev = FakeDevice(max_chunk=64)  # its INFO reports target_slot = 1 (B)
    info = update(dev, {0: img_a, 1: img_b})
    assert info["target_slot"] == 1
    assert bytes(dev.written) == img_b  # picked B, not A


def test_update_dual_image_missing_target_is_a_clear_error() -> None:
    dev = FakeDevice()
    with pytest.raises(UpdateError, match="targets slot B"):
        update(dev, {0: make_image(b"A", version=1)})
