"""Project scaffolding (`alloy new`).

Generates a self-contained downstream project tree for a chosen board:

- ``CMakeLists.txt`` -- consumes Alloy via ``add_subdirectory(${ALLOY_ROOT})`` with the
  active SDK path baked in,
- ``CMakePresets.json`` -- ``debug`` and ``release`` presets pointing at the SDK,
- ``src/main.cpp`` -- minimal blink starter,
- ``.vscode/{settings,tasks,launch}.json`` -- clangd, build/flash/monitor tasks, OpenOCD
  debug config when the board declares one,
- ``README.md`` and ``.gitignore``.

Board metadata is read from ``_boards.toml`` packaged with alloy-cli. The runtime side of
board configuration stays in ``cmake/board_manifest.cmake`` inside the SDK; this catalog
only captures what the scaffolder needs (toolchain name, OpenOCD config, debug probe).
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

if sys.version_info >= (3, 11):
    import tomllib  # type: ignore[import-not-found]
else:  # pragma: no cover
    import tomli as tomllib  # type: ignore[no-redef]

from jinja2 import Environment, FileSystemLoader, StrictUndefined

from . import sdk, toolchains


class ScaffoldError(RuntimeError):
    """Raised for user-facing scaffold failures."""


PROJECT_NAME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")


# --- board catalog ---------------------------------------------------------------------


def _boards_path() -> Path:
    return Path(__file__).with_name("_boards.toml")


def load_boards() -> dict:
    return tomllib.loads(_boards_path().read_text(encoding="utf-8"))


@dataclass(frozen=True)
class Board:
    name: str
    display_name: str
    vendor: str
    family: str
    toolchain: str
    debug_probe: str | None
    openocd_config: str | None
    openocd_config_files: tuple[str, ...]

    @property
    def has_openocd(self) -> bool:
        return self.openocd_config is not None or bool(self.openocd_config_files)


def get_board(name: str) -> Board:
    boards = load_boards()
    if name not in boards:
        known = ", ".join(sorted(boards))
        raise ScaffoldError(f"unknown board {name!r}; known: {known}")
    entry = boards[name]
    return Board(
        name=name,
        display_name=entry.get("display_name", name),
        vendor=entry.get("vendor", ""),
        family=entry.get("family", ""),
        toolchain=entry.get("toolchain", ""),
        debug_probe=entry.get("debug_probe"),
        openocd_config=entry.get("openocd_config"),
        openocd_config_files=tuple(entry.get("openocd_config_files", [])),
    )


# --- preflight -------------------------------------------------------------------------


@dataclass(frozen=True)
class Preflight:
    alloy_root: Path
    toolchain_bin: Path | None
    gdb_path: str
    toolchain_file: str | None


def _resolve_alloy_root(explicit: Path | None) -> Path:
    if explicit is not None:
        if not (explicit / "CMakeLists.txt").is_file():
            raise ScaffoldError(f"--alloy-root {explicit} is not an Alloy checkout")
        return explicit.resolve()
    active = sdk.active_runtime_path()
    if active is None:
        raise ScaffoldError(
            "no active SDK selected; run `alloy sdk install <version>` first, "
            "or pass --alloy-root <path>"
        )
    return active


def _resolve_toolchain(board: Board) -> tuple[Path | None, str, str | None]:
    """Returns (bin_dir, gdb_path, toolchain_file).

    Missing toolchain is non-fatal: the project still scaffolds; the user is told what to
    install. We do not auto-install here because the pinned SHAs may still be TODO; the
    explicit `alloy toolchain install` step is the consent boundary.
    """
    try:
        binary = toolchains.which(board.toolchain)
    except toolchains.ToolchainError:
        return None, _gdb_for(board.toolchain), None
    bin_dir = binary.parent
    gdb = bin_dir / _gdb_for(board.toolchain)
    return bin_dir, str(gdb if gdb.is_file() else _gdb_for(board.toolchain)), None


def _gdb_for(toolchain: str) -> str:
    if toolchain == "arm-none-eabi-gcc":
        return "arm-none-eabi-gdb"
    if toolchain == "avr-gcc":
        return "avr-gdb"
    return "gdb"


def preflight(board: Board, *, alloy_root: Path | None = None) -> Preflight:
    root = _resolve_alloy_root(alloy_root)
    bin_dir, gdb_path, toolchain_file = _resolve_toolchain(board)
    return Preflight(
        alloy_root=root,
        toolchain_bin=bin_dir,
        gdb_path=gdb_path,
        toolchain_file=toolchain_file,
    )


# --- generation ------------------------------------------------------------------------


def _templates_dir() -> Path:
    return Path(__file__).with_name("_templates")


def _make_env() -> Environment:
    return Environment(
        loader=FileSystemLoader(str(_templates_dir())),
        undefined=StrictUndefined,
        keep_trailing_newline=True,
        trim_blocks=False,
        lstrip_blocks=False,
    )


def _render(env: Environment, template: str, **ctx) -> str:
    return env.get_template(template).render(**ctx)


def _validate_project_name(name: str) -> None:
    if not PROJECT_NAME_RE.match(name):
        raise ScaffoldError(
            f"invalid project name {name!r}; must match {PROJECT_NAME_RE.pattern}"
        )


def _validate_destination(dest: Path) -> None:
    if dest.exists() and any(dest.iterdir()):
        raise ScaffoldError(f"destination {dest} exists and is not empty")


@dataclass(frozen=True)
class ScaffoldResult:
    project: str
    board: Board
    destination: Path
    files_written: tuple[Path, ...]
    preflight: Preflight


def scaffold(
    *,
    board_name: str,
    destination: Path,
    project_name: str | None = None,
    alloy_root: Path | None = None,
) -> ScaffoldResult:
    board = get_board(board_name)
    project = project_name or destination.name.replace("-", "_")
    _validate_project_name(project)

    dest = destination.resolve()
    _validate_destination(dest)

    pf = preflight(board, alloy_root=alloy_root)

    ctx = {
        "project": project,
        "board": board.name,
        "display_name": board.display_name,
        "vendor": board.vendor,
        "family": board.family,
        "alloy_root": str(pf.alloy_root),
        "toolchain_bin": str(pf.toolchain_bin) if pf.toolchain_bin else None,
        "toolchain_file": pf.toolchain_file,
        "has_openocd": board.has_openocd,
        "gdb_path": pf.gdb_path,
    }

    env = _make_env()
    dest.mkdir(parents=True, exist_ok=True)
    (dest / "src").mkdir(exist_ok=True)
    (dest / ".vscode").mkdir(exist_ok=True)

    written: list[Path] = []

    def write(rel: str, body: str) -> None:
        path = dest / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(body, encoding="utf-8")
        written.append(path)

    write("CMakeLists.txt", _render(env, "CMakeLists.txt.j2", **ctx))
    write("CMakePresets.json", _render(env, "CMakePresets.json.j2", **ctx))
    # Validate the rendered presets file is parseable JSON before we declare success.
    json.loads((dest / "CMakePresets.json").read_text(encoding="utf-8"))
    write("src/main.cpp", _render(env, "main.cpp.j2", **ctx))
    write(".gitignore", _render(env, "gitignore.j2", **ctx))
    write("README.md", _render(env, "README.md.j2", **ctx))
    write(".vscode/settings.json", _render(env, "vscode/settings.json.j2", **ctx))
    write(".vscode/tasks.json", _render(env, "vscode/tasks.json.j2", **ctx))
    write(".vscode/launch.json", _render(env, "vscode/launch.json.j2", **ctx))
    json.loads((dest / ".vscode/launch.json").read_text(encoding="utf-8"))
    json.loads((dest / ".vscode/tasks.json").read_text(encoding="utf-8"))
    json.loads((dest / ".vscode/settings.json").read_text(encoding="utf-8"))

    return ScaffoldResult(
        project=project,
        board=board,
        destination=dest,
        files_written=tuple(written),
        preflight=pf,
    )


# --- argparse wiring -------------------------------------------------------------------


def add_subparser(parent: argparse._SubParsersAction) -> None:
    new_p = parent.add_parser(
        "new",
        help="scaffold a new firmware project for a supported board",
    )
    new_p.add_argument("path", help="destination directory for the new project")
    new_p.add_argument("--board", required=True, help="target board (see `alloy new --list-boards`)")
    new_p.add_argument("--name", help="override project name (defaults to destination basename)")
    new_p.add_argument(
        "--alloy-root",
        type=Path,
        help="explicit path to an Alloy checkout (overrides the active SDK)",
    )
    new_p.set_defaults(func=_cmd_new)

    list_p = parent.add_parser("boards", help="list boards supported by `alloy new`")
    list_p.set_defaults(func=_cmd_list_boards)


def _cmd_new(args: argparse.Namespace) -> int:
    try:
        result = scaffold(
            board_name=args.board,
            destination=Path(args.path),
            project_name=args.name,
            alloy_root=args.alloy_root,
        )
    except ScaffoldError as exc:
        sys.stderr.write(f"error: {exc}\n")
        return 1

    sys.stdout.write(
        f"scaffolded {result.project} ({result.board.display_name}) at {result.destination}\n"
        f"  alloy root: {result.preflight.alloy_root}\n"
    )
    if result.preflight.toolchain_bin is None:
        sys.stdout.write(
            f"  toolchain {result.board.toolchain} is not installed; "
            f"run `alloy toolchain install {result.board.toolchain}` before configuring.\n"
        )
    else:
        sys.stdout.write(f"  toolchain bin: {result.preflight.toolchain_bin}\n")
    sys.stdout.write(
        "next steps:\n"
        f"  cd {result.destination}\n"
        "  cmake --preset debug\n"
        "  cmake --build --preset debug\n"
    )
    return 0


def _cmd_list_boards(_: argparse.Namespace) -> int:
    boards = load_boards()
    width = max(len(name) for name in boards)
    for name in sorted(boards):
        entry = boards[name]
        sys.stdout.write(
            f"{name:<{width}}  {entry.get('display_name', name)} "
            f"[{entry.get('vendor', '')}/{entry.get('family', '')}]\n"
        )
    return 0
