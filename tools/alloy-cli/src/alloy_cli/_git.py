"""Thin subprocess wrappers around ``git``.

The SDK manager and (later) vendored-mode scaffolding both need a small set of git
operations. We delegate to the system ``git`` binary instead of a Python git library to
keep the dependency graph trivial. All functions raise ``GitError`` on failure with the
captured stderr for the caller to surface.
"""

from __future__ import annotations

import shutil
import subprocess
from collections.abc import Sequence
from pathlib import Path


class GitError(RuntimeError):
    """Raised when a git subprocess exits non-zero or git is not installed."""


def _git_binary() -> str:
    binary = shutil.which("git")
    if binary is None:
        raise GitError("git is not installed or not on PATH")
    return binary


def run(args: Sequence[str], *, cwd: Path | None = None) -> str:
    """Run ``git <args>`` and return stdout. Raise ``GitError`` on failure."""

    cmd = [_git_binary(), *args]
    proc = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        raise GitError(
            f"git {' '.join(args)} failed (exit {proc.returncode}): "
            f"{proc.stderr.strip() or proc.stdout.strip()}"
        )
    return proc.stdout


def clone(url: str, dest: Path, *, ref: str | None = None) -> None:
    """Clone ``url`` into ``dest``. Optionally check out ``ref`` after cloning."""

    dest.parent.mkdir(parents=True, exist_ok=True)
    run(["clone", "--quiet", url, str(dest)])
    if ref is not None:
        checkout(dest, ref)


def fetch(repo: Path) -> None:
    run(["fetch", "--quiet", "--tags", "--prune", "origin"], cwd=repo)


def checkout(repo: Path, ref: str) -> None:
    run(["checkout", "--quiet", ref], cwd=repo)


def resolve_sha(repo: Path, ref: str = "HEAD") -> str:
    return run(["rev-parse", ref], cwd=repo).strip()


def is_repo(path: Path) -> bool:
    if not (path / ".git").exists():
        return False
    try:
        run(["rev-parse", "--is-inside-work-tree"], cwd=path)
        return True
    except GitError:
        return False
