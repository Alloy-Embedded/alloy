#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


def run(*args: str) -> str:
    completed = subprocess.run(args, check=True, capture_output=True, text=True)
    return completed.stdout


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Inspect an embedded ELF for startup readiness.")
    parser.add_argument("--elf", required=True)
    parser.add_argument("--readelf", required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--objdump", required=True)
    parser.add_argument("--vector-section", default=".isr_vector")
    parser.add_argument("--entry-symbol", default="Reset_Handler")
    parser.add_argument("--required-section", action="append", default=[])
    parser.add_argument("--required-symbol", action="append", default=[])
    parser.add_argument("--thumb-entry", action="store_true")
    return parser.parse_args()


def parse_sections(readelf: Path, elf: Path) -> dict[str, dict[str, int]]:
    output = run(str(readelf), "-S", str(elf))
    sections: dict[str, dict[str, int]] = {}
    pattern = re.compile(
        r"^\s*\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)"
    )
    for line in output.splitlines():
        match = pattern.match(line)
        if match:
            sections[match.group(1)] = {
                "address": int(match.group(2), 16),
                "size": int(match.group(3), 16),
            }
    return sections


def parse_entry_point(readelf: Path, elf: Path) -> int:
    output = run(str(readelf), "-h", str(elf))
    match = re.search(r"Entry point address:\s+0x([0-9a-fA-F]+)", output)
    if not match:
        raise RuntimeError("ELF entry point not found in readelf header output")
    return int(match.group(1), 16)


def parse_symbols(nm: Path, elf: Path) -> dict[str, int]:
    output = run(str(nm), str(elf))
    symbols: dict[str, int] = {}
    pattern = re.compile(r"^([0-9a-fA-F]+)\s+\w\s+(.+)$")
    for line in output.splitlines():
        match = pattern.match(line.strip())
        if match:
            symbols[match.group(2)] = int(match.group(1), 16)
    return symbols


def parse_vector_bytes(objdump: Path, elf: Path, section: str) -> bytes:
    output = run(str(objdump), "-s", "-j", section, str(elf))
    collected = bytearray()
    for line in output.splitlines():
        parts = line.strip().split()
        if len(parts) < 2:
            continue
        if not re.fullmatch(r"[0-9a-fA-F]+", parts[0]):
            continue
        for word in parts[1:]:
            if not re.fullmatch(r"[0-9a-fA-F]{8}", word):
                break
            collected.extend(bytes.fromhex(word))
    return bytes(collected)


def word_le(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 4], byteorder="little", signed=False)


def main() -> int:
    args = parse_args()

    elf = Path(args.elf)
    readelf = Path(args.readelf)
    nm = Path(args.nm)
    objdump = Path(args.objdump)

    sections = parse_sections(readelf, elf)
    symbols = parse_symbols(nm, elf)
    entry_point = parse_entry_point(readelf, elf)
    vector_bytes = parse_vector_bytes(objdump, elf, args.vector_section)

    errors: list[str] = []

    for section in args.required_section:
        if section not in sections:
            errors.append(f"missing required section: {section}")

    for symbol in args.required_symbol:
        if symbol not in symbols:
            errors.append(f"missing required symbol: {symbol}")

    if args.vector_section not in sections:
        errors.append(f"missing vector section: {args.vector_section}")
    elif sections[args.vector_section]["size"] < 8:
        errors.append(f"vector section {args.vector_section} is smaller than 8 bytes")

    if len(vector_bytes) < 8:
        errors.append(f"vector section dump for {args.vector_section} is smaller than 8 bytes")

    reset_symbol = symbols.get(args.entry_symbol)
    if reset_symbol is None:
        errors.append(f"missing entry symbol: {args.entry_symbol}")

    if len(vector_bytes) >= 8:
        initial_stack_pointer = word_le(vector_bytes, 0)
        reset_vector = word_le(vector_bytes, 4)

        if initial_stack_pointer == 0:
            errors.append("initial stack pointer in vector table is zero")
        if initial_stack_pointer % 8 != 0:
            errors.append(
                f"initial stack pointer 0x{initial_stack_pointer:08x} is not 8-byte aligned"
            )

        if reset_symbol is not None:
            accepted_entry_points = {reset_symbol}
            accepted_reset_vectors = {reset_symbol}
            if args.thumb_entry:
                accepted_entry_points.add(reset_symbol | 0x1)
                accepted_reset_vectors.add(reset_symbol | 0x1)

            if entry_point not in accepted_entry_points:
                errors.append(
                    f"ELF entry point 0x{entry_point:08x} does not match {args.entry_symbol}"
                )
            if reset_vector not in accepted_reset_vectors:
                errors.append(
                    f"reset vector 0x{reset_vector:08x} does not match {args.entry_symbol}"
                )
            if entry_point != reset_vector:
                errors.append(
                    f"ELF entry point 0x{entry_point:08x} does not match vector reset word 0x{reset_vector:08x}"
                )

    if errors:
        for error in errors:
            print(f"ELF startup inspection failed: {error}", file=sys.stderr)
        return 1

    checked_sections = ", ".join(args.required_section)
    checked_symbols = ", ".join(args.required_symbol)
    print(
        "ELF startup inspection passed "
        f"(sections: {checked_sections}; symbols: {checked_symbols}; entry: 0x{entry_point:08x})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
