#!/usr/bin/env python3
"""Compile every C++ snippet the docs tell a reader to paste.

Why this exists: four samples on the three pages a new user reads first did not
compile — `board::button.pressed()`, `uart.on_rx()`, `board::uart2_irq`,
`alloy::dev::tim3_t` on a die that has no TIM3. None of them was a typo; each
was prose written against a surface that had since moved. Nothing caught them
because nothing ever built them. A code sample that does not compile is a bug,
so this is the gate that turns "the docs are right" into a thing a machine
says.

What it does: pulls every ```cpp fence out of **every page under docs/**,
assembles each into `src/main.cpp` of a throwaway project, and runs
`alloy build` for the boards that page claims.

THE DEFAULT IS THAT A PAGE IS GATED. A page nobody has thought about is held
to `DEFAULT_BOARD` in `standalone` mode, so a new page arrives checked and a
new fence on an old page arrives checked. Escaping or bending the gate is a
decision somebody has to write down, in the page, where a reader of the source
can see it:

  ```cpp title="illustrative: <why>"
        The fence is prose — an error message quoted back, a sketch of an API,
        a fragment with an ellipsis in it. NOT built. The reason after the
        colon is REQUIRED; a bare `title="illustrative"` fails the gate. And
        because mkdocs renders `title=` as a caption above the block, the
        opt-out is not merely visible in the source: the READER is told this
        one is not a program. That is the point of putting it there rather
        than in a skip-list inside this script, where the page would look
        checked and not be.

  <!-- docgate: setup
       <C++ lines>
       -->
        Lines injected at namespace scope before the NEXT fence, for names a
        page uses but never shows being created (`btn`, a `process()` the
        streaming sample calls, a global the ISR sample writes). Invisible to
        the reader, visible in the source, and counted by `--audit` — because
        a page held up by hidden scaffolding is less checked than it looks,
        and the number should be sayable. On a `chained` page it persists for
        the rest of the page, like the fences themselves. `docgate: setup-local`
        is the same thing injected as STATEMENTS at the top of `main` instead
        of at file scope — which is not a formatting preference: a later fence
        that captures the name by `[&uart]` will not compile if the scaffolding
        gave it static storage duration.

  <!-- docgate: boards <id> [<id>…] -->
        The NEXT fence is a claim about other silicon, so hold it to that
        silicon. adc.md's `scan()` sample says "Available on F4/F7" in its own
        prose and is a compile error on the G0 by design; building it on the
        page's default board would report a working surface as a docs bug. A
        pinned fence stands alone — it does not inherit the page's chain
        (whose handles may not exist on that die) and does not join it.

  <!-- docgate: ungated — <why> -->
        The WHOLE page is out. Two pages use it, both design records that the
        nav already labels "not guides": their fences quote driver internals
        and sketch APIs under argument, several of them showing shapes that
        were REJECTED. Reason required, and every run prints the ungated list
        with its reason, so this is a thing somebody keeps justifying rather
        than a silence.

A fence that DECLARES rather than does — one that opens with an `#include`, a
`template`, a `namespace`, a `concept` or an `extern "C"` — is compiled at file
scope instead of inside `main`. Not a convenience: wrapping an `#include` in a
function produces a cascade of syntax errors from inside libstdc++, which is a
fact about this script and not about the docs.

Two page modes, in MANIFEST:

  standalone  each fence is its own program (the default).
  chained     each fence is compiled with every earlier non-illustrative fence
              of the same page prepended. This is not a convenience; it is
              what a reference page IS — peripherals.md opens a UART in one
              fence and calls `uart.on_receive` three fences later, and
              checking that second fence in isolation would mean deleting the
              only thing it is about.

A block that already has an `int main` is used verbatim; a shorter fragment is
wrapped in one, which is why fragments must be statements and not declarations.

Two deliberate limits, so a green run is not read as more than it is:

  * It asserts the snippet COMPILES for the named boards. It does not run it,
    and it says nothing about whether the snippet does what the prose says.
  * Warnings from `src/main.cpp` fail the run; warnings from framework headers
    do not. A reader pasting a sample must get a clean build of their own file,
    but a deprecation inside a driver is not this gate's business to police.

`--self-test` mutates the working tree four ways and demands a red for each,
restoring in a `finally`. Watched, on alloy 10a2f5a with a clean tree:

    GREEN  unmutated docs/ (markers)
    RED    a wrong method in a sample
           (peripherals.md `board::led.toggle()` -> `blink()`; one compile)
    RED    an opt-out with no reason
    RED    an ungated page with no reason
    RED    a setup block that floated away from its fence

The first case is the one this script exists for and the only one that costs
a cross-compile. The other three are marker defects, cost nothing, and are
therefore also caught by `--audit`, which CI runs before every full pass.

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
DOCS = ALLOY / "docs"

#: The board a page is held to when the manifest says nothing. A single
#: Cortex-M0+ target: cheapest die that carries a UART, I²C, SPI, ADC, PWM and
#: a DMA controller with rings, so most of the surface is reachable on it.
DEFAULT_BOARD = "nucleo_g071rb"

#: Pages that claim to build EVERYWHERE are held everywhere. The board list is
#: part of the claim: getting-started.md and index.md tell the reader "this
#: file builds on every supported board", so they are held to all nine.
ALL_BOARDS = (
    "nucleo_g071rb", "nucleo_g0b1re", "nucleo_f722ze", "nucleo_f767zi",
    "same70_xplained", "raspberry_pi_pico", "rp2040_zero",
    "esp32_devkit", "esp_wrover_kit",
)

#: page -> (boards, mode). Absent pages get (DEFAULT_BOARD,), "standalone".
MANIFEST: dict[str, tuple[tuple[str, ...], str]] = {
    "docs/index.md": (ALL_BOARDS, "standalone"),
    "docs/getting-started.md": (ALL_BOARDS, "standalone"),
    "docs/guide/portable-code.md": (ALL_BOARDS, "standalone"),
    # Reference pages: one fence sets a handle up, the next dozen use it.
    "docs/guide/peripherals.md": ((DEFAULT_BOARD,), "chained"),
    "docs/guide/adc.md": ((DEFAULT_BOARD,), "chained"),
    "docs/guide/dma.md": ((DEFAULT_BOARD,), "chained"),
    "docs/guide/pwm.md": ((DEFAULT_BOARD,), "chained"),
}

#: Marker in a fence's info string. `title="illustrative: <why>"`.
ILLUSTRATIVE = re.compile(r'title="illustrative(?::\s*(?P<why>[^"]*))?"')
#: `<!-- docgate: setup` … `-->` immediately above a fence.
SETUP_OPEN = re.compile(r"^<!--\s*docgate:\s*setup(?P<local>-local)?\s*$")
#: `<!-- docgate: boards nucleo_f722ze -->` — this ONE fence is a claim about
#: other silicon. adc.md's `scan()` sample says "Available on F4/F7" in its own
#: prose and is a compile error on the G0 by design; holding it to the page's
#: default board would report the surface working as a docs bug.
BOARDS_ONLY = re.compile(r"^<!--\s*docgate:\s*boards\s+(?P<ids>[^>]*?)\s*-->$")
#: `<!-- docgate: ungated — <why> -->` anywhere on a page: the WHOLE page is
#: out. Reserved for the two design records, whose fences quote driver
#: internals and sketch APIs under discussion — nothing on them is a paste
#: target. Reason required, and every run prints the list, so an ungated page
#: is a thing somebody has to keep justifying rather than a silence.
UNGATED = re.compile(
    r"<!--\s*docgate:\s*ungated\s*(?:(?:—|--)\s*(?P<why>[^>]*?))?\s*-->")
#: A fence that DECLARES rather than does — an include, a template, a
#: namespace, a concept, a C-linkage handler. None of those can go inside
#: `main`, and wrapping them there reports "a template declaration cannot
#: appear at block scope" (or, for an `#include`, a cascade of syntax errors
#: from inside libstdc++), which is a fact about this script and not about the
#: docs. Such a fence is compiled at file scope instead.
DECLARES = re.compile(
    r"^\s*(#include|template\s*<|namespace\s+\w+|concept\s+\w+|extern\s+\"C\")",
    re.M)

#: Warnings a doc sample earns by being a doc sample. A page that shows
#: `std::uint16_t raw = adc.read(3);` and stops there is doing its job; the
#: reader supplies the use. Everything ELSE stays fatal — in particular
#: -Wunused-result, which is how a `[[nodiscard]]` that a sample silently
#: dropped was caught on this gate's predecessor.
BENIGN = ("-Wunused-variable", "-Wunused-but-set-variable",
          "-Wunused-parameter", "-Wunused-local-typedefs")

WRAP_HEAD = """#include <alloy/board.hpp>
#include <cstdint>
using namespace alloy::literals;
"""
WRAP_MAIN = """int main() {
    board::init();
"""
WRAP_TAIL = """
    return 0;
}
"""


class Fence:
    __slots__ = ("line", "source", "skip", "why", "setup", "local", "only")

    def __init__(self, line: int, source: str, skip: bool, why: str,
                 setup: str, local: str, only: tuple[str, ...] | None):
        self.line, self.source = line, source
        self.skip, self.why, self.setup, self.local = skip, why, setup, local
        self.only = only


def fences(page: Path) -> tuple[list[Fence], list[str]]:
    """Every ```cpp fence in `page`, plus the marker errors found on the way."""
    out: list[Fence] = []
    errors: list[str] = []
    body: list[str] | None = None
    setup: list[str] | None = None
    setup_is_local = False
    pending_setup = pending_local = ""
    pending_only: tuple[str, ...] | None = None
    indent = start = 0
    skip = False
    why = ""

    for lineno, line in enumerate(page.read_text().splitlines(), 1):
        stripped = line.strip()
        if setup is not None:
            if stripped == "-->":
                if setup_is_local:
                    pending_local = "\n".join(setup) + "\n"
                else:
                    pending_setup = "\n".join(setup) + "\n"
                setup = None
            else:
                setup.append(stripped)
            continue
        if body is None:
            m_setup = SETUP_OPEN.match(stripped)
            if m_setup:
                setup, setup_is_local = [], bool(m_setup.group("local"))
                continue
            m_only = BOARDS_ONLY.match(stripped)
            if m_only:
                pending_only = tuple(m_only.group("ids").split())
                continue
            if stripped.startswith("```cpp"):
                body, indent, start = [], len(line) - len(line.lstrip()), lineno
                m = ILLUSTRATIVE.search(stripped)
                skip = m is not None
                why = (m.group("why") or "").strip() if m else ""
                if skip and not why:
                    errors.append(
                        f"{page.relative_to(ALLOY)}:{lineno}: "
                        'title="illustrative" with no reason after the colon — '
                        "say what this block is instead of a program"
                    )
            elif (stripped and (pending_setup or pending_local)
                  and not stripped.startswith("<!--")):
                # setup applies to the NEXT fence; a paragraph in between means
                # the author lost track of it.
                errors.append(
                    f"{page.relative_to(ALLOY)}:{lineno}: "
                    "`docgate: setup` block is not immediately above a ```cpp fence"
                )
                pending_setup = pending_local = ""
        elif stripped == "```":
            out.append(Fence(start, "\n".join(body) + "\n", skip, why,
                             pending_setup, pending_local, pending_only))
            body, pending_setup, pending_local = None, "", ""
            pending_only = None
        else:
            # Fences inside a `=== "tab"` are indented; strip exactly the
            # fence's own indent so the C++ keeps its relative shape.
            body.append(line[indent:] if line.startswith(" " * indent) else line)
    return out, errors


MAIN_RE = re.compile(r"\bint\s+main\s*\([^)]*\)\s*\{")


def assemble(fence: Fence, prior: list[str], setup: str, local: str) -> str:
    """One fence (plus its chain and setup) as a compilable translation unit.

    The chain is NESTED, not concatenated:

        int main() { board::init();
        { <fence 1>
        { <fence 2>
        { <the fence under test> } } } }

    so each fence sees every local the ones before it declared, and a fence
    that re-opens a handle the page already opened SHADOWS it instead of
    colliding with it. Reference pages re-open handles constantly — adc.md
    writes `auto adc = board::adc::open();` in four different sections — and
    flattening the chain turns that ordinary habit into a `conflicting
    declaration` the page has no way to fix.
    """
    # An `#include` cannot go inside `main` — but the rest of the fence often
    # can, and usually must: architecture.md's escape-hatch sample is an
    # include, a `using`, a reference binding AND a register write, and only
    # the first of those is a file-scope thing. So hoist the includes and let
    # the remainder decide where it goes.
    heads = "".join(ln + "\n" for ln in fence.source.splitlines()
                    if ln.strip().startswith("#include"))
    body_src = "\n".join(ln for ln in fence.source.splitlines()
                         if not ln.strip().startswith("#include")) + "\n"
    setup = setup + heads
    fence_src = body_src
    if DECLARES.search(fence_src) and not MAIN_RE.search(fence_src):
        # A declaration, which cannot live inside `main` — but the link still
        # needs one, or the diagnosis of a perfectly good fence is `ld: undefined
        # reference to main`.
        return (WRAP_HEAD + setup + fence_src
                + "\nint main() { board::init(); }\n")
    if MAIN_RE.search(fence_src):
        # Its own `main` — but still after WRAP_HEAD and
        # its page's setup, or a sample that calls the reader's `process()`
        # would be held to a definition the page never claimed to give.
        return WRAP_HEAD + setup + fence_src
    body = "".join("{\n" + p for p in prior) + "{\n" + fence_src
    return (WRAP_HEAD + setup + WRAP_MAIN + local
            + body + "}" * (len(prior) + 1) + WRAP_TAIL)


def contribution(fence: Fence) -> tuple[str, str]:
    """What a fence hands to the fences after it on a `chained` page, as
    (file-scope text, statements).

    A page's first fence is usually a whole program and the rest are fragments
    that assume its names. Pasting the whole program into the next fence's
    `main()` is a function definition inside a function, so it is split where
    the source splits it: everything before `int main` stays at FILE scope and
    everything inside it becomes statements. The scope matters, it is not
    bookkeeping — dma.md declares `samples` at file scope and a later fence
    captures a lambda by `[&uart]`, which cannot see a `samples` that this
    script quietly demoted into `main`. The `board::init()` the wrapper already
    supplies is dropped.
    """
    m = MAIN_RE.search(fence.source)
    if not m:
        lines = fence.source.splitlines()
        heads = "".join(ln + "\n" for ln in lines
                        if ln.strip().startswith("#include"))
        rest = "\n".join(ln for ln in lines
                         if not ln.strip().startswith("#include"))
        return heads, rest + "\n"
    body = fence.source[m.end():].rstrip()
    body = body[:body.rfind("}")] if "}" in body else body
    body = re.sub(r"^\s*board::init\(\);\s*$", "", body, flags=re.M)
    return fence.source[:m.start()], body + "\n"


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
                if "warning:" in ln and "src/main.cpp" in ln
                and not any(w in ln for w in BENIGN)]
    if warnings:
        return False, "\n".join(warnings[:4])
    return True, ""


def pages(only: list[str] | None) -> list[str]:
    found = sorted(
        str(p.relative_to(ALLOY))
        for p in DOCS.rglob("*.md")
        if re.search(r"^\s*```cpp", p.read_text(), re.M)
    )
    return [p for p in found if not only or p in only]


# ── proving it fires ─────────────────────────────────────────────────────
#
# Four mutations, each of which must take this script red. Three are marker
# defects and cost nothing; the fourth is a wrong method in a sample and costs
# one cross-compile, which is the case the whole script exists for. Mutations
# are applied to the working tree and restored in a `finally` — run it on a
# checkout, not on a tree with edits you have not saved.

MUTATIONS = (
    ("a wrong method in a sample", "build",
     "docs/guide/peripherals.md", "board::led.toggle();", "board::led.blink();"),
    ("an opt-out with no reason", "audit",
     "docs/guide/testing.md",
     'title="illustrative: the two macros, not a program"',
     'title="illustrative"'),
    ("an ungated page with no reason", "audit",
     "docs/design/dma-streams.md",
     "<!-- docgate: ungated — a decision record", "<!-- docgate: ungated -->\n<!--"),
    ("a setup block that floated away from its fence", "audit",
     "docs/guide/crash-reports.md",
     "-->\n```cpp\nif (alloy::fault::record crash;",
     "-->\n\nA paragraph between the setup and the fence.\n\n```cpp\n"
     "if (alloy::fault::record crash;"),
)


def self_test() -> int:
    import copy  # noqa: F401  (kept out of the hot path)
    bad = 0

    def once(mode: str, page: str) -> int:
        argv = ["--audit"] if mode == "audit" else ["--page", page]
        return subprocess.run(
            [sys.executable, str(Path(__file__).resolve()), *argv],
            capture_output=True, text=True, cwd=ALLOY).returncode

    clean = once("audit", "")
    print(f"{'GREEN' if clean == 0 else 'RED  '}  unmutated docs/ (markers)")
    bad += 1 if clean else 0

    for name, mode, rel, old, new in MUTATIONS:
        page = ALLOY / rel
        original = page.read_text()
        if old not in original:
            print(f"SKIP   {name}: the text it mutates is gone from {rel}")
            bad += 1
            continue
        try:
            page.write_text(original.replace(old, new, 1))
            rc = once(mode, rel)
        finally:
            page.write_text(original)
        print(f"{'RED  ' if rc else 'GREEN'}  {name}"
              + ("" if rc else " — NOT CAUGHT"))
        bad += 0 if rc else 1

    print("\nself-test " + ("green — every mutation was caught"
                            if not bad else f"FAILED — {bad} case(s)"))
    return 1 if bad else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--page", action="append",
                    help="only check this page (repeatable)")
    ap.add_argument("--board", action="append",
                    help="only build for this board (repeatable)")
    ap.add_argument("--self-test", action="store_true",
                    help="mutate docs/ four ways and demand a red for each")
    ap.add_argument("--audit", action="store_true",
                    help="report per-page coverage and opt-outs; build nothing")
    args = ap.parse_args()

    if args.self_test:
        return self_test()

    todo = pages(args.page)
    if args.page:
        missing = [p for p in args.page if p not in todo]
        if missing:
            print("no such page (or it has no ```cpp fence): "
                  + ", ".join(missing), file=sys.stderr)
            return 1

    marker_errors: list[str] = []
    plan: list[tuple[str, tuple[str, ...], str, list[Fence]]] = []
    ungated: list[tuple[str, str]] = []
    for name in todo:
        text = (ALLOY / name).read_text()
        out = UNGATED.search(text)
        if out:
            if not out.group("why"):
                marker_errors.append(f"{name}: `docgate: ungated` with no "
                                     "reason after the dash")
            ungated.append((name, out.group("why")))
            continue
        boards, mode = MANIFEST.get(name, ((DEFAULT_BOARD,), "standalone"))
        if args.board:
            boards = tuple(b for b in boards if b in args.board)
        fs, errs = fences(ALLOY / name)
        marker_errors += errs
        plan.append((name, boards, mode, fs))

    def say_ungated() -> None:
        for name, why in ungated:
            print(f"ungated  {name} — {why}")

    if args.audit:
        print(f"{'page':44} {'fences':>6} {'built':>6} {'illus':>6} "
              f"{'setup':>6}  mode/boards")
        tot = [0, 0, 0, 0]
        for name, boards, mode, fs in plan:
            built = sum(1 for f in fs if not f.skip)
            illus = sum(1 for f in fs if f.skip)
            setup = sum(1 for f in fs if f.setup or f.local)
            tot = [tot[0] + len(fs), tot[1] + built, tot[2] + illus,
                   tot[3] + setup]
            where = mode if len(boards) == 1 else f"{mode} x{len(boards)}"
            print(f"{name:44} {len(fs):6} {built:6} {illus:6} {setup:6}  "
                  f"{where} ({boards[0] if len(boards) == 1 else 'all'})")
        print(f"{'TOTAL':44} {tot[0]:6} {tot[1]:6} {tot[2]:6} {tot[3]:6}")
        say_ungated()
        for e in marker_errors:
            print("marker: " + e, file=sys.stderr)
        return 1 if marker_errors else 0

    if marker_errors:
        for e in marker_errors:
            print("marker: " + e, file=sys.stderr)
        return 1

    if shutil.which("alloy") is None:
        print("check_doc_snippets: no `alloy` on PATH — skipped")
        return 0
    if shutil.which("arm-none-eabi-g++") is None:
        print("check_doc_snippets: no arm-none-eabi-g++ on PATH — skipped")
        return 0

    failures: list[str] = []
    checked = 0

    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        new = subprocess.run(
            ["alloy", "new", "docgate", "--board", DEFAULT_BOARD],
            cwd=root, capture_output=True, text=True,
            env={**os.environ, "ALLOY_ROOT": str(ALLOY)},
        )
        if new.returncode != 0:
            print("check_doc_snippets: `alloy new` failed:\n"
                  + (new.stdout + new.stderr)[-800:], file=sys.stderr)
            return 1
        project = root / "docgate"
        main_cpp = project / "src" / "main.cpp"

        for name, boards, mode, fs in plan:
            chain: list[str] = []
            carried = carried_local = ""
            for fence in fs:
                if fence.skip:
                    continue
                setup = carried + fence.setup
                local = carried_local + fence.local
                # A fence pinned to other boards stands alone: it neither
                # inherits this page's chain (whose handles may not exist on
                # that die) nor contributes to it (its calls would not compile
                # for the fences that follow).
                use = boards if not fence.only else tuple(
                    b for b in fence.only
                    if not args.board or b in args.board)
                main_cpp.write_text(
                    assemble(fence, [] if fence.only else chain, setup, local))
                if mode == "chained" and not fence.only:
                    heads, stmts = contribution(fence)
                    chain.append(stmts)
                    # a chained page keeps its scaffolding, and its includes
                    carried, carried_local = setup + heads, local
                checked += 1
                for board in use:
                    ok, why = build(project, board)
                    label = f"{name}:{fence.line} [{board}]"
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
    say_ungated()
    print(f"\ndoc snippets green — {checked} block(s) across {len(plan)} page(s)"
          + (f", {len(ungated)} page(s) ungated" if ungated else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
