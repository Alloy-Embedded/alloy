#!/usr/bin/env python3
"""
regen_changed.py — identify alloy-devices families whose SVD source changed.

Compares the `svd_source.commit` (or `svd_source.sha256`) recorded in each
family's `artifact-manifest.json` against a previous snapshot manifest to
determine which families need regeneration.

Usage
-----
  # Compare current alloy-devices tree against a saved snapshot
  python3 scripts/regen_changed.py \
      --devices-root ../alloy-devices \
      --prev-snapshot dist/last_manifests.json \
      --output changed_families.txt

  # First run (no previous snapshot) — treat all families as changed
  python3 scripts/regen_changed.py \
      --devices-root ../alloy-devices \
      --output changed_families.txt

Output format (--output file or stdout)
----------------------------------------
  One "<vendor>/<family>" entry per line for each changed family.
  Empty output means nothing changed.

JSON snapshot format (--prev-snapshot / --save-snapshot)
---------------------------------------------------------
  {
    "st/stm32g0": { "commit": "abc123", "sha256": "...", "patches_sha256": "..." },
    ...
  }

Exit codes
----------
  0 — success (changed list written; may be empty)
  1 — usage or filesystem error
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def _read_artifact_manifest(family_dir: Path) -> dict:
    """Load artifact-manifest.json from a family directory, return {} on error."""
    manifest_path = family_dir / "artifact-manifest.json"
    if not manifest_path.exists():
        return {}
    try:
        return json.loads(manifest_path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return {}


def _family_fingerprint(manifest: dict) -> dict:
    """Extract the fields we compare to detect changes."""
    svd = manifest.get("svd_source", {})
    return {
        "commit":        svd.get("commit", ""),
        "sha256":        svd.get("sha256", ""),
        "patches_sha256": manifest.get("patches_sha256", ""),
    }


def _collect_families(devices_root: Path) -> dict[str, dict]:
    """
    Walk <devices_root>/<vendor>/<family>/artifact-manifest.json.
    Returns { "vendor/family": fingerprint_dict }.
    """
    result: dict[str, dict] = {}
    for manifest_path in sorted(devices_root.glob("*/*/artifact-manifest.json")):
        vendor = manifest_path.parent.parent.name
        family = manifest_path.parent.name
        key = f"{vendor}/{family}"
        data = _read_artifact_manifest(manifest_path.parent)
        result[key] = _family_fingerprint(data)
    return result


def _load_snapshot(path: Path) -> dict[str, dict]:
    """Load a previous snapshot JSON.  Returns {} if the file doesn't exist."""
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as exc:
        print(f"warning: could not read snapshot {path}: {exc}", file=sys.stderr)
        return {}


def _save_snapshot(path: Path, current: dict[str, dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(current, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _find_changed(
    current: dict[str, dict],
    previous: dict[str, dict],
) -> list[str]:
    """
    Return sorted list of "vendor/family" keys that differ between current and previous.
    Families present in current but absent in previous are always considered changed.
    Families absent in current (removed) are ignored.
    """
    changed: list[str] = []
    for key, fp in current.items():
        prev_fp = previous.get(key)
        if prev_fp is None or prev_fp != fp:
            changed.append(key)
    return sorted(changed)


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(
        description="Find alloy-devices families whose SVD source changed since the last run."
    )
    p.add_argument(
        "--devices-root",
        required=True,
        metavar="DIR",
        help="Path to an alloy-devices checkout (contains <vendor>/<family>/ subdirs).",
    )
    p.add_argument(
        "--prev-snapshot",
        metavar="FILE",
        default="",
        help="JSON snapshot from a previous run.  If absent, all families are marked changed.",
    )
    p.add_argument(
        "--save-snapshot",
        metavar="FILE",
        default="",
        help="Write the current fingerprints to this file after comparison (for next run).",
    )
    p.add_argument(
        "--output",
        metavar="FILE",
        default="",
        help="Write changed families to this file, one per line.  Default: stdout.",
    )
    p.add_argument(
        "--json",
        action="store_true",
        help="Output JSON instead of a plain line-separated list.",
    )
    p.add_argument(
        "--all",
        action="store_true",
        help="Treat all families as changed (ignore snapshot).",
    )
    args = p.parse_args(argv)

    devices_root = Path(args.devices_root)
    if not devices_root.is_dir():
        print(f"error: --devices-root not found: {devices_root}", file=sys.stderr)
        return 1

    current = _collect_families(devices_root)

    if not current:
        print(
            f"warning: no artifact-manifest.json files found under {devices_root}",
            file=sys.stderr,
        )

    if args.all:
        changed = sorted(current.keys())
    else:
        previous = _load_snapshot(Path(args.prev_snapshot)) if args.prev_snapshot else {}
        changed = _find_changed(current, previous)

    # Save snapshot if requested
    if args.save_snapshot:
        _save_snapshot(Path(args.save_snapshot), current)
        print(f"snapshot saved to {args.save_snapshot}", file=sys.stderr)

    # Format output
    if args.json:
        payload = json.dumps(
            {
                "changed_families": changed,
                "total_families": len(current),
                "unchanged_families": len(current) - len(changed),
            },
            indent=2,
        )
        text = payload + "\n"
    else:
        text = "\n".join(changed) + ("\n" if changed else "")

    if args.output:
        out_path = Path(args.output)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(text, encoding="utf-8")
        print(
            f"{len(changed)} of {len(current)} families changed → {args.output}",
            file=sys.stderr,
        )
    else:
        sys.stdout.write(text)
        print(
            f"{len(changed)} of {len(current)} families changed",
            file=sys.stderr,
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
