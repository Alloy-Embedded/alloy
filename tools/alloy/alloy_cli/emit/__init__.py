"""Code-generation orchestrator: chip + board data -> .alloy/generated tree.

The database is schema-validated and linted BEFORE any file is emitted —
bad data fails generation, never the compile (and never the device).
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from alloy_devices.lints import run_all
from alloy_devices.loader import load_database

from ..project import Project
from .board import emit_board_header, emit_board_source
from .common import EmitError, cpp_ip_namespace
from .device import curated_peripherals, emit_device_header, emit_routes_header
from .ip import emit_ip_header
from .linker import emit_linker_script
from .vectors import emit_vector_table, emit_xtensa_irq_data

_ARCH_NS = {
    "armv6m": "cortex_m",
    "armv7m": "cortex_m",
    "armv7em": "cortex_m",
    "armv8m_base": "cortex_m",
    "armv8m_main": "cortex_m",
    "xtensa_lx6": "xtensa",
}


def _write(path: Path, content: str, written: list[Path]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != content:
        path.write_text(content)
    written.append(path)


def generate(project: Project, db=None, layout: str = "flash") -> list[Path]:
    if db is None:
        db = load_database(project.devices_root)
    run_all(db)
    if db.errors:
        details = "\n".join(f"  {e}" for e in db.errors)
        raise EmitError(f"device database failed validation:\n{details}")

    board = project.load_board()
    chip_key = board["chip"]
    chip: dict[str, Any] | None = db.chips.get(chip_key)
    if chip is None:
        raise EmitError(f"board {board['id']}: chip '{chip_key}' not in database "
                        f"(known: {', '.join(sorted(db.chips))})")

    arch_ns = _arch_ns(chip)

    gen = project.gen_dir
    written: list[Path] = []
    _emit_chip_sources(gen, chip, db.registers, arch_ns, project.alloy_root, written, layout)
    _write(gen / "alloy" / "board.hpp",
           emit_board_header(board, chip, db.registers), written)
    _write(gen / "board.cpp",
           emit_board_source(board, chip, db.registers, arch_ns), written)
    return written


def _arch_ns(chip: dict[str, Any]) -> str:
    arch = chip["cores"][0]["arch"]
    ns = _ARCH_NS.get(arch)
    if ns is None:
        raise EmitError(f"architecture '{arch}' not supported by the walking skeleton")
    return ns


def _emit_chip_sources(gen: Path, chip: dict[str, Any],
                       registers: dict[str, dict[str, Any]], arch_ns: str,
                       alloy_root: Path, written: list[Path],
                       layout: str = "flash") -> None:
    """Chip-level artifacts — everything that depends only on the chip, not the
    board (ip headers, device.hpp, routes, vector table, linker, boot2). Shared
    by generate() and emit_chip_check() so the gen-all CI smoke runs the exact
    same emitters over every chip in the database.

    Uncurated stubs keep their facts (base/gate/irq) but carry no `ip` and emit
    no C++ — mirror device.py's curated_peripherals filter so codegen never
    dereferences a stub's missing `ip` key (the KeyError that crashed all 405
    builder-generated chips)."""
    driver_includes: list[str] = []
    for ip_key in sorted({p["ip"] for p in curated_peripherals(chip).values()}):
        vendor, ip = cpp_ip_namespace(ip_key)
        _write(gen / "alloy" / "ip" / vendor / f"{ip}.hpp",
               emit_ip_header(registers[ip_key]), written)
        # Data-driven driver selection: include the HAL driver for this IP
        # when the framework ships one (alloy/hal/<class>/<vendor>_<ip>.hpp).
        cls = registers[ip_key]["class"]
        driver = f"alloy/hal/{cls}/{vendor}_{ip}.hpp"
        if (alloy_root / "src" / driver).exists():
            driver_includes.append(driver)

    _write(gen / "alloy" / "device.hpp",
           emit_device_header(chip, registers, driver_includes), written)
    _write(gen / "alloy" / "routes_gen.hpp", emit_routes_header(chip), written)
    if chip.get("interrupts") and arch_ns == "cortex_m":
        _write(gen / "vector_table.c", emit_vector_table(chip), written)
    elif chip.get("interrupts") and arch_ns == "xtensa":
        _write(gen / "irq_data.c", emit_xtensa_irq_data(chip, registers), written)
    _write(gen / "linker.ld", emit_linker_script(chip, arch_ns, layout), written)
    if "boot" in chip:
        from .boot import emit_boot2  # noqa: PLC0415

        _write(gen / "boot2.c", emit_boot2(chip), written)


def emit_chip_check(chip: dict[str, Any], db, gen: Path,
                    alloy_root: Path) -> list[Path]:
    """Run chip-level codegen for a single chip into `gen`, no board required.

    Used by the gen-all smoke (scripts/gen_all_chips.py): the same pipeline that
    ships firmware runs over every chip in the database, so an emitter bug on any
    chip — including the 405 builder-generated ones that no board references —
    fails CI instead of shipping unconsumable data."""
    written: list[Path] = []
    _emit_chip_sources(gen, chip, db.registers, _arch_ns(chip), alloy_root, written)
    return written
