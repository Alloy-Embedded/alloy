#!/usr/bin/env python3
"""The cheap half of the docs gate: names, verbs and links.

`check_doc_snippets.py` needs a cross toolchain and takes twenty minutes. This
one needs nothing but Python and runs in under a second, which is why it — not
the compiler — is the gate wired into `scripts/check_contract.sh`. It checks
the three classes of claim that went wrong most often in the docs audit, all of
which are cheap to verify and were verified by nobody:

  BOARD NAMES.  Every board id a page names must be a directory under boards/.
  Two wrong ones shipped in `alloy boards`' own output on getting-started.md.
  Detected two ways: any token shaped like a shipped board family
  (`nucleo_*`, `same70_*`, `rp2040_*`, `esp32_*`, `esp_*`, `raspberry_*`) in
  ANY prose, and any token in board POSITION (`--board X`, `--boards a,b`,
  `alloy set-board X`, `board-info/-validate/-clone X`) inside a command.

  CLI VERBS.  Every `alloy <verb>` a page tells the reader to run must be a
  verb the CLI defines, and every `alloy <verb> <sub>` must be a real
  subcommand. Three shipped verbs were missing from the reference and one
  documented output belonged to no verb at all.

  LINKS AND ANCHORS.  Every relative link between docs pages must resolve, and
  every `#fragment` must be a heading that exists on the target page. mkdocs
  `--strict` catches the first only when `validation:` is configured, and this
  project's mkdocs.yml has no `validation:` block, so today it catches neither
  reliably and never catches anchors.

Where the truth comes from, and how it is kept from going stale:

  boards      the boards/ directory. There is no second copy to disagree with.
  verbs       scraped out of tools/alloy/alloy_cli/cli.py, so the gate runs
              with no CLI installed — AND cross-checked against `alloy --help`
              whenever the CLI *is* on PATH. If the scraper and the shipping
              parser disagree, that is a failure, not a shrug: a scraper that
              silently misses a verb would let a dead verb through, which is
              the exact bug this gate exists to catch.
  anchors     the headings in the target page, slugified the way
              markdown.extensions.toc does it, plus any explicit `{ #id }`.

Opting out. A page sometimes has to name something that is deliberately not
real — the board you are about to create, a placeholder in a usage line. Say
so in the page, next to the thing:

    <!-- docgate: placeholder my_board — the board this tutorial creates -->

Page-scoped, reason REQUIRED (a bare `placeholder my_board` fails). It lives
in the page and not in a list inside this script on purpose: a reader of the
source can see every exception, and an exception nobody can justify in one
line is usually a defect.

`--self-test` mutates a scratch copy of docs/ four ways — a dead board name, a
dead verb, a dead sub-verb, a dangling anchor — and demands a red for each.
A gate nobody has watched fail is a decoration.

Watched, on alloy 10a2f5a with a clean tree and `alloy` on PATH:

    GREEN  unmutated docs/ — 0 problem(s)
    RED    a board that does not exist
           getting-started.md:118: no such board 'nucleo_g0b1zz'
    RED    a CLI verb that does not exist
           getting-started.md:112: no such CLI verb 'alloy list-boards'
    RED    a subcommand that does not exist
           cli.md:264: `alloy lib` has no subcommand 'find'
                       (has ['add', 'info', 'list', 'search'])
    RED    an anchor that does not exist
           index.md:15: 'getting-started.md' has no heading '#no-such-thing'

Four for four. The mutations land on a scratch COPY of docs/, so a red here
cannot leave a mutated page behind.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ALLOY = Path(__file__).resolve().parent.parent
CLI_PY = ALLOY / "tools" / "alloy" / "alloy_cli" / "cli.py"

SHELL_LANGS = {"console", "bash", "sh", "shell"}

#: `alloy <verb> [<sub>]` at the head of a command. The `(?![\w])` matters:
#: without it `alloy dma_uart` (a firmware banner quoted in a checklist) reads
#: as the verb `dma`.
VERB_RE = re.compile(
    r"^\s*(?:\$\s+)?alloy\s+([a-z][a-z0-9-]*)(?![\w])"
    r"(?:\s+([a-z][a-z0-9-]*)(?![\w]))?"
)
BOARD_POS_RE = re.compile(
    r"(?:--boards?|set-board|board-info|board-validate|board-clone"
    r"|board\s*=\s*)\s+\"?([A-Za-z][A-Za-z0-9_,]*)"
)
PLACEHOLDER_RE = re.compile(
    r"<!--\s*docgate:\s*placeholder\s+(\S+)\s*(?:—|--)?\s*(.*?)\s*-->"
)
LINK_RE = re.compile(r"(?<!!)\[[^\]]*\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)")
HEADING_RE = re.compile(r"^(#{1,6})\s+(.*?)\s*$")
EXPLICIT_ID_RE = re.compile(r"\{\s*#([A-Za-z0-9_.:-]+)[^}]*\}\s*$")


# ── the truth ────────────────────────────────────────────────────────────

def shipped_boards() -> set[str]:
    return {p.name for p in (ALLOY / "boards").iterdir()
            if p.is_dir() and (p / "board.json").exists()}


def scraped_verbs() -> tuple[set[str], list[str]]:
    """Top-level verbs, read out of the CLI source. Two spellings exist."""
    src = CLI_PY.read_text()
    verbs = set(re.findall(r'\bsub\.add_parser\(\s*"([a-z0-9-]+)"', src))
    # The gen/build/flash/monitor/run pipeline shares one parser body, so it is
    # registered from a tuple of (name, handler) pairs in a loop rather than by
    # five `sub.add_parser` calls. Missing this form cost five verbs on the
    # first run of this gate — which is exactly why the CLI cross-check below
    # is not optional decoration.
    for block in re.findall(r"for cmd, func in \((.*?)\):", src, re.S):
        verbs |= set(re.findall(r'\(\s*"([a-z0-9-]+)"\s*,\s*cmd_', block))
    return verbs, []


def cli_verbs() -> tuple[set[str], dict[str, set[str]]] | None:
    """The SHIPPING parser's own answer, when a CLI is on PATH."""
    if shutil.which("alloy") is None:
        return None

    def usage_set(*argv: str) -> set[str]:
        proc = subprocess.run(["alloy", *argv, "--help"],
                              capture_output=True, text=True)
        if proc.returncode != 0:
            return set()
        m = re.search(r"\{([a-z0-9,\-]+)\}", proc.stdout)
        return set(m.group(1).split(",")) if m else set()

    top = usage_set()
    if not top:
        return None
    subs = {v: s for v in sorted(top) if (s := usage_set(v))}
    return top, subs


# ── the pages ────────────────────────────────────────────────────────────

def commands(page: Path) -> list[tuple[int, str]]:
    """Lines that are an alloy INVOCATION — a prompted line in a shell fence,
    or an inline `alloy …` span. Deliberately not "any line mentioning alloy":
    console fences are full of firmware output, and `alloy uart_echo ready` is
    a banner, not a verb."""
    out: list[tuple[int, str]] = []
    lang: str | None = None
    for i, line in enumerate(page.read_text().splitlines(), 1):
        s = line.strip()
        if lang is None and s.startswith("```"):
            rest = s[3:].strip()
            lang = (rest.split()[0].split("{")[0] if rest else "none") or "none"
            continue
        if lang is not None:
            if s.startswith("```"):
                lang = None
            elif lang in SHELL_LANGS and s.startswith("$ "):
                out.append((i, s[2:]))
            continue
        for m in re.finditer(r"`([^`]+)`", line):
            if m.group(1).startswith("alloy "):
                out.append((i, m.group(1)))
    return out


def anchors(page: Path) -> set[str]:
    """Every `#fragment` the rendered page will answer to."""
    ids: set[str] = set()
    lang: str | None = None
    for line in page.read_text().splitlines():
        s = line.strip()
        if s.startswith("```"):
            lang = None if lang is not None else "fence"
            continue
        if lang is not None:
            continue
        # A long page can hang a target off something that is not a heading —
        # peripheral-surface.md anchors three of its arguments this way.
        ids |= set(re.findall(r"<a\s+(?:id|name)=\"([^\"]+)\"", line))
        m = HEADING_RE.match(s)
        if not m:
            continue
        text = m.group(2)
        explicit = EXPLICIT_ID_RE.search(text)
        if explicit:
            ids.add(explicit.group(1))
            text = EXPLICIT_ID_RE.sub("", text)
        ids.add(slugify(text))
    return ids


def slugify(text: str) -> str:
    """markdown.extensions.toc's default slugify, with '-' as separator."""
    text = re.sub(r"`([^`]*)`", r"\1", text)          # inline code
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)  # links
    text = re.sub(r"[*_]{1,3}([^*_]+)[*_]{1,3}", r"\1", text)
    text = re.sub(r"<[^>]+>", "", text)
    text = re.sub(r"[^\w\s-]", "", text, flags=re.UNICODE).strip().lower()
    return re.sub(r"[-\s]+", "-", text)


# ── the checks ───────────────────────────────────────────────────────────

def check(docs: Path, verbose: bool = False) -> list[str]:
    boards = shipped_boards()
    family = sorted({b.split("_")[0] for b in boards}, key=len, reverse=True)
    family_re = re.compile(r"\b(?:" + "|".join(family) + r")_[a-z0-9_]+")

    verbs, _ = scraped_verbs()
    live = cli_verbs()
    problems: list[str] = []
    if live is not None:
        top, subs = live
        if top != verbs:
            problems.append(
                "scripts/check_doc_facts.py: the verb scraper and the shipping "
                f"CLI disagree — scraper-only {sorted(verbs - top)}, "
                f"CLI-only {sorted(top - verbs)}. Fix scraped_verbs()."
            )
        verbs = top
    else:
        subs = {}

    pages = sorted(docs.rglob("*.md"))
    anchor_cache = {p: anchors(p) for p in pages}
    counts = {"board": 0, "verb": 0, "link": 0}

    for page in pages:
        rel = page.relative_to(ALLOY) if ALLOY in page.parents else page
        text = page.read_text()
        lines = text.splitlines()

        allowed: set[str] = set()
        for m in PLACEHOLDER_RE.finditer(text):
            if not m.group(2):
                problems.append(f"{rel}: `docgate: placeholder {m.group(1)}` "
                                "with no reason — say why it is not real")
            allowed.add(m.group(1))

        # 1. board names, by family shape, anywhere on the page
        for i, line in enumerate(lines, 1):
            for m in family_re.finditer(line):
                counts["board"] += 1
                if m.group(0) not in boards and m.group(0) not in allowed:
                    problems.append(f"{rel}:{i}: no such board '{m.group(0)}' "
                                    "(boards/ has no directory by that name)")

        # 2. board names in board position, and alloy verbs, in commands
        for i, cmd in commands(page):
            for m in BOARD_POS_RE.finditer(cmd):
                for tok in m.group(1).split(","):
                    if not tok:
                        continue
                    counts["board"] += 1
                    if tok not in boards and tok not in allowed:
                        problems.append(
                            f"{rel}:{i}: no such board '{tok}' in `{cmd}`")
            m = VERB_RE.match(cmd)
            if not m:
                continue
            verb, sub = m.group(1), m.group(2)
            counts["verb"] += 1
            if verb not in verbs and verb not in allowed:
                problems.append(f"{rel}:{i}: no such CLI verb "
                                f"'alloy {verb}' in `{cmd}`")
            elif sub and verb in subs and sub not in subs[verb]:
                problems.append(f"{rel}:{i}: `alloy {verb}` has no subcommand "
                                f"'{sub}' (has {sorted(subs[verb])})")

        # 3. links between pages, and their anchors
        in_fence = False
        for i, line in enumerate(lines, 1):
            if line.strip().startswith("```"):
                in_fence = not in_fence
                continue
            if in_fence:
                continue
            # `[](void*)` inside an inline code span is a lambda, not a link.
            line = re.sub(r"`[^`]*`", lambda m: " " * len(m.group(0)), line)
            for m in LINK_RE.finditer(line):
                target = m.group(1)
                if re.match(r"^(https?:|mailto:|#|<)", target):
                    if target.startswith("#"):
                        counts["link"] += 1
                        if target[1:] not in anchor_cache[page]:
                            problems.append(f"{rel}:{i}: no heading '{target}' "
                                            "on this page")
                    continue
                path, _, frag = target.partition("#")
                if not path:
                    continue
                counts["link"] += 1
                dest = (page.parent / path).resolve()
                if not dest.exists():
                    problems.append(f"{rel}:{i}: dangling link '{target}'")
                elif frag and dest.suffix == ".md":
                    known = anchor_cache.get(dest)
                    if known is None:
                        known = anchors(dest)
                    if frag not in known:
                        problems.append(
                            f"{rel}:{i}: '{path}' has no heading '#{frag}'")

    if verbose:
        print(f"checked {counts['board']} board mentions, {counts['verb']} "
              f"alloy invocations and {counts['link']} links across "
              f"{len(pages)} pages")
        if live is None:
            print("note: no `alloy` on PATH — verbs came from cli.py only, "
                  "and subcommands were NOT checked")
    return problems


# ── proving it fires ─────────────────────────────────────────────────────

MUTATIONS = (
    ("a board that does not exist",
     "docs/getting-started.md", "nucleo_g0b1re", "nucleo_g0b1zz"),
    ("a CLI verb that does not exist",
     "docs/getting-started.md", "$ alloy boards", "$ alloy list-boards"),
    ("a subcommand that does not exist",
     "docs/guide/cli.md", "alloy lib search <text>", "alloy lib find <text>"),
    ("an anchor that does not exist",
     "docs/index.md", "getting-started.md", "getting-started.md#no-such-thing"),
)


def self_test() -> int:
    bad = 0
    with tempfile.TemporaryDirectory() as tmp:
        base = Path(tmp) / "docs"
        shutil.copytree(ALLOY / "docs", base)
        clean = check(base)
        clean = [p for p in clean if "verb scraper" not in p]
        print(f"{'GREEN' if not clean else 'RED  '}  unmutated docs/"
              f" — {len(clean)} problem(s)")
        if clean:
            bad += 1
            for p in clean[:5]:
                print("    " + p)
        for name, rel, old, new in MUTATIONS:
            page = base / Path(rel).relative_to("docs")
            original = page.read_text()
            if old not in original:
                print(f"SKIP   {name}: '{old}' is no longer in {rel}")
                bad += 1
                continue
            page.write_text(original.replace(old, new, 1))
            found = [p for p in check(base) if "verb scraper" not in p]
            fired = len(found) > len(clean)
            print(f"{'RED  ' if fired else 'GREEN'}  {name}"
                  + (f" — {found[len(clean)]}" if fired else " — NOT CAUGHT"))
            if not fired:
                bad += 1
            page.write_text(original)
    print("\nself-test " + ("green — every mutation was caught"
                            if not bad else f"FAILED — {bad} case(s)"))
    return 1 if bad else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--self-test", action="store_true",
                    help="mutate a scratch copy of docs/ and demand a red")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    if args.self_test:
        return self_test()

    problems = check(ALLOY / "docs", verbose=args.verbose)
    if problems:
        print(f"{len(problems)} doc fact(s) do not hold:\n", file=sys.stderr)
        for p in problems:
            print("  " + p, file=sys.stderr)
        return 1
    print("doc facts green — every board, verb, link and anchor resolves")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
