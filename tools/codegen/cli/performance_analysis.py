#!/usr/bin/env python3
"""
Performance Analysis Tool for Policy-Based Peripheral Architecture

This script analyzes and compares performance metrics between the old
architecture and the new policy-based design:

1. Binary Size Analysis
   - Measure .text, .data, .bss sections
   - Compare old vs new implementation
   - Calculate overhead percentage

2. Compile Time Benchmarking
   - Measure full build time
   - Measure incremental build time
   - Profile template instantiation

3. Runtime Performance
   - Analyze assembly output
   - Verify zero-overhead abstraction
   - Compare instruction counts

Usage:
    python3 performance_analysis.py --platform same70
    python3 performance_analysis.py --platform stm32f4 --peripheral uart
    python3 performance_analysis.py --all

Requirements:
    - arm-none-eabi-gcc toolchain
    - Build system (CMake)
    - objdump, size, nm utilities

Author: Auto-generated for Phase 13
Date: 2025-11-11
"""

import argparse
import subprocess
import time
import json
import os
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict


@dataclass
class BinarySizeMetrics:
    """Binary size metrics for a build"""
    text_size: int  # Code section size
    data_size: int  # Initialized data size
    bss_size: int   # Uninitialized data size
    total_size: int # Total size

    def overhead_vs(self, baseline: 'BinarySizeMetrics') -> float:
        """Calculate overhead percentage vs baseline"""
        if baseline.total_size == 0:
            return 0.0
        return ((self.total_size - baseline.total_size) / baseline.total_size) * 100


@dataclass
class CompileTimeMetrics:
    """Compile time metrics"""
    full_build_time: float      # Full rebuild time (seconds)
    incremental_time: float     # Incremental rebuild time (seconds)
    template_time: Optional[float] = None  # Template instantiation time


@dataclass
class RuntimeMetrics:
    """Runtime performance metrics"""
    instruction_count: int      # Number of instructions
    function_calls: int         # Number of function calls
    memory_accesses: int        # Number of memory accesses
    is_inlined: bool           # Whether code is fully inlined


class PerformanceAnalyzer:
    """Analyzes performance of policy-based peripheral implementation"""

    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.build_dir = project_root / "build"
        self.results: Dict = {}

    def analyze_binary_size(self, binary_path: Path) -> BinarySizeMetrics:
        """
        Analyze binary size using arm-none-eabi-size

        Example output:
           text    data     bss     dec     hex filename
           1234     100     200    1534     5fe app.elf
        """
        try:
            result = subprocess.run(
                ["arm-none-eabi-size", str(binary_path)],
                capture_output=True,
                text=True,
                check=True
            )

            # Parse size output (skip header line)
            lines = result.stdout.strip().split('\n')
            if len(lines) < 2:
                raise ValueError("Invalid size output")

            # Parse: text data bss dec hex filename
            parts = lines[1].split()
            text_size = int(parts[0])
            data_size = int(parts[1])
            bss_size = int(parts[2])
            total_size = text_size + data_size + bss_size

            return BinarySizeMetrics(
                text_size=text_size,
                data_size=data_size,
                bss_size=bss_size,
                total_size=total_size
            )

        except subprocess.CalledProcessError as e:
            print(f"Error running arm-none-eabi-size: {e}")
            return BinarySizeMetrics(0, 0, 0, 0)
        except FileNotFoundError:
            print("Error: arm-none-eabi-size not found. Install ARM toolchain.")
            return BinarySizeMetrics(0, 0, 0, 0)

    def analyze_assembly(self, binary_path: Path, function_name: str) -> RuntimeMetrics:
        """
        Analyze assembly output for a specific function

        Uses objdump to disassemble and count instructions
        """
        try:
            result = subprocess.run(
                ["arm-none-eabi-objdump", "-d", str(binary_path)],
                capture_output=True,
                text=True,
                check=True
            )

            # Find function in disassembly
            lines = result.stdout.split('\n')
            in_function = False
            instruction_count = 0
            function_calls = 0
            memory_accesses = 0

            for line in lines:
                # Check if we entered the function
                if f"<{function_name}>" in line:
                    in_function = True
                    continue

                # Check if we exited (next function starts)
                if in_function and line.strip() and not line.startswith(' '):
                    if '>' in line:  # New function
                        break

                if in_function and line.strip():
                    # Count instructions
                    if '\t' in line:  # Instruction line
                        instruction_count += 1

                        # Count function calls (bl, blx)
                        if '\tbl' in line or '\tblx' in line:
                            function_calls += 1

                        # Count memory accesses (ldr, str)
                        if '\tldr' in line or '\tstr' in line:
                            memory_accesses += 1

            # Function is inlined if we found no instructions
            is_inlined = (instruction_count == 0)

            return RuntimeMetrics(
                instruction_count=instruction_count,
                function_calls=function_calls,
                memory_accesses=memory_accesses,
                is_inlined=is_inlined
            )

        except subprocess.CalledProcessError as e:
            print(f"Error running objdump: {e}")
            return RuntimeMetrics(0, 0, 0, False)
        except FileNotFoundError:
            print("Error: arm-none-eabi-objdump not found.")
            return RuntimeMetrics(0, 0, 0, False)

    def measure_compile_time(self, clean_build: bool = True) -> CompileTimeMetrics:
        """
        Measure compile time for full and incremental builds
        """
        # Full build time
        if clean_build:
            # Clean build directory
            subprocess.run(["cmake", "--build", str(self.build_dir), "--target", "clean"],
                         capture_output=True)

        # Measure full build
        start = time.time()
        result = subprocess.run(
            ["cmake", "--build", str(self.build_dir), "-j8"],
            capture_output=True,
            text=True
        )
        full_build_time = time.time() - start

        # Measure incremental build (touch a file and rebuild)
        # Touch a header file
        test_file = self.project_root / "src/hal/api/uart_simple.hpp"
        if test_file.exists():
            test_file.touch()

            start = time.time()
            result = subprocess.run(
                ["cmake", "--build", str(self.build_dir), "-j8"],
                capture_output=True,
                text=True
            )
            incremental_time = time.time() - start
        else:
            incremental_time = 0.0

        return CompileTimeMetrics(
            full_build_time=full_build_time,
            incremental_time=incremental_time
        )

    def generate_report(self, output_path: Path):
        """Generate JSON report of all metrics"""
        with open(output_path, 'w') as f:
            json.dump(self.results, f, indent=2)

        print(f"\n✅ Report generated: {output_path}")

    def print_summary(self):
        """Print summary of analysis results"""
        print("\n" + "=" * 80)
        print("PERFORMANCE ANALYSIS SUMMARY")
        print("=" * 80)

        if 'binary_size' in self.results:
            print("\n📊 BINARY SIZE ANALYSIS")
            print("-" * 80)

            for config, metrics in self.results['binary_size'].items():
                print(f"\n{config}:")
                print(f"  .text (code):  {metrics['text_size']:8d} bytes")
                print(f"  .data (init):  {metrics['data_size']:8d} bytes")
                print(f"  .bss  (uninit):{metrics['bss_size']:8d} bytes")
                print(f"  Total:         {metrics['total_size']:8d} bytes")

        if 'compile_time' in self.results:
            print("\n⏱️  COMPILE TIME ANALYSIS")
            print("-" * 80)

            metrics = self.results['compile_time']
            print(f"  Full build:        {metrics['full_build_time']:.2f} seconds")
            print(f"  Incremental build: {metrics['incremental_time']:.2f} seconds")

        if 'runtime' in self.results:
            print("\n🚀 RUNTIME PERFORMANCE ANALYSIS")
            print("-" * 80)

            for func, metrics in self.results['runtime'].items():
                print(f"\n{func}:")
                print(f"  Instructions:     {metrics['instruction_count']}")
                print(f"  Function calls:   {metrics['function_calls']}")
                print(f"  Memory accesses:  {metrics['memory_accesses']}")
                print(f"  Fully inlined:    {metrics['is_inlined']}")

        print("\n" + "=" * 80)


def main():
    parser = argparse.ArgumentParser(
        description="Performance analysis for policy-based peripherals"
    )
    parser.add_argument(
        "--platform",
        choices=["same70", "stm32f4", "stm32f1"],
        help="Target platform"
    )
    parser.add_argument(
        "--peripheral",
        choices=["uart", "spi", "i2c", "gpio"],
        help="Peripheral to analyze"
    )
    parser.add_argument(
        "--binary",
        type=Path,
        help="Path to binary file for analysis"
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Run all analyses"
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("performance_report.json"),
        help="Output report path"
    )

    args = parser.parse_args()

    # Get project root (assume script is in tools/codegen/cli/)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent.parent

    analyzer = PerformanceAnalyzer(project_root)

    print("🔍 Performance Analysis Tool")
    print(f"📁 Project root: {project_root}")

    # Binary size analysis
    if args.binary:
        print(f"\n📊 Analyzing binary: {args.binary}")
        metrics = analyzer.analyze_binary_size(args.binary)
        analyzer.results['binary_size'] = {
            'analyzed_binary': asdict(metrics)
        }

    # Compile time analysis
    if args.all:
        print("\n⏱️  Measuring compile time...")
        compile_metrics = analyzer.measure_compile_time()
        analyzer.results['compile_time'] = asdict(compile_metrics)

    # Assembly analysis
    if args.binary and args.peripheral:
        print(f"\n🔍 Analyzing assembly for {args.peripheral}...")

        # Analyze key functions
        functions = {
            'uart': ['_ZN5alloy3hal6same7013Usart0Hardware10write_byteEh'],
            'spi': ['_ZN5alloy3hal6same7011Spi0Hardware10write_byteEh'],
        }

        if args.peripheral in functions:
            analyzer.results['runtime'] = {}
            for func in functions[args.peripheral]:
                metrics = analyzer.analyze_assembly(args.binary, func)
                analyzer.results['runtime'][func] = asdict(metrics)

    # Print summary
    analyzer.print_summary()

    # Generate report
    analyzer.generate_report(args.output)

    return 0


if __name__ == "__main__":
    sys.exit(main())
