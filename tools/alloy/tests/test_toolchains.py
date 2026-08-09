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


# ---------------------------------------------------------------------------
# Air-gapped installs (`--from` / ALLOY_TOOLS_MIRROR / `--offline`).
#
# The point of a mirror is that WHERE the bytes come from stops mattering: the
# pinned sha256 is the whole security property, and it is applied identically
# to a local file and to a download. These tests drive install_tool() against a
# synthetic one-tool manifest and a real tarball on disk, so they exercise the
# actual verify/extract path — nothing here is allowed to reach the network,
# and _download is stubbed to blow up if anything tries.
# ---------------------------------------------------------------------------

import hashlib
import tarfile

from alloy_cli.emit.common import EmitError


def _make_archive(directory: Path, name: str = "fake-tool-1.0.tar.gz") -> tuple[Path, str]:
    """A minimal but REAL tarball with the layout install_tool expects
    (one top-level dir carrying bin/faketool)."""
    payload = directory / "payload" / "fake-tool-1.0" / "bin"
    payload.mkdir(parents=True)
    (payload / "faketool").write_text("#!/bin/sh\necho fake\n")
    archive = directory / name
    with tarfile.open(archive, "w:gz") as tf:
        tf.add(payload.parent, arcname="fake-tool-1.0")
    return archive, hashlib.sha256(archive.read_bytes()).hexdigest()


def _stub_manifest(monkeypatch, url: str, sha: str) -> None:
    plat = toolchains._platform_key()
    monkeypatch.setattr(toolchains, "_manifest", lambda: {
        "tools": {
            "fake-tool": {
                "check": "faketool",
                "families": ["*"],
                "kind": "archive",
                "platforms": {plat: {"url": url, "sha256": sha,
                                     "strip_prefix": "fake-tool-1.0",
                                     "bin_subdir": "bin"}},
            }
        }
    })


def _no_network(monkeypatch) -> None:
    def boom(*_args, **_kwargs):
        raise AssertionError("the offline path must not touch the network")
    monkeypatch.setattr(toolchains, "_download", boom)


def test_install_from_mirror_verifies_and_extracts(tmp_path, monkeypatch) -> None:
    mirror = tmp_path / "mirror"
    mirror.mkdir()
    archive, sha = _make_archive(mirror)
    _stub_manifest(monkeypatch, f"https://example.invalid/{archive.name}", sha)
    monkeypatch.setattr(toolchains, "TOOLS_DIR", tmp_path / "tools")
    _no_network(monkeypatch)

    dest = toolchains.install_tool("fake-tool", mirror=mirror, offline=True)
    assert (dest / "bin" / "faketool").is_file()


def test_mirror_archive_with_a_wrong_digest_is_refused(tmp_path, monkeypatch) -> None:
    mirror = tmp_path / "mirror"
    mirror.mkdir()
    archive, sha = _make_archive(mirror)
    # Pin a digest the mirrored bytes do not have — a poisoned USB stick.
    _stub_manifest(monkeypatch, f"https://example.invalid/{archive.name}", "0" * 64)
    monkeypatch.setattr(toolchains, "TOOLS_DIR", tmp_path / "tools")
    _no_network(monkeypatch)

    with pytest.raises(EmitError, match="sha256 mismatch"):
        toolchains.install_tool("fake-tool", mirror=mirror, offline=True)
    assert not (tmp_path / "tools" / "fake-tool").exists()
    assert sha != "0" * 64  # the fixture really did produce a different digest


def test_missing_from_mirror_names_the_fetch_command(tmp_path, monkeypatch) -> None:
    mirror = tmp_path / "empty"
    mirror.mkdir()
    _stub_manifest(monkeypatch, "https://example.invalid/absent.tar.gz", "0" * 64)
    monkeypatch.setattr(toolchains, "TOOLS_DIR", tmp_path / "tools")
    _no_network(monkeypatch)

    with pytest.raises(EmitError) as excinfo:
        toolchains.install_tool("fake-tool", mirror=mirror, offline=True)
    message = str(excinfo.value)
    assert "absent.tar.gz" in message
    assert "alloy setup --fetch" in message


def test_offline_without_a_mirror_names_the_mirror_option(tmp_path, monkeypatch) -> None:
    _stub_manifest(monkeypatch, "https://example.invalid/absent.tar.gz", "0" * 64)
    monkeypatch.setattr(toolchains, "TOOLS_DIR", tmp_path / "tools")
    _no_network(monkeypatch)

    with pytest.raises(EmitError) as excinfo:
        toolchains.install_tool("fake-tool", offline=True)
    message = str(excinfo.value)
    assert "--from" in message
    assert "alloy setup --fetch" in message
    assert "ALLOY_TOOLS_MIRROR" in message


def test_unreachable_network_names_the_mirror_option(tmp_path, monkeypatch) -> None:
    """The message a build server with no route actually sees. `_download` is
    the real one here; only urlopen is replaced, by something that fails the
    way a firewalled host fails."""
    _stub_manifest(monkeypatch, "https://example.invalid/absent.tar.gz", "0" * 64)
    monkeypatch.setattr(toolchains, "TOOLS_DIR", tmp_path / "tools")

    def refuse(*_args, **_kwargs):
        raise OSError("Network is unreachable")
    monkeypatch.setattr(toolchains.urllib.request, "urlopen", refuse)

    with pytest.raises(EmitError) as excinfo:
        toolchains.install_tool("fake-tool")
    assert "alloy setup --fetch" in str(excinfo.value)


def test_mirror_dir_resolution(tmp_path, monkeypatch) -> None:
    monkeypatch.delenv("ALLOY_TOOLS_MIRROR", raising=False)
    assert toolchains.mirror_dir(None) is None
    monkeypatch.setenv("ALLOY_TOOLS_MIRROR", str(tmp_path))
    assert toolchains.mirror_dir(None) == tmp_path
    explicit = tmp_path / "other"
    explicit.mkdir()
    assert toolchains.mirror_dir(str(explicit)) == explicit  # --from wins over the env
    with pytest.raises(EmitError, match="not a directory"):
        toolchains.mirror_dir(str(tmp_path / "nope"))


def test_mirror_name_is_the_manifest_basename() -> None:
    for _tool, _plat, pspec in _archive_entries():
        name = toolchains.mirror_name(pspec["url"])
        assert "/" not in name and name


def test_mirror_file_names_do_not_collide() -> None:
    """A mirror is a flat directory keyed by file name — two manifest archives
    sharing one basename would have the second silently shadow the first."""
    seen: dict[str, str] = {}
    for tool, plat, pspec in _archive_entries():
        name = toolchains.mirror_name(pspec["url"])
        assert name not in seen, f"{tool}/{plat} collides with {seen[name]} in a mirror"
        seen[name] = f"{tool}/{plat}"


def test_platform_keys_cover_the_manifest() -> None:
    keys = toolchains.platform_keys()
    assert toolchains._platform_key() in keys
    for _tool, plat, _pspec in _archive_entries():
        assert plat in keys


def test_fetch_rejects_an_unknown_platform(tmp_path) -> None:
    with pytest.raises(EmitError, match="unknown platform"):
        toolchains.fetch(tmp_path, ["solaris-sparc"], None, json_progress=False)


def test_fetch_writes_manifest_named_archives_and_skips_verified_ones(
    tmp_path, monkeypatch
) -> None:
    staging = tmp_path / "staging"
    staging.mkdir()
    archive, sha = _make_archive(staging)
    _stub_manifest(monkeypatch, f"https://example.invalid/{archive.name}", sha)

    calls: list[str] = []

    def fake_download(name, url, target, json_progress):  # noqa: ARG001
        calls.append(url)
        target.write_bytes(archive.read_bytes())
        return sha
    monkeypatch.setattr(toolchains, "_download", fake_download)

    mirror = tmp_path / "mirror"
    assert toolchains.fetch(mirror, [toolchains._platform_key()], None,
                            json_progress=False) == 0
    assert (mirror / archive.name).is_file()
    assert len(calls) == 1

    # Second run: the archive is already there with the right digest, so the
    # slow link is not used again.
    assert toolchains.fetch(mirror, [toolchains._platform_key()], None,
                            json_progress=False) == 0
    assert len(calls) == 1


def test_fetch_refuses_to_mirror_a_bad_digest(tmp_path, monkeypatch) -> None:
    staging = tmp_path / "staging"
    staging.mkdir()
    archive, _sha = _make_archive(staging)
    _stub_manifest(monkeypatch, f"https://example.invalid/{archive.name}", "0" * 64)

    def fake_download(name, url, target, json_progress):  # noqa: ARG001
        target.write_bytes(archive.read_bytes())
        return hashlib.sha256(archive.read_bytes()).hexdigest()
    monkeypatch.setattr(toolchains, "_download", fake_download)

    mirror = tmp_path / "mirror"
    assert toolchains.fetch(mirror, [toolchains._platform_key()], None,
                            json_progress=False) == 1
    assert not (mirror / archive.name).exists()


def test_setup_prints_the_path_export_its_own_install_requires(
    tmp_path, monkeypatch, capsys
) -> None:
    """An install into ~/.alloy/tools is not yet a build.

    `alloy build` resolves the cross compiler on PATH and nowhere else, so on an
    air-gapped host a perfect mirror still ends in "The CXX compiler
    identification is unknown" unless the operator exports the path. `setup`
    has the directory in hand at that moment; it must say so.
    """
    installed = tmp_path / "tools" / "arm-gnu-toolchain"
    (installed / "bin").mkdir(parents=True)

    monkeypatch.setattr(toolchains, "check_status", lambda _f: [
        {"tool": "arm-gnu-toolchain", "status": "missing", "kind": "archive",
         "installable": True, "remedy": "", "path": None},
    ])
    monkeypatch.setattr(toolchains, "install_tool",
                        lambda *_a, **_k: installed)

    assert toolchains.setup(None, check_only=False, json_out=False,
                            json_progress=False) == 0
    out = capsys.readouterr().out
    assert "export PATH=" in out
    assert str(installed / "bin") in out


def test_setup_says_nothing_about_path_when_it_installed_nothing(
    monkeypatch, capsys
) -> None:
    """The negative control for the line above: no install, no advice."""
    monkeypatch.setattr(toolchains, "check_status", lambda _f: [
        {"tool": "arm-gnu-toolchain", "status": "path", "kind": "archive",
         "installable": True, "remedy": "", "path": "/usr/bin/arm-none-eabi-gcc"},
    ])
    assert toolchains.setup(None, check_only=False, json_out=False,
                            json_progress=False) == 0
    assert "export PATH=" not in capsys.readouterr().out
