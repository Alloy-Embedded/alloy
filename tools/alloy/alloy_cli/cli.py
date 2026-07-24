"""alloy CLI.

Commands:
    new        scaffold a portable project (the scaffold IS the CI-built blink)
    boards     list boards (--json for IDE integrations)
    gen        regenerate .alloy/generated from the device database
    build      gen + render CMake tree + compile
    flash      build + program the board
    monitor    bidirectional serial monitor
    run        flash + monitor
    clean      remove per-board build trees
    set-board  change the board in alloy.toml
    setup      verify/install toolchains into ~/.alloy/tools
    debug-info debug-server facts for a board (--json)
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from alloy_devices.loader import load_database

from . import __version__
from .build import build
from .emit import generate
from .emit.common import EmitError
from .project import Project, ProjectError, load_project

_MAIN_CPP = """\
// Portable hello — blink AND echo, so the board never looks dead after a
// flash. Identical bytes on every supported board; zero #ifdefs.
#include <alloy/board.hpp>

#include <cstdint>

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy hello: blinking + echoing\\r\\n");

    std::uint32_t last_toggle = alloy::uptime_ms();
    while (true) {
        std::uint8_t byte{};
        if (uart.read(byte)) {
            uart.write(byte);
        }
        if (alloy::uptime_ms() - last_toggle >= 500u) {
            board::led.toggle();
            last_toggle = alloy::uptime_ms();
        }
    }
}
"""

_ALLOY_TOML = """\
[project]
name = "{name}"

[board]
id = "{board}"

[alloy]
root = "{root}"
"""

_GITIGNORE = """\
.alloy/
compile_commands.json
"""


def _project(args: argparse.Namespace) -> Project:
    return load_project(
        Path(getattr(args, "project", ".") or "."),
        board_override=getattr(args, "board", None),
    )


def cmd_new(args: argparse.Namespace) -> int:
    from .project import _find_alloy_root  # noqa: PLC0415

    target = Path(args.name)
    if target.exists():
        print(f"error: {target} already exists", file=sys.stderr)
        return 1
    alloy_root = _find_alloy_root(Path.cwd())
    boards_dir = alloy_root / "boards"
    if not (boards_dir / args.board / "board.json").exists():
        known = sorted(p.name for p in boards_dir.iterdir() if (p / "board.json").exists())
        print(f"error: unknown board '{args.board}' — known: {', '.join(known)}", file=sys.stderr)
        return 1
    (target / "src").mkdir(parents=True)
    from .project import packaged_alloy_root  # noqa: PLC0415

    if alloy_root == packaged_alloy_root():
        # Wheel installs are relocatable: resolution finds the package at
        # runtime; baking a version-specific site-packages path would rot.
        toml_text = _ALLOY_TOML.split("[alloy]")[0].format(
            name=target.name, board=args.board
        )
    else:
        toml_text = _ALLOY_TOML.format(
            name=target.name, board=args.board, root=alloy_root
        )
    (target / "alloy.toml").write_text(toml_text)
    (target / "src" / "main.cpp").write_text(_MAIN_CPP)
    (target / ".gitignore").write_text(_GITIGNORE)
    print(f"created {target}/ (board: {args.board}, framework: {alloy_root})")
    print(f"next:  cd {target} && alloy run")
    return 0


def cmd_boards(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    project_dir = Path(getattr(args, "project", ".") or ".")
    try:
        project = load_project(project_dir)
        boards_dir = project.alloy_root / "boards"
    except ProjectError:
        from .project import _find_alloy_root  # noqa: PLC0415

        boards_dir = _find_alloy_root(project_dir.resolve()) / "boards"
    rows = []
    for board_json in sorted(boards_dir.glob("*/board.json")):
        board = json.loads(board_json.read_text())
        rows.append(board)
    if getattr(args, "json", False):
        print(json.dumps({
            "schema": "alloy.boards.v1",
            "boards": [
                {
                    "id": b["id"],
                    "name": b.get("name", ""),
                    "chip": b["chip"],
                    "probe": b.get("probe", {}).get("kind"),
                    "roles": sorted(b.get("roles", {})),
                }
                for b in rows
            ],
        }, indent=2))
        return 0
    for b in rows:
        print(f"{b['id']:24} {b.get('name', ''):32} chip={b['chip']}")
    return 0


def cmd_gen(args: argparse.Namespace) -> int:
    project = _project(args)
    written = generate(project)
    print(f"generated {len(written)} file(s) -> {project.gen_dir}")
    return 0


def cmd_build(args: argparse.Namespace) -> int:
    project = _project(args)
    db = load_database(project.devices_root)
    generate(project, db)
    board = project.load_board()
    chip = db.chips[board["chip"]]
    elf = build(project, chip)
    print(f"\nbuilt {elf}")
    return 0


def cmd_flash(args: argparse.Namespace) -> int:
    from .flash import flash  # noqa: PLC0415

    project = _project(args)
    db = load_database(project.devices_root)
    generate(project, db)
    board = project.load_board()
    chip = db.chips[board["chip"]]
    elf = build(project, chip)
    runner = flash(board, chip, elf)
    print(f"\nflashed {elf.name} via {runner}")
    return 0


def cmd_monitor(args: argparse.Namespace) -> int:
    from .monitor import monitor  # noqa: PLC0415

    project = _project(args)
    monitor(project.load_board())
    return 0


def cmd_run(args: argparse.Namespace) -> int:
    from .emit.common import EmitError  # noqa: PLC0415
    from .monitor import monitor  # noqa: PLC0415

    rc = cmd_flash(args)
    if rc != 0:
        return rc
    project = _project(args)
    try:
        monitor(project.load_board())
    except EmitError as exc:
        print(f"note: {exc}")
    return 0


def cmd_clean(args: argparse.Namespace) -> int:
    import shutil  # noqa: PLC0415

    root = Path(getattr(args, "project", ".") or ".").resolve()
    if not (root / "alloy.toml").exists():
        print(f"error: {root} is not an alloy project (no alloy.toml)", file=sys.stderr)
        return 1
    base = root / ".alloy"
    targets = [base] if args.all else (
        [base / "build-tree" / args.board, base / "generated" / args.board]
        if getattr(args, "board", None)
        else [base / "build-tree"]
    )
    removed = 0
    for target in targets:
        if target.exists():
            shutil.rmtree(target)
            removed += 1
            print(f"removed {target}")
    if removed == 0:
        print("nothing to clean")
    return 0


def cmd_set_board(args: argparse.Namespace) -> int:
    import re  # noqa: PLC0415

    root = Path(getattr(args, "project", ".") or ".").resolve()
    toml_path = root / "alloy.toml"
    if not toml_path.exists():
        print(f"error: {root} is not an alloy project (no alloy.toml)", file=sys.stderr)
        return 1
    # Validate against known boards first.
    project = load_project(root)
    boards_dir = project.alloy_root / "boards"
    if not (boards_dir / args.board_id / "board.json").exists():
        known = sorted(p.name for p in boards_dir.iterdir() if (p / "board.json").exists())
        print(f"error: unknown board '{args.board_id}' — known: {', '.join(known)}",
              file=sys.stderr)
        return 1
    text = toml_path.read_text()
    new_text, n = re.subn(
        r'(\[board\]\s*\n\s*id\s*=\s*")[^"]*(")',
        rf"\g<1>{args.board_id}\g<2>",
        text,
        count=1,
    )
    if n != 1:
        print("error: could not locate [board] id in alloy.toml", file=sys.stderr)
        return 1
    toml_path.write_text(new_text)
    print(f"board set to {args.board_id}")
    return 0


def cmd_setup(args: argparse.Namespace) -> int:
    from .toolchains import setup  # noqa: PLC0415

    families = set(args.family) if args.family else None
    return setup(families, check_only=args.check,
                 json_out=args.json, json_progress=args.json_progress)


def cmd_debug_info(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .debug import debug_info  # noqa: PLC0415

    project = _project(args)
    board = project.load_board()
    db = load_database(project.devices_root)
    chip = db.chips[board["chip"]]
    elf = project.build_dir / "out" / f"{project.name}.elf"
    info = debug_info(board, chip, elf if elf.exists() else None)
    info["schema"] = "alloy.debug_info.v1"
    if getattr(args, "json", False):
        print(json.dumps(info, indent=2))
    else:
        for key, value in info.items():
            print(f"{key:14} {value}")
    return 0


def cmd_test(args: argparse.Namespace) -> int:
    import shutil  # noqa: PLC0415

    from .project import _find_alloy_root  # noqa: PLC0415

    alloy_root = _find_alloy_root(Path.cwd())
    tests_dir = alloy_root / "tests"
    if not (tests_dir / "CMakeLists.txt").exists():
        print(f"no tests/ under {alloy_root} — host tests ship with the framework "
              "source tree, not the installed wheel", file=sys.stderr)
        return 1
    if shutil.which("cmake") is None:
        print("cmake not found on PATH (needed for the host test build)", file=sys.stderr)
        return 1

    build_dir = alloy_root / ".alloy" / "host-tests"
    configure = ["cmake", "-S", str(tests_dir), "-B", str(build_dir)]
    if shutil.which("ninja") is not None:
        configure += ["-G", "Ninja"]
    if args.no_sanitize:
        configure += ["-DALLOY_TEST_SANITIZE=OFF"]
    subprocess.run(configure, check=True)
    subprocess.run(["cmake", "--build", str(build_dir)], check=True)
    return subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "--output-on-failure"]
    ).returncode


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
    p_boards.add_argument("--json", action="store_true")
    p_boards.set_defaults(func=cmd_boards)

    p_clean = sub.add_parser("clean", help="remove per-board build trees")
    p_clean.add_argument("--project", default=".")
    p_clean.add_argument("--board", help="clean only this board's trees")
    p_clean.add_argument("--all", action="store_true", help="remove the whole .alloy/")
    p_clean.set_defaults(func=cmd_clean)

    p_setb = sub.add_parser("set-board", help="change the board in alloy.toml")
    p_setb.add_argument("board_id")
    p_setb.add_argument("--project", default=".")
    p_setb.set_defaults(func=cmd_set_board)

    p_setup = sub.add_parser("setup", help="verify/install toolchains")
    p_setup.add_argument("--family", action="append",
                         help="limit to a chip family (repeatable)")
    p_setup.add_argument("--check", action="store_true", help="report only")
    p_setup.add_argument("--json", action="store_true")
    p_setup.add_argument("--json-progress", action="store_true")
    p_setup.set_defaults(func=cmd_setup)

    p_dbg = sub.add_parser("debug-info", help="debug-server facts for a board")
    p_dbg.add_argument("--project", default=".")
    p_dbg.add_argument("--board")
    p_dbg.add_argument("--json", action="store_true")
    p_dbg.set_defaults(func=cmd_debug_info)

    p_test = sub.add_parser("test", help="build + run the host unit tests")
    p_test.add_argument("--no-sanitize", action="store_true",
                        help="disable AddressSanitizer/UBSan")
    p_test.set_defaults(func=cmd_test)

    for cmd, func in (("gen", cmd_gen), ("build", cmd_build), ("flash", cmd_flash),
                      ("monitor", cmd_monitor), ("run", cmd_run)):
        p = sub.add_parser(cmd)
        p.add_argument("--project", default=".")
        p.add_argument("--board", help="override the board declared in alloy.toml")
        p.set_defaults(func=func)

    args = parser.parse_args()
    try:
        sys.exit(args.func(args))
    except (ProjectError, EmitError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
    except subprocess.CalledProcessError as exc:
        cmd = exc.cmd[0] if isinstance(exc.cmd, list) else str(exc.cmd)
        print(f"error: {cmd} failed (exit {exc.returncode})", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        sys.exit(130)


if __name__ == "__main__":
    main()
