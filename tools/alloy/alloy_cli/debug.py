"""Debug-server facts per board — consumed by IDE integrations.

The openocd interface/target maps live HERE (flash.py imports them): one
source of truth for tool-integration knowledge. `debug_info` emits what a
Cortex-Debug launch.json needs; unsupported families say so honestly.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

# OpenOCD script maps (tool integration, not silicon facts).
OPENOCD_TARGET = {
    "stm32g0": "stm32g0x",
    "stm32f4": "stm32f4x",
    "stm32f7": "stm32f7x",
    "stm32g4": "stm32g4x",
    "same70": "atsamv",
    "rp2040": "rp2040",
}

OPENOCD_INTERFACE = {
    "stlink": "stlink",
    "cmsis-dap": "cmsis-dap",
}


def debug_info(board: dict[str, Any], chip: dict[str, Any],
               elf: Path | None) -> dict[str, Any]:
    probe = board.get("probe") or {}
    kind = probe.get("kind", "")
    interface = OPENOCD_INTERFACE.get(kind)
    target = OPENOCD_TARGET.get(chip["family"])

    info: dict[str, Any] = {
        "board": board["id"],
        "chip": f"{chip['vendor']}/{chip['part']}",
        "elf": str(elf) if elf else None,
        "svd": None,  # SVD export is a future phase
    }
    if interface and target:
        info.update(
            supported=True,
            servertype="openocd",
            interface_cfg=f"interface/{interface}.cfg",
            target_cfg=f"target/{target}.cfg",
            device=chip["part"],
            chip_id=probe.get("chip_id"),
            gdb="arm-none-eabi-gdb",
        )
    else:
        reason = (
            "ESP32 interactive debug needs openocd-esp32 + xtensa gdb (future phase)"
            if kind == "esptool"
            else "BOOTSEL boards need a debug probe wired to SWD (picoprobe/debugprobe)"
            if kind == "bootsel"
            else f"no openocd mapping for probe kind '{kind}' / family '{chip['family']}'"
        )
        info.update(supported=False, reason=reason)
    return info
