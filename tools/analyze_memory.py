#!/usr/bin/env python3
"""
Alloy Memory Analyzer

Parses linker map files and ELF binaries to generate memory usage reports
for embedded MCUs.

Supports:
- ARM Cortex-M (arm-none-eabi-ld)
- Renesas RL78 (rl78-elf-ld)
- Xtensa ESP32 (xtensa-esp32-elf-ld)

Usage:
    python analyze_memory.py --elf build/my_app.elf --map build/my_app.map \\
                             --mcu STM32F103C8 --ram-size 20480 --flash-size 65536
"""

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple


@dataclass
class Section:
    """Represents a memory section (.text, .data, .bss, etc.)"""
    name: str
    address: int
    size: int
    source_file: Optional[str] = None


@dataclass
class Symbol:
    """Represents a symbol from the linker map"""
    name: str
    address: int
    size: int
    section: str
    source_file: Optional[str] = None


@dataclass
class MemoryUsage:
    """Memory usage summary"""
    flash_used: int  # .text + .rodata
    ram_used: int    # .data + .bss
    sections: Dict[str, Section]
    symbols: List[Symbol]


class LinkerMapParser:
    """Parse linker map files from different toolchains"""

    def __init__(self, map_file: Path):
        self.map_file = map_file
        self.content = map_file.read_text()
        self.sections: Dict[str, Section] = {}
        self.symbols: List[Symbol] = []

    def parse(self) -> MemoryUsage:
        """Parse the linker map file"""
        # Detect toolchain type
        if "arm-none-eabi" in self.content.lower() or ".ARM.exidx" in self.content:
            return self._parse_arm_map()
        elif "rl78-elf" in self.content.lower():
            return self._parse_rl78_map()
        elif "xtensa" in self.content.lower():
            return self._parse_xtensa_map()
        else:
            # Default to ARM format
            return self._parse_arm_map()

    def _parse_arm_map(self) -> MemoryUsage:
        """Parse ARM Cortex-M linker map (arm-none-eabi-ld)"""

        # Parse memory configuration section
        mem_config_match = re.search(
            r"Memory Configuration\s*\n\s*Name\s+Origin\s+Length.*?\n(.*?)(?:\n\n|Linker script)",
            self.content,
            re.DOTALL
        )

        # Parse section headers
        # Look for patterns like:
        # .text           0x08000000     0x1234
        section_pattern = re.compile(
            r"^(\.\w+(?:\.\w+)?)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)",
            re.MULTILINE
        )

        for match in section_pattern.finditer(self.content):
            section_name = match.group(1)
            address = int(match.group(2), 16)
            size = int(match.group(3), 16)

            if size > 0:  # Only include non-empty sections
                self.sections[section_name] = Section(
                    name=section_name,
                    address=address,
                    size=size
                )

        # Calculate flash and RAM usage
        flash_sections = ['.text', '.rodata', '.data']
        ram_sections = ['.data', '.bss']

        flash_used = sum(
            self.sections[sec].size for sec in flash_sections
            if sec in self.sections
        )
        ram_used = sum(
            self.sections[sec].size for sec in ram_sections
            if sec in self.sections
        )

        return MemoryUsage(
            flash_used=flash_used,
            ram_used=ram_used,
            sections=self.sections,
            symbols=self.symbols
        )

    def _parse_rl78_map(self) -> MemoryUsage:
        """Parse RL78 linker map (rl78-elf-ld)"""
        # Simplified implementation - extend as needed
        return self._parse_arm_map()  # RL78 format is similar

    def _parse_xtensa_map(self) -> MemoryUsage:
        """Parse Xtensa ESP32 linker map (xtensa-esp32-elf-ld)"""
        # Simplified implementation - extend as needed
        return self._parse_arm_map()  # Xtensa format is similar


class ELFAnalyzer:
    """Analyze ELF binary using binutils (size, nm, objdump)"""

    def __init__(self, elf_file: Path):
        self.elf_file = elf_file

    def get_size_info(self) -> Dict[str, int]:
        """Run 'size' command on ELF file"""
        try:
            result = subprocess.run(
                ["arm-none-eabi-size", str(self.elf_file)],
                capture_output=True,
                text=True,
                check=True
            )

            # Parse output (Berkeley format):
            #    text    data     bss     dec     hex filename
            #    1234     512     256    2002     7d2 app.elf
            lines = result.stdout.strip().split('\n')
            if len(lines) >= 2:
                values = lines[1].split()
                return {
                    'text': int(values[0]),
                    'data': int(values[1]),
                    'bss': int(values[2]),
                }
        except (subprocess.CalledProcessError, FileNotFoundError, IndexError):
            pass

        return {'text': 0, 'data': 0, 'bss': 0}

    def get_top_symbols(self, count: int = 10) -> List[Tuple[str, int]]:
        """Get top N symbols by size using 'nm'"""
        try:
            result = subprocess.run(
                ["arm-none-eabi-nm", "--size-sort", "--reverse-sort", str(self.elf_file)],
                capture_output=True,
                text=True,
                check=True
            )

            symbols = []
            for line in result.stdout.strip().split('\n')[:count]:
                parts = line.split()
                if len(parts) >= 4:
                    size = int(parts[1], 16)
                    name = parts[3]
                    symbols.append((name, size))

            return symbols
        except (subprocess.CalledProcessError, FileNotFoundError):
            return []


class MemoryReport:
    """Generate human-readable memory report"""

    def __init__(
        self,
        usage: MemoryUsage,
        mcu: str,
        ram_size: int,
        flash_size: int,
        elf_file: Optional[Path] = None
    ):
        self.usage = usage
        self.mcu = mcu
        self.ram_size = ram_size
        self.flash_size = flash_size
        self.elf_file = elf_file

    def generate_text_report(self) -> str:
        """Generate text report"""
        lines = [
            "=" * 50,
            "Alloy Memory Usage Report",
            "=" * 50,
            f"Target MCU: {self.mcu}",
            f"RAM: {self._format_bytes(self.ram_size)}",
            f"Flash: {self._format_bytes(self.flash_size)}",
            "",
            "Flash Usage:",
            "-" * 50,
        ]

        flash_pct = (self.usage.flash_used / self.flash_size * 100) if self.flash_size > 0 else 0
        lines.append(
            f"Total: {self.usage.flash_used:,} / {self.flash_size:,} bytes ({flash_pct:.1f}%)"
        )

        # Section breakdown
        for sec_name in ['.text', '.rodata', '.data']:
            if sec_name in self.usage.sections:
                sec = self.usage.sections[sec_name]
                lines.append(f"  {sec_name:12}: {sec.size:,} bytes")

        lines.extend([
            "",
            "RAM Usage:",
            "-" * 50,
        ])

        ram_pct = (self.usage.ram_used / self.ram_size * 100) if self.ram_size > 0 else 0
        lines.append(
            f"Total: {self.usage.ram_used:,} / {self.ram_size:,} bytes ({ram_pct:.1f}%)"
        )

        for sec_name in ['.data', '.bss']:
            if sec_name in self.usage.sections:
                sec = self.usage.sections[sec_name]
                lines.append(f"  {sec_name:12}: {sec.size:,} bytes")

        # Top symbols (if ELF available)
        if self.elf_file and self.elf_file.exists():
            analyzer = ELFAnalyzer(self.elf_file)
            top_symbols = analyzer.get_top_symbols(10)
            if top_symbols:
                lines.extend([
                    "",
                    "Top Memory Consumers:",
                    "-" * 50,
                ])
                for i, (name, size) in enumerate(top_symbols, 1):
                    lines.append(f"{i:2}. {name:40}: {size:,} bytes")

        # Memory budget check
        lines.extend([
            "",
            "Memory Budget Check:",
            "-" * 50,
        ])

        # Determine budget category
        if self.ram_size <= 8192:
            budget_name = "Tiny (2-8KB RAM)"
            budget_overhead = 512
        elif self.ram_size <= 32768:
            budget_name = "Small (8-32KB RAM)"
            budget_overhead = 2048
        elif self.ram_size <= 131072:
            budget_name = "Medium (32-128KB RAM)"
            budget_overhead = 8192
        else:
            budget_name = "Large (128+KB RAM)"
            budget_overhead = 16384

        check_mark = "✅" if self.usage.ram_used <= budget_overhead else "❌"
        lines.append(f"{check_mark} Target: < {budget_overhead} bytes Alloy overhead ({budget_name})")
        lines.append(f"   Actual: {self.usage.ram_used:,} bytes")

        if flash_pct > 90:
            lines.extend([
                "",
                "⚠️  Warnings:",
                "-" * 50,
                "- Flash usage > 90%! Consider enabling ALLOY_MINIMAL_BUILD.",
            ])

        if ram_pct > 80:
            if flash_pct <= 90:
                lines.extend([
                    "",
                    "⚠️  Warnings:",
                    "-" * 50,
                ])
            lines.append("- RAM usage > 80%! Review buffer sizes and stack allocation.")

        lines.append("=" * 50)
        return '\n'.join(lines)

    @staticmethod
    def _format_bytes(size: int) -> str:
        """Format bytes as human-readable string"""
        if size < 1024:
            return f"{size} bytes"
        elif size < 1024 * 1024:
            return f"{size / 1024:.1f} KB"
        else:
            return f"{size / (1024 * 1024):.1f} MB"


def main():
    parser = argparse.ArgumentParser(
        description="Analyze memory usage from linker map and ELF files"
    )
    parser.add_argument("--elf", type=Path, required=True, help="Path to ELF binary")
    parser.add_argument("--map", type=Path, required=True, help="Path to linker map file")
    parser.add_argument("--mcu", required=True, help="Target MCU name")
    parser.add_argument("--ram-size", type=int, required=True, help="RAM size in bytes")
    parser.add_argument("--flash-size", type=int, required=True, help="Flash size in bytes")
    parser.add_argument("--json", action="store_true", help="Output JSON format")

    args = parser.parse_args()

    # Validate inputs
    if not args.map.exists():
        print(f"Error: Map file not found: {args.map}", file=sys.stderr)
        return 1

    if not args.elf.exists():
        print(f"Error: ELF file not found: {args.elf}", file=sys.stderr)
        return 1

    # Parse map file
    parser = LinkerMapParser(args.map)
    usage = parser.parse()

    # Generate report
    report = MemoryReport(
        usage=usage,
        mcu=args.mcu,
        ram_size=args.ram_size,
        flash_size=args.flash_size,
        elf_file=args.elf
    )

    if args.json:
        # TODO: Implement JSON output
        print("JSON output not yet implemented", file=sys.stderr)
        return 1
    else:
        print(report.generate_text_report())

    return 0


if __name__ == "__main__":
    sys.exit(main())
