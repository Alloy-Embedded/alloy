"""Smoke tests for the alloy-cli entry point.

These tests deliberately avoid invoking real toolchains or CMake. They verify only that:
- the package exposes a callable ``main`` and a ``__version__`` string,
- the top-level help and version paths work without an Alloy checkout,
- the runtime locator finds a checkout via ``ALLOY_ROOT`` and rejects bogus values,
- delegation to ``alloyctl`` happens when a real checkout is provided.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

import pytest

from alloy_cli import __version__
from alloy_cli.main import main
from alloy_cli.runtime import (
    ENV_VAR,
    RuntimeNotFoundError,
    find_runtime_root,
)

REPO_ROOT = Path(__file__).resolve().parents[3]


def _make_fake_root(tmp_path: Path) -> Path:
    (tmp_path / "scripts").mkdir()
    (tmp_path / "cmake").mkdir()
    (tmp_path / "scripts" / "alloyctl.py").write_text(
        "def main():\n    print('alloyctl-stub')\n    return 0\n"
    )
    (tmp_path / "cmake" / "board_manifest.cmake").write_text("# stub\n")
    return tmp_path


def test_version_string_is_nonempty() -> None:
    assert isinstance(__version__, str) and __version__


def test_top_level_help_does_not_require_runtime(monkeypatch, capsys) -> None:
    monkeypatch.delenv(ENV_VAR, raising=False)
    assert main(["--help"]) == 0
    out = capsys.readouterr().out
    assert "alloy" in out and "new" in out


def test_version_flag(monkeypatch, capsys) -> None:
    monkeypatch.delenv(ENV_VAR, raising=False)
    assert main(["--version"]) == 0
    assert __version__ in capsys.readouterr().out


def test_missing_runtime_returns_clear_error(monkeypatch, tmp_path, capsys) -> None:
    """All four resolution paths must fail before the locator gives up."""
    from alloy_cli import config

    monkeypatch.delenv(ENV_VAR, raising=False)
    # Point ALLOY_HOME at an empty directory so the active-SDK fallback finds
    # nothing; otherwise a developer's installed SDK would shadow the test.
    monkeypatch.setenv(config.ENV_HOME, str(tmp_path / "empty-home"))
    monkeypatch.chdir(tmp_path)
    rc = main(["doctor"])
    assert rc == 2
    assert "ALLOY_ROOT" in capsys.readouterr().err


def test_bogus_alloy_root_is_rejected(monkeypatch, tmp_path) -> None:
    monkeypatch.setenv(ENV_VAR, str(tmp_path))
    with pytest.raises(RuntimeNotFoundError):
        find_runtime_root()


def test_find_runtime_root_walks_up_from_subdir(monkeypatch, tmp_path) -> None:
    root = _make_fake_root(tmp_path)
    nested = root / "a" / "b"
    nested.mkdir(parents=True)
    monkeypatch.delenv(ENV_VAR, raising=False)
    monkeypatch.chdir(nested)
    assert find_runtime_root() == root


def test_delegation_invokes_alloyctl_main(monkeypatch, tmp_path, capsys) -> None:
    root = _make_fake_root(tmp_path)
    monkeypatch.setenv(ENV_VAR, str(root))
    assert main(["doctor"]) == 0
    assert "alloyctl-stub" in capsys.readouterr().out


def test_real_repo_is_recognised_as_runtime() -> None:
    assert (REPO_ROOT / "scripts" / "alloyctl.py").is_file(), (
        "smoke test assumes execution inside the alloy repo"
    )
    assert find_runtime_root(REPO_ROOT) == REPO_ROOT


@pytest.mark.skipif(
    not (REPO_ROOT / "scripts" / "alloyctl.py").is_file(),
    reason="real alloyctl.py not present in this checkout",
)
def test_console_script_help_via_subprocess(tmp_path) -> None:
    """When installed (e.g. via pipx), `alloy --help` runs without any checkout."""
    env = dict(os.environ)
    env.pop(ENV_VAR, None)
    result = subprocess.run(
        [sys.executable, "-m", "alloy_cli.main", "--help"],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=15,
    )
    assert result.returncode == 0, result.stderr
    assert "alloy" in result.stdout
