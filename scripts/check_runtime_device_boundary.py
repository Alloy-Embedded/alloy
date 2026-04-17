#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]

MONITORED_ROOTS = (
    Path("src/device"),
    Path("src/arch"),
    Path("tests/compile"),
    Path("tests/compile_tests"),
)

OPTIONAL_ROOTS = (
    Path("src/hal/connect"),
    Path("src/hal/claim"),
    Path("src/hal/gpio"),
    Path("src/hal/uart"),
    Path("src/hal/spi"),
    Path("src/hal/i2c"),
    Path("src/hal/dma"),
)

SOURCE_SUFFIXES = {".hpp", ".hh", ".h", ".cpp", ".cc", ".cxx", ".ipp"}

FORBIDDEN_PATTERNS: tuple[tuple[str, re.Pattern[str]], ...] = (
    (
        "legacy vendor include",
        re.compile(r'#include\s*[<"]hal/vendors/[^">]+[">]'),
    ),
    (
        "direct generated descriptor include",
        re.compile(r'#include\s*[<"][^">]*generated/[^">]*[">]'),
    ),
    (
        "direct published family include",
        re.compile(r'#include\s*[<"](st|microchip|nxp)/[^">]+[">]'),
    ),
    (
        "direct alloy-devices filesystem reference",
        re.compile(r"alloy-devices"),
    ),
)


@dataclass(frozen=True)
class Violation:
    path: Path
    line_number: int
    rule: str
    line: str


def iter_monitored_files() -> list[Path]:
    roots = [*MONITORED_ROOTS]
    roots.extend(path for path in OPTIONAL_ROOTS if (REPO_ROOT / path).exists())

    files: list[Path] = []
    for root in roots:
        absolute_root = REPO_ROOT / root
        if not absolute_root.exists():
            continue
        for file_path in absolute_root.rglob("*"):
            if file_path.is_file() and file_path.suffix in SOURCE_SUFFIXES:
                files.append(file_path)
    return sorted(files)


def scan_file(path: Path) -> list[Violation]:
    violations: list[Violation] = []
    relative_path = path.relative_to(REPO_ROOT)
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        for rule, pattern in FORBIDDEN_PATTERNS:
            if pattern.search(line):
                violations.append(
                    Violation(
                        path=relative_path,
                        line_number=line_number,
                        rule=rule,
                        line=line.strip(),
                    )
                )
    return violations


def main() -> int:
    files = iter_monitored_files()
    violations: list[Violation] = []
    for file_path in files:
        violations.extend(scan_file(file_path))

    if not violations:
        print("Runtime device boundary check passed.")
        print("Checked roots:")
        for root in MONITORED_ROOTS:
            print(f"  - {root}")
        for root in OPTIONAL_ROOTS:
            if (REPO_ROOT / root).exists():
                print(f"  - {root}")
        return 0

    print("Runtime device boundary violations found:")
    for violation in violations:
        print(
            f"  - {violation.path}:{violation.line_number}: {violation.rule}: {violation.line}"
        )
    print("")
    print("New runtime code must import device data through src/device only.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
