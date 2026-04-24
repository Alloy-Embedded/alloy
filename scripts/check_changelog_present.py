#!/usr/bin/env python3
"""Release gate: assert CHANGELOG.md has a section for the declared project version.

The project version lives in `CMakeLists.txt` as `project(alloy ... VERSION X.Y.Z ...)`.
A release must ship a `CHANGELOG.md` at the repo root containing a section whose header
matches the same version (e.g. `## [0.1.0]` or `## [0.1.0] — ...`).

Exits non-zero with an actionable message if the file is missing or the version entry is
absent.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
CMAKE = REPO_ROOT / "CMakeLists.txt"
CHANGELOG = REPO_ROOT / "CHANGELOG.md"

VERSION_RE = re.compile(
    r"project\(alloy\s*\n\s*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)",
    re.MULTILINE,
)


def _project_version() -> str | None:
    try:
        text = CMAKE.read_text(encoding="utf-8")
    except OSError:
        return None
    match = VERSION_RE.search(text)
    return match.group(1) if match else None


def _has_section_for(version: str) -> bool:
    try:
        text = CHANGELOG.read_text(encoding="utf-8")
    except OSError:
        return False
    # Accept `## [X.Y.Z]` optionally followed by separator/date.
    pattern = rf"^##\s+\[{re.escape(version)}\](?:\s|$)"
    return re.search(pattern, text, re.MULTILINE) is not None


def main() -> int:
    version = _project_version()
    if version is None:
        print(
            f"error: could not find `project(alloy VERSION X.Y.Z)` in {CMAKE}",
            file=sys.stderr,
        )
        return 2
    if not CHANGELOG.exists():
        print(
            f"error: {CHANGELOG} missing. Every release MUST ship a repo-root CHANGELOG.md.",
            file=sys.stderr,
        )
        return 1
    if not _has_section_for(version):
        print(
            f"error: CHANGELOG.md does not contain a `## [{version}]` section matching the "
            f"declared project version in CMakeLists.txt.",
            file=sys.stderr,
        )
        print(
            "hint: add a section titled `## [{v}] — YYYY-MM-DD` that summarises what ships "
            "in this version.".format(v=version),
            file=sys.stderr,
        )
        return 1
    print(f"Changelog present check passed (version={version}).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
