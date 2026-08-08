"""The generated update root of trust (emit/ota_key.py) — the KEY ROTATION ring.

`key_id` is a POSITIONAL index into this ring, so the emitter's contract is
mostly about what it must REFUSE: a ring where retiring a key would silently
re-point images at a different one, or where no key is left to sign with. Those
refusals are the feature; the happy path is a byte array.
"""

from __future__ import annotations

from pathlib import Path

import pytest

from alloy_cli.emit.common import EmitError
from alloy_cli.emit.ota_key import (
    MAX_RING_KEYS,
    emit_ota_key_header,
    load_key_ring,
    load_public_key,
)

KEY_A = bytes(range(32))
KEY_B = bytes(range(32, 64))


def _pub(tmp_path: Path, name: str, key: bytes) -> str:
    (tmp_path / name).write_text(key.hex() + "\n")
    return name


def test_no_key_at_all_is_unconfigured(tmp_path: Path) -> None:
    assert load_key_ring({}, tmp_path) is None
    hdr = emit_ota_key_header(None)
    assert "configured = false" in hdr
    assert "key_count = 0" in hdr
    # The header must still DECLARE everything firmware names, or the
    # `if constexpr (configured)` branch would not compile in an unsigned build.
    assert "public_keys[1][32]" in hdr
    assert "public_key = &public_keys[0][0]" in hdr


def test_single_public_key_is_a_ring_of_one(tmp_path: Path) -> None:
    ota = {"public_key": _pub(tmp_path, "k.pub", KEY_A)}
    assert load_key_ring(ota, tmp_path) == [KEY_A]
    hdr = emit_ota_key_header([KEY_A])
    assert "configured = true" in hdr
    assert "key_count = 1" in hdr
    assert "public_keys[1][32]" in hdr
    assert "0x00, 0x01, 0x02" in hdr


def test_ring_preserves_order_and_marks_retired_slots(tmp_path: Path) -> None:
    ota = {"public_keys": ["retired", _pub(tmp_path, "b.pub", KEY_B)]}
    ring = load_key_ring(ota, tmp_path)
    assert ring == [bytes(32), KEY_B]
    hdr = emit_ota_key_header(ring)
    assert "key_count = 2" in hdr
    assert "key_id 0  — RETIRED" in hdr
    # key_id 1 must still be the real key: retiring must not shift indices.
    assert hdr.index("key_id 1") > hdr.index("key_id 0")
    assert f"0x{KEY_B[0]:02X}" in hdr


@pytest.mark.parametrize("spelling", ["", "retired", "REVOKED", " Retired "])
def test_retired_spellings(tmp_path: Path, spelling: str) -> None:
    ota = {"public_keys": [spelling, _pub(tmp_path, "b.pub", KEY_B)]}
    assert load_key_ring(ota, tmp_path)[0] == bytes(32)


def test_both_spellings_at_once_is_refused(tmp_path: Path) -> None:
    ota = {"public_key": _pub(tmp_path, "a.pub", KEY_A), "public_keys": ["a.pub"]}
    with pytest.raises(EmitError, match="not both"):
        load_key_ring(ota, tmp_path)


def test_an_all_retired_ring_is_refused(tmp_path: Path) -> None:
    """Every key retired = an un-updatable device. Refuse at codegen, where it
    is a one-line fix, not in the field."""
    with pytest.raises(EmitError, match="every entry is retired"):
        load_key_ring({"public_keys": ["retired", ""]}, tmp_path)


def test_a_duplicated_key_is_refused(tmp_path: Path) -> None:
    """Two ring slots holding the SAME key means retiring one leaves the other
    accepting the identical signatures — revocation that quietly does nothing."""
    a = _pub(tmp_path, "a.pub", KEY_A)
    with pytest.raises(EmitError, match="SAME key"):
        load_key_ring({"public_keys": [a, a]}, tmp_path)


def test_empty_and_oversized_rings_are_refused(tmp_path: Path) -> None:
    with pytest.raises(EmitError, match="non-empty list"):
        load_key_ring({"public_keys": []}, tmp_path)
    many = [KEY_A.hex()] + [f"{i:064x}" for i in range(1, MAX_RING_KEYS + 1)]
    with pytest.raises(EmitError, match="ring limit"):
        load_key_ring({"public_keys": many}, tmp_path)


def test_a_non_string_entry_is_refused(tmp_path: Path) -> None:
    with pytest.raises(EmitError, match="expected a string"):
        load_key_ring({"public_keys": [1]}, tmp_path)


def test_inline_hex_and_bad_keys(tmp_path: Path) -> None:
    assert load_public_key(KEY_A.hex(), tmp_path) == KEY_A
    with pytest.raises(EmitError, match="32 bytes"):
        load_public_key("00ff", tmp_path)
    with pytest.raises(EmitError, match="not hex"):
        load_public_key("z" * 64, tmp_path)
