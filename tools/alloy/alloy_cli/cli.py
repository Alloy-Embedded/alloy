"""alloy CLI.

Commands:
    new     scaffold a portable project (the scaffold IS the CI-built blink)
    boards  list boards known to this framework checkout
    gen     regenerate .alloy/generated from the device database
    build   gen + render CMake tree + compile
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from alloy_devices.loader import load_database

from . import __version__
from .build import build
from .emit import generate
from .emit.common import EmitError
from .project import Project, ProjectError, load_project

_MAIN_CPP = """\
// Portable blink — identical bytes on every supported board.
#include <alloy/board.hpp>
using namespace alloy::literals;

int main() {
    board::init();
    while (true) {
        board::led.toggle();
        alloy::sleep_for(500ms);
    }
}
"""

_ALLOY_TOML = """\
[project]
name = "{name}"

[board]
id = "{board}"
"""

_GITIGNORE = """\
.alloy/
compile_commands.json
"""


def _project(args: argparse.Namespace) -> Project:
    return load_project(Path(getattr(args, "project", ".") or "."))


def cmd_new(args: argparse.Namespace) -> int:
    target = Path(args.name)
    if target.exists():
        print(f"error: {target} already exists", file=sys.stderr)
        return 1
    (target / "src").mkdir(parents=True)
    (target / "alloy.toml").write_text(_ALLOY_TOML.format(name=target.name, board=args.board))
    (target / "src" / "main.cpp").write_text(_MAIN_CPP)
    (target / ".gitignore").write_text(_GITIGNORE)
    print(f"created {target}/ (board: {args.board})")
    print(f"next:  cd {target} && alloy build")
    return 0


def cmd_boards(args: argparse.Namespace) -> int:
    project_dir = Path(getattr(args, "project", ".") or ".")
    try:
        project = load_project(project_dir)
        boards_dir = project.alloy_root / "boards"
    except ProjectError:
        from .project import _find_alloy_root  # noqa: PLC0415

        boards_dir = _find_alloy_root(project_dir.resolve()) / "boards"
    for board_json in sorted(boards_dir.glob("*/board.json")):
        import json  # noqa: PLC0415

        board = json.loads(board_json.read_text())
        print(f"{board['id']:24} {board.get('name', ''):32} chip={board['chip']}")
    return 0


def cmd_gen(args: argparse.Namespace) -> int:
    project = _project(args)
    written = generate(project)
    print(f"generated {len(written)} file(s) -> {project.gen_dir}")
    return 0


def cmd_build(args: argparse.Namespace) -> int:
    project = _project(args)
    generate(project)
    board = project.load_board()
    db = load_database(project.devices_root)
    chip = db.chips[board["chip"]]
    elf = build(project, chip)
    print(f"\nbuilt {elf}")
    return 0


def cmd_flash(args: argparse.Namespace) -> int:
    from .flash import flash  # noqa: PLC0415

    project = _project(args)
    generate(project)
    board = project.load_board()
    db = load_database(project.devices_root)
    chip = db.chips[board["chip"]]
    elf = build(project, chip)
    runner = flash(board, chip, elf)
    print(f"\nflashed {elf.name} via {runner}")
    return 0


def main() -> None:
    parser = argparse.ArgumentParser(prog="alloy", description=__doc__)
    parser.add_argument("--version", action="version", version=__version__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_new = sub.add_parser("new", help="scaffold a new project")
    p_new.add_argument("name")
    p_new.add_argument("--board", required=True)
    p_new.set_defaults(func=cmd_new)

    p_boards = sub.add_parser("boards", help="list known boards")
    p_boards.add_argument("--project", default=".")
    p_boards.set_defaults(func=cmd_boards)

    for cmd, func in (("gen", cmd_gen), ("build", cmd_build), ("flash", cmd_flash)):
        p = sub.add_parser(cmd)
        p.add_argument("--project", default=".")
        p.set_defaults(func=func)

    args = parser.parse_args()
    try:
        sys.exit(args.func(args))
    except (ProjectError, EmitError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
