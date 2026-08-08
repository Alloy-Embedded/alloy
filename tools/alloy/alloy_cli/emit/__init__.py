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
from .board import emit_board_header, emit_board_source, flash_reserved_bytes
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


def generate(project: Project, db=None, layout: str = "flash",
             slot: str | None = None) -> list[Path]:
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
    _emit_chip_sources(gen, chip, db.registers, arch_ns, project.alloy_root, written,
                       layout, flash_reserved_bytes(board), _slot_window(chip, slot))
    _write(gen / "alloy" / "board.hpp",
           emit_board_header(board, chip, db.registers), written)
    _write(gen / "board.cpp",
           emit_board_source(board, chip, db.registers, arch_ns), written)
    # The product layer — only when a product is selected ([product] name or
    # --product); a project with no product dimension is byte-for-byte
    # unchanged. Validate-before-emit, same doctrine as run_all above: a bad
    # product TOML fails generation with every problem listed, never the
    # compile.
    if project.product_id is not None:
        _emit_product(project, gen, written)
    # lwIP's config header is a generated fact sheet (features/pools/checksums
    # derived from the board's MAC + the project's [net] table), not a hand-tuned
    # C file — emitted only for boards that declare an ethernet role, which is the
    # same gate the build seam uses to compile lwIP at all. `#include "lwipopts.h"`
    # resolves against gen/ (already on the include path).
    if board.get("roles", {}).get("ethernet"):
        from .lwipopts import net_profile, render_lwipopts  # noqa: PLC0415

        _write(gen / "lwipopts.h",
               render_lwipopts(net_profile(board, chip, project.net_options())),
               written)
    # The update root of trust (alloy.toml [ota] public_key / public_keys) —
    # always emitted so `if constexpr (alloy::ota_key::configured)` compiles
    # either way.
    from .ota_key import emit_ota_key_header, load_key_ring  # noqa: PLC0415

    ring = load_key_ring(project.ota_options(), project.root)
    _write(gen / "alloy" / "ota_key.hpp", emit_ota_key_header(ring), written)
    return written


def _emit_product(project: Project, gen: Path, written: list[Path]) -> None:
    """Product artifacts: product.hpp (constexpr params/caps/strategy aliases),
    product_nvm.hpp (nvm_kv keys + defaults, runtime-configured fields), and
    the plain-C twin product.h. gen/ is already on the include path, so
    `#include <alloy/product.hpp>` resolves with zero build changes."""
    from ..product_validate import validate_product  # noqa: PLC0415
    from ..products import known_products, load_family, load_product, resolve  # noqa: PLC0415
    from .product import (  # noqa: PLC0415
        emit_product_c_header,
        emit_product_header,
        emit_product_nvm_header,
    )

    pdir = project.root / "products"
    known = known_products(pdir)
    family = load_family(pdir)
    product = load_product(pdir, project.product_id)
    issues = validate_product(family, product, known)
    errors = [i for i in issues if i["level"] == "error"]
    if errors:
        details = "\n".join(f"  {i['field'] or 'product'}: {i['message']}"
                            for i in errors)
        raise EmitError(f"product '{project.product_id}' failed validation "
                        "(alloy product-validate shows the same list):\n"
                        f"{details}")
    model = resolve(family, product, known)
    _write(gen / "alloy" / "product.hpp", emit_product_header(model), written)
    _write(gen / "alloy" / "product_nvm.hpp", emit_product_nvm_header(model),
           written)
    _write(gen / "product.h", emit_product_c_header(model), written)


def _slot_window(chip: dict[str, Any], slot: str | None) -> tuple[int, int] | None:
    """FLASH link window for an A/B build: the bootloader region, or a slot at
    its app offset (the first app_offset bytes of a slot hold the image header +
    padding, never code). None for the default whole-flash build."""
    if slot is None:
        return None
    from .slots import APP_OFFSET, slot_layout  # noqa: PLC0415

    lay = slot_layout(chip)  # raises EmitError when the chip can't do A/B
    if slot == "bl":
        return lay.bootloader.base, lay.bootloader.size
    region = lay.slot_a if slot == "a" else lay.slot_b
    return region.base + APP_OFFSET, region.size - APP_OFFSET


def _arch_ns(chip: dict[str, Any]) -> str:
    arch = chip["cores"][0]["arch"]
    ns = _ARCH_NS.get(arch)
    if ns is None:
        raise EmitError(f"architecture '{arch}' not supported by the walking skeleton")
    return ns


def _emit_chip_sources(gen: Path, chip: dict[str, Any],
                       registers: dict[str, dict[str, Any]], arch_ns: str,
                       alloy_root: Path, written: list[Path],
                       layout: str = "flash", flash_reserved: int = 0,
                       flash_window: tuple[int, int] | None = None) -> None:
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
    _write(gen / "linker.ld",
           emit_linker_script(chip, arch_ns, layout, flash_reserved, flash_window),
           written)
    # A/B slot table — emitted whenever the chip's flash IP has a known layout so
    # apps/bootloaders can `#include <alloy/slots.hpp>`; chips without a supported
    # flash controller simply don't get one (and gen-all stays green).
    from .slots import emit_slots_header, has_slot_layout  # noqa: PLC0415

    if has_slot_layout(chip):
        _write(gen / "alloy" / "slots.hpp", emit_slots_header(chip), written)
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
