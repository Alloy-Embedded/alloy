"""Project scaffolding (`alloy new`).

Every scaffolded project owns its own ``board/`` directory and consumes the runtime via
``ALLOY_BOARD=custom`` (see ``docs/CUSTOM_BOARDS.md`` in the runtime). There are three
shapes the board layer can take, depending on what the user supplies:

1. ``--board <name>`` -- the runtime ships an in-tree board for that name. We copy
   ``boards/<name>/`` from the active SDK into ``<project>/board/`` so the user starts
   from a working board they can extend.
2. ``--mcu <part>`` with a catalog match -- the catalog (``_boards.toml``) maps the
   part number to a known board; we treat it like (1).
3. ``--mcu <part>`` without a catalog match -- we resolve the descriptor in
   ``alloy-devices`` (under the active SDK) and generate a skeleton board the user
   fills in. Memory regions in the skeleton linker script come from the descriptor's
   ``capabilities.json`` when present; otherwise they are TODO and the CLI warns.

In every case the runtime/device boundary stays intact: the runtime never sees a board
declared inside its tree but unknown to it, and the user's project is always the source
of truth for board-level hardware choices.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
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

# Architectures accepted by the runtime's ALLOY_BOARD=custom branch.
VALID_ARCHES = (
    "cortex-m0plus",
    "cortex-m4",
    "cortex-m7",
    "riscv32",
    "xtensa",
    "avr",
    "native",
)


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
    device: str
    mcu: str
    toolchain: str
    debug_probe: str | None
    openocd_config: str | None
    openocd_config_files: tuple[str, ...]

    @property
    def has_openocd(self) -> bool:
        return self.openocd_config is not None or bool(self.openocd_config_files)


def _board_from_entry(name: str, entry: dict) -> Board:
    return Board(
        name=name,
        display_name=entry.get("display_name", name),
        vendor=entry.get("vendor", ""),
        family=entry.get("family", ""),
        device=entry.get("device", ""),
        mcu=entry.get("mcu", ""),
        toolchain=entry.get("toolchain", ""),
        debug_probe=entry.get("debug_probe"),
        openocd_config=entry.get("openocd_config"),
        openocd_config_files=tuple(entry.get("openocd_config_files", [])),
    )


def get_board(name: str) -> Board:
    boards = load_boards()
    if name not in boards:
        raise ScaffoldError(f"unknown board {name!r}; known: {', '.join(sorted(boards))}")
    return _board_from_entry(name, boards[name])


def find_board_by_mcu(mcu: str) -> Board | None:
    """Return the catalog board whose ``mcu`` field matches ``mcu`` case-insensitively."""
    target = mcu.upper()
    for name, entry in load_boards().items():
        if str(entry.get("mcu", "")).upper() == target:
            return _board_from_entry(name, entry)
    return None


# --- MCU resolution against alloy-devices ----------------------------------------------


@dataclass(frozen=True)
class DeviceFacts:
    vendor: str
    family: str
    device: str
    mcu: str
    arch: str | None
    flash_origin: str | None
    flash_length_bytes: int | None
    ram_origin: str | None
    ram_length_bytes: int | None

    @property
    def derived_from_descriptor(self) -> bool:
        return self.flash_length_bytes is not None and self.ram_length_bytes is not None


def _devices_root_for_sdk(sdk_root: Path) -> Path | None:
    """The SDK manager installs alloy-devices alongside the runtime."""
    candidate = sdk_root.parent / "devices"
    return candidate if candidate.is_dir() else None


def _read_capabilities(device_dir: Path) -> dict | None:
    candidate = device_dir / "capabilities.json"
    if not candidate.is_file():
        return None
    try:
        return json.loads(candidate.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return None


def _extract_memory(caps: dict, region: str) -> tuple[str | None, int | None]:
    """Best-effort extraction of (origin, length) from a capabilities.json memory entry.

    The descriptor schema is not part of this CLI's stable contract; we accept several
    common shapes and fall back to ``(None, None)`` so the caller can warn the user.
    """
    if not caps:
        return None, None
    candidates: list[dict] = []
    memory = caps.get("memory")
    if isinstance(memory, dict):
        if region in memory and isinstance(memory[region], dict):
            candidates.append(memory[region])
        regions = memory.get("regions")
        if isinstance(regions, list):
            candidates.extend(r for r in regions if isinstance(r, dict))
    elif isinstance(memory, list):
        candidates.extend(m for m in memory if isinstance(m, dict))

    for entry in candidates:
        kind = str(entry.get("kind") or entry.get("name") or entry.get("type") or "").lower()
        if region not in kind and not (region == "flash" and "rom" in kind):
            continue
        origin = entry.get("origin") or entry.get("start") or entry.get("address")
        length = entry.get("length") or entry.get("size") or entry.get("size_bytes")
        if isinstance(origin, int):
            origin = f"0x{origin:08X}"
        if isinstance(length, str):
            try:
                length = int(length, 0)
            except ValueError:
                length = None
        return (str(origin) if origin is not None else None,
                int(length) if isinstance(length, int) else None)
    return None, None


def resolve_mcu(mcu: str, *, devices_root: Path) -> DeviceFacts:
    """Find the descriptor matching ``mcu`` under ``devices_root`` and read its facts.

    Raises ``ScaffoldError`` when no descriptor matches. The MCU match is case-insensitive
    and accepts any descriptor whose directory name is a prefix of the lower-cased MCU
    (so ``STM32G474RET6`` matches a directory named ``stm32g474re``, ``stm32g474``, etc.).
    """
    if not devices_root.is_dir():
        raise ScaffoldError(
            f"alloy-devices not found at {devices_root}. "
            "Install the SDK with `alloy sdk install <version>` (which fetches devices), "
            "or pass --devices-root."
        )
    needle = mcu.lower()
    best: tuple[Path, str, str, str] | None = None  # (dir, vendor, family, device)
    best_len = 0
    for vendor_dir in sorted(devices_root.iterdir()):
        if not vendor_dir.is_dir():
            continue
        for family_dir in sorted(vendor_dir.iterdir()):
            devices = family_dir / "generated" / "runtime" / "devices"
            if not devices.is_dir():
                continue
            for device_dir in sorted(devices.iterdir()):
                if not device_dir.is_dir():
                    continue
                name = device_dir.name.lower()
                if needle.startswith(name) and len(name) > best_len:
                    best = (device_dir, vendor_dir.name, family_dir.name, device_dir.name)
                    best_len = len(name)
    if best is None:
        raise ScaffoldError(
            f"no descriptor under {devices_root} matches MCU {mcu!r}. "
            "Add support in alloy-devices, or pass --board <name> if the part has a "
            "foundational board."
        )
    device_dir, vendor, family, device = best
    caps = _read_capabilities(device_dir) or {}
    flash_origin, flash_length = _extract_memory(caps, "flash")
    ram_origin, ram_length = _extract_memory(caps, "ram")
    return DeviceFacts(
        vendor=vendor,
        family=family,
        device=device,
        mcu=mcu,
        arch=str(caps.get("arch") or caps.get("architecture") or "") or None,
        flash_origin=flash_origin,
        flash_length_bytes=flash_length,
        ram_origin=ram_origin,
        ram_length_bytes=ram_length,
    )


# --- preflight -------------------------------------------------------------------------


@dataclass(frozen=True)
class Preflight:
    alloy_root: Path
    toolchain_bin: Path | None
    gdb_path: str
    devices_root: Path | None


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


def _resolve_toolchain(toolchain_name: str) -> tuple[Path | None, str]:
    try:
        binary = toolchains.which(toolchain_name)
    except toolchains.ToolchainError:
        return None, _gdb_for(toolchain_name)
    return binary.parent, _gdb_for(toolchain_name)


def _gdb_for(toolchain: str) -> str:
    if toolchain == "arm-none-eabi-gcc":
        return "arm-none-eabi-gdb"
    if toolchain == "avr-gcc":
        return "avr-gdb"
    return "gdb"


# --- generation ------------------------------------------------------------------------


def _templates_dir() -> Path:
    return Path(__file__).with_name("_templates")


def _make_env() -> Environment:
    return Environment(
        loader=FileSystemLoader(str(_templates_dir())),
        undefined=StrictUndefined,
        keep_trailing_newline=True,
    )


def _validate_project_name(name: str) -> None:
    if not PROJECT_NAME_RE.match(name):
        raise ScaffoldError(
            f"invalid project name {name!r}; must match {PROJECT_NAME_RE.pattern}"
        )


def _validate_destination(dest: Path) -> None:
    if dest.exists() and any(dest.iterdir()):
        raise ScaffoldError(f"destination {dest} exists and is not empty")


@dataclass(frozen=True)
class BoardLayer:
    """Description of a board layer ready to be written into ``<project>/board/``.

    ``sources`` lists files that must be added to the user's executable target
    (typically ``board.cpp`` and ``syscalls.cpp``).
    """
    header_name: str
    linker_script_name: str
    sources: tuple[str, ...]
    files_to_copy: tuple[tuple[Path, str], ...]  # (source_path, destination_filename)
    rendered_files: tuple[tuple[str, str], ...]  # (destination_filename, contents)
    vendor: str
    family: str
    device: str
    arch: str
    mcu: str
    flash_size_bytes: int | None
    display_name: str
    toolchain: str
    has_openocd: bool


def _layer_from_in_tree_board(
    board: Board, alloy_root: Path, env: Environment
) -> tuple[BoardLayer, list[str]]:
    """Build a board layer by copying the matching in-tree board folder.

    Returns the layer plus a list of human-facing warnings (e.g. when the in-tree
    board ships no linker script and we have to render a TODO placeholder).
    """
    src = alloy_root / "boards" / board.name
    if not src.is_dir():
        raise ScaffoldError(
            f"the active SDK does not ship boards/{board.name}/ "
            f"(expected at {src}); SDK may be incomplete."
        )

    board_files = sorted(src.iterdir())
    header_candidates = [p for p in board_files if p.name == "board.hpp"]
    linker_candidates = [p for p in board_files if p.suffix == ".ld"]
    if not header_candidates:
        raise ScaffoldError(f"boards/{board.name}/board.hpp missing in SDK")

    sources = tuple(
        p.name for p in board_files if p.suffix == ".cpp" and p.name in {"board.cpp", "syscalls.cpp"}
    )
    arch = _arch_for_board(alloy_root, board)

    rendered: list[tuple[str, str]] = []
    warnings: list[str] = []
    if linker_candidates:
        linker_script_name = linker_candidates[0].name
    else:
        # Some in-tree boards (ESP32-C3, ESP32-S3) ship no linker script because
        # the runtime relies on a vendor-supplied linker fragment that does not yet
        # have a bare-metal substitute. Render a TODO placeholder so the project
        # still validates against the ALLOY_BOARD=custom contract; the user must
        # supply real memory regions before the build will produce a firmware image.
        linker_script_name = "linker.ld"
        ctx = {
            "vendor": board.vendor,
            "family": board.family,
            "device": board.device,
            "mcu": board.mcu or board.device,
            "flash_origin": "0x00000000  /* TODO: set from datasheet */",
            "flash_length": "/* TODO */ 64K",
            "ram_origin": "0x20000000  /* TODO: set from datasheet */",
            "ram_length": "/* TODO */ 16K",
            "derived_from_descriptor": False,
        }
        rendered.append(
            (linker_script_name, env.get_template("board_skeleton/linker.ld.j2").render(**ctx))
        )
        warnings.append(
            f"boards/{board.name}/ in the active SDK does not ship a linker script. "
            f"A TODO placeholder was written to board/{linker_script_name}; fill in "
            "the FLASH/RAM origin and length for your target before building."
        )

    layer = BoardLayer(
        header_name="board.hpp",
        linker_script_name=linker_script_name,
        sources=sources,
        files_to_copy=tuple((p, p.name) for p in board_files if p.is_file()),
        rendered_files=tuple(rendered),
        vendor=board.vendor,
        family=board.family,
        device=board.device,
        arch=arch,
        mcu=board.mcu,
        flash_size_bytes=None,
        display_name=board.display_name,
        toolchain=board.toolchain,
        has_openocd=board.has_openocd,
    )
    return layer, warnings


_ARCH_BY_BOARD_FALLBACK = {
    "stm32g0": "cortex-m0plus",
    "stm32f4": "cortex-m4",
    "same70": "cortex-m7",
    "rp2040": "cortex-m0plus",
    "avr-da": "avr",
    "esp32c3": "riscv32",
    "esp32s3": "xtensa",
}


def _arch_for_board(alloy_root: Path, board: Board) -> str:
    """Look up the board's arch from the SDK's board manifest as the source of truth.

    Falls back to a family-based mapping if the manifest cannot be parsed (e.g. its
    syntax changes); the runtime will reject an invalid value at configure time, so the
    fallback is safe.
    """
    manifest = alloy_root / "cmake" / "board_manifest.cmake"
    if manifest.is_file():
        text = manifest.read_text(encoding="utf-8")
        pattern = re.compile(
            rf'BOARD_NAME STREQUAL "{re.escape(board.name)}".*?set\(_arch "([^"]+)"',
            re.DOTALL,
        )
        match = pattern.search(text)
        if match:
            return match.group(1)
    return _ARCH_BY_BOARD_FALLBACK.get(board.family, "cortex-m4")


def _layer_from_descriptor(
    facts: DeviceFacts,
    *,
    arch: str | None,
    toolchain: str,
    env: Environment,
) -> tuple[BoardLayer, list[str]]:
    """Generate a skeleton board layer from descriptor facts. Returns the layer and a
    list of human-facing warnings the CLI should print."""
    warnings: list[str] = []

    resolved_arch = arch or facts.arch
    if resolved_arch is None:
        raise ScaffoldError(
            f"could not determine arch for {facts.vendor}/{facts.family}/{facts.device}; "
            "pass --arch <one-of-cortex-m0plus,cortex-m4,...>"
        )
    if resolved_arch not in VALID_ARCHES:
        raise ScaffoldError(
            f"invalid --arch {resolved_arch!r}; valid: {', '.join(VALID_ARCHES)}"
        )

    flash_origin = facts.flash_origin or "0x00000000  /* TODO: set from datasheet */"
    flash_length = (
        f"{facts.flash_length_bytes // 1024}K" if facts.flash_length_bytes else "/* TODO */ 64K"
    )
    ram_origin = facts.ram_origin or "0x20000000  /* TODO: set from datasheet */"
    ram_length = (
        f"{facts.ram_length_bytes // 1024}K" if facts.ram_length_bytes else "/* TODO */ 16K"
    )
    if not facts.derived_from_descriptor:
        warnings.append(
            "the descriptor for "
            f"{facts.vendor}/{facts.family}/{facts.device} did not declare memory regions "
            "in a recognised shape; the generated linker script has TODO placeholders. "
            "Edit board/linker.ld with FLASH/RAM origin and length from the datasheet."
        )

    ctx = {
        "vendor": facts.vendor,
        "family": facts.family,
        "device": facts.device,
        "mcu": facts.mcu,
        "flash_origin": flash_origin,
        "flash_length": flash_length,
        "ram_origin": ram_origin,
        "ram_length": ram_length,
        "derived_from_descriptor": facts.derived_from_descriptor,
    }
    rendered = (
        ("board.hpp", env.get_template("board_skeleton/board.hpp.j2").render(**ctx)),
        ("board_config.hpp", env.get_template("board_skeleton/board_config.hpp.j2").render(**ctx)),
        ("board.cpp", env.get_template("board_skeleton/board.cpp.j2").render(**ctx)),
        ("syscalls.cpp", env.get_template("board_skeleton/syscalls.cpp.j2").render(**ctx)),
        ("linker.ld", env.get_template("board_skeleton/linker.ld.j2").render(**ctx)),
    )

    layer = BoardLayer(
        header_name="board.hpp",
        linker_script_name="linker.ld",
        sources=("board.cpp", "syscalls.cpp"),
        files_to_copy=(),
        rendered_files=rendered,
        vendor=facts.vendor,
        family=facts.family,
        device=facts.device,
        arch=resolved_arch,
        mcu=facts.mcu,
        flash_size_bytes=facts.flash_length_bytes,
        display_name=facts.mcu,
        toolchain=toolchain,
        has_openocd=False,
    )
    return layer, warnings


# --- top-level scaffold ----------------------------------------------------------------


@dataclass(frozen=True)
class ScaffoldResult:
    project: str
    destination: Path
    layer: BoardLayer
    files_written: tuple[Path, ...]
    preflight: Preflight
    warnings: tuple[str, ...]


def _cpu_flags_for_arch(arch: str) -> list[str]:
    """Return CPU/ABI compile flags appropriate for ``arch``.

    The flags must be applied at the top of the consuming project's CMakeLists
    (BEFORE add_subdirectory(alloy)) so that both the alloy targets and the
    project's own executable inherit them. alloy's platform CMake uses
    `add_compile_options(...)` which is directory-scoped and does not flow up
    to a parent CMakeLists.
    """
    if arch == "cortex-m0plus":
        return ["-mcpu=cortex-m0plus", "-mthumb"]
    if arch == "cortex-m4":
        return ["-mcpu=cortex-m4", "-mthumb", "-mfloat-abi=soft"]
    if arch == "cortex-m7":
        return ["-mcpu=cortex-m7", "-mthumb", "-mfloat-abi=soft"]
    if arch == "riscv32":
        return ["-march=rv32imc", "-mabi=ilp32"]
    if arch == "xtensa":
        return ["-mlongcalls"]
    # avr, native, and unknown: no global flags here. AVR's -mmcu=... is part
    # of the toolchain file the avr-da boards already wire up.
    return []


def _toolchain_file_for_arch(arch: str, alloy_root: Path) -> str | None:
    """Return the absolute path to the alloy toolchain CMake file for ``arch``,
    or None when no cross-compilation is required (host build)."""
    mapping = {
        "cortex-m0plus": "arm-none-eabi.cmake",
        "cortex-m4": "arm-none-eabi.cmake",
        "cortex-m7": "arm-none-eabi.cmake",
        "avr": "avr-gcc.cmake",
        "riscv32": "riscv32-esp-elf.cmake",
        "xtensa": "xtensa-esp32s3-elf.cmake",
        "native": None,
    }
    name = mapping.get(arch)
    if name is None:
        return None
    return str(alloy_root / "cmake" / "toolchains" / name)


def _toolchain_for_arch(arch: str) -> str:
    if arch == "avr":
        return "avr-gcc"
    if arch == "native":
        return "gcc"
    if arch == "xtensa":
        # Espressif consolidated all Xtensa ESP variants behind a single driver
        # (`xtensa-esp-elf-gcc`) starting in 13.x; LX6 (ESP32) and LX7 (ESP32-S3)
        # both target it via runtime config. The arch enum is expected to split into
        # xtensa-lx6 / xtensa-lx7 with add-esp32-classic-family, but the toolchain
        # binary stays the same.
        return "xtensa-esp-elf-gcc"
    if arch == "riscv32":
        return "riscv32-esp-elf-gcc"
    return "arm-none-eabi-gcc"


def _build_layer(
    *,
    board_name: str | None,
    mcu: str | None,
    arch: str | None,
    alloy_root: Path,
    devices_root: Path | None,
    env: Environment,
) -> tuple[BoardLayer, list[str]]:
    if board_name is not None:
        return _layer_from_in_tree_board(get_board(board_name), alloy_root, env)

    assert mcu is not None
    catalog_match = find_board_by_mcu(mcu)
    if catalog_match is not None:
        return _layer_from_in_tree_board(catalog_match, alloy_root, env)

    if devices_root is None:
        raise ScaffoldError(
            f"MCU {mcu!r} is not in the foundational catalog and no alloy-devices "
            "checkout was found alongside the active SDK. Install the SDK or pass "
            "--devices-root <path>."
        )
    facts = resolve_mcu(mcu, devices_root=devices_root)
    return _layer_from_descriptor(
        facts, arch=arch, toolchain=_toolchain_for_arch(arch or facts.arch or ""), env=env
    )


def scaffold(
    *,
    board_name: str | None = None,
    mcu: str | None = None,
    arch: str | None = None,
    destination: Path,
    project_name: str | None = None,
    alloy_root: Path | None = None,
    devices_root: Path | None = None,
) -> ScaffoldResult:
    if (board_name is None) == (mcu is None):
        raise ScaffoldError("exactly one of --board or --mcu must be provided")

    project = project_name or destination.name.replace("-", "_")
    _validate_project_name(project)

    dest = destination.resolve()
    _validate_destination(dest)

    resolved_root = _resolve_alloy_root(alloy_root)
    resolved_devices = devices_root or _devices_root_for_sdk(resolved_root)

    env = _make_env()
    layer, warnings = _build_layer(
        board_name=board_name,
        mcu=mcu,
        arch=arch,
        alloy_root=resolved_root,
        devices_root=resolved_devices,
        env=env,
    )

    toolchain_bin, gdb_path = _resolve_toolchain(layer.toolchain)
    preflight_info = Preflight(
        alloy_root=resolved_root,
        toolchain_bin=toolchain_bin,
        gdb_path=gdb_path,
        devices_root=resolved_devices,
    )

    ctx = {
        "project": project,
        "display_name": layer.display_name,
        "vendor": layer.vendor,
        "family": layer.family,
        "device": layer.device,
        "arch": layer.arch,
        "mcu": layer.mcu or None,
        "flash_size_bytes": layer.flash_size_bytes,
        "alloy_root": str(resolved_root),
        "board_header_name": layer.header_name,
        "linker_script_name": layer.linker_script_name,
        "board_sources": list(layer.sources),
        "toolchain_bin": str(toolchain_bin) if toolchain_bin else None,
        "toolchain_file": _toolchain_file_for_arch(layer.arch, resolved_root),
        "cpu_flags": _cpu_flags_for_arch(layer.arch),
        "has_openocd": layer.has_openocd,
        "gdb_path": gdb_path,
    }

    dest.mkdir(parents=True, exist_ok=True)
    (dest / "src").mkdir(exist_ok=True)
    (dest / "board").mkdir(exist_ok=True)
    (dest / ".vscode").mkdir(exist_ok=True)

    written: list[Path] = []

    def write(rel: str, body: str) -> None:
        path = dest / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(body, encoding="utf-8")
        written.append(path)

    # Board layer: copy from SDK or render from skeleton templates.
    for src_path, dest_name in layer.files_to_copy:
        target = dest / "board" / dest_name
        shutil.copy2(src_path, target)
        written.append(target)
    for dest_name, body in layer.rendered_files:
        write(f"board/{dest_name}", body)

    # Top-level project files.
    write("CMakeLists.txt", env.get_template("CMakeLists.txt.j2").render(**ctx))
    write("CMakePresets.json", env.get_template("CMakePresets.json.j2").render(**ctx))
    json.loads((dest / "CMakePresets.json").read_text(encoding="utf-8"))
    write("src/main.cpp", env.get_template("main.cpp.j2").render(**ctx))
    write(".gitignore", env.get_template("gitignore.j2").render(**ctx))
    write("README.md", env.get_template("README.md.j2").render(**ctx))
    write(".vscode/settings.json", env.get_template("vscode/settings.json.j2").render(**ctx))
    write(".vscode/tasks.json", env.get_template("vscode/tasks.json.j2").render(**ctx))
    write(".vscode/launch.json", env.get_template("vscode/launch.json.j2").render(**ctx))
    for name in ("settings.json", "tasks.json", "launch.json"):
        json.loads((dest / ".vscode" / name).read_text(encoding="utf-8"))

    return ScaffoldResult(
        project=project,
        destination=dest,
        layer=layer,
        files_written=tuple(written),
        preflight=preflight_info,
        warnings=tuple(warnings),
    )


# --- argparse wiring -------------------------------------------------------------------


def add_subparser(parent: argparse._SubParsersAction) -> None:
    new_p = parent.add_parser(
        "new",
        help="scaffold a new firmware project; project owns its board/ directory",
    )
    new_p.add_argument("path", help="destination directory for the new project")
    target = new_p.add_mutually_exclusive_group(required=True)
    target.add_argument("--board", help="copy this in-tree board into the project (e.g. nucleo_g071rb)")
    target.add_argument("--mcu", help="MCU part number (e.g. STM32G474RET6)")
    new_p.add_argument("--name", help="override project name (defaults to destination basename)")
    new_p.add_argument(
        "--alloy-root",
        type=Path,
        help="explicit path to an Alloy checkout (overrides the active SDK)",
    )
    new_p.add_argument(
        "--devices-root",
        type=Path,
        help="explicit path to alloy-devices (overrides the SDK lookup)",
    )
    new_p.add_argument(
        "--arch",
        choices=VALID_ARCHES,
        help="override the architecture for raw-MCU scaffolding when the descriptor lacks it",
    )
    new_p.set_defaults(func=_cmd_new)

    list_p = parent.add_parser("boards", help="list boards supported by `alloy new`")
    list_p.set_defaults(func=_cmd_list_boards)


def _cmd_new(args: argparse.Namespace) -> int:
    try:
        result = scaffold(
            board_name=args.board,
            mcu=args.mcu,
            arch=args.arch,
            destination=Path(args.path),
            project_name=args.name,
            alloy_root=args.alloy_root,
            devices_root=args.devices_root,
        )
    except ScaffoldError as exc:
        sys.stderr.write(f"error: {exc}\n")
        return 1

    layer = result.layer
    sys.stdout.write(
        f"scaffolded {result.project} ({layer.display_name}) at {result.destination}\n"
        f"  alloy root:    {result.preflight.alloy_root}\n"
        f"  device tuple:  {layer.vendor}/{layer.family}/{layer.device} (arch={layer.arch})\n"
    )
    if result.preflight.toolchain_bin is None:
        sys.stdout.write(
            f"  toolchain {layer.toolchain} is not installed; "
            f"run `alloy toolchain install {layer.toolchain}` before configuring.\n"
        )
    else:
        sys.stdout.write(f"  toolchain bin: {result.preflight.toolchain_bin}\n")
    for warning in result.warnings:
        sys.stdout.write(f"  warning: {warning}\n")
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
        mcu = entry.get("mcu", "")
        sys.stdout.write(
            f"{name:<{width}}  {entry.get('display_name', name)} "
            f"[{entry.get('vendor', '')}/{entry.get('family', '')}{', ' + mcu if mcu else ''}]\n"
        )
    return 0
