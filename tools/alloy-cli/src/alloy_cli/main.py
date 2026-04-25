"""Console entry point for the ``alloy`` command.

Phase 1 keeps the implementation in ``scripts/alloyctl.py`` inside the runtime checkout and
delegates argument parsing to it. The CLI shape (``alloy <subcommand>``) is the contract;
the underlying module is replaceable. Once the SDK manager lands, ``find_runtime_root`` will
prefer ``~/.alloy/sdk/<version>`` over walking up from the working directory.
"""

from __future__ import annotations

import sys

from . import __version__
from .runtime import RuntimeNotFoundError, find_runtime_root, load_alloyctl

PROG = "alloy"


def _print_top_level_help() -> None:
    sys.stdout.write(
        f"{PROG} {__version__} -- Alloy multi-vendor bare-metal runtime CLI\n"
        "\n"
        f"usage: {PROG} <command> [options]\n"
        "\n"
        "Phase 1 delegates to scripts/alloyctl.py inside an Alloy runtime checkout.\n"
        f"Set ALLOY_ROOT to the checkout, or run {PROG} from inside one.\n"
        "\n"
        "Common commands:\n"
        "  new        scaffold a downstream firmware starter for a board\n"
        "  configure  configure a CMake build dir for a board\n"
        "  build      build a target for a board\n"
        "  flash      flash a target to a board (optionally building first)\n"
        "  monitor    open a serial monitor for a board\n"
        "  doctor     preflight: toolchain, probe, python deps, descriptors\n"
        "  info       print machine-readable environment report (JSON)\n"
        "\n"
        f"Run `{PROG} <command> --help` for command-specific options.\n"
    )


def _is_help_only(argv: list[str]) -> bool:
    return not argv or argv[0] in {"-h", "--help"}


def _is_version_only(argv: list[str]) -> bool:
    return len(argv) == 1 and argv[0] in {"-V", "--version"}


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)

    if _is_version_only(args):
        sys.stdout.write(f"{PROG} {__version__}\n")
        return 0

    if _is_help_only(args):
        _print_top_level_help()
        return 0

    try:
        root = find_runtime_root()
    except RuntimeNotFoundError as exc:
        sys.stderr.write(f"error: {exc}\n")
        return 2

    module = load_alloyctl(root)

    saved_argv = sys.argv
    sys.argv = [PROG, *args]
    try:
        result = module.main()
    finally:
        sys.argv = saved_argv

    return int(result) if isinstance(result, int) else 0


if __name__ == "__main__":
    raise SystemExit(main())
