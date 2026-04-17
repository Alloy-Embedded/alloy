#!/usr/bin/env python3

from __future__ import annotations

import argparse
import difflib
import re
import subprocess
import sys
from pathlib import Path


def run_command(command: list[str]) -> str:
    completed = subprocess.run(command, capture_output=True, text=True, check=False)
    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed ({completed.returncode}): {' '.join(command)}\n"
            f"stdout:\n{completed.stdout}\n"
            f"stderr:\n{completed.stderr}"
        )
    return completed.stdout


def parse_pairs(raw_pairs: list[str]) -> list[tuple[str, str]]:
    pairs: list[tuple[str, str]] = []
    for raw in raw_pairs:
        manual, separator, runtime = raw.partition(":")
        if separator != ":" or not manual or not runtime:
            raise ValueError(f"invalid --pair value: {raw!r}")
        pairs.append((manual, runtime))
    return pairs


def load_symbol_sizes(nm: str, artifact: Path) -> dict[str, int]:
    output = run_command([nm, "-S", "--size-sort", str(artifact)])
    sizes: dict[str, int] = {}
    pattern = re.compile(r"^[0-9A-Fa-f]+\s+([0-9A-Fa-f]+)\s+\w\s+(\S+)$")
    for raw_line in output.splitlines():
        line = raw_line.strip()
        match = pattern.match(line)
        if match is None:
            continue
        sizes[match.group(2)] = int(match.group(1), 16)
    return sizes


def disassemble_symbol(objdump: str, artifact: Path, symbol: str) -> list[str]:
    output = run_command(
        [
            objdump,
            "-d",
            "-C",
            "--no-show-raw-insn",
            f"--disassemble={symbol}",
            str(artifact),
        ]
    )

    lines: list[str] = []
    capture = False
    symbol_header = re.compile(rf"^[0-9A-Fa-f]+\s+<{re.escape(symbol)}>:$")
    instruction_line = re.compile(r"^\s*[0-9A-Fa-f]+:\s*(.+)$")

    for raw_line in output.splitlines():
        line = raw_line.rstrip()
        if symbol_header.match(line):
            capture = True
            continue
        if capture and line.endswith(">:"):
            break
        if not capture:
            continue
        match = instruction_line.match(line)
        if match is None:
            continue
        instruction = match.group(1).split("@", 1)[0]
        instruction = re.sub(r"\s+", " ", instruction).strip()
        if instruction:
            lines.append(instruction)

    if not lines:
        raise RuntimeError(f"symbol {symbol!r} was not found in objdump output for {artifact}")

    return lines


def verify_pair(objdump: str, symbol_sizes: dict[str, int], artifact: Path, manual: str,
                runtime: str) -> list[str]:
    failures: list[str] = []

    if manual not in symbol_sizes:
        failures.append(f"missing manual symbol in nm output: {manual}")
        return failures
    if runtime not in symbol_sizes:
        failures.append(f"missing runtime symbol in nm output: {runtime}")
        return failures

    manual_size = symbol_sizes[manual]
    runtime_size = symbol_sizes[runtime]
    if manual_size != runtime_size:
        failures.append(
            f"{runtime} size mismatch: runtime={runtime_size} bytes manual={manual_size} bytes"
        )

    manual_instructions = disassemble_symbol(objdump, artifact, manual)
    runtime_instructions = disassemble_symbol(objdump, artifact, runtime)

    if manual_instructions != runtime_instructions:
        diff = "\n".join(
            difflib.unified_diff(
                manual_instructions,
                runtime_instructions,
                fromfile=manual,
                tofile=runtime,
                lineterm="",
            )
        )
        failures.append(f"{runtime} instruction mismatch:\n{diff}")

    return failures


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify that runtime-lite SAME70 hot paths compile to the same assembly as manual register access."
    )
    parser.add_argument("--artifact", required=True, type=Path)
    parser.add_argument("--objdump", required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--pair", action="append", default=[])
    args = parser.parse_args()

    pairs = parse_pairs(args.pair)
    if not pairs:
        raise ValueError("at least one --pair manual_symbol:runtime_symbol is required")

    symbol_sizes = load_symbol_sizes(args.nm, args.artifact)

    failures: list[str] = []
    for manual, runtime in pairs:
        failures.extend(verify_pair(args.objdump, symbol_sizes, args.artifact, manual, runtime))

    if failures:
        sys.stderr.write("same70 zero-overhead verification failed:\n")
        for failure in failures:
            sys.stderr.write(f"- {failure}\n")
        return 1

    print(f"verified {len(pairs)} zero-overhead symbol pair(s) in {args.artifact}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
