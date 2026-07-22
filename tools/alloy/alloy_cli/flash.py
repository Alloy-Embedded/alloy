"""Flash runners.

The board declares its probe (board.json "probe"); this module picks the
first available runner on the host: probe-rs (preferred), openocd, st-flash.
Runner choice is host tooling BEHAVIOR — the only board/chip FACTS used are
the declared probe kind, the probe-rs chip id and the chip family.
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path
from typing import Any

from .emit.common import EmitError

# OpenOCD target script per chip family (tool-integration map, not silicon).
_OPENOCD_TARGET = {
    "stm32g0": "stm32g0x",
    "stm32f4": "stm32f4x",
    "stm32g4": "stm32g4x",
}

_OPENOCD_INTERFACE = {
    "stlink": "stlink",
    "cmsis-dap": "cmsis-dap",
}


def flash(board: dict[str, Any], chip: dict[str, Any], elf: Path) -> str:
    probe = board.get("probe")
    if probe is None:
        raise EmitError(f"board {board['id']} declares no probe — cannot flash")

    if shutil.which("probe-rs") and "chip_id" in probe:
        subprocess.run(
            ["probe-rs", "download", "--chip", probe["chip_id"], str(elf)], check=True
        )
        subprocess.run(["probe-rs", "reset", "--chip", probe["chip_id"]], check=True)
        return "probe-rs"

    if shutil.which("openocd"):
        interface = _OPENOCD_INTERFACE.get(probe.get("kind", ""))
        target = _OPENOCD_TARGET.get(chip["family"])
        if interface and target:
            subprocess.run(
                ["openocd",
                 "-f", f"interface/{interface}.cfg",
                 "-f", f"target/{target}.cfg",
                 "-c", f"program {{{elf}}} verify reset exit"],
                check=True,
            )
            return "openocd"

    if shutil.which("st-flash") and probe.get("kind") == "stlink":
        flash_base = next(m["base"] for m in chip["memories"] if m["kind"] == "flash")
        bin_path = elf.with_suffix(".bin")
        objcopy = shutil.which("arm-none-eabi-objcopy")
        if objcopy is None:
            raise EmitError("st-flash fallback needs arm-none-eabi-objcopy on PATH")
        subprocess.run([objcopy, "-O", "binary", str(elf), str(bin_path)], check=True)
        subprocess.run(["st-flash", "--reset", "write", str(bin_path), flash_base], check=True)
        return "st-flash"

    raise EmitError(
        "no usable flash runner found — install probe-rs, openocd or stlink tools"
    )
