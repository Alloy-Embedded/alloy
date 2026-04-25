"""Locate and load the Alloy runtime checkout that backs the CLI.

Phase 1 delegates every subcommand to the existing `scripts/alloyctl.py` shipped inside an
Alloy runtime checkout. Later phases (SDK manager) will replace this lookup with a versioned
cache under `~/.alloy/sdk`. Keeping the discovery logic in one module makes that swap a
single-file change.
"""

from __future__ import annotations

import importlib.util
import os
import sys
import types
from pathlib import Path

ENV_VAR = "ALLOY_ROOT"
SENTINELS = ("scripts/alloyctl.py", "cmake/board_manifest.cmake")


class RuntimeNotFoundError(RuntimeError):
    """Raised when no Alloy runtime checkout can be located."""


def _looks_like_runtime(path: Path) -> bool:
    return all((path / sentinel).is_file() for sentinel in SENTINELS)


def _walk_up(start: Path) -> Path | None:
    for candidate in (start, *start.parents):
        if _looks_like_runtime(candidate):
            return candidate
    return None


def find_runtime_root(explicit: str | os.PathLike[str] | None = None) -> Path:
    """Resolve the Alloy runtime checkout to delegate into.

    Resolution order:
    1. ``explicit`` argument (when the caller already knows the path).
    2. ``ALLOY_ROOT`` environment variable.
    3. Walk up from the current working directory looking for sentinel files.
    """

    if explicit is not None:
        candidate = Path(explicit).expanduser().resolve()
        if _looks_like_runtime(candidate):
            return candidate
        raise RuntimeNotFoundError(
            f"{candidate} does not look like an Alloy runtime checkout "
            f"(missing {', '.join(SENTINELS)})."
        )

    env_value = os.environ.get(ENV_VAR)
    if env_value:
        candidate = Path(env_value).expanduser().resolve()
        if _looks_like_runtime(candidate):
            return candidate
        raise RuntimeNotFoundError(
            f"{ENV_VAR}={candidate} does not point at an Alloy runtime checkout "
            f"(missing {', '.join(SENTINELS)})."
        )

    walked = _walk_up(Path.cwd().resolve())
    if walked is not None:
        return walked

    raise RuntimeNotFoundError(
        "Could not locate an Alloy runtime checkout. "
        f"Set {ENV_VAR} to the runtime path, or run from inside a checkout."
    )


def load_alloyctl(root: Path) -> types.ModuleType:
    """Load ``scripts/alloyctl.py`` from a runtime checkout as the ``alloyctl`` module."""

    script = root / "scripts" / "alloyctl.py"
    spec = importlib.util.spec_from_file_location("alloyctl", script)
    if spec is None or spec.loader is None:
        raise RuntimeNotFoundError(f"Could not load module spec from {script}.")
    module = importlib.util.module_from_spec(spec)
    sys.modules["alloyctl"] = module
    spec.loader.exec_module(module)
    return module
