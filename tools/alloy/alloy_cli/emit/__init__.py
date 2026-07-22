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
from .device import emit_device_header, emit_routes_header
from .ip import emit_ip_header
from .linker import emit_linker_script
from .vectors import emit_vector_table

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


def generate(project: Project) -> list[Path]:
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

    arch = chip["cores"][0]["arch"]
    arch_ns = _ARCH_NS.get(arch)
    if arch_ns is None:
        raise EmitError(f"architecture '{arch}' not supported by the walking skeleton")

    gen = project.gen_dir
    written: list[Path] = []

    driver_includes: list[str] = []
    for ip_key in sorted({p["ip"] for p in chip["peripherals"].values()}):
        vendor, ip = cpp_ip_namespace(ip_key)
        _write(gen / "alloy" / "ip" / vendor / f"{ip}.hpp",
               emit_ip_header(db.registers[ip_key]), written)
        # Data-driven driver selection: include the HAL driver for this IP
        # when the framework ships one (alloy/hal/<class>/<vendor>_<ip>.hpp).
        cls = db.registers[ip_key]["class"]
        driver = f"alloy/hal/{cls}/{vendor}_{ip}.hpp"
        if (project.alloy_root / "src" / driver).exists():
            driver_includes.append(driver)

    _write(gen / "alloy" / "device.hpp",
           emit_device_header(chip, db.registers, driver_includes), written)
    _write(gen / "alloy" / "routes_gen.hpp", emit_routes_header(chip), written)
    _write(gen / "alloy" / "board.hpp", emit_board_header(board, chip), written)
    _write(gen / "board.cpp", emit_board_source(board, chip, db.registers, arch_ns), written)
    if chip.get("interrupts") and arch_ns == "cortex_m":
        _write(gen / "vector_table.c", emit_vector_table(chip), written)
    _write(gen / "linker.ld", emit_linker_script(chip, arch_ns), written)
    if "boot" in chip:
        from .boot import emit_boot2  # noqa: PLC0415

        _write(gen / "boot2.c", emit_boot2(chip), written)

    return written
