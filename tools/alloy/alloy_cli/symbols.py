"""What is in the image, and WHERE — the question `size` cannot answer.

`size` gives four numbers. They tell you the image fits; they do not tell you
whether the code you wrote landed where you told the linker to put it, and they
cannot tell you whether a concept-templated facade inlined away or quietly
became a real function you are now paying for on every call.

This reads the ELF's symbol table, sorts by address, demangles, and attributes
each symbol to the memory region and output section it actually occupies.

WHY THIS EXISTS AT ALL. A firmware estate I read had an elaborate,
well-reasoned linker script placing sixteen hot functions in zero-wait
tightly-coupled memory — and no startup code anywhere that copied the section
in. The intent was documented; the mechanism was absent; the fast path ran from
flash for the life of the product. An address-sorted symbol dump would have
shown it in ten seconds, because `.fastcode` would have been full of symbols at
addresses no loader ever wrote. `size` showed nothing wrong, because nothing
about the totals was wrong.

Two products come out of it:

  * `alloy symbols` — the dump, for a person or the IDE.
  * a `[budget]` table in alloy.toml — per-section byte ceilings, checked on
    every build. A budget is how "this must stay in fast RAM and must stay
    small" becomes a build failure instead of a code-review habit.

Reading a built ELF is cheap, so neither ever triggers a compile.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path
from typing import Any

from .build import _arch_ns, _xtensa_prefix
from .project import Project

# `nm --numeric-sort --print-size --demangle` lines look like:
#   08000188 0000004c T Reset_Handler
#   20000000 00000004 b alloy::detail::tick_count
# A symbol with no size (an absolute, a section start) omits the second column:
#   08000000 A _sidata
_NM_SIZED = re.compile(r"^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+(\S)\s+(.+)$")
_NM_BARE = re.compile(r"^([0-9a-fA-F]+)\s+(\S)\s+(.+)$")

#: nm type letters that denote code or occupied storage, lower-case = local.
#: 'U' (undefined), 'w'/'V' (weak), 'N' (debug) and 'A' (absolute) occupy no
#: image bytes and are dropped — counting them would double-count or invent.
_OCCUPYING = set("TtDdBbRr")


def nm_tool(chip: dict[str, Any]) -> str | None:
    """The toolchain's `nm`, or None when it isn't installed.

    A missing toolchain makes the report unavailable, never fatal — same
    contract as `size_tool`, so the IDE can render "not built yet" instead of
    an error.
    """
    if _arch_ns(chip) == "xtensa":
        tool = f"{_xtensa_prefix()}nm"
        return tool if shutil.which(tool) else None
    if _arch_ns(chip) == "rl78":
        return shutil.which("rl78-elf-nm")
    return shutil.which("arm-none-eabi-nm")


def objdump_tool(chip: dict[str, Any]) -> str | None:
    """The toolchain's `objdump`, used only for the section table."""
    if _arch_ns(chip) == "xtensa":
        tool = f"{_xtensa_prefix()}objdump"
        return tool if shutil.which(tool) else None
    if _arch_ns(chip) == "rl78":
        return shutil.which("rl78-elf-objdump")
    return shutil.which("arm-none-eabi-objdump")


def parse_nm(output: str) -> list[dict[str, Any]]:
    """Symbols from `nm --numeric-sort --print-size --demangle`.

    Sizeless symbols are kept with size 0 rather than dropped: `_sfastcode` and
    friends are exactly the markers you want to see in an address-sorted dump,
    because they are where a section is SUPPOSED to begin.
    """
    symbols: list[dict[str, Any]] = []
    for line in output.splitlines():
        sized = _NM_SIZED.match(line)
        if sized:
            addr, size, kind, name = sized.groups()
            symbols.append({"addr": int(addr, 16), "size": int(size, 16),
                            "kind": kind, "name": name.strip()})
            continue
        bare = _NM_BARE.match(line)
        if bare:
            addr, kind, name = bare.groups()
            symbols.append({"addr": int(addr, 16), "size": 0,
                            "kind": kind, "name": name.strip()})
    symbols.sort(key=lambda s: (s["addr"], s["name"]))
    return symbols


# `objdump -h` section rows:
#  Idx Name          Size      VMA       LMA       File off  Algn
#    1 .text         000051a8  08000200  08000200  00010200  2**4
_SECTION_ROW = re.compile(
    r"^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s")


def parse_sections(output: str) -> list[dict[str, Any]]:
    """Section table from `objdump -h`, with the flags line that follows each row.

    VMA and LMA are both kept, and that is the point rather than a detail: a
    section whose LMA differs from its VMA MUST be copied by startup, and a dump
    that hides the distinction hides the whole class of bug this module exists to
    catch.

    THE FLAGS ARE NOT OPTIONAL. objdump reports an LMA for `.bss` too — it is
    just wherever the previous loaded section ended — so deciding "needs a copy"
    from `vma != lma` alone marks every NOLOAD section as copied. The first
    version of this file did exactly that and claimed the stack region was
    copied from flash. A section is only copied if it has CONTENTS and LOAD.
    """
    sections: list[dict[str, Any]] = []
    pending: dict[str, Any] | None = None
    for line in output.splitlines():
        row = _SECTION_ROW.match(line)
        if row:
            name, size, vma, lma = row.groups()
            pending = {"name": name, "size": int(size, 16),
                       "vma": int(vma, 16), "lma": int(lma, 16),
                       "flags": [], "alloc": False, "loaded": False,
                       "needs_copy": False}
            sections.append(pending)
            continue
        # The line right after a section row carries its flags, comma separated
        # and upper case: "CONTENTS, ALLOC, LOAD, READONLY, CODE".
        if pending is not None:
            flags = [f.strip() for f in line.strip().split(",") if f.strip()]
            if flags and all(f.replace("_", "").isalpha() and f.isupper() for f in flags):
                pending["flags"] = flags
                pending["alloc"] = "ALLOC" in flags
                pending["loaded"] = "LOAD" in flags and "CONTENTS" in flags
                pending["needs_copy"] = (pending["loaded"] and pending["size"] > 0
                                         and pending["vma"] != pending["lma"])
            pending = None
    return sections


def attribute(symbols: list[dict[str, Any]],
              sections: list[dict[str, Any]]) -> None:
    """Tag each symbol with the section whose VMA range contains it.

    In place. Symbols outside every section (absolutes, linker-defined markers
    at a region boundary) get `section = None`, which is honest — inventing an
    owner for them would misattribute exactly the bytes a budget cares about.
    """
    ranges = sorted(
        ((s["vma"], s["vma"] + s["size"], s["name"]) for s in sections if s["size"]),
        key=lambda r: r[0])
    for sym in symbols:
        sym["section"] = next(
            (name for start, end, name in ranges if start <= sym["addr"] < end), None)


def section_totals(symbols: list[dict[str, Any]]) -> dict[str, int]:
    """Bytes of symbol per section — what a budget is measured against.

    Only occupying symbol kinds count. This can read slightly under a section's
    own size (alignment padding and literal pools belong to no symbol), so a
    budget is a ceiling on SYMBOLS, not on the section. Stated here because the
    two numbers differing is otherwise alarming.
    """
    totals: dict[str, int] = {}
    for sym in symbols:
        if sym["kind"] in _OCCUPYING and sym.get("section"):
            totals[sym["section"]] = totals.get(sym["section"], 0) + sym["size"]
    return totals


def check_budget(sections: list[dict[str, Any]],
                 budget: dict[str, Any]) -> list[dict[str, Any]]:
    """Compare each budgeted section against its ceiling.

    Measured against the SECTION size from objdump, not the symbol total —
    padding and literal pools are bytes the image really pays for, so a budget
    that ignored them would be a budget you could overrun without noticing.

    A budgeted section that is MISSING **or EMPTY** is a FAILURE, not a pass.
    Both are the same state — nothing was placed there — and it is precisely
    the state that looks fine on every number while the hot path runs from
    flash.

    The empty case is not hypothetical and it is not obvious: deleting the
    ALLOY_FASTCODE attribute from a function does not remove `.fastcode` from
    the image, it leaves the section behind at zero bytes. The first version of
    this function only checked for absence, and reported that zero-byte section
    as "0 B / 64 B ok" — passing the exact failure the budget exists to catch.
    A budget is a ceiling on something that is THERE; if you want a section to
    be empty, do not budget it.
    """
    by_name = {s["name"]: s for s in sections}
    results: list[dict[str, Any]] = []
    for name, limit in budget.items():
        section = by_name.get(name)
        if section is None or not section["size"]:
            where = ("is not in the image" if section is None
                     else "is present but EMPTY (0 B)")
            results.append({"section": name, "limit": int(limit),
                            "used": None if section is None else 0,
                            "ok": False,
                            "reason": f"section '{name}' {where} — nothing was "
                                      f"placed there. A budget bounds something "
                                      f"that exists."})
            continue
        used = section["size"]
        results.append({"section": name, "limit": int(limit), "used": used,
                        "ok": used <= int(limit),
                        "reason": None if used <= int(limit) else
                                  f"{used} B over a {int(limit)} B budget"})
    return results


def symbol_report(project: Project, chip: dict[str, Any],
                  elf: Path | None = None) -> dict[str, Any]:
    """The alloy.symbols.v1 envelope.

    Never raises for a missing ELF or a missing toolchain — both are ordinary
    states, same as `alloy size`.
    """
    elf = elf or (project.build_dir / "out" / f"{project.name}.elf")
    symbols: list[dict[str, Any]] = []
    sections: list[dict[str, Any]] = []
    reason: str | None = None

    if not elf.exists():
        reason = f"no build yet for board '{project.board_id}' — run `alloy build`"
    else:
        nm, objdump = nm_tool(chip), objdump_tool(chip)
        if nm is None or objdump is None:
            reason = "the toolchain's `nm`/`objdump` are not on PATH — run `alloy setup`"
        else:
            nm_out = subprocess.run(
                [nm, "--numeric-sort", "--print-size", "--demangle", str(elf)],
                capture_output=True, text=True, check=False)
            od_out = subprocess.run([objdump, "-h", str(elf)],
                                    capture_output=True, text=True, check=False)
            symbols = parse_nm(nm_out.stdout)
            sections = parse_sections(od_out.stdout)
            attribute(symbols, sections)
            if not symbols:
                reason = f"`{Path(nm).name}` reported no symbols for {elf.name}"

    available = bool(symbols)
    budget = project.budget_options()
    return {
        "schema": "alloy.symbols.v1",
        "board": project.board_id,
        "chip": chip.get("part"),
        "elf": str(elf) if elf.exists() else None,
        "available": available,
        "reason": reason,
        "sections": sections,
        "section_symbol_bytes": section_totals(symbols) if available else {},
        "symbols": symbols,
        "budget": check_budget(sections, budget) if (available and budget) else [],
    }


def budget_failures(report: dict[str, Any]) -> list[dict[str, Any]]:
    """The budget rows that failed — what makes a build exit non-zero."""
    return [row for row in report.get("budget", []) if not row["ok"]]


def print_report(report: dict[str, Any], *, top: int = 25,
                 section: str | None = None) -> None:
    """The human view: the section table, then the biggest symbols.

    Sorted by SIZE here rather than by address, because a person scanning a
    terminal wants "what is big" — the address order lives in the JSON, which
    is what a tool diffing two builds should read.
    """
    # Only sections that occupy the device. Debug info is most of an ELF by
    # size and none of it by footprint, so listing it would bury the five rows
    # that matter.
    sections = [s for s in report["sections"] if s.get("alloc") and s["size"]]
    if sections:
        print("section          size      vma        lma")
        for sec in sections:
            if sec["needs_copy"]:
                note = "  copied from flash at startup"
            elif not sec["loaded"]:
                note = "  zeroed/reserved, not loaded"
            else:
                note = ""
            print(f"{sec['name']:<16} {sec['size']:>6}  {sec['vma']:#010x} "
                  f"{sec['lma']:#010x}{note}")
        print()

    symbols = [s for s in report["symbols"] if s["kind"] in _OCCUPYING and s["size"]]
    if section:
        symbols = [s for s in symbols if s.get("section") == section]
    symbols.sort(key=lambda s: -s["size"])
    if symbols:
        label = f" in {section}" if section else ""
        print(f"largest symbols{label}:")
        for sym in symbols[:top]:
            print(f"{sym['size']:>6}  {sym['addr']:#010x}  "
                  f"{sym.get('section') or '?':<12} {sym['name']}")

    for row in report.get("budget", []):
        used = "absent" if row["used"] is None else f"{row['used']} B"
        verdict = "ok" if row["ok"] else f"OVER — {row['reason']}"
        print(f"\nbudget {row['section']:<14} {used:>10} / {row['limit']} B  {verdict}")
