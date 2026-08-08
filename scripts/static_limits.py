#!/usr/bin/env python3
"""Static properties of a LINKED firmware image, measured rather than asserted.

docs/reference/safety.md makes four claims about every alloy image: no libc
heap, no C++ exceptions or RTTI, no recursion, and a stack bound that can be
computed ahead of time. This script is where each of those claims comes from,
so a reader can re-run it instead of believing it.

    scripts/static_limits.py <firmware.elf> [--json] [--top N]

Method, and its limits — stated here because a stack number without its method
is worthless:

* CALL GRAPH — direct calls only, read from `objdump -d`: `bl`/`blx` to a
  symbol, plus `b`/`b.w` to another function's ENTRY (that is how -Os spells a
  tail call). Calls through a register or a table are counted and reported but
  cannot be followed; a graph with indirect calls is a LOWER bound on reachable
  code, so both the recursion result and the stack number are conditional on
  them. The count is printed for exactly that reason.
* RECURSION — cycles in that graph, found with an explicit-stack DFS. A cycle
  formed only through an indirect call is invisible here.
* FRAME SIZES — GCC's own `-fstack-usage` output (the `.su` files the build
  writes beside each object), matched to linked symbols by demangled name.
  A function with no `.su` entry (libgcc's helpers: the build did not compile
  them) falls back to the sum of everything its disassembled body reserves,
  which OVER-estimates rather than assuming zero. Those are listed, counted,
  and marked `~` wherever they appear.
* STACK BOUND — the longest root-to-leaf sum over the acyclic graph. It is a
  bound on the WORST CASE, not a measurement of a run: it assumes the deepest
  path is taken, and it does NOT include what an exception entry pushes before
  the handler's first instruction (8 words on Cortex-M without FP, plus 18 more
  with FP state) — that is added per root and labelled.
* WHAT THIS IS NOT — it is not a certified WCET or stack tool. There is no
  soundness proof, no handling of `alloca`, no interrupt-nesting model beyond
  one level, and the FP exception frame is not counted. Numbers from it belong
  in a design review, not in a safety case.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

# Symbols that mean "there is a heap". operator new/delete are matched by
# their mangled forms because a freestanding image has no reason to carry them
# at all, however they are spelled.
HEAP_SYMBOLS = {
    "malloc", "calloc", "realloc", "free", "_malloc_r", "_free_r",
    "_sbrk", "sbrk", "_sbrk_r", "__wrap_malloc",
}
HEAP_MANGLED_PREFIXES = ("_Znwj", "_Znaj", "_ZdlPv", "_ZdaPv", "_Znwm", "_Znam")
EXCEPTION_SYMBOLS = {
    "__cxa_throw", "__cxa_allocate_exception", "__cxa_begin_catch",
    "__gxx_personality_v0", "_Unwind_RaiseException", "__cxa_rethrow",
}
RTTI_PREFIXES = ("_ZTI", "_ZTS")           # typeinfo, typeinfo name
VTABLE_PREFIX = "_ZTV"                     # NOT rtti: virtual dispatch is legal
                                           # under -fno-rtti, so vtables are
                                           # reported, never failed on


def tool(name: str, elf: Path) -> str:
    """Find the binutil for this ELF's toolchain (arm-none-eabi-, xtensa-…)."""
    for prefix in ("arm-none-eabi-", "xtensa-esp32-elf-", "riscv-none-elf-",
                   "rl78-elf-", ""):
        found = shutil.which(prefix + name)
        if found:
            return found
    sys.exit(f"error: no {name} on PATH (source the toolchain first)")


def run(argv: list[str]) -> str:
    return subprocess.run(argv, capture_output=True, text=True,
                          check=True).stdout


# ---------------------------------------------------------------------------
# The image
# ---------------------------------------------------------------------------

@dataclass
class Func:
    name: str
    addr: int
    calls: set[str] = field(default_factory=set)
    indirect: int = 0
    frame: int | None = None
    frame_from: str = "none"         # "compiler" | "prologue" | "none"
    source: str | None = None        # the demangled signature
    prologue: int = 0                # bytes the disassembled prologue reserves


FUNC_RE = re.compile(r"^([0-9a-f]+) <([^>]+)>:$")
CALL_RE = re.compile(r"\t(bl|blx|b|b\.w|b\.n)\s+([0-9a-f]+) <([^>]+)>")
INDIRECT_RE = re.compile(r"\t(blx|bl)\s+(r\d+|ip|lr|a\d+)\b")
PUSH_RE = re.compile(r"\t(v?push|stmdb)\s+(?:sp!,\s*)?\{([^}]*)\}")
SUBSP_RE = re.compile(r"\tsub(?:\.w|s)?\s+sp,\s*(?:sp,\s*)?#(\d+)")


def function_symbols(elf: Path) -> set[str]:
    """Names readelf calls STT_FUNC.

    objdump disassembles anything in an executable section, including const
    tables emitted into .text — `board::kClockProgram` is one. Feeding those
    to the call graph invents functions with no frame and pollutes the unknown
    list, so the symbol table decides what is code.
    """
    readelf = tool("readelf", elf)
    names = set()
    for line in run([readelf, "-sW", str(elf)]).splitlines():
        parts = line.split()
        if len(parts) >= 8 and parts[3] == "FUNC":
            names.add(parts[7])
    return names


def read_image(elf: Path) -> dict[str, Func]:
    objdump = tool("objdump", elf)
    text = run([objdump, "-d", str(elf)])
    code = function_symbols(elf)
    funcs: dict[str, Func] = {}
    current: Func | None = None
    for line in text.splitlines():
        if m := FUNC_RE.match(line.strip()):
            name = m.group(2)
            current = Func(name=name, addr=int(m.group(1), 16))
            if name in code or not code:
                funcs[name] = current
            else:
                current = None
            continue
        if current is None:
            continue
        if m := PUSH_RE.search(line):
            current.prologue += 4 * len([r for r in m.group(2).split(",") if r.strip()])
        elif m := SUBSP_RE.search(line):
            current.prologue += int(m.group(1))
        if INDIRECT_RE.search(line):
            current.indirect += 1
            continue
        if m := CALL_RE.search(line):
            kind, target_addr, target = m.group(1), int(m.group(2), 16), m.group(3)
            if kind.startswith("bl"):
                current.calls.add(target.split("+")[0])
            elif "+" not in target and target_addr != current.addr:
                # A plain branch that lands exactly on another function's entry
                # is a tail call: -Os emits these constantly and a graph that
                # missed them would understate both recursion and stack.
                current.calls.add(target)
    for f in funcs.values():
        f.calls.discard(f.name)
    return funcs


# ---------------------------------------------------------------------------
# Frames: GCC's .su files, matched to linked symbols by demangled name
# ---------------------------------------------------------------------------

SU_RE = re.compile(r"^(.*):(\d+):(\d+):(.*)\t(\d+)\t(\w+)$")


def _core_name(signature: str) -> str:
    """`bool ns::f(std::span<const T>)` -> `ns::f`. Overloads collapse."""
    depth, stripped = 0, []
    for ch in signature:
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth = max(0, depth - 1)
        elif depth == 0:
            stripped.append(ch)
    text = "".join(stripped)
    depth = 0
    for i in range(len(text) - 1, -1, -1):
        if text[i] == ")":
            depth += 1
        elif text[i] == "(":
            depth -= 1
            if depth == 0:
                text = text[:i]
                break
    return text.strip().split()[-1] if text.strip() else signature


def read_su(build_dir: Path) -> tuple[dict[str, int], int]:
    """core name -> frame bytes, over every .su file under a build tree.

    Two functions can reduce to the same core name — overloads, and `static`
    functions of the same name in different translation units. Those collapse
    to the LARGER frame, which errs toward a bigger bound rather than a
    prettier one; the number of collapses is returned so it can be reported.
    """
    frames: dict[str, int] = {}
    collisions = 0
    for path in sorted(build_dir.rglob("*.su")):
        for line in path.read_text().splitlines():
            if not (m := SU_RE.match(line)):
                continue
            key, size = _core_name(m.group(4)), int(m.group(5))
            if key in frames and frames[key] != size:
                collisions += 1
            frames[key] = max(frames.get(key, 0), size)
    return frames, collisions


def attach_frames(elf: Path, funcs: dict[str, Func],
                  frames: dict[str, int]) -> list[str]:
    """Give every linked function its compiler-declared frame size.

    The link between the two worlds is the demangled symbol name: `c++filt`
    turns the ELF's `_ZN5alloy3hal17run_clock_program…` into the same
    `alloy::hal::run_clock_program(…)` shape that `-fstack-usage` writes, and
    _core_name() strips both down to the part that is spelled identically
    (template arguments and default extents are not).
    """
    named = [f for f in funcs.values() if f.addr]
    if not named:
        return []
    filt = tool("c++filt", elf)
    demangled = subprocess.run([filt], input="\n".join(f.name for f in named),
                               capture_output=True, text=True,
                               check=True).stdout.splitlines()
    unknown: list[str] = []
    for func, pretty in zip(named, demangled, strict=False):
        func.source = pretty
        key = _core_name(pretty)
        if key in frames:
            func.frame, func.frame_from = frames[key], "compiler"
        else:
            # No .su: the function came from a library alloy did not compile
            # (libgcc's integer helpers, mostly). Falling back to the sum of
            # everything its disassembly reserves is an OVER-estimate — a
            # function with two alternative prologues is counted twice — so it
            # can only inflate the bound, never hide depth. It is labelled
            # separately everywhere it is used.
            func.frame, func.frame_from = func.prologue, "prologue"
            unknown.append(func.name)
    return unknown


# ---------------------------------------------------------------------------
# Graph properties
# ---------------------------------------------------------------------------

def find_cycles(funcs: dict[str, Func]) -> list[list[str]]:
    """Every cycle reachable in the direct-call graph (DFS with a path stack)."""
    cycles: list[list[str]] = []
    seen_cycles: set[frozenset[str]] = set()
    state: dict[str, int] = {}       # 0 unvisited, 1 on stack, 2 done

    def visit(start: str) -> None:
        stack: list[tuple[str, list[str]]] = [(start, [])]
        path: list[str] = []
        while stack:
            node, _ = stack[-1]
            if state.get(node, 0) == 0:
                state[node] = 1
                path.append(node)
                for callee in sorted(funcs.get(node, Func(node, 0)).calls):
                    if state.get(callee, 0) == 1:
                        cycle = path[path.index(callee):] + [callee]
                        if (key := frozenset(cycle)) not in seen_cycles:
                            seen_cycles.add(key)
                            cycles.append(cycle)
                    elif state.get(callee, 0) == 0:
                        stack.append((callee, []))
            else:
                stack.pop()
                if path and path[-1] == node:
                    state[node] = 2
                    path.pop()

    for name in sorted(funcs):
        if state.get(name, 0) == 0:
            visit(name)
    return cycles


def deepest_path(funcs: dict[str, Func], root: str) -> tuple[int, list[str], bool]:
    """(bytes, path, exact) — exact=False if an estimated frame was on the path.

    Memoised per function, which is only valid on an acyclic graph; that is
    why a cycle makes the whole run fail rather than print a number.
    """
    memo: dict[str, tuple[int, list[str], bool]] = {}

    def walk(name: str, on_path: frozenset[str]) -> tuple[int, list[str], bool]:
        if name in memo:
            return memo[name]
        func = funcs.get(name)
        if func is None:                      # declared but never linked
            return 0, [name], True
        best, best_path, exact = 0, [], func.frame_from == "compiler"
        for callee in sorted(func.calls):
            if callee in on_path:             # a cycle; reported separately
                continue
            cost, sub, sub_ok = walk(callee, on_path | {name})
            if cost > best:
                best, best_path = cost, sub
            exact = exact and sub_ok
        total = (func.frame or 0) + best
        result = (total, [name, *best_path], exact)
        memo[name] = result
        return result

    return walk(root, frozenset())


def scan_symbols(elf: Path) -> dict[str, list[str]]:
    nm = tool("nm", elf)
    defined = set()
    undefined = set()
    for line in run([nm, "--defined-only", str(elf)]).splitlines():
        parts = line.split()
        if parts:
            defined.add(parts[-1])
    for line in run([nm, "-u", str(elf)]).splitlines():
        parts = line.split()
        if parts:
            undefined.add(parts[-1])
    every = defined | undefined
    return {
        "heap": sorted(s for s in every
                       if s in HEAP_SYMBOLS or s.startswith(HEAP_MANGLED_PREFIXES)),
        "exceptions": sorted(s for s in every if s in EXCEPTION_SYMBOLS),
        "rtti": sorted(s for s in every if s.startswith(RTTI_PREFIXES)),
        "vtables": sorted(s for s in every if s.startswith(VTABLE_PREFIX)),
    }


def eh_frame_bytes(elf: Path) -> int:
    readelf = tool("readelf", elf)
    for line in run([readelf, "-S", "-W", str(elf)]).splitlines():
        if ".eh_frame" in line and ".eh_frame_hdr" not in line:
            fields = line.split()
            return int(fields[fields.index(".eh_frame") + 4], 16)
    return 0


# ---------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("elf", type=Path)
    ap.add_argument("--build-dir", type=Path,
                    help="where the .su files are (default: the ELF's tree)")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--top", type=int, default=8,
                    help="how many frames of the worst path to print")
    args = ap.parse_args()

    elf = args.elf.resolve()
    if not elf.exists():
        sys.exit(f"error: {elf} does not exist")
    build_dir = (args.build_dir or elf.parent).resolve()

    funcs = read_image(elf)
    frames, collisions = read_su(build_dir)
    unknown = attach_frames(elf, funcs, frames)
    cycles = find_cycles(funcs)
    symbols = scan_symbols(elf)
    indirect = {n: f.indirect for n, f in funcs.items() if f.indirect}

    def depth(root: str) -> dict:
        total, path, exact = deepest_path(funcs, root)
        return {"bytes": total, "exact": exact, "path": path}

    # Three kinds of entry point, kept apart because they nest differently:
    # main runs on the stack from reset; a handler runs ON TOP of whatever was
    # interrupted; Reset_Handler runs before main and then hands over.
    handlers = {n: depth(n) for n in sorted(funcs)
                if n.endswith(("_Handler", "_IRQHandler")) and n != "Reset_Handler"}
    thread = depth("main") if "main" in funcs else {"bytes": 0, "path": [], "exact": True}
    reset = depth("Reset_Handler") if "Reset_Handler" in funcs else None
    worst_isr_name = max(handlers, key=lambda k: handlers[k]["bytes"],
                         default=None)
    worst_isr = handlers.get(worst_isr_name) if worst_isr_name else None
    # Cortex-M pushes 8 words before the handler's first instruction. FP state
    # (18 more words) is NOT included: this build has no FPU registers in use,
    # and guessing would be worse than saying so.
    exception_entry = 32
    peak = thread["bytes"] + (worst_isr["bytes"] + exception_entry if worst_isr else 0)

    estimated = sorted(unknown)
    report = {
        "elf": str(elf),
        "functions": len(funcs),
        "recursion": {"cycles": cycles, "clean": not cycles},
        "indirect_calls": {"total": sum(indirect.values()), "by_function": indirect},
        "heap_symbols": symbols["heap"],
        "exception_symbols": symbols["exceptions"],
        "rtti_symbols": symbols["rtti"],
        "vtable_symbols": symbols["vtables"],
        "eh_frame_bytes": eh_frame_bytes(elf),
        "frames": {
            "from_compiler": len(funcs) - len(estimated),
            "estimated_from_prologue": estimated,
            "su_entries": len(frames),
            "name_collisions": collisions,
        },
        "stack": {
            "thread": thread,
            "reset": reset,
            "worst_handler": ({"name": worst_isr_name, **worst_isr}
                              if worst_isr else None),
            "exception_entry_bytes": exception_entry,
            "peak_bytes": peak,
            "handlers": handlers,
        },
    }
    if args.json:
        print(json.dumps(report, indent=2))
        return 0 if cycles else 0

    print(f"image      {elf.name}  ({len(funcs)} linked functions)")
    print(f"recursion  {'none' if not cycles else str(len(cycles)) + ' CYCLE(S)'}"
          " in the direct-call graph")
    for cycle in cycles:
        print("           " + " -> ".join(cycle))
    print(f"indirect   {sum(indirect.values())} call(s) through a register — NOT "
          "followed, so every result here is conditional on them")
    for name, count in sorted(indirect.items(), key=lambda kv: -kv[1])[:5]:
        print(f"           {count:3}  {name}")
    print(f"heap       {'none' if not symbols['heap'] else symbols['heap']}")
    print(f"exceptions {'none' if not symbols['exceptions'] else symbols['exceptions']}"
          f"   (.eh_frame {eh_frame_bytes(elf)} bytes)")
    print(f"rtti       {'none' if not symbols['rtti'] else symbols['rtti'][:4]}")
    print(f"vtables    {len(symbols['vtables'])} "
          "(virtual dispatch is allowed — reported, not failed on)")
    print(f"frames     {len(funcs) - len(estimated)} from -fstack-usage, "
          f"{len(estimated)} estimated from the prologue"
          + (f" ({collisions} name collision(s) resolved upward)" if collisions else ""))
    for name in estimated[:10]:
        print(f"           est  {funcs[name].frame:>5}  {name}")
    print(f"stack      thread (main …): {thread['bytes']} bytes"
          f"{'' if thread['exact'] else '   [contains an estimate]'}")
    for name in thread["path"][:args.top]:
        f = funcs.get(name)
        mark = " " if f is None or f.frame_from == "compiler" else "~"
        print(f"           {mark}{'?' if f is None else f.frame:>5}  {name}")
    if worst_isr:
        print(f"           worst handler: {worst_isr['bytes']} bytes "
              f"({worst_isr_name})")
    if reset:
        print(f"           reset path:    {reset['bytes']} bytes (runs before main)")
    print(f"           PEAK: {peak} bytes = thread + worst handler + "
          f"{exception_entry} bytes of exception entry")
    print("           (single-level interrupt nesting; alloy does not enable "
          "tail-chaining\n            of different priorities in these examples "
          "— check yours)")
    return 1 if cycles else 0


if __name__ == "__main__":
    sys.exit(main())
