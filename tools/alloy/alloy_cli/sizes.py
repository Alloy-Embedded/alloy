"""What the built firmware costs, against what the chip actually has.

The build already runs `size` and prints its table into the terminal, where it
scrolls away. This turns the same numbers into an envelope the IDE can render as
a bar — and, when the chip supports A/B update, measures the image against the
SLOT rather than against whole flash, which is the number that decides whether a
field update will fit.

Reading an ELF that is already built is cheap (one `size` call), so this never
triggers a compile: `alloy size` reports the last build, or says there isn't one.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path
from typing import Any

from .build import _arch_ns, _xtensa_prefix
from .emit.common import EmitError
from .project import Project

# Berkeley `size` output: a header line, then one row per object.
#    text	   data	    bss	    dec	    hex	filename
#   21400	    108	   2320	  23828	   5d14	blink.elf
_SIZE_ROW = re.compile(r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+([0-9a-fA-F]+)\s+(.+)$")


def parse_size(output: str) -> dict[str, int] | None:
    """Sections from `size`'s Berkeley format.

    flash = text + data (initialised data ships in flash and is copied to RAM at
    startup); ram = data + bss. Getting that split wrong is the classic way to
    under-report flash by exactly the size of .data.
    """
    for line in output.splitlines():
        match = _SIZE_ROW.match(line)
        if match:
            text, data, bss = (int(match.group(i)) for i in (1, 2, 3))
            return {
                "text": text, "data": data, "bss": bss,
                "flash": text + data, "ram": data + bss,
            }
    return None


def size_tool(chip: dict[str, Any]) -> str | None:
    """The `size` binary for this chip's toolchain, or None when it isn't
    installed — a missing toolchain makes the report unavailable, never fatal."""
    if _arch_ns(chip) == "xtensa":
        tool = f"{_xtensa_prefix()}size"
        return tool if shutil.which(tool) else None
    return shutil.which("arm-none-eabi-size")


def _memory(chip: dict[str, Any], which: str) -> dict[str, Any] | None:
    """The region the app's code ("code") or data ("data") was linked into.

    Asked of the linker emitter rather than guessed, so the percentage always
    refers to the region the ELF actually occupies.
    """
    from .emit.linker import app_regions  # noqa: PLC0415

    mem = app_regions(chip, _arch_ns(chip)).get(which)
    if mem is None:
        return None
    base = mem["base"]
    return {
        "name": mem.get("name"),
        "base": int(base, 16) if isinstance(base, str) else int(base),
        "size": int(mem["size"]),
    }


def _region(used: int | None, total: dict[str, int] | None) -> dict[str, Any]:
    out: dict[str, Any] = {"used": used, "total": total["size"] if total else None,
                           "base": total["base"] if total else None,
                           # Which memory this is measured against — "flash" is
                           # irom on a chip that has no flash of its own.
                           "region": total["name"] if total else None}
    if used is not None and total and total["size"]:
        out["percent"] = round(100.0 * used / total["size"], 1)
    else:
        out["percent"] = None
    return out


def _slots(chip: dict[str, Any], flash_used: int | None) -> dict[str, Any] | None:
    """A/B partitioning, when the chip's flash IP supports it. `fits` answers the
    only question that matters before a field update."""
    from .emit import slots as slots_emit  # noqa: PLC0415

    if not slots_emit.has_slot_layout(chip):
        return None
    try:
        layout = slots_emit.slot_layout(chip)
    except EmitError:
        return None
    # The image occupies app_offset padding plus the app itself in its slot.
    image = None if flash_used is None else flash_used + slots_emit.APP_OFFSET
    return {
        "page_size": layout.page_size,
        "app_offset": slots_emit.APP_OFFSET,
        "image_bytes": image,
        "regions": [
            {"name": name, "base": region.base, "size": region.size,
             "fits": None if image is None or name not in ("slot_a", "slot_b")
                     else image <= region.size}
            for name, region in (
                ("bootloader", layout.bootloader), ("slot_a", layout.slot_a),
                ("slot_b", layout.slot_b), ("store", layout.store))
        ],
    }


def size_report(project: Project, chip: dict[str, Any],
                elf: Path | None = None) -> dict[str, Any]:
    """The alloy.size.v1 envelope. Never raises for a missing ELF or a missing
    toolchain — both are ordinary states the IDE should show, not errors."""
    elf = elf or (project.build_dir / "out" / f"{project.name}.elf")
    sections: dict[str, int] | None = None
    reason: str | None = None

    if not elf.exists():
        reason = f"no build yet for board '{project.board_id}' — run `alloy build`"
    else:
        tool = size_tool(chip)
        if tool is None:
            reason = "the toolchain's `size` is not on PATH — run `alloy setup`"
        else:
            result = subprocess.run([tool, str(elf)], capture_output=True, text=True,
                                    check=False)
            sections = parse_size(result.stdout)
            if sections is None:
                reason = f"could not parse `{Path(tool).name}` output for {elf.name}"

    return {
        "schema": "alloy.size.v1",
        "board": project.board_id,
        "chip": chip.get("part"),
        "elf": str(elf) if elf.exists() else None,
        "available": sections is not None,
        "reason": reason,
        "sections": sections,
        "flash": _region(sections["flash"] if sections else None, _memory(chip, "code")),
        "ram": _region(sections["ram"] if sections else None, _memory(chip, "data")),
        "slots": _slots(chip, sections["flash"] if sections else None),
    }
