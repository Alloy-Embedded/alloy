#!/usr/bin/env python3
"""Release gate: build the docs site with --strict to catch broken links/nav drift.

Runs `mkdocs build --strict` so any missing nav entry, broken internal link, or markdown
parse error fails the gate before it can reach the published site.
"""
from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
MKDOCS_YAML = REPO_ROOT / "mkdocs.yml"


def main() -> int:
    if not MKDOCS_YAML.exists():
        print(f"error: {MKDOCS_YAML} not found", file=sys.stderr)
        return 2
    if shutil.which("mkdocs") is None:
        print(
            "error: mkdocs not on PATH. Install with `python3 -m pip install mkdocs-material`.",
            file=sys.stderr,
        )
        return 2
    with tempfile.TemporaryDirectory() as td:
        site_dir = Path(td) / "site"
        cmd = [
            "mkdocs",
            "build",
            "--strict",
            "--config-file",
            str(MKDOCS_YAML),
            "--site-dir",
            str(site_dir),
        ]
        completed = subprocess.run(cmd, cwd=REPO_ROOT)
    if completed.returncode != 0:
        print("Docs site build FAILED (strict mode).", file=sys.stderr)
        return 1
    print("Docs site check passed.")
    print(f"Built with: {' '.join(cmd)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
