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


_MAIN_CPP_CLEAN = """\
// Clean project for a chosen MCU. board::init() brings up the clock; there are
// no roles yet — add them (led, debug_uart, i2c, spi…) in boards/{board}/board.json
// with the pins for YOUR hardware, then use them here (e.g. board::led.toggle()).
// Any example under the framework's examples/ shows the shape.
#include <alloy/board.hpp>

int main() {{
    board::init();
    for (;;) {{
        // your code here
    }}
}}
"""


def _new_from_chip(args: argparse.Namespace, target: Path, alloy_root: Path) -> int:
    from .chips import chip_clock, clean_board_json  # noqa: PLC0415
    from .project import _find_devices_root, packaged_alloy_root  # noqa: PLC0415

    devices_root = _find_devices_root(alloy_root)
    profiles, default_profile = chip_clock(devices_root, args.chip)
    clock = args.clock or default_profile
    if clock not in profiles:
        print(f"error: clock '{clock}' not a profile of {args.chip} — "
              f"available: {', '.join(profiles)}", file=sys.stderr)
        return 1

    board_id = target.name
    (target / "src").mkdir(parents=True)
    (target / "boards" / board_id).mkdir(parents=True)
    (target / "boards" / board_id / "board.json").write_text(
        clean_board_json(board_id, args.chip, clock))

    if alloy_root == packaged_alloy_root():
        toml_text = _ALLOY_TOML.split("[alloy]")[0].format(name=target.name, board=board_id)
    else:
        toml_text = _ALLOY_TOML.format(name=target.name, board=board_id, root=alloy_root)
    (target / "alloy.toml").write_text(toml_text)
    (target / "src" / "main.cpp").write_text(_MAIN_CPP_CLEAN.format(board=board_id))
    (target / ".gitignore").write_text(_GITIGNORE)

    print(f"created {target}/ — clean board for {args.chip} (clock: {clock})")
    print(f"  edit boards/{board_id}/board.json to add pins/roles for your hardware")
    print(f"  clock profiles for {args.chip}: {', '.join(profiles)}")
    print(f"next:  cd {target} && alloy build")
    return 0


def cmd_new(args: argparse.Namespace) -> int:
    from .project import _find_alloy_root  # noqa: PLC0415

    target = Path(args.name)
    if target.exists():
        print(f"error: {target} already exists", file=sys.stderr)
        return 1
    alloy_root = _find_alloy_root(Path.cwd())

    if getattr(args, "chip", None):
        return _new_from_chip(args, target, alloy_root)

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


def cmd_chips(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .chips import list_chips  # noqa: PLC0415
    from .project import _find_alloy_root, _find_devices_root  # noqa: PLC0415

    alloy_root = _find_alloy_root(Path.cwd())
    rows = list_chips(_find_devices_root(alloy_root))
    if args.vendor:
        rows = [r for r in rows if r["vendor"] == args.vendor]
    if getattr(args, "json", False):
        print(json.dumps({"schema": "alloy.chips.v1", "chips": rows}, indent=2))
        return 0
    if not rows:
        print("no chips" + (f" for vendor '{args.vendor}'" if args.vendor else ""))
        return 0
    for r in rows:
        print(f"{r['id']:28} {r.get('family', ''):14} {r.get('core') or ''}")
    return 0


def cmd_clock(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .chips import chip_info  # noqa: PLC0415
    from .clock_solver import solve  # noqa: PLC0415
    from .project import _find_alloy_root, _find_devices_root  # noqa: PLC0415

    alloy_root = _find_alloy_root(Path.cwd())
    info = chip_info(_find_devices_root(alloy_root), args.chip)
    family = info.get("family")
    if not family:
        print(f"error: no family for chip '{args.chip}'", file=sys.stderr)
        return 1
    result = solve(family, round(args.mhz * 1_000_000))
    print(json.dumps(result, indent=2))
    return 0


def cmd_chip_info(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .chips import chip_info  # noqa: PLC0415
    from .project import _find_alloy_root, _find_devices_root  # noqa: PLC0415

    alloy_root = _find_alloy_root(Path.cwd())
    info = chip_info(_find_devices_root(alloy_root), args.chip)
    print(json.dumps(info, indent=2))
    return 0


def _layout(args: argparse.Namespace) -> str:
    return "ram" if getattr(args, "ram", False) else "flash"


def _slot(args: argparse.Namespace) -> str | None:
    slot = getattr(args, "slot", None)
    if slot and getattr(args, "ram", False):
        raise EmitError("--slot and --ram are mutually exclusive (a slot build is "
                        "a flash placement)")
    return slot


def cmd_gen(args: argparse.Namespace) -> int:
    project = _project(args)
    written = generate(project, layout=_layout(args), slot=_slot(args))
    print(f"generated {len(written)} file(s) -> {project.gen_dir}")
    return 0


def cmd_build(args: argparse.Namespace) -> int:
    project = _project(args)
    db = load_database(project.devices_root)
    generate(project, db, layout=_layout(args), slot=_slot(args))
    board = project.load_board()
    chip = db.chips[board["chip"]]
    elf = build(project, chip)
    print(f"\nbuilt {elf}")
    return 0


def cmd_flash(args: argparse.Namespace) -> int:
    from .flash import flash  # noqa: PLC0415

    project = _project(args)
    db = load_database(project.devices_root)
    generate(project, db, layout=_layout(args))
    board = project.load_board()
    chip = db.chips[board["chip"]]
    elf = build(project, chip)
    runner = flash(board, chip, elf, ram=getattr(args, "ram", False))
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


def _find_renode() -> str | None:
    """Locate a runnable Renode launcher: PATH first, then an alloy-managed install
    under ~/.alloy/tools/ (the same place `alloy` keeps its cross toolchains). This
    lets `alloy emulate` run locally right after a Renode is dropped in there,
    without the user editing their PATH. Newest version wins."""
    import os  # noqa: PLC0415
    import shutil  # noqa: PLC0415

    on_path = shutil.which("renode")
    if on_path:
        return on_path
    tools = Path.home() / ".alloy" / "tools"
    # macOS .app bundle first, then Linux/portable layouts.
    for pattern in ("renode-*/Renode.app/Contents/MacOS/renode",
                    "renode*/Renode.app/Contents/MacOS/renode",
                    "renode*/renode", "renode*/bin/renode"):
        for hit in sorted(tools.glob(pattern), reverse=True):
            if hit.is_file() and os.access(hit, os.X_OK):
                return str(hit)
    return None


def cmd_emulate(args: argparse.Namespace) -> int:
    from .emit.renode import (  # noqa: PLC0415
        debug_uart_name,
        emit_renode_platform,
        emit_renode_script,
        renode_supported,
    )

    project = _project(args)
    db = load_database(project.devices_root)
    generate(project, db)
    board = project.load_board()
    chip = db.chips[board["chip"]]
    if not renode_supported(chip, board):
        print(f"error: board '{board['id']}' has no Renode emulation mapping "
              "(family/arch or debug UART not modeled by Renode)", file=sys.stderr)
        return 1

    elf = build(project, chip)
    out = elf.parent
    repl = out / f"{board['id']}.repl"
    resc = out / f"{board['id']}.resc"
    repl.write_text(emit_renode_platform(chip, board))
    resc.write_text(emit_renode_script(chip, board, str(repl), str(elf)))
    uart = debug_uart_name(chip, board)
    # `uart:` is machine-readable so CI can pass sysbus.<name> to the Robot test
    # without re-deriving a fact already in board.json.
    print(f"platform: {repl}\nscript:   {resc}\nuart:     sysbus.{uart}")
    if getattr(args, "emit_only", False):
        return 0

    renode = _find_renode()
    if renode is None:
        print("error: no runnable Renode found — put it on PATH, or drop a Renode "
              "install under ~/.alloy/tools/ (e.g. ~/.alloy/tools/renode-1.16.1/"
              "Renode.app), or pass --emit-only to just write the .repl/.resc",
              file=sys.stderr)
        return 1
    return subprocess.call(
        [renode, "--console", "--disable-xwt",
         "-e", f"include @{resc.name}",
         "-e", f"showAnalyzer {uart}",
         "-e", "start"],
        cwd=out,
    )


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

    # lwipopts.h is generated from data, same as firmware (facts, not a hand-tuned
    # header). The host profile turns lwIP's loopback (127.0.0.1) on so the Socket
    # facade gets a real TCP handshake with no hardware. Emit it before configuring
    # and hand the dir to the test CMake, which puts it on the include path.
    from .emit.lwipopts import NetProfile, render_lwipopts  # noqa: PLC0415

    lwipopts_dir = build_dir / "gen"
    lwipopts_dir.mkdir(parents=True, exist_ok=True)
    # dhcp=True so the DHCP client host test (net_dhcp_client_leases) links; loopback
    # + DHCP coexist and the other net tests are unaffected. ACD stays ON (the
    # firmware default) so the test drives the exact ACK -> CHECKING -> acd -> bind
    # path silicon runs, not a divergent immediate-bind shortcut.
    (lwipopts_dir / "lwipopts.h").write_text(
        render_lwipopts(NetProfile(host=True, dhcp=True)))

    configure = ["cmake", "-S", str(tests_dir), "-B", str(build_dir),
                 f"-DALLOY_LWIPOPTS_DIR={lwipopts_dir}"]
    if shutil.which("ninja") is not None:
        configure += ["-G", "Ninja"]
    if args.no_sanitize:
        configure += ["-DALLOY_TEST_SANITIZE=OFF"]
    subprocess.run(configure, check=True)
    subprocess.run(["cmake", "--build", str(build_dir)], check=True)
    return subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "--output-on-failure"]
    ).returncode


def cmd_lib(args: argparse.Namespace) -> int:
    from . import libs  # noqa: PLC0415
    from .project import _find_alloy_root  # noqa: PLC0415

    if args.lib_command == "add":
        return libs.cmd_lib_add(Path(args.project), args.name)
    alloy_root = _find_alloy_root(Path.cwd())
    if args.lib_command == "list":
        return libs.cmd_lib_list(alloy_root, category=args.category,
                                 json_out=getattr(args, "json", False))
    if args.lib_command == "search":
        return libs.cmd_lib_search(alloy_root, args.term)
    if args.lib_command == "info":
        return libs.cmd_lib_info(alloy_root, args.name)
    return 1


def cmd_frame_audit(args: argparse.Namespace) -> int:
    from .frame_audit import cmd_frame_audit as _audit  # noqa: PLC0415

    return _audit(Path(args.project), args.board, args.elf, args.objdump)


def main() -> None:
    parser = argparse.ArgumentParser(prog="alloy", description=__doc__)
    parser.add_argument("--version", action="version", version=__version__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_new = sub.add_parser("new", help="scaffold a new project")
    p_new.add_argument("name")
    p_new_src = p_new.add_mutually_exclusive_group(required=True)
    p_new_src.add_argument("--board", help="a curated board id (see `alloy boards`)")
    p_new_src.add_argument("--chip", help="any MCU id (see `alloy chips`) — makes a clean, "
                                          "editable board you fill in yourself")
    p_new.add_argument("--clock", help="clock profile for --chip (default: the chip's safe profile)")
    p_new.set_defaults(func=cmd_new)

    p_boards = sub.add_parser("boards", help="list known boards")
    p_boards.add_argument("--project", default=".")
    p_boards.add_argument("--json", action="store_true")
    p_boards.set_defaults(func=cmd_boards)

    p_chips = sub.add_parser("chips", help="list MCUs you can scaffold a clean board for")
    p_chips.add_argument("--vendor", help="filter by vendor (st, espressif, …)")
    p_chips.add_argument("--json", action="store_true", help="machine-readable (IDE integration)")
    p_chips.set_defaults(func=cmd_chips)

    p_chipinfo = sub.add_parser("chip-info",
                                help="clock profiles + pins + peripherals for one chip (JSON)")
    p_chipinfo.add_argument("chip", help="an MCU id (see `alloy chips`)")
    p_chipinfo.set_defaults(func=cmd_chip_info)

    p_clock = sub.add_parser("clock",
                             help="solve a PLL clock for a target frequency (JSON)")
    p_clock.add_argument("--chip", required=True, help="an MCU id (see `alloy chips`)")
    p_clock.add_argument("--mhz", type=float, required=True, help="target system clock in MHz")
    p_clock.set_defaults(func=cmd_clock)

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

    p_emu = sub.add_parser(
        "emulate", help="run the built firmware headless in Renode (data-generated platform)")
    p_emu.add_argument("--project", default=".")
    p_emu.add_argument("--board", help="override the board declared in alloy.toml")
    p_emu.add_argument("--emit-only", action="store_true",
                       help="write the .repl/.resc but do not launch Renode")
    p_emu.set_defaults(func=cmd_emulate)

    p_test = sub.add_parser("test", help="build + run the host unit tests")
    p_test.add_argument("--no-sanitize", action="store_true",
                        help="disable AddressSanitizer/UBSan")
    p_test.set_defaults(func=cmd_test)

    p_fa = sub.add_parser("frame-audit",
                          help="report async coroutine frame sizes vs task_storage<N>")
    p_fa.add_argument("--project", default=".")
    p_fa.add_argument("--board", help="override the board declared in alloy.toml")
    p_fa.add_argument("--elf", help="audit this ELF directly (skip project lookup)")
    p_fa.add_argument("--objdump", help="objdump binary to use (default: auto on PATH)")
    p_fa.set_defaults(func=cmd_frame_audit)

    p_lib = sub.add_parser("lib", help="discover and vendor ecosystem libraries")
    lib_sub = p_lib.add_subparsers(dest="lib_command", required=True)
    p_lib_list = lib_sub.add_parser("list", help="list registry libraries")
    p_lib_list.add_argument("--category", help="filter by category (sensor/display/…)")
    p_lib_list.add_argument("--json", action="store_true", help="machine-readable (IDE integration)")
    p_lib_list.set_defaults(func=cmd_lib)
    p_lib_search = lib_sub.add_parser("search", help="search the registry")
    p_lib_search.add_argument("term")
    p_lib_search.set_defaults(func=cmd_lib)
    p_lib_info = lib_sub.add_parser("info", help="show a library's manifest")
    p_lib_info.add_argument("name")
    p_lib_info.set_defaults(func=cmd_lib)
    p_lib_add = lib_sub.add_parser("add", help="vendor a library into ./libs and wire the build")
    p_lib_add.add_argument("name")
    p_lib_add.add_argument("--project", default=".")
    p_lib_add.set_defaults(func=cmd_lib)

    for cmd, func in (("gen", cmd_gen), ("build", cmd_build), ("flash", cmd_flash),
                      ("monitor", cmd_monitor), ("run", cmd_run)):
        p = sub.add_parser(cmd)
        p.add_argument("--project", default=".")
        p.add_argument("--board", help="override the board declared in alloy.toml")
        if cmd != "monitor":
            p.add_argument("--ram", action="store_true",
                           help="run-from-RAM: link every section into RAM and load it "
                                "there via the debugger (no flash erase; fast iterate)")
            p.add_argument("--slot", choices=("bl", "a", "b"),
                           help="A/B-update placement: link into the bootloader region "
                                "(bl) or a firmware slot (a/b) instead of whole flash")
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
