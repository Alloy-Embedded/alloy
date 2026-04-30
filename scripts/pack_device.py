#!/usr/bin/env python3
"""
pack_device.py — Pack an alloy-devices family directory into a versioned tar.gz.

Usage:
    python scripts/pack_device.py \
        --family-dir ../alloy-devices/st/stm32g0 \
        --vendor st --family stm32g0 \
        --version 1.0.0 \
        --output dist/

The produced archive extracts to:
    <vendor>/<family>/generated/...
    <vendor>/<family>/artifact-manifest.json

So unpacking into ~/.alloy/devices/ gives:
    ~/.alloy/devices/<vendor>/<family>/generated/...

Run with --update-checksums dist/checksums.json to append the SHA256 entry.

Options:
    --family-dir DIR       Source directory (the <vendor>/<family> tree in alloy-devices)
    --vendor NAME          Vendor identifier (e.g. st, microchip, nxp)
    --family NAME          Family identifier (e.g. stm32g0, same70, imxrt10xx)
    --version VERSION      Release version string (e.g. 1.0.0)
    --output DIR           Output directory for the tar.gz (default: current dir)
    --update-checksums F   JSON file to update with the new package SHA256 entry
    --dry-run              Print what would be archived without writing
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
import tarfile
from pathlib import Path


def _collect_files(base: Path) -> list[Path]:
    """Return all files under *base* in deterministic (sorted) order."""
    files: list[Path] = []
    for root, dirs, filenames in os.walk(base):
        dirs.sort()  # ensure deterministic directory traversal
        for name in sorted(filenames):
            files.append(Path(root) / name)
    return files


def _sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def pack(
    family_dir: Path,
    vendor: str,
    family: str,
    version: str,
    output_dir: Path,
    dry_run: bool = False,
) -> Path:
    """Create <vendor>-<family>-<version>.tar.gz and return its path."""
    if not family_dir.is_dir():
        print(f"error: family-dir not found: {family_dir}", file=sys.stderr)
        raise SystemExit(1)

    output_dir.mkdir(parents=True, exist_ok=True)
    pkg_name = f"{vendor}-{family}-{version}.tar.gz"
    pkg_path = output_dir / pkg_name

    # Collect files in deterministic order
    files = _collect_files(family_dir)
    if not files:
        print(f"error: no files found under {family_dir}", file=sys.stderr)
        raise SystemExit(1)

    # Archive path prefix: <vendor>/<family>/
    prefix = f"{vendor}/{family}"

    if dry_run:
        print(f"[dry-run] would create: {pkg_path}")
        for f in files:
            rel = f.relative_to(family_dir)
            print(f"  {prefix}/{rel}")
        return pkg_path

    print(f"Packing {len(files)} files -> {pkg_path}")
    with tarfile.open(pkg_path, "w:gz") as tf:
        for f in files:
            rel = f.relative_to(family_dir)
            arcname = f"{prefix}/{rel}"
            # Zero out timestamps for reproducibility
            info = tf.gettarinfo(str(f), arcname=arcname)
            info.mtime = 0
            info.uid = 0
            info.gid = 0
            info.uname = ""
            info.gname = ""
            with open(f, "rb") as fh:
                tf.addfile(info, fh)

    sha256 = _sha256_file(pkg_path)
    print(f"SHA256: sha256:{sha256}")
    print(f"Created: {pkg_path}")
    return pkg_path


def update_checksums(checksums_file: Path, pkg_name: str, sha256: str) -> None:
    """Add/update an entry in checksums.json."""
    data: dict[str, str] = {}
    if checksums_file.exists():
        data = json.loads(checksums_file.read_text())
    data[pkg_name] = f"sha256:{sha256}"
    # Sort keys for deterministic output
    checksums_file.write_text(
        json.dumps(dict(sorted(data.items())), indent=2) + "\n"
    )
    print(f"Updated: {checksums_file}")


def main() -> int:
    p = argparse.ArgumentParser(
        description="Pack an alloy-devices family directory into a versioned tar.gz"
    )
    p.add_argument("--family-dir", required=True, type=Path,
                   help="Source directory (the <vendor>/<family> tree in alloy-devices)")
    p.add_argument("--vendor", required=True, help="Vendor identifier (e.g. st)")
    p.add_argument("--family", required=True, help="Family identifier (e.g. stm32g0)")
    p.add_argument("--version", required=True, help="Release version string (e.g. 1.0.0)")
    p.add_argument("--output", default=".", type=Path, help="Output directory (default: .)")
    p.add_argument("--update-checksums", metavar="FILE", type=Path,
                   help="JSON file to update with the new package SHA256 entry")
    p.add_argument("--dry-run", action="store_true",
                   help="Print what would be archived without writing")
    args = p.parse_args()

    pkg_path = pack(
        family_dir=args.family_dir.resolve(),
        vendor=args.vendor,
        family=args.family,
        version=args.version,
        output_dir=args.output.resolve(),
        dry_run=args.dry_run,
    )

    if args.update_checksums and not args.dry_run:
        sha256 = _sha256_file(pkg_path)
        update_checksums(args.update_checksums, pkg_path.name, sha256)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
