"""SDK manager: install, list, select, and inspect Alloy runtime versions.

The active version, when set, is what ``runtime.find_runtime_root`` will resolve to when
no ``ALLOY_ROOT`` is set and the user is not inside a checkout. This makes the typical
flow `pipx install alloy-cli && alloy sdk install <version> && alloy new ...` work from
any directory.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

from . import _git, config


class SdkError(RuntimeError):
    """Raised for user-facing SDK manager failures."""


def _install_repo(url: str, dest: Path, ref: str) -> tuple[str, str]:
    """Clone ``url`` to ``dest`` and check out ``ref``. Returns (resolved_ref, sha)."""

    if dest.exists():
        if not _git.is_repo(dest):
            raise SdkError(f"{dest} exists and is not a git repository; refusing to overwrite")
        _git.fetch(dest)
        _git.checkout(dest, ref)
    else:
        _git.clone(url, dest, ref=ref)
    return ref, _git.resolve_sha(dest)


def install(
    version: str,
    *,
    runtime_ref: str | None = None,
    devices_ref: str | None = None,
    force: bool = False,
) -> config.Manifest:
    """Install ``version`` into ``$ALLOY_HOME/sdk/<version>/``.

    ``runtime_ref`` defaults to ``version``; ``devices_ref`` defaults to ``main`` and is
    expected to be pinned by future releases via a runtime-side manifest. ``force=True``
    wipes an existing version directory before reinstalling.
    """

    cfg = config.load()
    version_dir = config.sdk_version_dir(version)
    if version_dir.exists() and force:
        shutil.rmtree(version_dir)

    runtime_ref = runtime_ref or version
    devices_ref = devices_ref or "main"

    runtime_dir = version_dir / "runtime"
    devices_dir = version_dir / "devices"

    runtime_ref, runtime_sha = _install_repo(cfg.sources.runtime, runtime_dir, runtime_ref)
    devices_ref, devices_sha = _install_repo(cfg.sources.devices, devices_dir, devices_ref)

    manifest = config.Manifest(
        version=version,
        runtime_ref=runtime_ref,
        runtime_sha=runtime_sha,
        devices_ref=devices_ref,
        devices_sha=devices_sha,
    )
    config.save_manifest(manifest)

    if cfg.active_version is None:
        cfg.active_version = version
        config.save(cfg)

    return manifest


def list_versions() -> list[str]:
    root = config.sdk_root()
    if not root.is_dir():
        return []
    return sorted(p.name for p in root.iterdir() if (p / "runtime").is_dir())


def use(version: str) -> None:
    if not config.sdk_version_dir(version).is_dir():
        raise SdkError(
            f"version {version!r} is not installed; run `alloy sdk install {version}` first"
        )
    cfg = config.load()
    cfg.active_version = version
    config.save(cfg)


def active_runtime_path() -> Path | None:
    cfg = config.load()
    if cfg.active_version is None:
        return None
    runtime = config.sdk_version_dir(cfg.active_version) / "runtime"
    return runtime if runtime.is_dir() else None


# --- argparse wiring -------------------------------------------------------------------


def add_subparsers(parent: argparse._SubParsersAction) -> None:
    sdk_p = parent.add_parser("sdk", help="manage installed Alloy runtime versions")
    sdk_sub = sdk_p.add_subparsers(dest="sdk_cmd", required=True)

    install_p = sdk_sub.add_parser("install", help="install a runtime version")
    install_p.add_argument("version", help="version label (typically a runtime tag)")
    install_p.add_argument("--runtime-ref", help="git ref for the runtime (default: <version>)")
    install_p.add_argument("--devices-ref", help="git ref for alloy-devices (default: main)")
    install_p.add_argument("--force", action="store_true", help="reinstall even if present")
    install_p.set_defaults(func=_cmd_install)

    list_p = sdk_sub.add_parser("list", help="list installed versions")
    list_p.set_defaults(func=_cmd_list)

    use_p = sdk_sub.add_parser("use", help="select the active version")
    use_p.add_argument("version")
    use_p.set_defaults(func=_cmd_use)

    path_p = sdk_sub.add_parser("path", help="print the active runtime path")
    path_p.set_defaults(func=_cmd_path)


def _cmd_install(args: argparse.Namespace) -> int:
    try:
        manifest = install(
            args.version,
            runtime_ref=args.runtime_ref,
            devices_ref=args.devices_ref,
            force=args.force,
        )
    except (SdkError, _git.GitError) as exc:
        sys.stderr.write(f"error: {exc}\n")
        return 1
    sys.stdout.write(
        f"installed alloy {manifest.version}\n"
        f"  runtime {manifest.runtime_ref} ({manifest.runtime_sha[:12]})\n"
        f"  devices {manifest.devices_ref} ({manifest.devices_sha[:12]})\n"
    )
    return 0


def _cmd_list(_: argparse.Namespace) -> int:
    cfg = config.load()
    versions = list_versions()
    if not versions:
        sys.stdout.write("no versions installed; run `alloy sdk install <version>`\n")
        return 0
    for v in versions:
        marker = "*" if v == cfg.active_version else " "
        sys.stdout.write(f"{marker} {v}\n")
    return 0


def _cmd_use(args: argparse.Namespace) -> int:
    try:
        use(args.version)
    except SdkError as exc:
        sys.stderr.write(f"error: {exc}\n")
        return 1
    sys.stdout.write(f"active version set to {args.version}\n")
    return 0


def _cmd_path(_: argparse.Namespace) -> int:
    path = active_runtime_path()
    if path is None:
        sys.stderr.write("no active version; run `alloy sdk use <version>`\n")
        return 1
    sys.stdout.write(f"{path}\n")
    return 0
