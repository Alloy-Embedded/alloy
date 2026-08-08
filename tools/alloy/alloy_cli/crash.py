"""`alloy crash` — turn a device's crash report into file:line evidence.

The firmware's half of the story (src/alloy/fault.hpp) ends with a UART line:

    RECOVERED FROM A FAULT  pc=0x... lr=0x... status=0x... (2 in a row)

`pc` is where the previous boot died and `lr` is where it would have returned —
but as raw addresses they answer nothing. This verb closes the loop on the
host, where the ELF and the toolchain live: paste the line (or pass --pc/--lr)
and each address becomes a function and a file:line via addr2line, and the
ARMv7-M fault status word becomes words (which fault, and whether the recorded
faulting address is exact).

Symbolizing needs the ELF the device was actually running. By default that is
the current board's last build — the same resolution `alloy size` uses — which
is right whenever "the device on my desk runs what I just built". For a crash
line that came back from the field, pass --elf with the build that shipped.

Tool location follows the setup doctrine (toolchains.py): PATH first, then the
managed install under ~/.alloy/tools. No toolchain, no symbolication — the
CFSR decode still runs, because it needs only the value.
"""

from __future__ import annotations

import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

# One report line carries several `name=0xVALUE` tokens; take any of the ones
# the record defines, in any order, so a future firmware printing more fields
# (or fewer) still parses.
_TOKEN = re.compile(r"\b(pc|lr|sp|psr|status|address)=0x([0-9A-Fa-f]+)")
_STREAK = re.compile(r"\((\d+) in a row\)")


def parse_report_line(line: str) -> dict[str, int]:
    """Every `name=0x...` token the record defines, plus `consecutive` from the
    "(N in a row)" suffix when present. Unknown text is ignored, so the whole
    UART line can be pasted verbatim, ANSI noise and all."""
    out = {name: int(value, 16) for name, value in _TOKEN.findall(line)}
    if streak := _STREAK.search(line):
        out["consecutive"] = int(streak.group(1))
    return out


# --- CFSR decode -----------------------------------------------------------
#
# The Configurable Fault Status Register is ARCHITECTURAL: one 32-bit word at a
# fixed address on every ARMv7-M and ARMv8-M mainline core, three sub-registers
# packed together — MemManage (MMFSR, bits [7:0]), BusFault (BFSR, [15:8]),
# UsageFault (UFSR, [31:16]). Bit assignments per the ARMv7-M Architecture
# Reference Manual, ARM DDI 0403E.b, section B3.2.15 (and B3.2.16-18 for the
# MMFAR/BFAR validity bits). ARMv6-M (Cortex-M0/M0+) has no CFSR at all; the
# firmware records 0 there, honestly.

_MMARVALID = 1 << 7
_BFARVALID = 1 << 15

_CFSR_BITS: tuple[tuple[int, str, str, str], ...] = (
    (0, "MemManage", "IACCVIOL", "instruction fetch from a region the MPU forbids"),
    (1, "MemManage", "DACCVIOL", "data access the MPU forbids"),
    (3, "MemManage", "MUNSTKERR", "MPU violation while unstacking on exception return"),
    (4, "MemManage", "MSTKERR", "MPU violation while stacking on exception entry"),
    (5, "MemManage", "MLSPERR", "MPU violation during lazy FP state preservation"),
    (8, "BusFault", "IBUSERR", "bus error on an instruction fetch"),
    (9, "BusFault", "PRECISERR",
     "precise data bus error — pc IS the faulting instruction"),
    (10, "BusFault", "IMPRECISERR",
     "imprecise data bus error — a buffered write failed later; "
     "pc is downstream of the real cause"),
    (11, "BusFault", "UNSTKERR", "bus error while unstacking on exception return"),
    (12, "BusFault", "STKERR", "bus error while stacking on exception entry"),
    (13, "BusFault", "LSPERR", "bus error during lazy FP state preservation"),
    (16, "UsageFault", "UNDEFINSTR", "undefined instruction"),
    (17, "UsageFault", "INVSTATE",
     "invalid EPSR.T — a jump to an address whose Thumb bit is clear "
     "(the classic corrupted function pointer)"),
    (18, "UsageFault", "INVPC", "invalid EXC_RETURN value on exception return"),
    (19, "UsageFault", "NOCP", "coprocessor instruction with none present/enabled"),
    (24, "UsageFault", "UNALIGNED", "unaligned access with UNALIGN_TRP set"),
    (25, "UsageFault", "DIVBYZERO", "SDIV/UDIV by zero with DIV_0_TRP set"),
)


def decode_cfsr(status: int) -> list[dict[str, Any]]:
    """Every set fault bit, as words. Empty for 0 — which is what an ARMv6-M
    device always reports (no CFSR exists there), and what an ARMv7-M device
    reports for a fault that set no configurable-fault bits."""
    return [
        {"bit": bit, "fault": fault, "name": name, "meaning": meaning}
        for bit, fault, name, meaning in _CFSR_BITS
        if status & (1 << bit)
    ]


def address_validity(status: int) -> str | None:
    """Which fault-address register the record's `address` came from: "mmfar",
    "bfar", or None when the CFSR says no address was captured (then a recorded
    address of 0 means "none", not "faulted at 0")."""
    if status & _MMARVALID:
        return "mmfar"
    if status & _BFARVALID:
        return "bfar"
    return None


# --- symbolication ---------------------------------------------------------


def locate_addr2line() -> str | None:
    """PATH first, then the managed toolchain under ~/.alloy/tools — the same
    order `alloy setup` honours, so whichever toolchain built the ELF is the
    one that decodes it."""
    exe = "arm-none-eabi-addr2line" + (".exe" if sys.platform == "win32" else "")
    if on_path := shutil.which(exe):
        return on_path
    from .toolchains import TOOLS_DIR  # noqa: PLC0415

    managed = TOOLS_DIR / "arm-gnu-toolchain" / "bin" / exe
    if managed.exists():
        return str(managed)
    for candidate in TOOLS_DIR.glob(f"*/bin/{exe}"):  # historic layout
        return str(candidate)
    return None


def is_exc_return(value: int) -> bool:
    """EXC_RETURN values (top nibble all-ones) are not code addresses: an `lr`
    like this means the fault preempted an exception handler, and symbolizing
    it would produce confident nonsense."""
    return (value >> 28) == 0xF


def query_address(reg: str, value: int) -> int:
    """The address actually worth asking addr2line about.

    The stacked pc is the faulting instruction itself — use it as-is. An lr is
    a Thumb RETURN address (bit 0 set, pointing AFTER the call), so the line it
    lands on can be the statement after the call; stepping back into the BL
    instruction ((lr & ~1) - 2) names the call site instead, which is the line
    a human wants. sp/psr/address are data, not code, but symbolizing them was
    asked for explicitly when they reach here.
    """
    if reg == "lr":
        return max((value & ~1) - 2, 0)
    return value


def symbolize(tool: str, elf: Path, frames: list[tuple[str, int]]) -> list[dict[str, Any]]:
    """One addr2line run for all addresses. Each frame comes back with the
    function name and file:line, or "??" where the ELF has no answer (an
    address outside the image, or a build without -g)."""
    queries = [(reg, value, query_address(reg, value)) for reg, value in frames]
    result = subprocess.run(
        [tool, "-f", "-C", "-e", str(elf)] + [f"0x{q:x}" for _, _, q in queries],
        capture_output=True, text=True, check=True)
    # -f prints two lines per address: function, then file:line.
    lines = result.stdout.splitlines()
    out: list[dict[str, Any]] = []
    for i, (reg, value, _) in enumerate(queries):
        function = lines[2 * i] if 2 * i < len(lines) else "??"
        location = lines[2 * i + 1] if 2 * i + 1 < len(lines) else "??"
        out.append({"reg": reg, "value": value, "function": function,
                    "location": location,
                    "note": "call site — lr is where execution would have returned"
                            if reg == "lr" else None})
    return out


# --- the report ------------------------------------------------------------


def crash_report(values: dict[str, int], elf: Path | None,
                 tool: str | None) -> dict[str, Any]:
    """The alloy.crash.v1 envelope. Symbolication degrades honestly — a missing
    ELF or toolchain nulls the file:line fields and says why, but never hides
    the CFSR decode, which needs neither."""
    frames: list[dict[str, Any]] = []
    reason: str | None = None
    code_regs = [(reg, values[reg]) for reg in ("pc", "lr") if reg in values]
    to_query = [(reg, v) for reg, v in code_regs if not is_exc_return(v)]

    if to_query:
        if elf is None or not elf.exists():
            reason = ("no ELF to symbolize against — build first, or pass --elf "
                      "with the binary the device was running")
        elif tool is None:
            reason = ("arm-none-eabi-addr2line not found on PATH or in "
                      "~/.alloy/tools — run `alloy setup`")
        else:
            frames = symbolize(tool, elf, to_query)
    for reg, value in code_regs:
        if is_exc_return(value):
            frames.append({"reg": reg, "value": value, "function": None,
                           "location": None,
                           "note": "EXC_RETURN — the fault preempted an exception "
                                   "handler; not a code address"})

    status = values.get("status")
    status_block: dict[str, Any] | None = None
    if status is not None:
        validity = address_validity(status)
        status_block = {
            "value": status,
            "faults": decode_cfsr(status),
            "address_from": validity,
            # The record's address field is only meaningful when the CFSR says
            # a fault-address register was captured.
            "faulting_address": values.get("address") if validity else None,
        }

    return {
        "schema": "alloy.crash.v1",
        "elf": str(elf) if elf and elf.exists() else None,
        "symbolized": bool(frames) and any(f["function"] for f in frames),
        "reason": reason,
        "frames": frames,
        "status": status_block,
        "consecutive": values.get("consecutive"),
    }


def print_report(report: dict[str, Any]) -> None:
    for frame in report["frames"]:
        where = (f"{frame['function']}  {frame['location']}"
                 if frame["function"] else "")
        note = f"  ({frame['note']})" if frame["note"] else ""
        print(f"{frame['reg']:7} 0x{frame['value']:08x}  {where}{note}")
    if report["reason"]:
        print(f"        (not symbolized: {report['reason']})")
    status = report["status"]
    if status is not None:
        print(f"status  0x{status['value']:08x}", end="")
        if not status["faults"]:
            print("  no fault status recorded — normal on Cortex-M0/M0+ "
                  "(ARMv6-M has no CFSR)")
        else:
            print()
            for fault in status["faults"]:
                print(f"        {fault['fault']} {fault['name']} — {fault['meaning']}")
            if status["address_from"]:
                addr = status["faulting_address"]
                shown = f"0x{addr:08x}" if addr is not None else "(not in the report)"
                print(f"        faulting address {shown} "
                      f"(exact, from {status['address_from'].upper()})")
    if report["consecutive"] is not None:
        print(f"        {report['consecutive']} crash(es) in a row")
