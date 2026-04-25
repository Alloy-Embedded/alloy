"""Console entry point for the ``alloy`` command.

Native subcommands (currently ``sdk``) are handled by this package directly. Everything
else is delegated to ``scripts/alloyctl.py`` inside the active Alloy runtime checkout, as
located by :mod:`alloy_cli.runtime`. The CLI shape (``alloy <subcommand>``) is the
contract; the underlying delegation is replaceable.
"""

from __future__ import annotations

import argparse
import sys

from . import __version__, sdk
from .runtime import RuntimeNotFoundError, find_runtime_root, load_alloyctl

PROG = "alloy"

# Subcommands implemented natively in alloy-cli (not delegated to alloyctl).
NATIVE_COMMANDS = {"sdk"}


def _print_top_level_help() -> None:
    sys.stdout.write(
        f"{PROG} {__version__} -- Alloy multi-vendor bare-metal runtime CLI\n"
        "\n"
        f"usage: {PROG} <command> [options]\n"
        "\n"
        "Native commands (handled by alloy-cli):\n"
        "  sdk        manage installed Alloy runtime versions\n"
        "\n"
        "Delegated commands (handled by the runtime checkout):\n"
        "  new        scaffold a downstream firmware starter for a board\n"
        "  configure  configure a CMake build dir for a board\n"
        "  build      build a target for a board\n"
        "  flash      flash a target to a board (optionally building first)\n"
        "  monitor    open a serial monitor for a board\n"
        "  doctor     preflight: toolchain, probe, python deps, descriptors\n"
        "  info       print machine-readable environment report (JSON)\n"
        "\n"
        f"Run `{PROG} <command> --help` for command-specific options.\n"
        "\n"
        "The runtime checkout is resolved in this order:\n"
        "  1. ALLOY_ROOT environment variable\n"
        "  2. walk up from the current working directory\n"
        f"  3. the SDK version selected with `{PROG} sdk use`\n"
    )


def _is_help_only(argv: list[str]) -> bool:
    return not argv or argv[0] in {"-h", "--help"}


def _is_version_only(argv: list[str]) -> bool:
    return len(argv) == 1 and argv[0] in {"-V", "--version"}


def _build_native_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog=PROG, add_help=False)
    sub = parser.add_subparsers(dest="cmd", required=True)
    sdk.add_subparsers(sub)
    return parser


def _run_native(argv: list[str]) -> int:
    parser = _build_native_parser()
    args = parser.parse_args(argv)
    return int(args.func(args) or 0)


def _delegate(argv: list[str]) -> int:
    try:
        root = find_runtime_root()
    except RuntimeNotFoundError as exc:
        sys.stderr.write(f"error: {exc}\n")
        return 2

    module = load_alloyctl(root)

    saved_argv = sys.argv
    sys.argv = [PROG, *argv]
    try:
        result = module.main()
    finally:
        sys.argv = saved_argv
    return int(result) if isinstance(result, int) else 0


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)

    if _is_version_only(args):
        sys.stdout.write(f"{PROG} {__version__}\n")
        return 0

    if _is_help_only(args):
        _print_top_level_help()
        return 0

    if args[0] in NATIVE_COMMANDS:
        return _run_native(args)

    return _delegate(args)


if __name__ == "__main__":
    raise SystemExit(main())
