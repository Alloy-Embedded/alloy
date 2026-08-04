"""Unit tests for `alloy emulate`'s Renode discovery.

`_find_renode` is what lets `alloy emulate` run locally without the user editing
PATH: it prefers a `renode` on PATH, then falls back to an alloy-managed install
under ~/.alloy/tools/. The fallback glob + newest-wins ordering is easy to get
subtly wrong, so pin it here (PATH is monkeypatched so the test never depends on
the host actually having Renode installed).
"""

from __future__ import annotations

import os
from pathlib import Path

from alloy_cli.cli import _find_renode


def _make_renode(app_dir: Path) -> Path:
    macos = app_dir / "Renode.app" / "Contents" / "MacOS"
    macos.mkdir(parents=True)
    binary = macos / "renode"
    binary.write_text("#!/bin/sh\n")
    binary.chmod(0o755)
    return binary


def test_path_renode_wins(tmp_path, monkeypatch):
    on_path = tmp_path / "bin" / "renode"
    on_path.parent.mkdir(parents=True)
    on_path.write_text("#!/bin/sh\n")
    on_path.chmod(0o755)
    monkeypatch.setattr("shutil.which", lambda _name: str(on_path))
    assert _find_renode() == str(on_path)


def test_falls_back_to_alloy_tools_newest(tmp_path, monkeypatch):
    monkeypatch.setattr("shutil.which", lambda _name: None)
    monkeypatch.setattr(Path, "home", classmethod(lambda _cls: tmp_path))
    tools = tmp_path / ".alloy" / "tools"
    old = _make_renode(tools / "renode-1.15.0")
    new = _make_renode(tools / "renode-1.16.1")
    found = _find_renode()
    # newest version sorts last, so reverse=True picks 1.16.1 over 1.15.0
    assert found == str(new)
    assert found != str(old)
    assert os.access(new, os.X_OK)


def test_returns_none_when_nothing_installed(tmp_path, monkeypatch):
    monkeypatch.setattr("shutil.which", lambda _name: None)
    monkeypatch.setattr(Path, "home", classmethod(lambda _cls: tmp_path))
    assert _find_renode() is None
