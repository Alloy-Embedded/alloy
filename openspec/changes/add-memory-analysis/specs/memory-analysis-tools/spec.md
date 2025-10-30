# Specification: Memory Analysis Tools

## Overview

This specification defines the memory analysis infrastructure for Alloy, enabling developers to understand and optimize memory usage on resource-constrained MCUs.

## Scope

### In Scope
- CMake memory analysis targets
- Linker map parser (Python tool)
- Memory usage report generation
- Compilation flags for minimal builds
- Static assertion templates for validation
- Memory budget documentation standards

### Out of Scope
- Runtime memory profiling (Phase 2)
- Heap fragmentation analysis (bare-metal doesn't use heap)
- Dynamic allocation tracking (HAL doesn't use heap)

## Requirements

### Functional Requirements

#### FR-1: CMake Memory Report Target

**SHALL** provide a CMake target `memory-report` that:
- Generates memory usage report after successful build
- Works with host, ARM, and RL78 toolchains
- Outputs human-readable text report
- Exits with error if tool fails

```cmake
cmake --build build --target memory-report
```

#### FR-2: Linker Map Parsing

**SHALL** provide Python script `tools/analyze_memory.py` that:
- Parses linker map files from arm-none-eabi-ld, rl78-elf-ld, xtensa-ld
- Extracts .text, .rodata, .data, .bss sections
- Identifies top N memory consumers (symbols)
- Calculates Alloy framework overhead vs. user code

**Input**: `<executable>.map` (linker map file)
**Output**: JSON with memory breakdown + text report

#### FR-3: Minimal Build Mode

**SHALL** provide CMake option `ALLOY_MINIMAL_BUILD`:

```cmake
option(ALLOY_MINIMAL_BUILD "Optimize for smallest memory footprint" OFF)
```

When enabled, **SHALL** add compilation flags:
- `-Os` (optimize for size)
- `-ffunction-sections` (each function in separate section)
- `-fdata-sections` (each data in separate section)
- `-flto` (link-time optimization)
- `-Wl,--gc-sections` (remove unused sections)
- `-Wl,--print-memory-usage` (show memory usage)

#### FR-4: Memory Budget Validation

**SHALL** provide static assertion macros:

```cpp
// In alloy/core/memory.hpp
namespace alloy::core {

// Validate type size
#define ALLOY_ASSERT_MAX_SIZE(Type, MaxBytes) \
    static_assert(sizeof(Type) <= MaxBytes, \
        #Type " exceeds memory budget of " #MaxBytes " bytes")

// Validate zero-overhead type
#define ALLOY_ASSERT_ZERO_OVERHEAD(Type) \
    static_assert(std::is_trivially_copyable_v<Type>, \
        #Type " must be trivially copyable"); \
    static_assert(std::is_standard_layout_v<Type>, \
        #Type " must have standard layout")

} // namespace alloy::core
```

Usage in HAL implementations:
```cpp
template<uint8_t PIN>
class GpioPin {
    ALLOY_ASSERT_ZERO_OVERHEAD(GpioPin<PIN>);
    ALLOY_ASSERT_MAX_SIZE(GpioPin<PIN>, 4);
    // ...
};
```

#### FR-5: Memory Budget Documentation Standard

**SHALL** provide template for documenting module memory budgets in each HAL module's README:

```markdown
## Memory Footprint

| Component | RAM (bytes) | Flash (bytes) | Notes |
|-----------|-------------|---------------|-------|
| `GpioPin<N>` | 0 | ~20 | Zero-size empty class |
| `UartDriver<64,64>` | 128 + 16 | ~500 | Buffers + state machine |
| Total (typical) | 144 | 520 | With one UART instance |

**Tested on**: STM32F103 (GCC 11.3, -Os, LTO enabled)
```

#### FR-6: CI Memory Tracking

**SHALL** add GitHub Actions workflow `.github/workflows/memory-check.yml`:
- Builds example for each target MCU
- Generates memory report
- Compares to baseline (stored in repo or previous commit)
- Comments on PR if memory usage increases significantly (> 5%)
- Stores memory report as artifact

### Non-Functional Requirements

#### NFR-1: Performance
- Linker map parsing **SHALL** complete in < 5 seconds for maps up to 10MB
- Memory report generation **SHALL** add < 10 seconds to build time

#### NFR-2: Accuracy
- Memory overhead calculations **SHALL** be accurate within ±2% of actual usage
- Validation via comparison with `arm-none-eabi-nm` and `arm-none-eabi-size`

#### NFR-3: Portability
- Parser **SHALL** work with Python 3.10+
- Parser **SHALL** have zero external dependencies (stdlib only)
- CMake targets **SHALL** work on Linux, macOS, Windows

#### NFR-4: Maintainability
- Parser **SHALL** be unit tested with example map files
- Error messages **SHALL** be clear when parsing fails

## Architecture

### Component Diagram

```
┌─────────────────────────────────────┐
│   User runs: make memory-report     │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│    CMake Custom Target              │
│  1. Invoke size/objdump             │
│  2. Call analyze_memory.py          │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  tools/analyze_memory.py            │
│  - Parse linker map                 │
│  - Extract sections                 │
│  - Identify symbols                 │
│  - Calculate overhead               │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Output: JSON + Text Report         │
│  - Total RAM/Flash usage            │
│  - Per-section breakdown            │
│  - Top memory consumers             │
│  - Budget compliance check          │
└─────────────────────────────────────┘
```

### File Structure

```
tools/
├── analyze_memory.py          # Main parser script
├── memory_report_template.txt # Report template (Jinja2)
└── test/
    ├── test_analyze_memory.py
    └── fixtures/
        ├── arm_linker.map     # Example ARM map
        ├── rl78_linker.map    # Example RL78 map
        └── xtensa_linker.map  # Example ESP32 map

cmake/
└── memory_analysis.cmake      # CMake functions

src/core/
└── memory.hpp                 # Static assertion macros

.github/workflows/
└── memory-check.yml           # CI workflow
```

## Detailed Design

### CMake Implementation

File: `cmake/memory_analysis.cmake`

```cmake
# Find required tools
find_program(OBJDUMP ${CMAKE_OBJDUMP})
find_program(SIZE ${CMAKE_SIZE})
find_program(NM ${CMAKE_NM})

# Function to add memory analysis to a target
function(alloy_add_memory_analysis TARGET_NAME)
    # Get target properties
    get_target_property(TARGET_FILE ${TARGET_NAME} LOCATION)

    # Define memory-report target
    add_custom_target(memory-report-${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E echo "=== Memory Analysis for ${TARGET_NAME} ==="
        COMMAND ${SIZE} --format=berkeley ${TARGET_FILE}
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "=== Detailed Report ==="
        COMMAND ${Python3_EXECUTABLE}
            ${ALLOY_ROOT}/tools/analyze_memory.py
            ${TARGET_FILE}.map
            --mcu ${ALLOY_MCU}
            --ram-size ${ALLOY_RAM_SIZE}
            --flash-size ${ALLOY_FLASH_SIZE}
        DEPENDS ${TARGET_NAME}
        VERBATIM
    )
endfunction()
```

### Python Parser Implementation

File: `tools/analyze_memory.py`

```python
#!/usr/bin/env python3
"""
Alloy Memory Analyzer
Parses linker map files and generates memory usage reports.
"""

import re
import sys
import argparse
from dataclasses import dataclass
from typing import Dict, List
from pathlib import Path

@dataclass
class MemorySection:
    name: str
    address: int
    size: int

@dataclass
class Symbol:
    name: str
    address: int
    size: int
    section: str

class LinkerMapParser:
    """Parser for linker map files (arm-none-eabi-ld, rl78-elf-ld)."""

    def __init__(self, map_file: Path):
        self.map_file = map_file
        self.sections: Dict[str, MemorySection] = {}
        self.symbols: List[Symbol] = []

    def parse(self):
        """Parse the linker map file."""
        with open(self.map_file, 'r') as f:
            content = f.read()

        self._parse_sections(content)
        self._parse_symbols(content)

    def _parse_sections(self, content: str):
        """Extract memory sections (.text, .data, .bss, etc)."""
        # Pattern for GNU ld section output
        # Example: .text          0x00000000     0x1234
        pattern = r'^\s*(\.[\w.]+)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)'

        for match in re.finditer(pattern, content, re.MULTILINE):
            name = match.group(1)
            address = int(match.group(2), 16)
            size = int(match.group(3), 16)

            self.sections[name] = MemorySection(name, address, size)

    def _parse_symbols(self, content: str):
        """Extract symbols and their sizes."""
        # Pattern for symbol table entries
        # Example: 0x00001234   uart_buffer
        pattern = r'^\s*0x([0-9a-fA-F]+)\s+(\w+)'

        for match in re.finditer(pattern, content, re.MULTILINE):
            address = int(match.group(1), 16)
            name = match.group(2)

            # Size determination requires nm output or section analysis
            # Simplified here - full implementation would use nm
            self.symbols.append(Symbol(name, address, 0, "unknown"))

class MemoryReport:
    """Generates human-readable memory usage reports."""

    def __init__(self, parser: LinkerMapParser, mcu: str,
                 ram_size: int, flash_size: int):
        self.parser = parser
        self.mcu = mcu
        self.ram_size = ram_size
        self.flash_size = flash_size

    def generate(self) -> str:
        """Generate full text report."""
        sections = self.parser.sections

        # Calculate totals
        flash_used = (sections.get('.text', MemorySection('', 0, 0)).size +
                      sections.get('.rodata', MemorySection('', 0, 0)).size)

        ram_used = (sections.get('.data', MemorySection('', 0, 0)).size +
                    sections.get('.bss', MemorySection('', 0, 0)).size)

        report = []
        report.append("=" * 45)
        report.append("Alloy Memory Usage Report")
        report.append("=" * 45)
        report.append(f"Target MCU: {self.mcu}")
        report.append("")

        report.append("Flash Usage:")
        report.append("-" * 45)
        report.append(f"Total: {flash_used:,} / {self.flash_size:,} bytes "
                     f"({flash_used/self.flash_size*100:.1f}%)")

        for section_name in ['.text', '.rodata']:
            if section_name in sections:
                size = sections[section_name].size
                report.append(f"  {section_name}: {size:,} bytes")

        report.append("")
        report.append("RAM Usage:")
        report.append("-" * 45)
        report.append(f"Total: {ram_used:,} / {self.ram_size:,} bytes "
                     f"({ram_used/self.ram_size*100:.1f}%)")

        for section_name in ['.data', '.bss']:
            if section_name in sections:
                size = sections[section_name].size
                report.append(f"  {section_name}: {size:,} bytes")

        # Budget check
        report.append("")
        report.append("Memory Budget Check:")
        report.append("-" * 45)

        # Determine MCU class and budget
        if self.ram_size <= 8192:
            budget = 512
            mcu_class = "Tiny"
        elif self.ram_size <= 32768:
            budget = 2048
            mcu_class = "Small"
        elif self.ram_size <= 131072:
            budget = 8192
            mcu_class = "Medium"
        else:
            budget = 16384
            mcu_class = "Large"

        # Estimate Alloy overhead (simplified - would need symbol analysis)
        alloy_overhead = ram_used * 0.15  # Estimate 15% is framework

        status = "✅" if alloy_overhead <= budget else "❌"
        report.append(f"{status} Target: < {budget} bytes overhead ({mcu_class} MCU)")
        report.append(f"{status} Estimated: ~{int(alloy_overhead)} bytes")

        return "\n".join(report)

def main():
    parser = argparse.ArgumentParser(
        description="Analyze memory usage from linker map file"
    )
    parser.add_argument("map_file", type=Path, help="Path to .map file")
    parser.add_argument("--mcu", required=True, help="MCU name")
    parser.add_argument("--ram-size", type=int, required=True,
                       help="Total RAM size in bytes")
    parser.add_argument("--flash-size", type=int, required=True,
                       help="Total Flash size in bytes")
    parser.add_argument("--output", type=Path, help="Output file (default: stdout)")

    args = parser.parse_args()

    # Parse map file
    map_parser = LinkerMapParser(args.map_file)
    map_parser.parse()

    # Generate report
    reporter = MemoryReport(map_parser, args.mcu,
                           args.ram_size, args.flash_size)
    report = reporter.generate()

    # Output
    if args.output:
        args.output.write_text(report)
    else:
        print(report)

if __name__ == "__main__":
    main()
```

### Static Assertion Header

File: `src/core/memory.hpp`

```cpp
#ifndef ALLOY_CORE_MEMORY_HPP
#define ALLOY_CORE_MEMORY_HPP

#include <type_traits>
#include <cstddef>

namespace alloy::core {

// Validate type size doesn't exceed budget
#define ALLOY_ASSERT_MAX_SIZE(Type, MaxBytes) \
    static_assert(sizeof(Type) <= MaxBytes, \
        #Type " exceeds memory budget of " #MaxBytes " bytes")

// Validate type is zero-overhead (trivial)
#define ALLOY_ASSERT_ZERO_OVERHEAD(Type) \
    static_assert(std::is_trivially_copyable_v<Type>, \
        #Type " must be trivially copyable for zero overhead"); \
    static_assert(std::is_standard_layout_v<Type>, \
        #Type " must have standard layout")

// Validate type alignment
#define ALLOY_ASSERT_ALIGNMENT(Type, Alignment) \
    static_assert(alignof(Type) <= Alignment, \
        #Type " alignment exceeds " #Alignment " bytes")

} // namespace alloy::core

#endif // ALLOY_CORE_MEMORY_HPP
```

## Testing

### Unit Tests

File: `tools/test/test_analyze_memory.py`

```python
import pytest
from pathlib import Path
from analyze_memory import LinkerMapParser, MemoryReport

def test_parse_arm_map():
    """Test parsing ARM linker map file."""
    fixture = Path(__file__).parent / "fixtures" / "arm_linker.map"
    parser = LinkerMapParser(fixture)
    parser.parse()

    assert '.text' in parser.sections
    assert '.data' in parser.sections
    assert '.bss' in parser.sections
    assert parser.sections['.text'].size > 0

def test_memory_report_generation():
    """Test report generation."""
    fixture = Path(__file__).parent / "fixtures" / "arm_linker.map"
    parser = LinkerMapParser(fixture)
    parser.parse()

    reporter = MemoryReport(parser, "STM32F103", 20480, 65536)
    report = reporter.generate()

    assert "Alloy Memory Usage Report" in report
    assert "STM32F103" in report
    assert "Flash Usage:" in report
    assert "RAM Usage:" in report
```

### Integration Tests

```bash
# Build example for RL78
cmake -B build-rl78 -DALLOY_BOARD=cf_rl78 -DALLOY_MINIMAL_BUILD=ON
cmake --build build-rl78 --target blinky
cmake --build build-rl78 --target memory-report

# Verify output contains expected sections
grep "RAM Usage:" build-rl78/memory-report.txt
grep "< 2048 bytes" build-rl78/memory-report.txt
```

## Validation Criteria

1. ✅ `memory-report` target builds successfully on host, ARM, RL78
2. ✅ Python parser handles all three toolchain map formats
3. ✅ `ALLOY_MINIMAL_BUILD=ON` reduces example footprint by ≥15%
4. ✅ Static assertions compile and catch violations
5. ✅ CI workflow tracks memory usage per commit
6. ✅ Documentation template used in at least one HAL module

## Dependencies

- CMake 3.25+
- Python 3.10+ (standard library only)
- arm-none-eabi-binutils (size, nm, objdump)
- rl78-elf-binutils
- xtensa-esp32-elf-binutils

## References

- GNU ld manual: https://sourceware.org/binutils/docs/ld/
- ADR-013: Low-Memory Support
- architecture.md: Section 12
