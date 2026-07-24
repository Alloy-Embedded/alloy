"""Unit tests for `alloy lib` — registry parsing, discovery, and vendoring."""

from __future__ import annotations

import tomllib
from pathlib import Path

import pytest

from alloy_cli import libs
from alloy_cli.project import ProjectError

ALLOY_ROOT = Path(__file__).resolve().parents[3]  # .../alloy


def test_registry_parses_and_has_sht31() -> None:
    reg = libs._registry(ALLOY_ROOT)
    assert "sht31" in reg
    assert reg["sht31"]["category"] == "sensor"
    assert "I2cBus" in reg["sht31"]["concepts"]


def test_every_registry_entry_has_the_required_fields() -> None:
    for name, spec in libs._registry(ALLOY_ROOT).items():
        for field in ("version", "category", "summary", "concepts", "source"):
            assert field in spec, f"{name} missing '{field}'"
        # A path: source must exist and carry a manifest.
        kind, _, ref = spec["source"].partition(":")
        if kind == "path":
            assert (ALLOY_ROOT / ref / "alloy.lib.toml").exists(), f"{name}: {ref} has no manifest"


def test_every_library_manifest_matches_its_registry_entry() -> None:
    reg = libs._registry(ALLOY_ROOT)
    for name, spec in reg.items():
        kind, _, ref = spec["source"].partition(":")
        if kind != "path":
            continue
        manifest = tomllib.loads((ALLOY_ROOT / ref / "alloy.lib.toml").read_text())["lib"]
        assert manifest["name"] == name
        assert manifest["version"] == spec["version"], f"{name}: version drift registry vs manifest"


def test_search_matches_summary_and_category() -> None:
    assert libs.cmd_lib_search(ALLOY_ROOT, "humidity") == 0
    assert libs.cmd_lib_search(ALLOY_ROOT, "sensor") == 0
    assert libs.cmd_lib_search(ALLOY_ROOT, "no-such-thing-xyz") == 1


def test_info_rejects_unknown_lib() -> None:
    assert libs.cmd_lib_info(ALLOY_ROOT, "does-not-exist") == 1


def test_record_in_toml_creates_and_dedupes(tmp_path: Path) -> None:
    toml = tmp_path / "alloy.toml"
    toml.write_text('[project]\nname = "x"\n\n[board]\nid = "nucleo_g071rb"\n')
    libs._record_in_toml(toml, "sht31", "0.1.0")
    data = tomllib.loads(toml.read_text())
    assert data["libs"]["sht31"] == "0.1.0"
    # Idempotent: adding again doesn't duplicate.
    libs._record_in_toml(toml, "sht31", "0.1.0")
    assert toml.read_text().count("sht31") == 1


def test_add_rejects_unknown_lib(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("ALLOY_ROOT", str(ALLOY_ROOT))
    (tmp_path / "alloy.toml").write_text('[project]\nname="x"\n[board]\nid="nucleo_g071rb"\n')
    assert libs.cmd_lib_add(tmp_path, "does-not-exist") == 1


def test_add_vendors_and_records(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("ALLOY_ROOT", str(ALLOY_ROOT))
    (tmp_path / "alloy.toml").write_text('[project]\nname="x"\n[board]\nid="nucleo_g071rb"\n')
    assert libs.cmd_lib_add(tmp_path, "sht31") == 0
    assert (tmp_path / "libs" / "sht31" / "include" / "sht31.hpp").exists()
    data = tomllib.loads((tmp_path / "alloy.toml").read_text())
    assert data["libs"]["sht31"] == "0.1.0"


def test_resolve_source_rejects_unknown_kind() -> None:
    with pytest.raises(ProjectError):
        libs._resolve_source(ALLOY_ROOT, "x", "ftp:whatever")
