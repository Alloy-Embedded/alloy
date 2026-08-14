"""The one line a host test cannot compile, pinned at the source level.

tests/test_crc.cpp proves crc_detail::finalize() IS the xorout — but it proves
nothing about whether the DRIVER calls it. The adversarial review of the
UID/CRC landing demonstrated the gap: change st_crc_v3.hpp::value() to
`return r().DR;` and the whole host suite stays green, because the driver
needs generated IP headers no host test can include.

So the chain is closed the way test_claim_surface.py closes its own: a source
pin. It is a tripwire, not a proof — but together with test_crc.cpp it makes
the argument airtight in two steps a reviewer can follow: the driver calls
finalize (HERE), and finalize is the correct xorout (test_crc.cpp). Deleting
either link now fails something.
"""

from __future__ import annotations

from pathlib import Path

_DRIVER = (Path(__file__).resolve().parents[3]
           / "src" / "alloy" / "hal" / "crc" / "st_crc_v3.hpp")


def _value_body() -> str:
    text = _DRIVER.read_text()
    start = text.index("std::uint32_t value()")
    return text[start:text.index("}", start) + 1]


def test_the_driver_reads_dr_only_through_finalize() -> None:
    body = _value_body()
    assert "crc_detail::finalize(" in body, (
        "st_crc_v3.hpp::value() no longer routes DR through "
        "crc_detail::finalize() — the xorout the silicon has no register for "
        "would be silently dropped, and every checksum would differ from "
        "alloy::ota::crc::crc32_of on the same bytes"
    )


def test_no_bare_dr_return_escapes_the_xorout() -> None:
    body = _value_body()
    bare = body.replace("crc_detail::finalize(r().DR)", "")
    assert "r().DR" not in bare, (
        "value() contains a DR read that does not pass through finalize() — "
        "one path with the xorout and one without is worse than either alone"
    )
