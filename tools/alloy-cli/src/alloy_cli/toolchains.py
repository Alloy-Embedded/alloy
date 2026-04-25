"""Toolchain manager: install, list, and locate pinned cross-toolchains.

Pin schema lives in ``_toolchain_pins.toml`` shipped with the package. Tests can supply
their own pin file via ``ALLOY_TOOLCHAIN_PINS`` so the manager is exercised against
synthetic ``file://`` URLs without hitting the network.

Layout::

    $ALLOY_HOME/toolchains/<name>/<version>/
        ├── ...unpacked archive contents...
        └── .alloy_install.json   # pin metadata, written on success
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import sys
import tarfile
import tempfile
import urllib.parse
import urllib.request
import zipfile
from dataclasses import dataclass
from pathlib import Path

if sys.version_info >= (3, 11):
    import tomllib  # type: ignore[import-not-found]
else:  # pragma: no cover
    import tomli as tomllib  # type: ignore[no-redef]

from . import config

ENV_PINS = "ALLOY_TOOLCHAIN_PINS"


class ToolchainError(RuntimeError):
    """Raised for user-facing toolchain manager failures."""


# --- pin discovery ---------------------------------------------------------------------


def _shipped_pins_path() -> Path:
    return Path(__file__).with_name("_toolchain_pins.toml")


def pins_path() -> Path:
    override = os.environ.get(ENV_PINS)
    return Path(override).expanduser().resolve() if override else _shipped_pins_path()


def load_pins() -> dict:
    return tomllib.loads(pins_path().read_text(encoding="utf-8"))


# --- platform detection ----------------------------------------------------------------


def detect_platform() -> str:
    system = platform.system().lower()
    machine = platform.machine().lower()
    if system not in {"darwin", "linux"}:
        raise ToolchainError(
            f"unsupported host system {system!r}; macOS and Linux only in this release"
        )
    if machine in {"x86_64", "amd64"}:
        arch = "x64"
    elif machine in {"arm64", "aarch64"}:
        arch = "arm64"
    else:
        raise ToolchainError(f"unsupported host arch {machine!r}")
    return f"{system}-{arch}"


# --- pin resolution --------------------------------------------------------------------


@dataclass(frozen=True)
class Pin:
    name: str
    version: str
    binary: str
    platform: str
    url: str
    sha256: str
    archive: str
    strip_components: int
    bin_subdir: str


def resolve_pin(name: str, version: str | None = None, host: str | None = None) -> Pin:
    pins = load_pins()
    if name not in pins:
        raise ToolchainError(
            f"unknown toolchain {name!r}; known: {', '.join(sorted(pins)) or '(none)'}"
        )
    entry = pins[name]
    version = version or entry.get("default_version")
    if not version:
        raise ToolchainError(f"toolchain {name!r} has no default_version pinned")
    versions = entry.get("versions") or {}
    if version not in versions:
        raise ToolchainError(
            f"toolchain {name!r} has no pin for version {version!r}; "
            f"known: {', '.join(sorted(versions)) or '(none)'}"
        )
    by_platform = versions[version]
    host = host or detect_platform()
    if host not in by_platform:
        raise ToolchainError(
            f"toolchain {name}@{version} has no pin for host {host!r}; "
            f"available: {', '.join(sorted(by_platform))}"
        )
    spec = by_platform[host]
    return Pin(
        name=name,
        version=version,
        binary=entry.get("binary", name),
        platform=host,
        url=spec["url"],
        sha256=spec["sha256"],
        archive=spec.get("archive", "tar.gz"),
        strip_components=int(spec.get("strip_components", 0)),
        bin_subdir=spec.get("bin_subdir", "bin"),
    )


# --- install ---------------------------------------------------------------------------


def toolchain_root() -> Path:
    return config.home_dir() / "toolchains"


def install_dir(name: str, version: str) -> Path:
    return toolchain_root() / name / version


def _validate_sha(sha: str) -> None:
    if sha == "TODO":
        raise ToolchainError(
            "this toolchain pin is not yet validated (sha256 = TODO). "
            "Provide a real pin via ALLOY_TOOLCHAIN_PINS or wait for the next release."
        )
    if len(sha) != 64 or not all(c in "0123456789abcdef" for c in sha.lower()):
        raise ToolchainError(f"invalid sha256 in pin: {sha!r}")


def _download(url: str, dest: Path) -> None:
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme not in {"http", "https", "file"}:
        raise ToolchainError(f"unsupported URL scheme {parsed.scheme!r}: {url}")
    with urllib.request.urlopen(url) as response, dest.open("wb") as fh:
        shutil.copyfileobj(response, fh)


def _verify_sha256(path: Path, expected: str) -> None:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            digest.update(chunk)
    actual = digest.hexdigest()
    if actual.lower() != expected.lower():
        raise ToolchainError(
            f"checksum mismatch: expected {expected}, got {actual}"
        )


def _extract(archive_path: Path, dest: Path, kind: str, strip: int) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    if kind in {"tar.gz", "tar.xz", "tar.bz2", "tar"}:
        mode = {"tar.gz": "r:gz", "tar.xz": "r:xz", "tar.bz2": "r:bz2", "tar": "r:"}[kind]
        with tarfile.open(archive_path, mode) as tf:
            members = tf.getmembers()
            if strip:
                members = _strip_members(members, strip)
            for member in members:
                tf.extract(member, dest, filter="data")
    elif kind == "zip":
        if strip:
            raise ToolchainError("strip_components is not supported for zip archives")
        with zipfile.ZipFile(archive_path) as zf:
            zf.extractall(dest)
    else:
        raise ToolchainError(f"unsupported archive kind {kind!r}")


def _strip_members(members, strip: int):
    out = []
    for member in members:
        parts = member.name.split("/")
        if len(parts) <= strip:
            continue
        member.name = "/".join(parts[strip:])
        out.append(member)
    return out


@dataclass(frozen=True)
class InstallRecord:
    name: str
    version: str
    platform: str
    install_dir: Path
    bin_dir: Path

    def to_json(self) -> str:
        return json.dumps(
            {
                "name": self.name,
                "version": self.version,
                "platform": self.platform,
                "install_dir": str(self.install_dir),
                "bin_dir": str(self.bin_dir),
            },
            indent=2,
        )


def install(
    name: str,
    *,
    version: str | None = None,
    force: bool = False,
) -> InstallRecord:
    pin = resolve_pin(name, version)
    _validate_sha(pin.sha256)

    target = install_dir(pin.name, pin.version)
    if target.exists() and not force:
        raise ToolchainError(
            f"{pin.name}@{pin.version} already installed at {target}; pass --force to reinstall"
        )
    if target.exists():
        shutil.rmtree(target)

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        archive = tmp_path / "download"
        _download(pin.url, archive)
        _verify_sha256(archive, pin.sha256)
        _extract(archive, target, pin.archive, pin.strip_components)

    bin_dir = target / pin.bin_subdir
    if not bin_dir.is_dir():
        raise ToolchainError(
            f"expected {bin_dir} to exist after install; check pin.bin_subdir"
        )

    record = InstallRecord(
        name=pin.name,
        version=pin.version,
        platform=pin.platform,
        install_dir=target,
        bin_dir=bin_dir,
    )
    (target / ".alloy_install.json").write_text(record.to_json(), encoding="utf-8")
    return record


def list_installed() -> list[tuple[str, str]]:
    root = toolchain_root()
    if not root.is_dir():
        return []
    out: list[tuple[str, str]] = []
    for name_dir in sorted(root.iterdir()):
        if not name_dir.is_dir():
            continue
        for version_dir in sorted(name_dir.iterdir()):
            if (version_dir / ".alloy_install.json").is_file():
                out.append((name_dir.name, version_dir.name))
    return out


def which(name: str, version: str | None = None) -> Path:
    pins = load_pins()
    if name not in pins:
        raise ToolchainError(f"unknown toolchain {name!r}")
    entry = pins[name]
    binary = entry.get("binary", name)
    version = version or entry.get("default_version")
    if version is None:
        raise ToolchainError(f"toolchain {name!r} has no default_version pinned")
    target = install_dir(name, version)
    if not (target / ".alloy_install.json").is_file():
        raise ToolchainError(
            f"{name}@{version} is not installed; run `alloy toolchain install {name}`"
        )
    record = json.loads((target / ".alloy_install.json").read_text(encoding="utf-8"))
    candidate = Path(record["bin_dir"]) / binary
    if not candidate.is_file():
        raise ToolchainError(f"binary {binary} not found at {candidate}")
    return candidate


# --- argparse wiring -------------------------------------------------------------------


def add_subparsers(parent: argparse._SubParsersAction) -> None:
    tc = parent.add_parser("toolchain", help="manage pinned cross-toolchains")
    sub = tc.add_subparsers(dest="toolchain_cmd", required=True)

    install_p = sub.add_parser("install", help="download and install a pinned toolchain")
    install_p.add_argument("spec", help="<name> or <name>@<version>")
    install_p.add_argument("--force", action="store_true", help="reinstall if present")
    install_p.set_defaults(func=_cmd_install)

    list_p = sub.add_parser("list", help="list installed toolchains")
    list_p.set_defaults(func=_cmd_list)

    which_p = sub.add_parser("which", help="print the path to a toolchain binary")
    which_p.add_argument("spec", help="<name> or <name>@<version>")
    which_p.set_defaults(func=_cmd_which)


def _split_spec(spec: str) -> tuple[str, str | None]:
    if "@" in spec:
        name, version = spec.split("@", 1)
        return name, version or None
    return spec, None


def _cmd_install(args: argparse.Namespace) -> int:
    name, version = _split_spec(args.spec)
    try:
        record = install(name, version=version, force=args.force)
    except ToolchainError as exc:
        sys.stderr.write(f"error: {exc}\n")
        return 1
    sys.stdout.write(
        f"installed {record.name}@{record.version} ({record.platform})\n"
        f"  bin: {record.bin_dir}\n"
    )
    return 0


def _cmd_list(_: argparse.Namespace) -> int:
    items = list_installed()
    if not items:
        sys.stdout.write("no toolchains installed; run `alloy toolchain install <name>`\n")
        return 0
    for name, version in items:
        sys.stdout.write(f"{name}@{version}\n")
    return 0


def _cmd_which(args: argparse.Namespace) -> int:
    name, version = _split_spec(args.spec)
    try:
        sys.stdout.write(f"{which(name, version)}\n")
    except ToolchainError as exc:
        sys.stderr.write(f"error: {exc}\n")
        return 1
    return 0
