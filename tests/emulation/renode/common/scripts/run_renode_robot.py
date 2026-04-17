#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import tempfile
from pathlib import Path


def parse_key_value(entry: str, *, flag: str) -> tuple[str, str]:
    if "=" not in entry:
        raise argparse.ArgumentTypeError(f"{flag} expects NAME=VALUE, got: {entry}")
    name, value = entry.split("=", 1)
    if not name:
        raise argparse.ArgumentTypeError(f"{flag} requires a non-empty NAME: {entry}")
    if not value:
        raise argparse.ArgumentTypeError(f"{flag} requires a non-empty VALUE: {entry}")
    return name, value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run a Renode robot scenario with staged file links.")
    parser.add_argument("--renode-test", required=True)
    parser.add_argument("--renode", required=True)
    parser.add_argument("--robot", required=True)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--renode-config")
    parser.add_argument("--nm")
    parser.add_argument("--symbol-var", action="append", default=[], metavar="NAME=SYMBOL")
    parser.add_argument("--file-var", action="append", default=[], metavar="NAME=PATH")
    parser.add_argument("--variable", action="append", default=[], metavar="NAME=VALUE")
    return parser.parse_args()


def read_symbol_address(nm: Path, elf: Path, symbol: str) -> str:
    result = subprocess.run(
        [str(nm), "-g", str(elf)],
        check=True,
        capture_output=True,
        text=True,
    )
    pattern = re.compile(rf"^([0-9a-fA-F]+)\s+\w\s+{re.escape(symbol)}$")
    for line in result.stdout.splitlines():
        match = pattern.match(line.strip())
        if match:
            return f"0x{match.group(1)}"
    raise RuntimeError(f"Symbol '{symbol}' not found in {elf}")


def stage_link(temp_dir: Path, source: Path, preferred_name: str | None = None) -> Path:
    suffix = "".join(source.suffixes)
    stem = preferred_name if preferred_name else source.stem
    link_path = temp_dir / f"{stem}{suffix}"
    counter = 1
    while link_path.exists():
        link_path = temp_dir / f"{stem}_{counter}{suffix}"
        counter += 1
    link_path.symlink_to(source)
    return link_path


def main() -> int:
    args = parse_args()

    renode_test = Path(args.renode_test)
    renode = Path(args.renode)
    robot = Path(args.robot)
    elf = Path(args.elf)
    platform = Path(args.platform)
    renode_config = Path(args.renode_config) if args.renode_config else None
    nm = Path(args.nm) if args.nm else None

    symbol_vars = [parse_key_value(entry, flag="--symbol-var") for entry in args.symbol_var]
    file_vars = [parse_key_value(entry, flag="--file-var") for entry in args.file_var]
    plain_vars = [parse_key_value(entry, flag="--variable") for entry in args.variable]

    if symbol_vars and nm is None:
        raise RuntimeError("--nm is required when using --symbol-var")

    with tempfile.TemporaryDirectory(prefix="alloy-renode-") as temp_dir_name:
        temp_dir = Path(temp_dir_name)

        staged_robot = stage_link(temp_dir, robot, preferred_name=robot.stem)
        staged_elf = stage_link(temp_dir, elf, preferred_name="firmware")
        staged_platform = stage_link(temp_dir, platform, preferred_name=platform.stem)
        staged_renode_config = (
            stage_link(temp_dir, renode_config, preferred_name="renode")
            if renode_config
            else None
        )

        variables = {
            "RENODE": str(renode),
            "ELF": str(staged_elf),
            "PLATFORM_REPL": str(staged_platform),
        }

        for name, value in plain_vars:
            variables[name] = value

        for name, symbol in symbol_vars:
            variables[name] = read_symbol_address(nm, elf, symbol)

        for name, source_path in file_vars:
            staged_path = stage_link(temp_dir, Path(source_path).resolve(), preferred_name=name.lower())
            variables[name] = str(staged_path)

        command = [str(renode_test), str(staged_robot)]
        if staged_renode_config:
            command.extend(["--renode-config", str(staged_renode_config)])
        for name, value in variables.items():
            command.extend(["--variable", f"{name}:{value}"])

        completed = subprocess.run(command, cwd=temp_dir)
        return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
