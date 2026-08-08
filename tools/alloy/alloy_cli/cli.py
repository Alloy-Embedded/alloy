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
        product_override=getattr(args, "product", None),
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


def _roots(args: argparse.Namespace) -> tuple[Path, Path | None]:
    """(framework root, project root or None). Board lookups work outside a
    project too — `alloy board-info nucleo_g071rb` from anywhere in the repo."""
    from .project import _find_alloy_root  # noqa: PLC0415

    project_dir = Path(getattr(args, "project", ".") or ".")
    try:
        project = load_project(project_dir)
    except ProjectError:
        return _find_alloy_root(project_dir.resolve()), None
    return project.alloy_root, project.root


def cmd_board_info(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .board_info import board_info  # noqa: PLC0415
    from .project import _find_devices_root  # noqa: PLC0415

    alloy_root, project_root = _roots(args)
    board_id = args.board_id
    if not board_id:
        if project_root is None:
            print("error: no board given and no alloy.toml here", file=sys.stderr)
            return 1
        board_id = load_project(project_root).board_id
    info = board_info(_find_devices_root(alloy_root), alloy_root, project_root, board_id)
    if getattr(args, "json", False):
        print(json.dumps(info, indent=2))
        return 0
    print(f"{info['id']}  {info['name']}")
    print(f"  chip     {info['chip']} ({info['part']})")
    print(f"  source   {info['source']}" + ("" if info["editable"] else "  (read-only)"))
    print(f"  clock    {info['clock']['profile']} — {info['clock']['description']}")
    print(f"  roles    {', '.join(sorted(info['roles'])) or '(none)'}")
    on = [name for name, value in info["caps"].items() if value]
    print(f"  caps     {', '.join(on) or '(none)'}")
    for issue in info["issues"]:
        print(f"  {issue['level']}: [{issue['stage']}] {issue['message']}", file=sys.stderr)
    return 1 if any(i["level"] == "error" for i in info["issues"]) else 0


def cmd_board_clone(args: argparse.Namespace) -> int:
    from .board_info import clone_board  # noqa: PLC0415

    alloy_root, project_root = _roots(args)
    if project_root is None:
        print("error: run this inside an alloy project (no alloy.toml)", file=sys.stderr)
        return 1
    dest = clone_board(alloy_root, project_root, args.source_id, args.new_id)
    print(f"cloned {args.source_id} -> {dest}")
    print(f"next:  alloy set-board {args.new_id}   # then edit it visually or by hand")
    return 0


def cmd_ci_init(args: argparse.Namespace) -> int:
    from .ci_init import write_workflow  # noqa: PLC0415

    alloy_root, project_root = _roots(args)
    if project_root is None:
        print("error: run this inside an alloy project (no alloy.toml)", file=sys.stderr)
        return 1
    project = load_project(project_root)
    path, details = write_workflow(
        project, alloy_root, force=args.force,
        boards=[b.strip() for b in args.boards.split(",")] if args.boards else None,
        every_board=args.all)
    print(f"wrote {path.relative_to(project_root)}")
    print(f"  builds on: {', '.join(details['boards'])}")
    if details["local_boards"]:
        print(f"  validates: {', '.join(details['local_boards'])}")
    # The dev layout points alloy.toml at a local checkout, which CI cannot see.
    if (project_root / "alloy.toml").read_text().find("[alloy]") != -1:
        print("  note: alloy.toml pins a local framework path; CI installs the "
              "published CLI instead")
    return 0


def cmd_matrix(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .matrix import format_table, known_boards, run_matrix  # noqa: PLC0415

    alloy_root, project_root = _roots(args)
    if project_root is None:
        print("error: run this inside an alloy project (no alloy.toml)", file=sys.stderr)
        return 1
    board_ids = ([b.strip() for b in args.boards.split(",") if b.strip()]
                 if args.boards else known_boards(alloy_root, project_root))
    if not board_ids:
        print("error: no boards to build", file=sys.stderr)
        return 1

    product_ids: list[str] | None = None
    if getattr(args, "products", False):
        from .products import known_products  # noqa: PLC0415

        product_ids = known_products(project_root / "products")
        if not product_ids:
            print("error: --products, but this project has no products/*.toml",
                  file=sys.stderr)
            return 1

    as_json = getattr(args, "json", False)
    if not as_json:
        dims = f"{len(board_ids)} board(s)"
        if product_ids:
            dims += f" x {len(product_ids)} product(s)"
        print(f"building {dims} from the same src/\n")
    report = run_matrix(project_root, board_ids, as_json, product_ids=product_ids)
    print(json.dumps(report, indent=2) if as_json else "\n" + format_table(report))
    return 0 if report["ok"] else 1


def cmd_svd(args: argparse.Namespace) -> int:
    from .devices import load_chip, load_registers  # noqa: PLC0415
    from .emit.svd import emit_svd  # noqa: PLC0415
    from .project import _find_devices_root  # noqa: PLC0415

    alloy_root, project_root = _roots(args)
    devices_root = _find_devices_root(alloy_root)

    chip_id = args.chip
    if not chip_id:
        if project_root is None:
            print("error: no --chip given and no alloy.toml here", file=sys.stderr)
            return 1
        project = load_project(project_root)
        chip_id = project.load_board()["chip"]

    chip = load_chip(devices_root, chip_id)
    document, skipped = emit_svd(chip, load_registers(devices_root))

    out = Path(args.out) if args.out else (
        (project_root or Path(".")) / ".alloy" / f"{chip_id.replace('/', '_')}.svd")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(document)
    described = document.count("<peripheral>") + document.count("<peripheral ")
    print(f"wrote {out}  ({described} peripheral(s))")
    if skipped:
        # Say what is NOT in there. A viewer showing 12 of 40 peripherals with
        # no explanation reads as a broken debugger.
        print(f"not described (no curated register file): {', '.join(skipped)}")
    return 0


def cmd_board_validate(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .board_info import resolve_board  # noqa: PLC0415
    from .board_validate import validate_board_file  # noqa: PLC0415
    from .project import _find_devices_root  # noqa: PLC0415

    alloy_root, project_root = _roots(args)
    if args.file:
        path = Path(args.file)
    else:
        board_id = args.board_id
        if not board_id:
            if project_root is None:
                print("error: no board given and no alloy.toml here", file=sys.stderr)
                return 1
            board_id = load_project(project_root).board_id
        path, _ = resolve_board(alloy_root, project_root, board_id)
    report = validate_board_file(_find_devices_root(alloy_root), path,
                                 project_root)

    if getattr(args, "json", False):
        print(json.dumps(report, indent=2))
    else:
        for issue in report["issues"]:
            where = ".".join(p for p in (issue["role"], issue["field"]) if p)
            hint = f"  (try: {', '.join(issue['suggestions'])})" if issue["suggestions"] else ""
            print(f"{issue['level']}: {where or 'board'}: {issue['message']}{hint}",
                  file=sys.stderr if issue["level"] == "error" else sys.stdout)
        if report["ok"]:
            print(f"{report['id'] or path}: ok")
    return 0 if report["ok"] else 1


def cmd_product_validate(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .product_validate import validate_product_file  # noqa: PLC0415

    _, project_root = _roots(args)
    if project_root is None:
        print("error: run this inside an alloy project (products/ lives in the "
              "project root)", file=sys.stderr)
        return 1
    report = validate_product_file(
        project_root, name=getattr(args, "name", None),
        file=Path(args.file) if getattr(args, "file", None) else None)

    if getattr(args, "json", False):
        print(json.dumps(report, indent=2))
    else:
        for issue in report["issues"]:
            hint = (f"  (try: {', '.join(issue['suggestions'])})"
                    if issue["suggestions"] else "")
            print(f"{issue['level']}: {issue['field'] or 'product'}: "
                  f"{issue['message']}{hint}",
                  file=sys.stderr if issue["level"] == "error" else sys.stdout)
        if report["ok"]:
            print(f"{report['id']}: ok (family: {report['family']})")
    return 0 if report["ok"] else 1


def cmd_size(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .devices import load_chip  # noqa: PLC0415
    from .sizes import size_report  # noqa: PLC0415

    project = _project(args)
    chip = load_chip(project.devices_root, project.load_board()["chip"])
    report = size_report(project, chip)
    if getattr(args, "json", False):
        print(json.dumps(report, indent=2))
        return 0
    if not report["available"]:
        print(f"no size report: {report['reason']}", file=sys.stderr)
        return 1
    for region in ("flash", "ram"):
        row = report[region]
        pct = f"{row['percent']:.1f}%" if row["percent"] is not None else "?"
        print(f"{region:6} {row['used']:>8} / {row['total']:>8} B  ({pct})")
    for region in (report["slots"] or {}).get("regions", []):
        if region["fits"] is not None:
            verdict = "fits" if region["fits"] else "TOO BIG"
            print(f"{region['name']:12} {region['size']:>8} B  image "
                  f"{report['slots']['image_bytes']} B — {verdict}")
    return 0


def cmd_crash(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from .crash import (  # noqa: PLC0415
        crash_report, locate_addr2line, parse_report_line, print_report,
    )

    values: dict[str, int] = {}
    if args.line:
        values.update(parse_report_line(args.line))
        if not values:
            print("error: --line carried no pc=0x…/lr=0x…/status=0x… tokens — "
                  "paste the device's RECOVERED FROM A FAULT line verbatim",
                  file=sys.stderr)
            return 1
    for reg in ("pc", "lr", "status", "address"):
        if (given := getattr(args, reg)) is not None:
            try:
                values[reg] = int(given, 16)
            except ValueError:
                print(f"error: --{reg} '{given}' is not a hex value", file=sys.stderr)
                return 1
    if not values:
        print("error: nothing to decode — pass --line, or --pc/--lr/--status",
              file=sys.stderr)
        return 1

    # The ELF is only needed for symbolication; a --status-only decode (or an
    # explicit --elf) must not require being inside a project at all.
    elf = Path(args.elf) if args.elf else None
    if elf is None and any(reg in values for reg in ("pc", "lr")):
        project = _project(args)
        elf = project.build_dir / "out" / f"{project.name}.elf"

    report = crash_report(values, elf, locate_addr2line())
    if getattr(args, "json", False):
        print(json.dumps(report, indent=2))
    else:
        print_report(report)
    # A decode that could not symbolize what it was asked to symbolize is a
    # failure the caller should see, even though the partial decode printed.
    return 1 if report["reason"] else 0


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
    hse_hz = round(args.hse * 1_000_000) if getattr(args, "hse", None) else None
    solved = solve(family, round(args.mhz * 1_000_000), external_hz=hse_hz,
                   bypass=getattr(args, "hse_bypass", False)) if args.mhz else None
    if not getattr(args, "graph", False):
        if solved is None:
            print("error: --mhz is required without --graph", file=sys.stderr)
            return 1
        print(json.dumps(solved, indent=2))
        return 0

    from .clock_graph import clock_graph  # noqa: PLC0415
    from .devices import load_chip, load_registers  # noqa: PLC0415

    devices_root = _find_devices_root(alloy_root)
    graph = clock_graph(load_chip(devices_root, args.chip), load_registers(devices_root),
                        chip_id=args.chip, profile_name=args.profile, solved=solved)
    print(json.dumps(graph, indent=2))
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

    if not getattr(args, "json", False):
        elf = build(project, chip)
        print(f"\nbuilt {elf}")
        return 0

    # --json: stdout must be JUST the envelope, but cmake/ninja/size write to
    # fd 1 from child processes, where redirect_stdout cannot reach. Point fd 1
    # at stderr for the duration so the whole build log stays visible and stdout
    # stays machine-readable.
    import json  # noqa: PLC0415
    import os  # noqa: PLC0415

    from .sizes import size_report  # noqa: PLC0415

    saved = os.dup(1)
    try:
        os.dup2(2, 1)
        elf = build(project, chip)
    finally:
        os.dup2(saved, 1)
        os.close(saved)
    report = size_report(project, chip, elf)
    report["schema"] = "alloy.build.v1"
    report["slot"] = _slot(args)
    report["layout"] = _layout(args)
    print(json.dumps(report, indent=2))
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
    from .monitor import monitor, monitor_ndjson  # noqa: PLC0415

    project = _project(args)
    board = project.load_board()
    if getattr(args, "json", False):
        # For an editor: one JSON object per line, and no terminal required.
        monitor_ndjson(board)
    else:
        monitor(board)
    return 0


def cmd_image(args: argparse.Namespace) -> int:
    from .ota_host import elf_to_bin, make_image  # noqa: PLC0415

    app = Path(args.app).read_bytes()
    if app[:4] == b"\x7fELF":  # accept the build's .elf directly — no objcopy needed
        app = elf_to_bin(app)
    out = Path(args.out) if args.out else Path(args.app).with_suffix(".img")
    image = make_image(app, args.set_version, app_offset=int(args.app_offset, 0),
                       key_id=args.key_id)
    if args.sign:
        from .ota_host import sign_image  # noqa: PLC0415

        key = Path(args.sign).read_text().strip()
        image = sign_image(image, bytes.fromhex(key))
    elif args.key_id:
        # A key_id on an UNSIGNED image is almost certainly a mistake: the field
        # only ever means anything to a device that checks signatures.
        print(f"warning: --key-id {args.key_id} on an image with no --sign — the "
              f"field is only read by devices that verify signatures",
              file=sys.stderr)
    out.write_bytes(image)
    print(f"image: {out}  ({len(image)} B = 32 B header + "
          f"{len(image) - 32} B payload, version {args.set_version}"
          f"{', SIGNED' if args.sign else ''}"
          f"{f', key_id {args.key_id}' if args.key_id else ''})")
    return 0


def cmd_keygen(args: argparse.Namespace) -> int:
    from .ota_host import generate_keypair  # noqa: PLC0415

    priv, pub = generate_keypair()
    out = Path(args.out)
    if out.exists() and not args.force:
        print(f"error: {out} exists — refusing to overwrite a signing key "
              f"(pass --force if you really mean it)", file=sys.stderr)
        return 1
    out.write_text(priv.hex() + "\n")
    out.chmod(0o600)
    pub_path = out.with_suffix(".pub")
    pub_path.write_text(pub.hex() + "\n")
    print(f"private key: {out}  (0600 — KEEP THIS SECRET AND BACKED UP: lose it "
          f"and fielded devices can never be updated again)\n"
          f"public key:  {pub_path}\n\n"
          f"Add to the project's alloy.toml to require signed updates:\n"
          f"  [ota]\n  public_key = \"{pub_path.name}\"\n\n"
          f"To ROTATE later, use the ring form instead — it is positional, so a\n"
          f"retired key keeps its slot and images keep naming the right key:\n"
          f"  [ota]\n  public_keys = [\"retired\", \"{pub_path.name}\"]\n"
          f"then sign with `alloy image --sign {out} --key-id 1`. Retiring a key\n"
          f"only takes effect on devices that receive the new BOOTLOADER.")
    return 0


def _secure_context(args: argparse.Namespace):
    """(board, chip, option map, slot layout|None) for `alloy secure`."""
    from . import secure  # noqa: PLC0415
    from .devices import load_chip, load_registers  # noqa: PLC0415
    from .emit.slots import slot_layout  # noqa: PLC0415

    project = _project(args)
    board = project.load_board()
    chip = load_chip(project.devices_root, board["chip"])
    omap = secure.option_map(chip, load_registers(project.devices_root))
    try:
        layout = slot_layout(chip)
    except EmitError:
        layout = None  # honest: status will say there is nothing to derive
    return board, chip, omap, layout


def cmd_secure_status(args: argparse.Namespace) -> int:
    from . import secure  # noqa: PLC0415

    board, chip, omap, layout = _secure_context(args)
    status = secure.read_status(board, chip, omap, layout)
    print(secure.format_status(board["id"], chip, omap, layout, status))
    if status.rdp_level == 0:
        print("hint: `alloy secure apply --rdp 1 --wrp-bootloader` locks a "
              "production unit (guarded — it explains itself first)")
    return 0


def cmd_secure_apply(args: argparse.Namespace) -> int:
    from . import secure  # noqa: PLC0415

    board, chip, omap, layout = _secure_context(args)
    if args.wrp_bootloader and layout is None:
        print(f"error: chip {chip['part']} has no A/B slot layout — the "
              f"bootloader WRP range derives from it, so there is nothing to "
              f"protect yet", file=sys.stderr)
        return 1

    status = secure.read_status(board, chip, omap, layout)
    print(secure.format_status(board["id"], chip, omap, layout, status))
    print()

    wrp_needed = bool(args.wrp_bootloader) and not status.bootloader_protected
    if args.wrp_bootloader and status.bootloader_protected:
        print("bootloader already write-protected — WRP unchanged")

    guards = secure.apply_guards(
        chip["part"], status.rdp_level, args.rdp, wrp_needed,
        accept_mass_erase=args.accept_mass_erase, permanent=args.permanent)
    for warning in guards.warnings:
        print(f"WARNING: {warning}\n")
    if guards.noop:
        print(guards.noop)
        return 0
    if guards.refusal:
        print(f"error: {guards.refusal}", file=sys.stderr)
        return 1

    plan = secure.plan_apply(omap, chip, layout, status, args.rdp, wrp_needed)
    if args.dry_run:
        print("dry run — openocd would execute:")
        for cmd in plan.openocd:
            print(f"  {cmd}")
        return 0

    # The typed phrase is the confirmation for the destructive/permanent paths;
    # --yes never bypasses it (it only stands in for the y/N of routine steps).
    # Both read stdin, so a factory line can pipe them — the warnings above
    # printed regardless.
    if guards.phrase:
        try:
            typed = input(f"type exactly `{guards.phrase}` to continue: ")
        except EOFError:
            typed = ""
        if typed.strip() != guards.phrase:
            print("error: confirmation phrase did not match — nothing was "
                  "written", file=sys.stderr)
            return 1
    elif guards.confirm and not args.yes:
        try:
            answer = input("proceed? [y/N] ")
        except EOFError:
            answer = ""
        if answer.strip().lower() not in ("y", "yes"):
            print("aborted — nothing was written")
            return 1

    verified, message = secure.run_apply(board, chip, plan)
    print(message)
    if verified:
        print("run `alloy secure status` any time — it is always safe")
    return 0


def _provision_context(args: argparse.Namespace):
    """(board, chip, layout) for `alloy provision`. Unlike `alloy secure`, a
    missing slot layout is FATAL here: the identity page's address comes from
    the layout, and there is no honest fallback address to guess."""
    from .devices import load_chip  # noqa: PLC0415
    from .emit.slots import slot_layout  # noqa: PLC0415

    project = _project(args)
    board = project.load_board()
    chip = load_chip(project.devices_root, board["chip"])
    try:
        layout = slot_layout(chip)
    except EmitError as exc:
        raise EmitError(
            f"chip {chip['part']} has no A/B slot layout, so it has no "
            f"provisioning page either — identity lives in a region the layout "
            f"carves out. ({exc})") from exc
    return board, chip, layout


def cmd_provision_write(args: argparse.Namespace) -> int:
    import tempfile  # noqa: PLC0415

    from . import provision  # noqa: PLC0415

    identity = provision.make_identity(args.serial, args.mac, args.hw_rev,
                                       args.batch)
    if args.output:  # offline: encode to a file, no probe, no board needed
        Path(args.output).write_bytes(provision.encode(identity))
        print(f"wrote {provision.RECORD_SIZE}-byte identity record to "
              f"{args.output} — {identity.describe()}")
        return 0

    board, chip, layout = _provision_context(args)
    base, size = provision.provision_region(layout)
    print(f"{board['id']} ({chip['vendor']}/{chip['part']})")
    print(f"  identity page: {base:#010x} +{size} B (from the slot layout)")
    print(f"  writing: {identity.describe()}")
    if args.dry_run:
        print("dry run — openocd would execute:")
        for cmd in provision.write_commands(base, Path("<record>.bin"),
                                            Path("<readback>.bin")):
            print(f"  {cmd}")
        return 0
    with tempfile.TemporaryDirectory(prefix="alloy-provision-") as tmp:
        on_device = provision.run_write(board, chip, layout, identity, Path(tmp))
    print(f"verified by readback: {on_device.describe()}")
    print("this device is provisioned. `alloy secure apply` LAST — on "
          "uniform-page flash it write-protects this very page.")
    return 0


def cmd_provision_read(args: argparse.Namespace) -> int:
    import tempfile  # noqa: PLC0415

    from . import provision  # noqa: PLC0415

    if args.file:  # offline: decode a record someone already dumped
        print(provision.decode(Path(args.file).read_bytes()).describe())
        return 0
    board, chip, layout = _provision_context(args)
    base, _ = provision.provision_region(layout)
    with tempfile.TemporaryDirectory(prefix="alloy-provision-") as tmp:
        identity = provision.run_read(board, chip, layout, Path(tmp))
    print(f"{board['id']} @ {base:#010x}: {identity.describe()}")
    return 0


def cmd_ports(args: argparse.Namespace) -> int:
    import json  # noqa: PLC0415

    from serial.tools import list_ports  # noqa: PLC0415

    ports = [{"device": p.device, "description": p.description or ""}
             for p in sorted(list_ports.comports(), key=lambda p: p.device)]
    if getattr(args, "json", False):
        print(json.dumps({"schema": "alloy.ports.v1", "ports": ports}))
    else:
        for p in ports:
            print(f"{p['device']}  {p['description']}")
        if not ports:
            print("no serial ports found")
    return 0


def cmd_update(args: argparse.Namespace) -> int:
    import serial  # noqa: PLC0415

    from .ota_host import UpdateError, update  # noqa: PLC0415

    # One positional image (you know the target slot), or --image-a/--image-b so
    # the right one is picked after the device reports its target in INFO.
    if args.image_a or args.image_b:
        if args.image:
            print("error: give either a single image or --image-a/--image-b, not both",
                  file=sys.stderr)
            return 2
        image: bytes | dict[int, bytes] = {}
        if args.image_a:
            image[0] = Path(args.image_a).read_bytes()
        if args.image_b:
            image[1] = Path(args.image_b).read_bytes()
    elif args.image:
        image = Path(args.image).read_bytes()
    else:
        print("error: an image is required (positional, or --image-a/--image-b)",
              file=sys.stderr)
        return 2
    # serial_for_url accepts device paths AND pyserial URLs (socket://host:port —
    # how the emulation E2E drives a Renode socket terminal — rfc2217://, etc.).
    link = serial.serial_for_url(args.port, baudrate=args.baud, timeout=args.timeout)

    def progress(done: int, total: int) -> None:
        print(f"\r  {done}/{total} B ({100 * done // total}%)", end="", flush=True)

    try:
        info = update(link, image, progress=progress)
    except UpdateError as exc:
        print(f"\nerror: {exc}", file=sys.stderr)
        return 1
    finally:
        link.close()
    print(f"\nupdate accepted -> slot {'B' if info['target_slot'] else 'A'} "
          f"(device was running version {info['running_version']}); "
          f"device now reboots into a TRIAL boot — it must be confirmed to stick")
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
    if args.all:
        targets = [base]
    elif getattr(args, "board", None):
        # A board's trees include its product-keyed variants (<board>+<product>).
        targets = []
        for kind in ("build-tree", "generated"):
            targets.append(base / kind / args.board)
            targets += sorted((base / kind).glob(f"{args.board}+*"))
    else:
        targets = [base / "build-tree"]
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
    # Validate against known boards first — including the project's OWN boards/,
    # which is where `alloy board-clone` puts an editable copy. Only checking the
    # framework's boards made a cloned board unselectable.
    from .board_info import list_board_ids, resolve_board  # noqa: PLC0415

    project = load_project(root)
    try:
        resolve_board(project.alloy_root, root, args.board_id)
    except EmitError:
        known = ", ".join(list_board_ids(project.alloy_root, root))
        print(f"error: unknown board '{args.board_id}' — known: {known}",
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

    p_binfo = sub.add_parser(
        "board-info", help="roles, capabilities, used pins and problems of a board "
                           "(curated or project-local)")
    p_binfo.add_argument("board_id", nargs="?",
                         help="board id (default: the one in alloy.toml)")
    p_binfo.add_argument("--project", default=".")
    p_binfo.add_argument("--json", action="store_true", help="machine-readable (IDE integration)")
    p_binfo.set_defaults(func=cmd_board_info)

    p_bclone = sub.add_parser(
        "board-clone", help="copy a board into this project as an editable one")
    p_bclone.add_argument("source_id", help="board to copy (see `alloy boards`)")
    p_bclone.add_argument("new_id", help="id for the copy (letters, digits, underscore)")
    p_bclone.add_argument("--project", default=".")
    p_bclone.set_defaults(func=cmd_board_clone)

    p_ci = sub.add_parser(
        "ci-init",
        help="write a GitHub Actions workflow that validates this project's "
             "boards and builds its sources for each of them")
    p_ci.add_argument("--project", default=".")
    p_ci.add_argument("--force", action="store_true", help="replace an existing workflow")
    p_ci.add_argument("--boards", help="comma-separated boards to build in CI")
    p_ci.add_argument("--all", action="store_true",
                      help="build every supported board, not just the ones this "
                           "project targets")
    p_ci.set_defaults(func=cmd_ci_init)

    p_mtx = sub.add_parser(
        "matrix",
        help="build this project for every supported board — the same src/, one "
             "table of what fits where")
    p_mtx.add_argument("--boards", help="comma-separated subset (default: all)")
    p_mtx.add_argument("--products", action="store_true",
                       help="cross every board with every products/*.toml — "
                            "the boards x products build table")
    p_mtx.add_argument("--project", default=".")
    p_mtx.add_argument("--json", action="store_true")
    p_mtx.set_defaults(func=cmd_matrix)

    p_svd = sub.add_parser(
        "svd",
        help="write a CMSIS-SVD file for the chip, so a debugger can show its "
             "peripheral registers")
    p_svd.add_argument("--chip", help="chip id (default: the project's board's chip)")
    p_svd.add_argument("-o", "--out", help="output path (default: .alloy/<chip>.svd)")
    p_svd.add_argument("--project", default=".")
    p_svd.set_defaults(func=cmd_svd)

    p_bval = sub.add_parser(
        "board-validate",
        help="every problem in a board, located, with the pins that would work "
             "(the route static_assert, moved to config time)")
    p_bval.add_argument("board_id", nargs="?",
                        help="board id (default: the one in alloy.toml)")
    p_bval.add_argument("--file", help="validate this board.json instead ('-' = stdin, "
                                       "so an editor can check before writing)")
    p_bval.add_argument("--project", default=".")
    p_bval.add_argument("--json", action="store_true")
    p_bval.set_defaults(func=cmd_board_validate)

    p_pval = sub.add_parser(
        "product-validate",
        help="every problem in a product TOML, located, with a way out "
             "(the mirror of board-validate for the product dimension)")
    p_pval.add_argument("name", nargs="?",
                        help="product name (default: the one alloy.toml "
                             "[product] selects)")
    p_pval.add_argument("--file", help="validate this product .toml instead "
                                       "('-' = stdin, so an editor can check "
                                       "before writing)")
    p_pval.add_argument("--project", default=".")
    p_pval.add_argument("--json", action="store_true")
    p_pval.set_defaults(func=cmd_product_validate)

    p_size = sub.add_parser(
        "size", help="flash/RAM the LAST build uses, against the chip's memories "
                     "(and its update slots)")
    p_size.add_argument("--project", default=".")
    p_size.add_argument("--board", help="override the board declared in alloy.toml")
    p_size.add_argument("--product",
                        help="override the product declared in alloy.toml — must "
                             "match the build being sized (the tree is keyed by "
                             "board+product)")
    p_size.add_argument("--json", action="store_true")
    p_size.set_defaults(func=cmd_size)

    p_crash = sub.add_parser(
        "crash", help="decode a device's crash report — pc/lr to file:line via "
                      "addr2line, and the ARMv7-M fault status into words")
    p_crash.add_argument("--line",
                         help="a pasted 'RECOVERED FROM A FAULT pc=0x… lr=0x… "
                              "status=0x…' UART line (parsed for any "
                              "pc/lr/sp/psr/status/address tokens)")
    p_crash.add_argument("--pc", help="faulting instruction address (hex)")
    p_crash.add_argument("--lr", help="link register at the fault (hex)")
    p_crash.add_argument("--status", help="the record's CFSR value (hex)")
    p_crash.add_argument("--address", help="the record's faulting address (hex)")
    p_crash.add_argument("--elf",
                         help="ELF the device was running (default: this "
                              "board's last build, as `alloy size` resolves it)")
    p_crash.add_argument("--project", default=".")
    p_crash.add_argument("--board", help="override the board declared in alloy.toml")
    p_crash.add_argument("--product",
                         help="override the product declared in alloy.toml — must "
                              "match the build being decoded")
    p_crash.add_argument("--json", action="store_true")
    p_crash.set_defaults(func=cmd_crash)

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
    p_clock.add_argument("--mhz", type=float,
                         help="target system clock in MHz (solve a PLL for it)")
    p_clock.add_argument("--graph", action="store_true",
                         help="the WHOLE clock: sources, buses, and the clock each "
                              "peripheral is fed, with what it implies")
    p_clock.add_argument("--profile",
                         help="with --graph: a named profile of the chip "
                              "(default: the boot-safe one)")
    p_clock.add_argument("--hse", type=float, metavar="MHZ",
                         help="run the PLL from the board's external oscillator at "
                              "this frequency in MHz (a crystal — what a product "
                              "uses; USB, CAN and high baud rates need one)")
    p_clock.add_argument("--hse-bypass", action="store_true",
                         help="with --hse: the input is a DRIVEN clock signal, not a "
                              "crystal (an active oscillator, or the 8 MHz an "
                              "ST-Link feeds a Nucleo)")
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
    p_dbg.add_argument("--product",
                       help="the product whose build tree to report on")
    p_dbg.add_argument("--json", action="store_true")
    p_dbg.set_defaults(func=cmd_debug_info)

    p_emu = sub.add_parser(
        "emulate", help="run the built firmware headless in Renode (data-generated platform)")
    p_emu.add_argument("--project", default=".")
    p_emu.add_argument("--board", help="override the board declared in alloy.toml")
    p_emu.add_argument("--product",
                       help="override the product declared in alloy.toml")
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
    p_fa.add_argument("--product",
                      help="the product whose build tree to audit")
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

    p_img = sub.add_parser(
        "image", help="pack an app binary into a signed-format update image "
                      "([header|payload], alloy/ota/image.hpp)")
    p_img.add_argument("app", help="raw app binary (objcopy -O binary of a --slot build)")
    p_img.add_argument("-o", "--out", help="output path (default: <app>.img)")
    p_img.add_argument("--set-version", type=int, required=True,
                       help="monotonic image version (higher = newer). A device "
                            "REFUSES an image below the highest version it can "
                            "prove it already holds (anti-rollback)")
    p_img.add_argument("--sign", metavar="KEYFILE",
                       help="Ed25519 private key from `alloy keygen` — appends a "
                            "signature trailer so signing-enabled devices accept it")
    p_img.add_argument("--key-id", type=int, default=0,
                       help="which entry of the device's trusted key ring "
                            "([ota] public_keys) signed this image; default 0 = "
                            "the first/original key")
    p_img.add_argument("--app-offset", default="0x200",
                       help="vector-table offset inside the slot (must match "
                            "the --slot link; default 0x200)")
    p_img.set_defaults(func=cmd_image)

    p_key = sub.add_parser(
        "keygen", help="generate an Ed25519 update-signing keypair")
    p_key.add_argument("-o", "--out", default="update_key",
                       help="private key path (public key gets .pub)")
    p_key.add_argument("--force", action="store_true",
                       help="overwrite an existing private key")
    p_key.set_defaults(func=cmd_keygen)

    p_sec = sub.add_parser(
        "secure", help="flash readout/write protection (RDP/WRP) via the "
                       "debug probe — production locking, heavily guarded")
    sec_sub = p_sec.add_subparsers(dest="secure_cmd", required=True)
    p_sst = sec_sub.add_parser(
        "status", help="read + decode the current RDP level and WRP ranges — "
                       "always safe, never writes")
    p_sst.set_defaults(func=cmd_secure_status)
    p_sap = sec_sub.add_parser(
        "apply", help="program option bytes (RDP level, bootloader WRP). "
                      "Destructive transitions refuse without explicit flags "
                      "AND a typed confirmation phrase")
    p_sap.add_argument("--rdp", type=int, choices=(0, 1, 2),
                       help="target readout-protection level. 1 -> 0 "
                            "MASS-ERASES the chip; 2 is PERMANENT")
    p_sap.add_argument("--wrp-bootloader", action="store_true",
                       help="write-protect the bootloader region (range "
                            "derived from the A/B slot layout, never typed)")
    p_sap.add_argument("--yes", action="store_true",
                       help="answer y to the routine confirmation (mass-erase "
                            "and permanent paths still demand their typed "
                            "phrase)")
    p_sap.add_argument("--accept-mass-erase", action="store_true",
                       help="required to regress RDP 1 -> 0: the hardware "
                            "erases ALL flash before dropping protection")
    p_sap.add_argument("--permanent", action="store_true",
                       help="required for --rdp 2: disables the debug port "
                            "FOREVER — a manufacturing decision")
    p_sap.add_argument("--dry-run", action="store_true",
                       help="print the openocd commands and exit without "
                            "touching the device")
    p_sap.set_defaults(func=cmd_secure_apply)
    for p in (p_sst, p_sap):
        p.add_argument("--project", default=".")
        p.add_argument("--board", help="override the board declared in alloy.toml")

    p_prov = sub.add_parser(
        "provision", help="write/read a board's per-device factory identity "
                          "(serial, MAC) in the layout's provisioning page — "
                          "outside both slots, so updates never touch it")
    prov_sub = p_prov.add_subparsers(dest="provision_cmd", required=True)
    p_pw = prov_sub.add_parser(
        "write", help="program this device's identity through the probe and "
                      "verify it by reading it back")
    p_pw.add_argument("--serial", required=True,
                      help="device serial number, at most 16 ASCII bytes")
    p_pw.add_argument("--mac", help="EUI-48, aa:bb:cc:dd:ee:ff (multicast and "
                                    "broadcast addresses are refused)")
    p_pw.add_argument("--hw-rev", type=int, default=0, metavar="N",
                      help="board/PCB revision (0..65535, default 0)")
    p_pw.add_argument("--batch", type=int, default=0, metavar="N",
                      help="factory lot id (0..4294967295, default 0)")
    p_pw.add_argument("-o", "--output", metavar="FILE",
                      help="OFFLINE: write the encoded record to a file "
                           "instead of a device (no probe, no board needed) — "
                           "for mass pre-programming and test fixtures")
    p_pw.add_argument("--dry-run", action="store_true",
                      help="print the openocd commands and exit without "
                           "touching the device")
    p_pw.set_defaults(func=cmd_provision_write)
    p_pr = prov_sub.add_parser(
        "read", help="read this device's identity back — always safe, never writes")
    p_pr.add_argument("--file", metavar="FILE",
                      help="OFFLINE: decode a previously dumped record instead "
                           "of reading a device")
    p_pr.set_defaults(func=cmd_provision_read)
    for p in (p_pw, p_pr):
        p.add_argument("--project", default=".")
        p.add_argument("--board", help="override the board declared in alloy.toml")

    p_ports = sub.add_parser("ports", help="list serial ports")
    p_ports.add_argument("--json", action="store_true")
    p_ports.set_defaults(func=cmd_ports)

    p_upd = sub.add_parser(
        "update", help="send an update image to a device over serial (the "
                       "bootloader's UART update window)")
    p_upd.add_argument("image", nargs="?",
                       help="packed image from `alloy image` (or use --image-a/-b)")
    p_upd.add_argument("--image-a", help="slot-A image; picked if the device targets A")
    p_upd.add_argument("--image-b", help="slot-B image; picked if the device targets B")
    p_upd.add_argument("--port", required=True, help="serial port (e.g. /dev/ttyUSB0)")
    p_upd.add_argument("--baud", type=int, default=115200)
    p_upd.add_argument("--timeout", type=float, default=5.0,
                       help="per-reply timeout in seconds (slot erase happens "
                            "during HELLO, so keep this generous)")
    p_upd.set_defaults(func=cmd_update)

    for cmd, func in (("gen", cmd_gen), ("build", cmd_build), ("flash", cmd_flash),
                      ("monitor", cmd_monitor), ("run", cmd_run)):
        p = sub.add_parser(cmd)
        p.add_argument("--project", default=".")
        p.add_argument("--board", help="override the board declared in alloy.toml")
        if cmd == "monitor":
            p.add_argument("--json", action="store_true",
                           help="stream NDJSON (one object per line) instead of raw "
                                "bytes, and take TX as whole lines on stdin")
        if cmd != "monitor":
            p.add_argument("--ram", action="store_true",
                           help="run-from-RAM: link every section into RAM and load it "
                                "there via the debugger (no flash erase; fast iterate)")
            p.add_argument("--slot", choices=("bl", "a", "b"),
                           help="A/B-update placement: link into the bootloader region "
                                "(bl) or a firmware slot (a/b) instead of whole flash")
            p.add_argument("--product",
                           help="override the product declared in alloy.toml "
                                "[product] (one firmware, many products)")
        if cmd == "build":
            p.add_argument("--json", action="store_true",
                           help="print an alloy.build.v1 envelope (ELF + memory use) on "
                                "stdout; the build log goes to stderr")
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
