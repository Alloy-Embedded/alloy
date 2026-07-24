"""Guards on the toolchain manifest (`alloy setup`).

The installer verifies every download against a pinned sha256 and refuses an
unpinned one unless ALLOY_ALLOW_UNPINNED=1. That safety net only holds if the
shipped manifest actually carries a real digest for every archive — a stray
`sha256: null` slipping back in would silently reopen the "install an
unverified toolchain" hole. These tests are that forcing function.
"""

from __future__ import annotations

import json
import re
from pathlib import Path

import pytest

import alloy_cli.toolchains as toolchains

_SHA256 = re.compile(r"^[0-9a-f]{64}$")


def _manifest() -> dict:
    return json.loads((Path(toolchains.__file__).parent / "toolchains.json").read_text())


def _archive_entries() -> list[tuple[str, str, dict]]:
    out: list[tuple[str, str, dict]] = []
    for tool, spec in _manifest()["tools"].items():
        if spec.get("kind") != "archive":
            continue
        for plat, pspec in spec["platforms"].items():
            out.append((tool, plat, pspec))
    return out


def test_manifest_parses_and_has_archives() -> None:
    entries = _archive_entries()
    assert entries, "expected at least one archive-kind toolchain in the manifest"


@pytest.mark.parametrize(
    "tool,plat,pspec",
    [pytest.param(t, p, s, id=f"{t}/{p}") for t, p, s in _archive_entries()],
)
def test_every_archive_is_pinned(tool: str, plat: str, pspec: dict) -> None:
    sha = pspec.get("sha256")
    assert sha is not None, f"{tool}/{plat}: sha256 is null — pin it (see `alloy setup`)"
    assert _SHA256.match(sha), f"{tool}/{plat}: sha256 {sha!r} is not 64 lowercase hex chars"


@pytest.mark.parametrize(
    "tool,plat,pspec",
    [pytest.param(t, p, s, id=f"{t}/{p}") for t, p, s in _archive_entries()],
)
def test_every_archive_has_a_url(tool: str, plat: str, pspec: dict) -> None:
    url = pspec.get("url", "")
    assert url.startswith("https://"), f"{tool}/{plat}: url must be https, got {url!r}"
