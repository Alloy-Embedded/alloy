"""Tests for the SDK manager.

We do not hit the real GitHub remotes. Each test creates two local "remote" git
repositories on disk, points the CLI's source URLs at them via ``ALLOY_HOME/config.toml``,
and exercises install/list/use/path through both the API and the ``alloy`` console
script entry point.
"""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

from alloy_cli import config, sdk
from alloy_cli.main import main as cli_main
from alloy_cli.runtime import find_runtime_root


def _git(*args: str, cwd: Path) -> None:
    subprocess.run(
        ["git", *args],
        cwd=cwd,
        check=True,
        capture_output=True,
        env={
            "GIT_AUTHOR_NAME": "test",
            "GIT_AUTHOR_EMAIL": "test@example.com",
            "GIT_COMMITTER_NAME": "test",
            "GIT_COMMITTER_EMAIL": "test@example.com",
            "PATH": __import__("os").environ.get("PATH", ""),
        },
    )


def _make_runtime_remote(path: Path, *, tag: str | None = None) -> None:
    path.mkdir(parents=True)
    _git("init", "--quiet", "--initial-branch=main", cwd=path)
    (path / "scripts").mkdir()
    (path / "cmake").mkdir()
    (path / "scripts" / "alloyctl.py").write_text(
        "def main():\n    print('alloyctl-from-sdk')\n    return 0\n"
    )
    (path / "cmake" / "board_manifest.cmake").write_text("# stub\n")
    _git("add", ".", cwd=path)
    _git("commit", "--quiet", "-m", "initial", cwd=path)
    if tag is not None:
        _git("tag", tag, cwd=path)


def _make_devices_remote(path: Path) -> None:
    path.mkdir(parents=True)
    _git("init", "--quiet", "--initial-branch=main", cwd=path)
    (path / "README.md").write_text("# devices stub\n")
    _git("add", ".", cwd=path)
    _git("commit", "--quiet", "-m", "initial", cwd=path)


@pytest.fixture
def alloy_home(tmp_path, monkeypatch):
    home = tmp_path / "home"
    monkeypatch.setenv(config.ENV_HOME, str(home))
    monkeypatch.delenv("ALLOY_ROOT", raising=False)
    return home


@pytest.fixture
def remotes(tmp_path):
    runtime = tmp_path / "remote-runtime.git"
    devices = tmp_path / "remote-devices.git"
    _make_runtime_remote(runtime, tag="v0.1.0")
    _make_devices_remote(devices)
    return runtime, devices


@pytest.fixture
def configured_sources(alloy_home, remotes):
    runtime, devices = remotes
    cfg = config.Config(
        active_version=None,
        sources=config.Sources(runtime=str(runtime), devices=str(devices)),
    )
    config.save(cfg)
    return cfg


def test_install_creates_versioned_layout(configured_sources, alloy_home):
    manifest = sdk.install("v0.1.0")

    runtime_dir = alloy_home / "sdk" / "v0.1.0" / "runtime"
    devices_dir = alloy_home / "sdk" / "v0.1.0" / "devices"
    assert (runtime_dir / "scripts" / "alloyctl.py").is_file()
    assert (devices_dir / "README.md").is_file()
    assert manifest.runtime_ref == "v0.1.0"
    assert len(manifest.runtime_sha) == 40
    assert len(manifest.devices_sha) == 40


def test_install_persists_manifest_and_activates_first_version(configured_sources):
    sdk.install("v0.1.0")
    cfg = config.load()
    assert cfg.active_version == "v0.1.0"
    loaded = config.load_manifest("v0.1.0")
    assert loaded is not None and loaded.version == "v0.1.0"


def test_list_versions_returns_installed(configured_sources):
    assert sdk.list_versions() == []
    sdk.install("v0.1.0")
    assert sdk.list_versions() == ["v0.1.0"]


def test_use_rejects_unknown_version(configured_sources):
    with pytest.raises(sdk.SdkError):
        sdk.use("v9.9.9")


def test_active_runtime_path_resolves_after_install(configured_sources):
    assert sdk.active_runtime_path() is None
    sdk.install("v0.1.0")
    path = sdk.active_runtime_path()
    assert path is not None and (path / "scripts" / "alloyctl.py").is_file()


def test_runtime_root_falls_back_to_active_sdk(configured_sources, tmp_path, monkeypatch):
    sdk.install("v0.1.0")
    monkeypatch.chdir(tmp_path)  # outside any checkout
    root = find_runtime_root()
    assert root == sdk.active_runtime_path()


def test_cli_sdk_install_and_path(configured_sources, capsys):
    rc = cli_main(["sdk", "install", "v0.1.0"])
    assert rc == 0
    out = capsys.readouterr().out
    assert "installed alloy v0.1.0" in out

    rc = cli_main(["sdk", "path"])
    assert rc == 0
    assert "v0.1.0" in capsys.readouterr().out


def test_cli_sdk_list_marks_active(configured_sources, capsys):
    cli_main(["sdk", "install", "v0.1.0"])
    capsys.readouterr()
    rc = cli_main(["sdk", "list"])
    assert rc == 0
    out = capsys.readouterr().out
    assert "* v0.1.0" in out


def test_cli_delegates_to_active_sdk(configured_sources, tmp_path, monkeypatch, capsys):
    cli_main(["sdk", "install", "v0.1.0"])
    capsys.readouterr()
    monkeypatch.chdir(tmp_path)  # outside any checkout
    rc = cli_main(["doctor"])
    assert rc == 0
    assert "alloyctl-from-sdk" in capsys.readouterr().out


def test_install_force_reinstalls(configured_sources, alloy_home):
    sdk.install("v0.1.0")
    runtime_dir = alloy_home / "sdk" / "v0.1.0" / "runtime"
    marker = runtime_dir / "marker"
    marker.write_text("untracked")
    sdk.install("v0.1.0", force=True)
    assert not marker.exists(), "force install should wipe the version directory"
