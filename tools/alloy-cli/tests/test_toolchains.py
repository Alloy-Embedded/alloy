"""Tests for the toolchain manager.

We do not download anything from the network. Each test:
- builds a tiny synthetic toolchain archive on disk (a `bin/<binary>` script),
- writes a pin file that points at it via a `file://` URL with the real sha256,
- exposes the pin file through ``ALLOY_TOOLCHAIN_PINS``,
- exercises install/list/which through both the API and the ``alloy`` console script.
"""

from __future__ import annotations

import hashlib
import tarfile
from pathlib import Path

import pytest

from alloy_cli import config, toolchains
from alloy_cli.main import main as cli_main


def _detect() -> str:
    return toolchains.detect_platform()


def _make_tarball(dest: Path, *, binary_name: str, body: str = "#!/bin/sh\necho ok\n") -> Path:
    """Write a tar.gz with a top-level ``pkg/`` dir containing ``bin/<binary_name>``."""
    work = dest.parent / f"{dest.name}.work"
    work.mkdir()
    pkg = work / "pkg"
    (pkg / "bin").mkdir(parents=True)
    binary = pkg / "bin" / binary_name
    binary.write_text(body)
    binary.chmod(0o755)
    with tarfile.open(dest, "w:gz") as tf:
        tf.add(pkg, arcname="pkg")
    return dest


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def _write_pins(path: Path, *, name: str, version: str, host: str, url: str, sha: str) -> None:
    path.write_text(
        f"""\
[{name}]
default_version = "{version}"
binary = "{name}"

[{name}.versions."{version}".{host}]
url = "{url}"
sha256 = "{sha}"
archive = "tar.gz"
strip_components = 1
bin_subdir = "bin"
"""
    )


@pytest.fixture
def alloy_home(tmp_path, monkeypatch):
    home = tmp_path / "home"
    monkeypatch.setenv(config.ENV_HOME, str(home))
    monkeypatch.delenv("ALLOY_ROOT", raising=False)
    return home


@pytest.fixture
def fake_toolchain(tmp_path, monkeypatch, alloy_home):
    archive = _make_tarball(tmp_path / "fake-arm.tar.gz", binary_name="arm-none-eabi-gcc")
    sha = _sha256(archive)
    pins = tmp_path / "pins.toml"
    _write_pins(
        pins,
        name="arm-none-eabi-gcc",
        version="1.0.0",
        host=_detect(),
        url=archive.as_uri(),
        sha=sha,
    )
    monkeypatch.setenv("ALLOY_TOOLCHAIN_PINS", str(pins))
    return {"archive": archive, "sha": sha, "pins": pins}


def test_detect_platform_returns_known_value():
    host = toolchains.detect_platform()
    assert host.startswith(("darwin", "linux"))


def test_resolve_pin_uses_default_version(fake_toolchain):
    pin = toolchains.resolve_pin("arm-none-eabi-gcc")
    assert pin.version == "1.0.0"
    assert pin.sha256 == fake_toolchain["sha"]


def test_resolve_pin_unknown_toolchain(fake_toolchain):
    with pytest.raises(toolchains.ToolchainError):
        toolchains.resolve_pin("nope")


def test_install_extracts_and_records(fake_toolchain, alloy_home):
    record = toolchains.install("arm-none-eabi-gcc")
    assert record.bin_dir == alloy_home / "toolchains" / "arm-none-eabi-gcc" / "1.0.0" / "bin"
    assert (record.bin_dir / "arm-none-eabi-gcc").is_file()
    assert (record.install_dir / ".alloy_install.json").is_file()


def test_install_refuses_when_already_present(fake_toolchain):
    toolchains.install("arm-none-eabi-gcc")
    with pytest.raises(toolchains.ToolchainError):
        toolchains.install("arm-none-eabi-gcc")


def test_install_force_overwrites(fake_toolchain, alloy_home):
    toolchains.install("arm-none-eabi-gcc")
    target = alloy_home / "toolchains" / "arm-none-eabi-gcc" / "1.0.0"
    marker = target / "marker"
    marker.write_text("x")
    toolchains.install("arm-none-eabi-gcc", force=True)
    assert not marker.exists()


def test_install_rejects_todo_pin(tmp_path, monkeypatch, alloy_home):
    pins = tmp_path / "pins.toml"
    _write_pins(
        pins,
        name="arm-none-eabi-gcc",
        version="1.0.0",
        host=_detect(),
        url="file:///does/not/matter",
        sha="TODO",
    )
    monkeypatch.setenv("ALLOY_TOOLCHAIN_PINS", str(pins))
    with pytest.raises(toolchains.ToolchainError, match="TODO"):
        toolchains.install("arm-none-eabi-gcc")


def test_install_aborts_on_checksum_mismatch(tmp_path, monkeypatch, alloy_home):
    archive = _make_tarball(tmp_path / "arm.tar.gz", binary_name="arm-none-eabi-gcc")
    pins = tmp_path / "pins.toml"
    bogus = "0" * 64
    _write_pins(
        pins,
        name="arm-none-eabi-gcc",
        version="1.0.0",
        host=_detect(),
        url=archive.as_uri(),
        sha=bogus,
    )
    monkeypatch.setenv("ALLOY_TOOLCHAIN_PINS", str(pins))
    with pytest.raises(toolchains.ToolchainError, match="checksum mismatch"):
        toolchains.install("arm-none-eabi-gcc")


def test_list_installed_after_install(fake_toolchain):
    assert toolchains.list_installed() == []
    toolchains.install("arm-none-eabi-gcc")
    assert toolchains.list_installed() == [("arm-none-eabi-gcc", "1.0.0")]


def test_which_returns_binary_path(fake_toolchain):
    toolchains.install("arm-none-eabi-gcc")
    path = toolchains.which("arm-none-eabi-gcc")
    assert path.name == "arm-none-eabi-gcc"
    assert path.is_file()


def test_which_when_not_installed(fake_toolchain):
    with pytest.raises(toolchains.ToolchainError, match="not installed"):
        toolchains.which("arm-none-eabi-gcc")


def test_cli_toolchain_install_and_list(fake_toolchain, capsys):
    rc = cli_main(["toolchain", "install", "arm-none-eabi-gcc"])
    assert rc == 0
    out = capsys.readouterr().out
    assert "installed arm-none-eabi-gcc@1.0.0" in out

    rc = cli_main(["toolchain", "list"])
    assert rc == 0
    assert "arm-none-eabi-gcc@1.0.0" in capsys.readouterr().out


def test_cli_toolchain_which(fake_toolchain, capsys):
    cli_main(["toolchain", "install", "arm-none-eabi-gcc"])
    capsys.readouterr()
    rc = cli_main(["toolchain", "which", "arm-none-eabi-gcc"])
    assert rc == 0
    assert "arm-none-eabi-gcc" in capsys.readouterr().out


def test_cli_toolchain_install_versioned_spec(fake_toolchain, capsys):
    rc = cli_main(["toolchain", "install", "arm-none-eabi-gcc@1.0.0"])
    assert rc == 0
    assert "1.0.0" in capsys.readouterr().out


def test_shipped_pins_file_parses():
    """The pin file shipped in the package must be valid TOML."""
    import sys

    if sys.version_info >= (3, 11):
        import tomllib  # type: ignore[import-not-found]
    else:
        import tomli as tomllib  # type: ignore[no-redef]
    data = tomllib.loads(toolchains._shipped_pins_path().read_text(encoding="utf-8"))
    assert "arm-none-eabi-gcc" in data
    assert "openocd" in data
    # Espressif toolchain pins covering ESP32-C3 (RISC-V) and ESP32-S3 (Xtensa LX7).
    assert "xtensa-esp32s3-elf-gcc" in data
    assert "riscv32-esp-elf-gcc" in data


def test_install_xtensa_esp32s3_toolchain_via_synthetic_pin(tmp_path, monkeypatch, alloy_home):
    """Exercise the install/list/which path for an Espressif-style pin.

    We do not hit dl.espressif.com; instead we build a tar.gz with the right binary
    name, point a pin override at it via ``ALLOY_TOOLCHAIN_PINS``, and assert the
    install lands in the per-version cache with a usable ``which`` lookup.
    """
    archive = _make_tarball(tmp_path / "xtensa.tar.gz", binary_name="xtensa-esp32s3-elf-gcc")
    sha = _sha256(archive)
    pins = tmp_path / "pins.toml"
    _write_pins(
        pins,
        name="xtensa-esp32s3-elf-gcc",
        version="esp-test",
        host=_detect(),
        url=archive.as_uri(),
        sha=sha,
    )
    monkeypatch.setenv("ALLOY_TOOLCHAIN_PINS", str(pins))

    record = toolchains.install("xtensa-esp32s3-elf-gcc")
    assert record.bin_dir == (
        alloy_home / "toolchains" / "xtensa-esp32s3-elf-gcc" / "esp-test" / "bin"
    )
    assert (record.bin_dir / "xtensa-esp32s3-elf-gcc").is_file()

    binary = toolchains.which("xtensa-esp32s3-elf-gcc")
    assert binary.name == "xtensa-esp32s3-elf-gcc"
