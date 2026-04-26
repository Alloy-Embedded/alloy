"""Tests for `alloy new` scaffolding (custom-board path).

Each test installs a synthetic SDK into a temp ALLOY_HOME. The synthetic SDK ships:
- a stub runtime (CMakeLists.txt + scripts/alloyctl.py + cmake/board_manifest.cmake),
- a `boards/<name>/` directory whose layout mirrors the real in-tree boards so the
  copy-from-SDK path can be exercised end to end,
- an `alloy-devices` checkout next to the runtime, with a small descriptor tree we use
  to validate the raw-MCU resolution path.

We assert on generated tree contents, never on a real CMake build.
"""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path

import pytest

from alloy_cli import config, scaffold
from alloy_cli.main import main as cli_main


# --- fixtures --------------------------------------------------------------------------


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
            "PATH": os.environ.get("PATH", ""),
        },
    )


def _make_runtime_remote(path: Path, *, tag: str = "v0.1.0") -> None:
    path.mkdir(parents=True)
    _git("init", "--quiet", "--initial-branch=main", cwd=path)
    (path / "scripts").mkdir()
    (path / "cmake").mkdir()
    (path / "scripts" / "alloyctl.py").write_text("def main(): return 0\n")
    # Fake board manifest fragment: just enough for arch lookup to succeed.
    (path / "cmake" / "board_manifest.cmake").write_text(
        '# fake manifest\n'
        'BOARD_NAME STREQUAL "nucleo_g071rb"\n'
        'set(_arch "cortex-m0plus")\n'
        'BOARD_NAME STREQUAL "esp32c3_devkitm"\n'
        'set(_arch "riscv32")\n'
        'BOARD_NAME STREQUAL "esp32s3_devkitc"\n'
        'set(_arch "xtensa-lx7")\n'
    )
    (path / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\nproject(alloy_stub)\n"
    )

    # Synthetic boards/<name>/ — mirrors the real in-tree shape.
    for board_name, header_body in (
        ("nucleo_g071rb", "namespace board { void init(); }"),
        ("same70_xplained", "namespace board { void init(); }"),
        ("raspberry_pi_pico", "namespace board { void init(); }"),
        ("esp32c3_devkitm", "namespace board { void init(); }"),
        ("esp32s3_devkitc", "namespace board { void init(); }"),
    ):
        bd = path / "boards" / board_name
        bd.mkdir(parents=True)
        (bd / "board.hpp").write_text(f"// stub for {board_name}\n{header_body}\n")
        (bd / "board_config.hpp").write_text(f"// stub config for {board_name}\n")
        (bd / "board.cpp").write_text("void board::init() {}\n")
        (bd / "syscalls.cpp").write_text("// stub syscalls\n")

    # Linker script naming follows the real convention (MCU.ld) for boards that ship
    # one. ESP32 boards have no in-tree linker scripts (the runtime relies on
    # vendor-supplied linker fragments), so the scaffold layer falls back to the
    # generic linker.ld name driven by the descriptor template.
    (path / "boards" / "nucleo_g071rb" / "STM32G071RBT6.ld").write_text("/* stub */\n")
    (path / "boards" / "same70_xplained" / "ATSAME70Q21.ld").write_text("/* stub */\n")
    (path / "boards" / "raspberry_pi_pico" / "rp2040.ld").write_text("/* stub */\n")

    _git("add", ".", cwd=path)
    _git("commit", "--quiet", "-m", "initial", cwd=path)
    _git("tag", tag, cwd=path)


def _make_devices_remote(path: Path) -> None:
    path.mkdir(parents=True)
    _git("init", "--quiet", "--initial-branch=main", cwd=path)
    # st/stm32g0/stm32g071rb -- known-board path
    g071 = path / "st" / "stm32g0" / "generated" / "runtime" / "devices" / "stm32g071rb"
    g071.mkdir(parents=True)
    (g071 / "capabilities.json").write_text(
        json.dumps(
            {
                "memory": {
                    "regions": [
                        {"kind": "flash", "origin": "0x08000000", "length": 131072},
                        {"kind": "ram", "origin": "0x20000000", "length": 36864},
                    ]
                },
                "arch": "cortex-m0plus",
            }
        )
    )
    # st/stm32g4/stm32g474re -- raw-MCU path (no in-tree board)
    g474 = path / "st" / "stm32g4" / "generated" / "runtime" / "devices" / "stm32g474re"
    g474.mkdir(parents=True)
    (g474 / "capabilities.json").write_text(
        json.dumps(
            {
                "memory": [
                    {"kind": "flash", "origin": "0x08000000", "length": 524288},
                    {"kind": "ram", "origin": "0x20000000", "length": 131072},
                ],
                "arch": "cortex-m4",
            }
        )
    )
    # st/stm32u5/stm32u575xx -- descriptor lacking memory data, exercises warnings
    u575 = path / "st" / "stm32u5" / "generated" / "runtime" / "devices" / "stm32u575xx"
    u575.mkdir(parents=True)
    (u575 / "capabilities.json").write_text(json.dumps({"arch": "cortex-m4"}))

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


def test_find_board_by_mcu_matches_case_insensitively():
    board = scaffold.find_board_by_mcu("stm32g071rbt6")
    assert board is not None and board.name == "nucleo_g071rb"


def test_find_board_by_mcu_returns_none_for_unknown():
    assert scaffold.find_board_by_mcu("STM32X9999") is None


# --- copy-from-SDK path (--board) ------------------------------------------------------


def test_scaffold_with_board_copies_in_tree_files(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    result = scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)
    assert (dest / "board" / "board.hpp").is_file()
    assert (dest / "board" / "board.cpp").is_file()
    assert (dest / "board" / "syscalls.cpp").is_file()
    assert (dest / "board" / "board_config.hpp").is_file()
    assert (dest / "board" / "STM32G071RBT6.ld").is_file()
    assert result.layer.linker_script_name == "STM32G071RBT6.ld"
    assert result.layer.sources == ("board.cpp", "syscalls.cpp")


def test_cmakelists_uses_custom_board_contract(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)
    body = (dest / "CMakeLists.txt").read_text()
    assert 'set(ALLOY_BOARD "custom"' in body
    assert "ALLOY_CUSTOM_BOARD_HEADER" in body
    assert "ALLOY_CUSTOM_LINKER_SCRIPT" in body
    assert "ALLOY_DEVICE_VENDOR" in body and '"st"' in body
    assert "ALLOY_DEVICE_NAME" in body and '"stm32g071rb"' in body
    assert 'ALLOY_DEVICE_ARCH' in body and '"cortex-m0plus"' in body
    assert "add_subdirectory(${ALLOY_ROOT}" in body
    assert "board/board.cpp" in body
    assert "board/syscalls.cpp" in body


def test_presets_no_longer_set_alloy_board(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)
    presets = json.loads((dest / "CMakePresets.json").read_text())
    debug = next(p for p in presets["configurePresets"] if p["name"] == "debug")
    assert "ALLOY_BOARD" not in debug["cacheVariables"]
    assert "ALLOY_ROOT" in debug["cacheVariables"]


def test_vscode_files_remain_valid_json(installed_sdk, tmp_path):
    dest = tmp_path / "myproj"
    scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)
    for name in ("settings.json", "tasks.json", "launch.json"):
        json.loads((dest / ".vscode" / name).read_text())


# --- --mcu with catalog match ----------------------------------------------------------


def test_scaffold_with_mcu_aliases_to_known_board(installed_sdk, tmp_path):
    dest = tmp_path / "viaomcu"
    result = scaffold.scaffold(mcu="STM32G071RBT6", destination=dest)
    assert (dest / "board" / "STM32G071RBT6.ld").is_file()
    assert result.layer.vendor == "st"
    assert result.layer.device == "stm32g071rb"


# --- --mcu raw (no in-tree board) ------------------------------------------------------


def test_scaffold_raw_mcu_generates_skeleton(installed_sdk, tmp_path, monkeypatch):
    # chdir outside any real alloy checkout so walk-up does not shadow the
    # synthetic SDK (which is the only place our raw MCU descriptor lives).
    monkeypatch.chdir(tmp_path)
    dest = tmp_path / "g474proj"
    result = scaffold.scaffold(mcu="STM32G474RET6", destination=dest)

    for name in ("board.hpp", "board_config.hpp", "board.cpp", "syscalls.cpp", "linker.ld"):
        assert (dest / "board" / name).is_file(), f"missing skeleton file board/{name}"
    assert result.layer.linker_script_name == "linker.ld"
    assert result.layer.vendor == "st"
    assert result.layer.family == "stm32g4"
    assert result.layer.device == "stm32g474re"
    assert result.layer.flash_size_bytes == 524288
    assert result.warnings == ()  # descriptor had memory data

    body = (dest / "board" / "board_config.hpp").read_text()
    assert "TODO" in body
    ld = (dest / "board" / "linker.ld").read_text()
    assert "0x08000000" in ld and "512K" in ld


def test_scaffold_raw_mcu_warns_when_descriptor_lacks_memory(
    installed_sdk, tmp_path, monkeypatch
):
    monkeypatch.chdir(tmp_path)
    dest = tmp_path / "u575proj"
    result = scaffold.scaffold(mcu="STM32U575XX", destination=dest)
    assert any("did not declare memory regions" in w for w in result.warnings)
    ld = (dest / "board" / "linker.ld").read_text()
    assert "TODO" in ld


def test_scaffold_raw_mcu_unknown_to_devices(installed_sdk, tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)
    dest = tmp_path / "noproject"
    with pytest.raises(scaffold.ScaffoldError, match="no descriptor"):
        scaffold.scaffold(mcu="MADEUP9999", destination=dest)


# --- preflight and validation ----------------------------------------------------------


def test_scaffold_requires_either_board_or_mcu(installed_sdk, tmp_path):
    dest = tmp_path / "x"
    with pytest.raises(scaffold.ScaffoldError, match="exactly one"):
        scaffold.scaffold(destination=dest)


def test_scaffold_rejects_both_board_and_mcu(installed_sdk, tmp_path):
    dest = tmp_path / "x"
    with pytest.raises(scaffold.ScaffoldError, match="exactly one"):
        scaffold.scaffold(board_name="nucleo_g071rb", mcu="STM32G071RBT6", destination=dest)


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


def test_preflight_requires_sdk_or_explicit_root(alloy_home, tmp_path, monkeypatch):
    """When no SDK, no env var, and no walk-up succeed, scaffold must fail clearly."""
    monkeypatch.chdir(tmp_path)  # outside any alloy checkout
    dest = tmp_path / "x"
    with pytest.raises(scaffold.ScaffoldError, match="could not locate an Alloy runtime"):
        scaffold.scaffold(board_name="nucleo_g071rb", destination=dest)


# --- CLI integration -------------------------------------------------------------------


def test_cli_new_with_board(installed_sdk, tmp_path, capsys):
    dest = tmp_path / "viacli"
    rc = cli_main(["new", str(dest), "--board", "nucleo_g071rb"])
    assert rc == 0
    assert (dest / "board" / "board.cpp").is_file()


def test_cli_new_with_mcu_known(installed_sdk, tmp_path, capsys):
    dest = tmp_path / "mcucli"
    rc = cli_main(["new", str(dest), "--mcu", "STM32G071RBT6"])
    assert rc == 0
    assert (dest / "board" / "STM32G071RBT6.ld").is_file()


def test_cli_new_with_mcu_raw(installed_sdk, tmp_path, monkeypatch, capsys):
    monkeypatch.chdir(tmp_path)
    dest = tmp_path / "rawmcu"
    rc = cli_main(["new", str(dest), "--mcu", "STM32G474RET6"])
    assert rc == 0
    assert (dest / "board" / "linker.ld").is_file()


def test_cli_new_rejects_both_targets(installed_sdk, tmp_path, capsys):
    dest = tmp_path / "x"
    # argparse's mutually-exclusive group surfaces this via SystemExit(2).
    with pytest.raises(SystemExit):
        cli_main(["new", str(dest), "--board", "nucleo_g071rb", "--mcu", "STM32G071RBT6"])


def test_cli_boards_lists_with_mcu(installed_sdk, capsys):
    rc = cli_main(["boards"])
    assert rc == 0
    out = capsys.readouterr().out
    assert "STM32G071RBT6" in out


# --- ESP32-C3 / ESP32-S3 (catalog + toolchain mapping) ---------------------------------


def test_scaffold_esp32c3_catalog_match(installed_sdk, tmp_path, monkeypatch):
    # chdir outside any real alloy checkout so walk-up does not shadow the
    # synthetic SDK (whose esp32c3 board ships no linker script -- the test
    # depends on the placeholder fallback firing).
    monkeypatch.chdir(tmp_path)
    dest = tmp_path / "esp32c3proj"
    result = scaffold.scaffold(board_name="esp32c3_devkitm", destination=dest)
    assert result.layer.vendor == "espressif"
    assert result.layer.family == "esp32c3"
    assert result.layer.arch == "riscv32"
    assert result.layer.toolchain == "riscv32-esp-elf-gcc"
    # The synthetic in-tree board ships no linker script -> placeholder is rendered.
    assert (dest / "board" / "linker.ld").is_file()
    assert any("does not ship a linker script" in w for w in result.warnings)


def test_scaffold_esp32s3_catalog_match(installed_sdk, tmp_path):
    dest = tmp_path / "esp32s3proj"
    result = scaffold.scaffold(board_name="esp32s3_devkitc", destination=dest)
    assert result.layer.arch == "xtensa-lx7"
    assert result.layer.toolchain == "xtensa-esp-elf-gcc"
    assert any("does not ship a linker script" in w for w in result.warnings)


def test_scaffold_with_mcu_esp32c3_aliases_to_board(installed_sdk, tmp_path):
    dest = tmp_path / "viacli3"
    result = scaffold.scaffold(mcu="ESP32-C3", destination=dest)
    assert result.layer.family == "esp32c3"
    assert result.layer.toolchain == "riscv32-esp-elf-gcc"


def test_scaffold_with_mcu_esp32s3_aliases_to_board(installed_sdk, tmp_path):
    dest = tmp_path / "viaclis3"
    result = scaffold.scaffold(mcu="ESP32-S3", destination=dest)
    assert result.layer.family == "esp32s3"
    assert result.layer.toolchain == "xtensa-esp-elf-gcc"


def test_cli_boards_lists_esp_targets(installed_sdk, capsys):
    rc = cli_main(["boards"])
    assert rc == 0
    out = capsys.readouterr().out
    assert "esp32c3_devkitm" in out and "ESP32-C3" in out
    assert "esp32s3_devkitc" in out and "ESP32-S3" in out


def test_toolchain_for_arch_maps_xtensa_and_riscv():
    # Both Xtensa variants resolve to the unified Espressif crosstool.
    assert scaffold._toolchain_for_arch("xtensa-lx6") == "xtensa-esp-elf-gcc"
    assert scaffold._toolchain_for_arch("xtensa-lx7") == "xtensa-esp-elf-gcc"
    assert scaffold._toolchain_for_arch("riscv32") == "riscv32-esp-elf-gcc"
