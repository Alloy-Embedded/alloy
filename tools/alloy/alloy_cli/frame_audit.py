"""`alloy frame-audit` — report the real size of every async coroutine frame.

No toolchain exposes a coroutine's frame size as a constant expression, so a
`task_storage<N>` is a guess: the compile-time guard catches N *too small* (a
build error), but nothing catches N *too large* (wasted RAM). This tool reads
the DWARF of a built firmware ELF — each frame is a `…​.Frame` struct with a
`DW_AT_byte_size` — and reports, per task, the real frame size on THIS toolchain
versus the declared `task_storage<N>`, so N can be tightened with evidence.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path
from typing import Any

from .project import ProjectError, load_project

_DIE = re.compile(r"^\s*<\d+><[0-9a-fA-F]+>:\s+Abbrev.*\((DW_TAG_\w+)\)")
_NAME = re.compile(r"DW_AT_name\s*:.*:\s*(\S+)\s*$")
_SIZE = re.compile(r"DW_AT_byte_size\s*:\s*(\d+)")
_DECL_N = re.compile(r"task_storageILj(\d+)E")


def _frames_from_dwarf(text: str) -> list[tuple[str, int]]:
    """Extract (mangled_frame_name, byte_size) for every DW_TAG_structure_type
    whose name ends in '.Frame' (the coroutine frame types GCC/Clang emit)."""
    frames: list[tuple[str, int]] = []
    in_struct = False
    name: str | None = None
    size: int | None = None

    def flush() -> None:
        nonlocal name, size
        if in_struct and name and name.endswith(".Frame") and size is not None:
            frames.append((name, size))
        name = None
        size = None

    for line in text.splitlines():
        die = _DIE.match(line)
        if die:
            flush()
            in_struct = die.group(1) == "DW_TAG_structure_type"
            continue
        if in_struct:
            if m := _NAME.search(line):
                name = m.group(1)
            if m := _SIZE.search(line):
                size = int(m.group(1))
    flush()
    return frames


def _demangle(sym: str, cppfilt: str | None) -> str:
    base = sym[: -len(".Frame")] if sym.endswith(".Frame") else sym
    if cppfilt:
        try:
            out = subprocess.run([cppfilt, base], capture_output=True, text=True, check=True)
            return out.stdout.strip() or base
        except (subprocess.SubprocessError, OSError):
            pass
    return base


def _suggest_n(frame_bytes: int) -> int:
    # A little headroom for cross-toolchain variation, rounded to 16.
    return ((frame_bytes + 16) // 16 + 1) * 16


def audit_elf(elf: Path, objdump: str, cppfilt: str | None) -> list[dict[str, Any]]:
    if not elf.exists():
        raise ProjectError(f"no ELF at {elf} — build the project first (alloy build)")
    if not shutil.which(objdump) and not Path(objdump).exists():
        raise ProjectError(f"objdump '{objdump}' not found — pass --objdump or add the toolchain to PATH")
    dwarf = subprocess.run([objdump, "--dwarf=info", str(elf)], capture_output=True, text=True)
    rows: list[dict[str, Any]] = []
    for mangled, size in _frames_from_dwarf(dwarf.stdout):
        decl = _DECL_N.search(mangled)
        declared = int(decl.group(1)) if decl else None
        rows.append(
            {
                "task": _demangle(mangled, cppfilt),
                "frame_bytes": size,
                "declared_n": declared,
                "waste": (declared - size) if declared is not None else None,
                "suggest_n": _suggest_n(size),
            }
        )
    return sorted(rows, key=lambda r: r["frame_bytes"], reverse=True)


# DWARF parsing (--dwarf=info) is architecture-agnostic, so any GNU objdump on
# PATH reads an ARM or Xtensa ELF's debug info. Try the toolchains alloy uses.
_OBJDUMPS = ("arm-none-eabi-objdump", "xtensa-esp-elf-objdump", "objdump", "llvm-objdump")
_CPPFILTS = ("arm-none-eabi-c++filt", "xtensa-esp-elf-c++filt", "c++filt")


def _first_on_path(names: tuple[str, ...]) -> str | None:
    for n in names:
        if shutil.which(n):
            return n
    return None


def cmd_frame_audit(project_dir: Path, board: str | None,
                    elf_override: str | None, objdump_override: str | None) -> int:
    if elf_override:
        elf = Path(elf_override)
    else:
        project = load_project(project_dir, board_override=board)
        elf = project.build_dir / "out" / f"{project.name}.elf"

    objdump = objdump_override or _first_on_path(_OBJDUMPS)
    if not objdump:
        raise ProjectError(
            "no objdump found on PATH (tried " + ", ".join(_OBJDUMPS) + ") — "
            "add the toolchain to PATH or pass --objdump")
    cppfilt = _first_on_path(_CPPFILTS)

    rows = audit_elf(elf, objdump, cppfilt)
    if not rows:
        print("no async coroutine frames found in the ELF (no alloy::async tasks, "
              "or the build stripped DWARF — build with the default -g3)")
        return 0

    name_w = min(max((len(r["task"]) for r in rows), default=4), 70)
    print(f"{'task':{name_w}}  {'frame':>6}  {'storage':>8}  {'waste':>6}  suggest")
    total_waste = 0
    for r in rows:
        decl = f"<{r['declared_n']}>" if r["declared_n"] is not None else "?"
        waste = r["waste"]
        total_waste += waste or 0
        flag = "  <- tighten" if waste is not None and waste >= 64 else ""
        task = r["task"] if len(r["task"]) <= name_w else r["task"][: name_w - 1] + "…"
        print(f"{task:{name_w}}  {r['frame_bytes']:>5}B  {decl:>8}  "
              f"{('' if waste is None else str(waste) + 'B'):>6}  task_storage<{r['suggest_n']}>{flag}")
    if total_waste:
        print(f"\n{total_waste} B of task_storage over-provisioned across "
              f"{len(rows)} task(s) — measured on this toolchain (frame size varies by compiler/flags).")
    return 0
