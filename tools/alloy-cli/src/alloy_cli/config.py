"""Per-user CLI configuration stored under ``~/.alloy/`` (or ``$ALLOY_HOME``).

Layout::

    $ALLOY_HOME/
    ├── config.toml          # active SDK version, source URLs
    └── sdk/<version>/
        ├── runtime/         # alloy runtime checkout
        ├── devices/         # alloy-devices checkout
        └── manifest.toml    # pinned commit SHAs for this version

The schema is intentionally tiny so we can hand-write it without a TOML library and read
it with ``tomllib`` (Python 3.11+) or ``tomli`` (Python 3.10).
"""

from __future__ import annotations

import os
import sys
from dataclasses import dataclass, field
from pathlib import Path

if sys.version_info >= (3, 11):
    import tomllib  # type: ignore[import-not-found]
else:  # pragma: no cover - exercised only on 3.10
    import tomli as tomllib  # type: ignore[import-not-found, no-redef]

ENV_HOME = "ALLOY_HOME"
DEFAULT_RUNTIME_URL = "https://github.com/Alloy-Embedded/alloy.git"
DEFAULT_DEVICES_URL = "https://github.com/Alloy-Embedded/alloy-devices.git"


def home_dir() -> Path:
    explicit = os.environ.get(ENV_HOME)
    if explicit:
        return Path(explicit).expanduser().resolve()
    return Path.home().resolve() / ".alloy"


def config_path() -> Path:
    return home_dir() / "config.toml"


def sdk_root() -> Path:
    return home_dir() / "sdk"


def sdk_version_dir(version: str) -> Path:
    return sdk_root() / version


@dataclass(frozen=True)
class Sources:
    runtime: str = DEFAULT_RUNTIME_URL
    devices: str = DEFAULT_DEVICES_URL


@dataclass
class Config:
    active_version: str | None = None
    sources: Sources = field(default_factory=Sources)


def load() -> Config:
    path = config_path()
    if not path.is_file():
        return Config()
    raw = tomllib.loads(path.read_text(encoding="utf-8"))
    sources = raw.get("sources", {}) or {}
    return Config(
        active_version=raw.get("active_version"),
        sources=Sources(
            runtime=sources.get("runtime", DEFAULT_RUNTIME_URL),
            devices=sources.get("devices", DEFAULT_DEVICES_URL),
        ),
    )


def _toml_escape(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def save(config: Config) -> None:
    path = config_path()
    path.parent.mkdir(parents=True, exist_ok=True)
    lines: list[str] = []
    if config.active_version is not None:
        lines.append(f"active_version = {_toml_escape(config.active_version)}")
        lines.append("")
    lines.append("[sources]")
    lines.append(f"runtime = {_toml_escape(config.sources.runtime)}")
    lines.append(f"devices = {_toml_escape(config.sources.devices)}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


@dataclass(frozen=True)
class Manifest:
    version: str
    runtime_ref: str
    runtime_sha: str
    devices_ref: str
    devices_sha: str


def manifest_path(version: str) -> Path:
    return sdk_version_dir(version) / "manifest.toml"


def load_manifest(version: str) -> Manifest | None:
    path = manifest_path(version)
    if not path.is_file():
        return None
    raw = tomllib.loads(path.read_text(encoding="utf-8"))
    runtime = raw.get("runtime", {}) or {}
    devices = raw.get("devices", {}) or {}
    return Manifest(
        version=raw.get("version", version),
        runtime_ref=runtime.get("ref", ""),
        runtime_sha=runtime.get("sha", ""),
        devices_ref=devices.get("ref", ""),
        devices_sha=devices.get("sha", ""),
    )


def save_manifest(manifest: Manifest) -> None:
    path = manifest_path(manifest.version)
    path.parent.mkdir(parents=True, exist_ok=True)
    body = (
        f"version = {_toml_escape(manifest.version)}\n"
        "\n"
        "[runtime]\n"
        f"ref = {_toml_escape(manifest.runtime_ref)}\n"
        f"sha = {_toml_escape(manifest.runtime_sha)}\n"
        "\n"
        "[devices]\n"
        f"ref = {_toml_escape(manifest.devices_ref)}\n"
        f"sha = {_toml_escape(manifest.devices_sha)}\n"
    )
    path.write_text(body, encoding="utf-8")
