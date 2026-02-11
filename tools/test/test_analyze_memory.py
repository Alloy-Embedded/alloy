from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(REPO_ROOT / "tools"))

from analyze_memory import LinkerMapParser, MemoryReport  # noqa: E402


FIXTURES_DIR = Path(__file__).resolve().parent / "fixtures"


def test_parse_arm_map_sections() -> None:
    parser = LinkerMapParser(FIXTURES_DIR / "arm_linker.map")
    usage = parser.parse()

    assert ".text" in usage.sections
    assert ".rodata" in usage.sections
    assert ".data" in usage.sections
    assert ".bss" in usage.sections

    assert usage.sections[".text"].size == 0x1000
    assert usage.sections[".rodata"].size == 0x200
    assert usage.flash_used == 0x1000 + 0x200 + 0x80
    assert usage.ram_used == 0x80 + 0x100


def test_parse_rl78_map_sections() -> None:
    parser = LinkerMapParser(FIXTURES_DIR / "rl78_linker.map")
    usage = parser.parse()

    assert ".text" in usage.sections
    assert ".data" in usage.sections
    assert usage.flash_used == 0x800 + 0x100 + 0x40
    assert usage.ram_used == 0x40 + 0xC0


def test_generate_json_report_structure(tmp_path: Path) -> None:
    parser = LinkerMapParser(FIXTURES_DIR / "arm_linker.map")
    usage = parser.parse()

    dummy_elf = tmp_path / "dummy.elf"
    dummy_elf.write_bytes(b"ELF")

    report = MemoryReport(
        usage=usage,
        mcu="STM32F401RE",
        ram_size=96 * 1024,
        flash_size=512 * 1024,
        elf_file=dummy_elf,
    )

    json_report = report.generate_json_report()

    assert json_report["mcu"] == "STM32F401RE"
    assert json_report["memory"]["flash"]["used_bytes"] == usage.flash_used
    assert json_report["memory"]["ram"]["used_bytes"] == usage.ram_used
    assert isinstance(json_report["sections"], list)
    assert "budget" in json_report
    assert "within_budget" in json_report["budget"]


def test_cli_json_output(tmp_path: Path) -> None:
    dummy_elf = tmp_path / "dummy.elf"
    dummy_elf.write_bytes(b"ELF")

    cmd = [
        sys.executable,
        str(REPO_ROOT / "tools" / "analyze_memory.py"),
        "--elf",
        str(dummy_elf),
        "--map",
        str(FIXTURES_DIR / "arm_linker.map"),
        "--mcu",
        "STM32F401RE",
        "--ram-size",
        str(96 * 1024),
        "--flash-size",
        str(512 * 1024),
        "--json",
    ]

    result = subprocess.run(cmd, capture_output=True, text=True, check=False)

    assert result.returncode == 0, result.stderr
    payload = json.loads(result.stdout)
    assert payload["tool"] == "analyze_memory.py"
    assert payload["mcu"] == "STM32F401RE"
