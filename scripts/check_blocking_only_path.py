#!/usr/bin/env python3
"""Guard the blocking+completion zero-overhead invariant.

The runtime async model promises that a user who consumes only blocking HAL
operations plus typed completion tokens does not have to link or touch the
`alloy::async` adapter layer. We enforce that invariant at the source level
by requiring a small, fixed set of TUs to stay free of the adapter:

- `examples/async_uart_timeout/main.cpp` (canonical user-facing example)
- `tests/compile_tests/test_blocking_only_completion_api.cpp` (compile guard)

These files MUST NOT:
- `#include "async.hpp"` (direct adapter include)
- `#include "runtime/async.hpp"` or `"runtime/async_uart.hpp"` (internal include)
- name `alloy::async::` or `runtime::async::` (token usage)

Running: `python3 scripts/check_blocking_only_path.py`
Exit code: 0 on pass, 1 on violation.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

GUARDED_FILES = [
    "examples/async_uart_timeout/main.cpp",
    "tests/compile_tests/test_blocking_only_completion_api.cpp",
]

FORBIDDEN_PATTERNS: list[tuple[str, re.Pattern[str]]] = [
    ("includes async.hpp", re.compile(r'#\s*include\s+"async\.hpp"')),
    ("includes runtime/async.hpp", re.compile(r'#\s*include\s+"runtime/async\.hpp"')),
    # Cover every async_<peripheral>.hpp wrapper, current and future.
    ("includes runtime/async_<peripheral>.hpp",
     re.compile(r'#\s*include\s+"runtime/async_[a-z0-9_]+\.hpp"')),
    ("names alloy::async::", re.compile(r"\balloy::async::")),
    ("names runtime::async::", re.compile(r"\bruntime::async::")),
]


def main() -> int:
    violations: list[str] = []
    for rel in GUARDED_FILES:
        path = ROOT / rel
        if not path.is_file():
            violations.append(f"missing guarded file: {rel}")
            continue
        text = path.read_text(encoding="utf-8")
        for lineno, line in enumerate(text.splitlines(), start=1):
            stripped = line.strip()
            # ignore comments entirely — guard is about real usage, not prose
            if stripped.startswith("//") or stripped.startswith("*"):
                continue
            for label, pattern in FORBIDDEN_PATTERNS:
                if pattern.search(line):
                    violations.append(
                        f"{rel}:{lineno}: {label}: {stripped}"
                    )

    if violations:
        print("Blocking-only path violations found:", file=sys.stderr)
        for v in violations:
            print(f"  - {v}", file=sys.stderr)
        print(
            "\nThe blocking+completion path must stay usable without the async "
            "adapter layer. Move adapter code into a separate example/test.",
            file=sys.stderr,
        )
        return 1

    print("Blocking-only path check passed.")
    print(f"Guarded files ({len(GUARDED_FILES)}):")
    for rel in GUARDED_FILES:
        print(f"  - {rel}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
