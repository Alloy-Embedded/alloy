#!/usr/bin/env python3
"""Compile every C++ snippet the docs tell a reader to paste.

Why this exists: four samples on the three pages a new user reads first did not
compile — `board::button.pressed()`, `uart.on_rx()`, `board::uart2_irq`,
`alloy::dev::tim3_t` on a die that has no TIM3. None of them was a typo; each
was prose written against a surface that had since moved. Nothing caught them
because nothing ever built them. A code sample that does not compile is a bug,
so this is the gate that turns "the docs are right" into a thing a machine
says.

What it does: pulls every ```cpp fence out of the pages named in PAGES,
assembles each into `src/main.cpp` of a throwaway project, and runs
`alloy build` for the boards that page claims. A block that already has a
`int main` is used verbatim; a shorter fragment is wrapped in one, which is why
fragments must be statements and not declarations.

Two deliberate limits, so a green run is not read as more than it is:

  * It asserts the snippet COMPILES for the named boards. It does not run it,
    and it says nothing about whether the snippet does what the prose says.
  * Warnings from `src/main.cpp` fail the run; warnings from framework headers
    do not. A reader pasting a sample must get a clean build of their own file,
    but a deprecation inside a driver is not this gate's business to police.

Needs a cross toolchain and the `alloy` CLI, like check_compile_errors.py.
Without either it exits 0 with a message rather than pretending to have run.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ALLOY = Path(__file__).resolve().parent.parent

#: page -> boards its snippets must build for.
#:
#: The board list is part of the claim: getting-started.md and index.md tell the
#: reader "this file builds everywhere", so they are held to all nine. A page
#: that shows a board-conditional API would name only the boards it claims, and
#: adding it here is how that claim becomes checkable.
ALL_BOARDS = (
    "nucleo_g071rb", "nucleo_g0b1re", "nucleo_f722ze", "nucleo_f767zi",
    "same70_xplained", "raspberry_pi_pico", "rp2040_zero",
    "esp32_devkit", "esp_wrover_kit",
)

PAGES: dict[str, tuple[str, ...]] = {
    "docs/index.md": ALL_BOARDS,
    "docs/getting-started.md": ALL_BOARDS,
    "docs/guide/portable-code.md": ALL_BOARDS,
}

#: A fence whose first line carries this marker is prose, not a program — an
#: error message being quoted back, or a sketch of an API that does not exist
#: yet. Skipping is a decision the page has to state out loud.
SKIP = "title=\"illustrative"

WRAP_HEAD = """#include <alloy/board.hpp>
#include <cstdint>
using namespace alloy::literals;
int main() {
    board::init();
"""
WRAP_TAIL = """
    return 0;
}
"""


def blocks(page: Path) -> list[tuple[int, str]]:
    """Every ```cpp fence in `page`, as (1-based start line, source)."""
    out: list[tuple[int, str]] = []
    body: list[str] | None = None
    indent = start = 0
    for lineno, line in enumerate(page.read_text().splitlines(), 1):
        stripped = line.strip()
        if body is None:
            if stripped.startswith("```cpp") and SKIP not in stripped:
                body, indent, start = [], len(line) - len(line.lstrip()), lineno
        elif stripped == "```":
            out.append((start, "\n".join(body) + "\n"))
            body = None
        else:
            # Fences inside a `=== "tab"` are indented; strip exactly the
            # fence's own indent so the C++ keeps its relative shape.
            body.append(line[indent:] if line.startswith(" " * indent) else line)
    return out


def build(project: Path, board: str) -> tuple[bool, str]:
    proc = subprocess.run(
        ["alloy", "build", "--board", board],
        cwd=project, capture_output=True, text=True,
        env={**os.environ, "ALLOY_ROOT": str(ALLOY)},
    )
    out = proc.stdout + proc.stderr
    if proc.returncode != 0:
        errors = [ln for ln in out.splitlines() if "error:" in ln]
        return False, "\n".join(errors[:4]) or out[-800:]
    warnings = [ln for ln in out.splitlines()
                if "warning:" in ln and "src/main.cpp" in ln]
    if warnings:
        return False, "\n".join(warnings[:4])
    return True, ""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--page", action="append",
                    help="only check this page (repeatable)")
    ap.add_argument("--board", action="append",
                    help="only build for this board (repeatable)")
    args = ap.parse_args()

    if shutil.which("alloy") is None:
        print("check_doc_snippets: no `alloy` on PATH — skipped")
        return 0
    if shutil.which("arm-none-eabi-g++") is None:
        print("check_doc_snippets: no arm-none-eabi-g++ on PATH — skipped")
        return 0

    pages = {p: b for p, b in PAGES.items() if not args.page or p in args.page}
    failures: list[str] = []
    checked = 0

    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        new = subprocess.run(
            ["alloy", "new", "docgate", "--board", ALL_BOARDS[0]],
            cwd=root, capture_output=True, text=True,
            env={**os.environ, "ALLOY_ROOT": str(ALLOY)},
        )
        if new.returncode != 0:
            print("check_doc_snippets: `alloy new` failed:\n"
                  + (new.stdout + new.stderr)[-800:], file=sys.stderr)
            return 1
        project = root / "docgate"
        main_cpp = project / "src" / "main.cpp"

        for page_name, boards in pages.items():
            page = ALLOY / page_name
            if not page.exists():
                failures.append(f"{page_name}: no such page")
                continue
            if args.board:
                boards = tuple(b for b in boards if b in args.board)
            for start, source in blocks(page):
                whole = re.search(r"\bint\s+main\s*\(", source) is not None
                main_cpp.write_text(source if whole
                                    else WRAP_HEAD + source + WRAP_TAIL)
                checked += 1
                for board in boards:
                    ok, why = build(project, board)
                    label = f"{page_name}:{start} [{board}]"
                    # Flushed: a full run is minutes of cross-compiles, and a
                    # gate that prints nothing until the end reads as hung.
                    print(f"{'ok  ' if ok else 'FAIL'}  {label}", flush=True)
                    if not ok:
                        failures.append(f"{label}\n{why}")
                        break  # one board's diagnosis is enough per block

    if failures:
        print(f"\n{len(failures)} doc snippet(s) do not build:\n", file=sys.stderr)
        for f in failures:
            print(f + "\n", file=sys.stderr)
        return 1
    print(f"\ndoc snippets green — {checked} block(s) across {len(pages)} page(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
