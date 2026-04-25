"""Tests for `alloy new` scaffolding.

Each test installs a synthetic SDK (a minimal "Alloy checkout" with the right sentinel
files plus a CMakeLists.txt) into a temp ALLOY_HOME and asks the scaffolder to generate
a project against it. We assert on the contents of the generated tree, not on a real
CMake build.
"""

from __future__ import annotations

import json
import subprocess
from pathlib import Path

import pytest

from alloy_cli import config, scaffold
from alloy_cli.main import main as cli_main


def _git(*args: str, cwd: Path) -> None:
    import os

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
            "PATH": os.environ.get("PATH", ""),
        },
    )


def _make_runtime_remote(path: Path, *, tag: str = "v0.1.0") -> None:
    path.mkdir(parents=True)
    _git("init", "--quiet", "--initial-branch=main", cwd=path)
    (path / "scripts").mkdir()
    (path / "cmake").mkdir()
    (path / "scripts" / "alloyctl.py").write_text("def main(): return 0\n")
    (path / "cmake" / "board_manifest.cmake").write_text("# stub\n")
    (path / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\nproject(alloy_stub)\n"
    )
    _git("add", ".", cwd=path)
    _git("commit", "--quiet", "-m", "initial", cwd=path)
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
def installed_sdk(tmp_path, alloy_home):
    from alloy_cli import sdk as sdk_mod

    runtime_remote = tmp_path / "remote-runtime.git"
    devices_remote = tmp_path / "remote-devices.git"
    _make_runtime_remote(runtime_remote)
    _make_devices_remote(devices_remote)
    cfg = config.Config(
        active_version=None,
        sources=config.Sources(runtime=str(runtime_remote), devices=str(devices_remote)),
    )
    config.save(cfg)
    sdk_mod.install("v0.1.0")
    return alloy_home / "sdk" / "v0.1.0" / "runtime"


# --- catalog ---------------------------------------------------------------------------


def test_load_boards_includes_known_targets():
    boards = scaffold.load_boards()
    for name in ("nucleo_g071rb", "same70_xplained", "raspberry_pi_pico"):
        assert name in boards


def test_get_board_unknown_raises():
    with pytest.raises(scaffold.ScaffoldError, match="unknown board"):
        scaffold.get_board("not_a_board")


def test_get_board_returns_metadata():
    board = scaffold.get_board("nucleo_g071rb")
    assert board.toolchain == "arm-none-eabi-gcc"
    assert board.has_openocd
    assert board.openocd_config_files


# --- preflight -------------------------------------------------------------------------


def test_preflight_requires_sdk_or_explicit_root(alloy_home):
    board = scaffold.get_board("nucleo_g071rb")
    with pytest.raises(scaffold.ScaffoldError, match="no active SDK"):
        scaffold.preflight(board)


def test_preflight_resolves_explicit_alloy_root(installed_sdk):
    board = scaffold.get_board("nucleo_g071rb")
    pf = scaffold.preflight(board, alloy_root=installed_sdk)
    assert pf.alloy_root == installed_sdk.resolve()
    assert pf.toolchain_bin is None  # no toolchain installed in this fixture


# --- scaffolding -----------------------------------------------------------------------


def test_scaffold_creates_full_project_tree(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    result = scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)
    assert result.project == "myproj"
    assert (dest / "CMakeLists.txt").is_file()
    assert (dest / "CMakePresets.json").is_file()
    assert (dest / "src" / "main.cpp").is_file()
    assert (dest / ".gitignore").is_file()
    assert (dest / "README.md").is_file()
    for name in ("settings.json", "tasks.json", "launch.json"):
        assert (dest / ".vscode" / name).is_file()


def test_scaffold_bakes_alloy_root_into_cmakelists(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)
    body = (dest / "CMakeLists.txt").read_text()
    assert str(installed_sdk) in body
    assert 'set(ALLOY_BOARD "nucleo_g071rb"' in body
    assert "add_subdirectory(${ALLOY_ROOT}" in body


def test_scaffold_emits_valid_presets_json(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)
    presets = json.loads((dest / "CMakePresets.json").read_text())
    names = {p["name"] for p in presets["configurePresets"]}
    assert names == {"debug", "release"}
    debug = next(p for p in presets["configurePresets"] if p["name"] == "debug")
    assert debug["cacheVariables"]["ALLOY_BOARD"] == "nucleo_g071rb"


def test_scaffold_emits_valid_vscode_json(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)
    for name in ("settings.json", "tasks.json", "launch.json"):
        json.loads((dest / ".vscode" / name).read_text())  # must parse


def test_scaffold_launch_json_has_openocd_for_board_with_probe(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)
    body = (dest / ".vscode" / "launch.json").read_text()
    assert "OpenOCD" in body
    assert ":3333" in body


def test_scaffold_rejects_invalid_project_name(installed_sdk, tmp_path):
    dest = tmp_path / "1bad-name"
    with pytest.raises(scaffold.ScaffoldError, match="invalid project name"):
        scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)


def test_scaffold_rejects_non_empty_destination(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    dest.mkdir()
    (dest / "junk").write_text("x")
    with pytest.raises(scaffold.ScaffoldError, match="not empty"):
        scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)


def test_scaffold_overrides_project_name(installed_sdk, tmp_path):
    dest = tmp_path / "weird-name"
    result = scaffold.scaffold(
        board_name="nucleo_g071rb", destination=dest, project_name="my_custom"
    )
    assert result.project == "my_custom"
    assert "project(my_custom" in (dest / "CMakeLists.txt").read_text()


# --- CLI integration -------------------------------------------------------------------


def test_cli_new_scaffolds(installed_sdk, tmp_path, capsys):
    dest = tmp_path / "viacli"
    rc = cli_main(["new", str(dest), "--board", "nucleo_g071rb"])
    assert rc == 0
    out = capsys.readouterr().out
    assert "scaffolded" in out
    assert (dest / "CMakeLists.txt").is_file()


def test_cli_new_without_sdk_returns_clear_error(alloy_home, tmp_path, capsys):
    dest = tmp_path / "noproject"
    rc = cli_main(["new", str(dest), "--board", "nucleo_g071rb"])
    assert rc == 1
    err = capsys.readouterr().err
    assert "no active SDK" in err
    assert not dest.exists() or not any(dest.iterdir())


def test_cli_new_unknown_board(installed_sdk, tmp_path, capsys):
    dest = tmp_path / "x"
    rc = cli_main(["new", str(dest), "--board", "bogus_board"])
    assert rc == 1
    assert "unknown board" in capsys.readouterr().err


def test_cli_boards_lists_known(capsys, alloy_home):
    rc = cli_main(["boards"])
    assert rc == 0
    out = capsys.readouterr().out
    assert "nucleo_g071rb" in out
    assert "same70_xplained" in out


def test_cli_new_accepts_explicit_alloy_root(tmp_path, alloy_home):
    runtime_remote = tmp_path / "alloy-checkout"
    _make_runtime_remote(runtime_remote)
    dest = tmp_path / "explicit"
    rc = cli_main(
        ["new", str(dest), "--board", "nucleo_g071rb", "--alloy-root", str(runtime_remote)]
    )
    assert rc == 0
    body = (dest / "CMakeLists.txt").read_text()
    assert str(runtime_remote.resolve()) in body
