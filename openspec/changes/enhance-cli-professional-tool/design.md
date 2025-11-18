# Enhanced CLI Design Document

**Change ID**: `enhance-cli-professional-tool`
**Status**: Proposal
**Created**: 2025-11-17
**Last Updated**: 2025-11-17

---

## Overview

This document provides the detailed technical design for transforming the Alloy code generation CLI into a comprehensive embedded development assistant. The CLI will provide instant MCU discovery, intelligent project initialization, automated code validation, and unified build/flash workflows.

**Purpose**: Create a professional-grade CLI tool that rivals industry tools like modm while maintaining simplicity and ease of use.

**Stakeholders**:
- **End Users**: Embedded developers using Alloy for projects
- **Contributors**: Developers adding new MCUs, boards, peripherals
- **Maintainers**: Framework developers managing code quality

**Constraints**:
- Must maintain backward compatibility with existing `codegen.py`
- Must support both CMake and Meson build systems
- Must run on Linux, macOS, and Windows
- Must validate generated code to guarantee correctness
- Python 3.11+ required (for modern typing features)

---

## Current State (As-Is)

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    Current CLI Architecture                     │
└─────────────────────────────────────────────────────────────────┘

User
  │
  └─> python3 codegen.py generate
        │
        ├─> SVD Parser (parse XML)
        ├─> Template Engine (render Jinja2)
        └─> File Writer (write to src/)

No validation ❌
No MCU discovery ❌
No project initialization ❌
No build integration ❌
```

### Current Workflow

```bash
# 1. Manual project setup (30+ minutes)
mkdir project && cd project
mkdir -p src include boards cmake
cp -r ~/alloy/boards/nucleo_f401re boards/
vim CMakeLists.txt  # Copy 50+ lines, modify paths...
vim src/main.cpp

# 2. Manual configuration
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake \
      -DALLOY_BOARD=nucleo_f401re \
      -DCMAKE_BUILD_TYPE=Release ..

# 3. Manual code generation
cd ../tools/codegen
python3 codegen.py generate

# 4. Manual build
cd ../../build
make

# 5. Manual flash
openocd -f board/st_nucleo_f4.cfg -c "program firmware.elf verify reset exit"
```

**Pain Points**:
- ❌ 30+ minutes for project setup
- ❌ Requires deep CMake knowledge
- ❌ No MCU/board discovery (must consult external docs)
- ❌ No validation until build fails
- ❌ Error-prone manual configuration
- ❌ No integrated flash/debug

---

## Proposed State (To-Be)

### Architecture Diagram

```
┌──────────────────────────────────────────────────────────────────────┐
│                    Enhanced CLI Architecture                         │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│                         Command Layer (Typer)                        │
├──────────────────────────────────────────────────────────────────────┤
│ Commands:                                                            │
│  • alloy list mcus/boards                                            │
│  • alloy show mcu/board <name>                                       │
│  • alloy search mcu <query>                                          │
│  • alloy init [--template] [--board]                                 │
│  • alloy build compile/flash/size/clean                              │
│  • alloy codegen validate [--all]                                    │
│  • alloy docs datasheet/api <name>                                   │
│  • alloy show pinout <board>                                         │
└──────────────────────────────────────────────────────────────────────┘
          │
          ▼
┌──────────────────────────────────────────────────────────────────────┐
│                       Service Layer (Business Logic)                 │
├──────────────────────────────────────────────────────────────────────┤
│ Services:                                                            │
│  • MCUService          - MCU discovery, search, details              │
│  • BoardService        - Board discovery, pinout rendering           │
│  • ValidationService   - 4-stage validation pipeline                 │
│  • ProjectService      - Project initialization, wizards             │
│  • BuildService        - Unified build/flash abstraction             │
│  • DocumentationService - Datasheet URLs, API docs                   │
└──────────────────────────────────────────────────────────────────────┘
          │
          ▼
┌──────────────────────────────────────────────────────────────────────┐
│                      Database Layer (JSON Storage)                   │
├──────────────────────────────────────────────────────────────────────┤
│ Databases:                                                           │
│  • mcus.json           - MCU specifications                          │
│  • boards.json         - Board configurations                        │
│  • peripherals.json    - Peripheral implementations                  │
│  • templates.json      - Project templates                           │
│  • datasheets.json     - Documentation URLs                          │
└──────────────────────────────────────────────────────────────────────┘
          │
          ▼
┌──────────────────────────────────────────────────────────────────────┐
│                    External Tools Integration                        │
├──────────────────────────────────────────────────────────────────────┤
│ • Clang (syntax validation)                                          │
│ • ARM GCC (compilation testing)                                      │
│ • SVD Parser (semantic validation)                                   │
│ • CMake/Meson (build systems)                                        │
│ • OpenOCD (flash/debug)                                              │
└──────────────────────────────────────────────────────────────────────┘
```

### Proposed Workflow

```bash
# 1. Discover MCU (instant)
$ alloy list mcus --vendor st --min-flash 512K
┌──────────────┬────────────┬───────┬──────┬────────────┐
│ Part Number  │ Core       │ Flash │ RAM  │ Boards     │
├──────────────┼────────────┼───────┼──────┼────────────┤
│ STM32F401RE  │ Cortex-M4F │ 512KB │ 96KB │ nucleo-... │
│ STM32F411RE  │ Cortex-M4F │ 512KB │ 128KB│ nucleo-... │
└──────────────┴────────────┴───────┴──────┴────────────┘

# 2. Interactive project initialization (2 minutes)
$ alloy init
? Select board: nucleo_f401re
? Select template: blinky
? Project name: my-project
✓ Created project structure
✓ Generated CMakeLists.txt
✓ Generated src/main.cpp
✓ Configured board files
✓ Project ready!

# 3. Build with validation (automatic)
$ cd my-project
$ alloy build compile
✓ Validating generated code...
  ✓ Syntax check (clang)
  ✓ Semantic check (SVD)
  ✓ Compilation test
[12/12] Linking firmware.elf
✓ Build successful (14.7 KB)

# 4. Flash to board (one command)
$ alloy build flash
✓ Detected ST-LINK v2.1
✓ Flashing firmware.elf...
✓ Verifying...
✓ Reset target
✓ Done!
```

**Benefits**:
- ✅ 2 minutes for project setup (vs 30+ minutes)
- ✅ Zero CMake knowledge required
- ✅ Built-in MCU/board discovery
- ✅ Automatic code validation
- ✅ Integrated build/flash
- ✅ Professional UX with Rich terminal output

---

## Goals

### Primary Goals

1. **Discovery System**
   - Browse all supported MCUs with specifications
   - Search MCUs by features ("USB + 512KB flash")
   - List supported boards with pinouts
   - Find pins for peripherals
   - Access datasheets directly from CLI

2. **Interactive Project Initialization**
   - Guided wizard for project setup (5 questions, 2 minutes)
   - Board selection with visual specs
   - Peripheral configuration with pin recommendations
   - Conflict detection and auto-resolution
   - Template support (blinky, uart, rtos)

3. **Code Generation Validation**
   - Stage 1: Syntax validation (Clang)
   - Stage 2: Semantic validation (SVD cross-reference)
   - Stage 3: Compilation testing (ARM GCC)
   - Stage 4: Automated test generation (Catch2)

4. **Build System Integration**
   - Unified build commands (`alloy build compile`)
   - Flash integration (`alloy build flash`)
   - Size analysis (`alloy build size`)
   - Clean abstraction over CMake/Meson

5. **Documentation Integration**
   - Open datasheets in browser
   - API documentation access
   - Example code browser
   - Interactive pinout explorer

### Non-Goals

- ❌ Web UI (future feature)
- ❌ VS Code extension (separate OpenSpec)
- ❌ Graphical pinout editor (future)
- ❌ Real-time debugging integration (separate)
- ❌ Code intelligence/autocomplete (IDE feature)

---

## Architectural Decisions

### Decision 1: Keep CMake as Primary Build System

**Decision**: Support CMake as default build system, add Meson as optional experimental feature.

**Rationale**:
- CMake is industry standard for embedded development
- All major vendors provide CMake support (ST, Nordic, NXP)
- Large ecosystem of examples and documentation
- Better IDE integration (CLion, VS Code)
- Existing Alloy investment in CMake

**Implementation**:
```python
# tools/codegen/cli/services/build_service.py
class BuildService:
    @staticmethod
    def detect_build_system(project_dir: Path) -> str:
        """Auto-detect build system"""
        if (project_dir / "CMakeLists.txt").exists():
            return "cmake"
        elif (project_dir / "meson.build").exists():
            return "meson"
        else:
            raise ValueError("No build system detected")

    @staticmethod
    def configure(project_dir: Path, build_system: str = "auto"):
        """Configure build"""
        if build_system == "auto":
            build_system = BuildService.detect_build_system(project_dir)

        if build_system == "cmake":
            return CMakeBuilder.configure(project_dir)
        elif build_system == "meson":
            return MesonBuilder.configure(project_dir)
```

**Alternatives Considered**:
- **Switch to Meson entirely**: Rejected because of ecosystem lock-in
- **Support both equally**: Rejected due to maintenance burden
- **Custom build system**: Rejected due to complexity

**Trade-offs**:
- ✅ Pro: Familiar to most embedded developers
- ✅ Pro: Better vendor support
- ❌ Con: CMake syntax is complex
- ❌ Con: Slower than Meson

---

### Decision 2: Database-Driven Architecture

**Decision**: Use JSON files for MCU/board/peripheral metadata instead of hardcoding in Python/CMake.

**Rationale**:
- Separation of data and logic
- Easy to update without code changes
- Can generate from SVD files automatically
- Enables community contributions (just add JSON)
- Fast loading with Python's json module
- Human-readable and git-friendly

**Implementation**:

**Database Schema** (`tools/codegen/database/mcus/stm32f4.json`):
```json
{
  "schema_version": "1.0",
  "family": {
    "id": "stm32f4",
    "vendor": "st",
    "display_name": "STM32F4 Series",
    "core": "Cortex-M4F",
    "features": ["FPU", "DSP Instructions"]
  },
  "mcus": [
    {
      "part_number": "STM32F401RET6",
      "display_name": "STM32F401RE",
      "core": "Cortex-M4F",
      "max_freq_mhz": 84,
      "memory": {
        "flash_kb": 512,
        "sram_kb": 96
      },
      "peripherals": {
        "uart": {"count": 3, "instances": ["USART1", "USART2", "USART6"]},
        "spi": {"count": 4, "max_speed_mbps": 42},
        "i2c": {"count": 3, "max_speed_khz": 1000}
      },
      "documentation": {
        "datasheet": "https://st.com/resource/en/datasheet/stm32f401re.pdf",
        "svd_file": "tools/codegen/svd/upstream/STMicro/STM32F401.svd"
      },
      "boards": ["nucleo_f401re"],
      "status": "production",
      "tags": ["beginner-friendly", "arduino-compatible"]
    }
  ]
}
```

**Service Layer** (`tools/codegen/cli/services/mcu_service.py`):
```python
from pathlib import Path
import json
from typing import List, Optional
from dataclasses import dataclass

@dataclass
class MCU:
    """MCU specification"""
    part_number: str
    display_name: str
    core: str
    max_freq_mhz: int
    flash_kb: int
    sram_kb: int
    peripherals: dict
    datasheet_url: str
    svd_file: Path
    boards: List[str]

class MCUService:
    """MCU discovery and information service"""

    _database: Optional[List[MCU]] = None

    @classmethod
    def load_database(cls) -> List[MCU]:
        """Load MCU database from JSON files"""
        if cls._database is not None:
            return cls._database

        database_dir = Path(__file__).parent.parent.parent / "database" / "mcus"
        mcus = []

        # Load all JSON files
        for json_file in database_dir.glob("*.json"):
            with open(json_file) as f:
                data = json.load(f)

            # Parse MCUs from family
            for mcu_data in data["mcus"]:
                mcus.append(MCU(
                    part_number=mcu_data["part_number"],
                    display_name=mcu_data["display_name"],
                    core=mcu_data["core"],
                    max_freq_mhz=mcu_data["max_freq_mhz"],
                    flash_kb=mcu_data["memory"]["flash_kb"],
                    sram_kb=mcu_data["memory"]["sram_kb"],
                    peripherals=mcu_data["peripherals"],
                    datasheet_url=mcu_data["documentation"]["datasheet"],
                    svd_file=Path(mcu_data["documentation"]["svd_file"]),
                    boards=mcu_data["boards"]
                ))

        cls._database = mcus
        return mcus

    @classmethod
    def list(cls, vendor: Optional[str] = None,
             min_flash: Optional[int] = None,
             with_peripheral: Optional[str] = None) -> List[MCU]:
        """List MCUs with filtering"""
        mcus = cls.load_database()

        # Apply filters
        if vendor:
            mcus = [m for m in mcus if vendor.lower() in m.part_number.lower()]

        if min_flash:
            mcus = [m for m in mcus if m.flash_kb >= min_flash]

        if with_peripheral:
            mcus = [m for m in mcus
                   if with_peripheral.lower() in m.peripherals]

        return mcus

    @classmethod
    def show(cls, part_number: str) -> MCU:
        """Get detailed MCU information"""
        mcus = cls.load_database()

        for mcu in mcus:
            if mcu.part_number.upper() == part_number.upper():
                return mcu

        raise ValueError(f"MCU {part_number} not found in database")

    @classmethod
    def search(cls, query: str) -> List[MCU]:
        """Search MCUs by features"""
        mcus = cls.load_database()

        # Parse query (e.g., "USB + 512KB + Cortex-M4")
        terms = [t.strip().lower() for t in query.split("+")]

        results = []
        for mcu in mcus:
            # Convert MCU to searchable text
            searchable = f"{mcu.part_number} {mcu.core} {mcu.flash_kb}KB " \
                        f"{' '.join(mcu.peripherals.keys())}"
            searchable = searchable.lower()

            # Check if all terms match
            if all(term in searchable for term in terms):
                results.append(mcu)

        return results
```

**Alternatives Considered**:
- **SQLite database**: Rejected due to overhead for simple queries
- **Hardcoded Python dicts**: Rejected due to maintenance burden
- **YAML files**: Rejected (JSON is faster to parse)

**Trade-offs**:
- ✅ Pro: Easy to update
- ✅ Pro: Community can contribute
- ✅ Pro: Can auto-generate from SVD
- ❌ Con: Requires database maintenance
- ❌ Con: Can become stale

---

### Decision 3: 4-Stage Validation Pipeline

**Decision**: Implement comprehensive validation with 4 independent stages.

**Rationale**:
- Syntax errors caught early (before compilation)
- Semantic errors detected (wrong register addresses)
- Compilation errors found in isolation
- Automated tests verify correctness

**Implementation**:

**Stage 1: Syntax Validation**
```python
# tools/codegen/cli/validators/syntax_validator.py
import subprocess
from pathlib import Path
from dataclasses import dataclass

@dataclass
class ValidationResult:
    passed: bool
    stage: str
    message: str
    errors: List[str] = None

class SyntaxValidator:
    """Validate C++ syntax using Clang"""

    @staticmethod
    def validate(file_path: Path) -> ValidationResult:
        """Parse file with Clang and check for syntax errors"""

        cmd = [
            "clang++",
            "-std=c++23",
            "-fsyntax-only",
            "-Xclang", "-ast-dump",
            "-I", "include/",
            "-I", "src/",
            str(file_path)
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode == 0:
            return ValidationResult(
                passed=True,
                stage="syntax",
                message="✅ Syntax valid"
            )
        else:
            # Parse errors from stderr
            errors = []
            for line in result.stderr.split('\n'):
                if 'error:' in line:
                    errors.append(line.strip())

            return ValidationResult(
                passed=False,
                stage="syntax",
                message=f"❌ {len(errors)} syntax error(s)",
                errors=errors
            )
```

**Stage 2: Semantic Validation**
```python
# tools/codegen/cli/validators/semantic_validator.py
from xml.etree import ElementTree as ET

class SemanticValidator:
    """Validate generated code against SVD specification"""

    @staticmethod
    def validate(file_path: Path, svd_file: Path) -> ValidationResult:
        """Cross-reference generated code with SVD"""

        # Parse generated code (extract base addresses, offsets)
        generated = parse_generated_code(file_path)

        # Parse SVD file
        tree = ET.parse(svd_file)
        root = tree.getroot()

        errors = []

        # Check peripheral base addresses
        for peripheral in generated.peripherals:
            # Find in SVD
            svd_peripheral = root.find(
                f".//peripheral[name='{peripheral.name}']"
            )

            if svd_peripheral is None:
                errors.append(f"Peripheral {peripheral.name} not in SVD")
                continue

            svd_base = int(svd_peripheral.find("baseAddress").text, 16)

            if peripheral.base_address != svd_base:
                errors.append(
                    f"{peripheral.name}: Base address mismatch "
                    f"(generated: {hex(peripheral.base_address)}, "
                    f"SVD: {hex(svd_base)})"
                )

        # Check register offsets
        for register in generated.registers:
            svd_register = root.find(
                f".//peripheral[name='{register.peripheral}']"
                f"//register[name='{register.name}']"
            )

            if svd_register is not None:
                svd_offset = int(svd_register.find("addressOffset").text, 16)

                if register.offset != svd_offset:
                    errors.append(
                        f"{register.name}: Offset mismatch "
                        f"(generated: {hex(register.offset)}, "
                        f"SVD: {hex(svd_offset)})"
                    )

        if errors:
            return ValidationResult(
                passed=False,
                stage="semantic",
                message=f"❌ {len(errors)} semantic error(s)",
                errors=errors
            )
        else:
            return ValidationResult(
                passed=True,
                stage="semantic",
                message="✅ Semantic validation passed"
            )
```

**Stage 3: Compilation Test**
```python
# tools/codegen/cli/validators/compile_validator.py
import tempfile

class CompileValidator:
    """Validate generated code compiles"""

    @staticmethod
    def validate(file_path: Path, board: str) -> ValidationResult:
        """Attempt to compile generated code"""

        with tempfile.TemporaryDirectory() as tmpdir:
            test_file = Path(tmpdir) / "test_compile.cpp"

            # Minimal test program
            test_code = f"""
            #include "{file_path.absolute()}"

            // Instantiate templates to force compilation
            using TestGpio = GpioPin<0x40020000, 0>;

            int main() {{
                TestGpio gpio;
                gpio.set();
                return 0;
            }}
            """

            test_file.write_text(test_code)

            # Compile test
            cmd = [
                "arm-none-eabi-gcc",
                "-std=c++23",
                "-mcpu=cortex-m4",
                "-mthumb",
                "-c",
                "-I", "include/",
                "-I", "src/",
                str(test_file),
                "-o", f"{tmpdir}/test.o"
            ]

            result = subprocess.run(cmd, capture_output=True, text=True)

            if result.returncode == 0:
                obj_size = Path(f"{tmpdir}/test.o").stat().st_size

                return ValidationResult(
                    passed=True,
                    stage="compilation",
                    message=f"✅ Compiles successfully ({obj_size} bytes)"
                )
            else:
                errors = [line.strip() for line in result.stderr.split('\n')
                         if 'error:' in line]

                return ValidationResult(
                    passed=False,
                    stage="compilation",
                    message=f"❌ Compilation failed: {len(errors)} error(s)",
                    errors=errors
                )
```

**Stage 4: Test Generation**
```python
# tools/codegen/cli/validators/test_generator.py
from jinja2 import Template

class TestGenerator:
    """Generate Catch2 tests for generated code"""

    @staticmethod
    def generate_tests(file_path: Path, output_path: Path):
        """Generate unit tests for peripheral drivers"""

        # Parse generated code
        code = parse_generated_code(file_path)

        # Test template
        test_template = Template("""
        #include <catch2/catch_test_macros.hpp>
        #include "{{ header_file }}"

        TEST_CASE("{{ peripheral_name }}: Base address", "[{{ peripheral_name }}]") {
            constexpr uint32_t expected = {{ base_address }};
            constexpr uint32_t actual = {{ peripheral_name }}::base_address;
            STATIC_REQUIRE(expected == actual);
        }

        {% for register in registers %}
        TEST_CASE("{{ peripheral_name }}: {{ register.name }} offset", "[{{ peripheral_name }}]") {
            constexpr uint32_t expected = {{ register.offset }};
            constexpr uint32_t actual = {{ peripheral_name }}::{{ register.name }}_OFFSET;
            STATIC_REQUIRE(expected == actual);
        }
        {% endfor %}
        """)

        # Render test file
        test_code = test_template.render(
            header_file=file_path.name,
            peripheral_name=code.peripheral_name,
            base_address=hex(code.base_address),
            registers=code.registers
        )

        output_path.write_text(test_code)
```

**Orchestration**:
```python
# tools/codegen/cli/services/validation_service.py
class ValidationService:
    """Code validation pipeline orchestration"""

    @staticmethod
    def validate_all(file_path: Path) -> bool:
        """Run all validation stages"""

        console = Console()

        with console.status("[bold green]Validating...") as status:
            # Stage 1: Syntax
            status.update("[bold green]Stage 1/4: Syntax validation...")
            syntax_result = SyntaxValidator.validate(file_path)

            if not syntax_result.passed:
                console.print(f"[red]{syntax_result.message}")
                for error in syntax_result.errors:
                    console.print(f"  [red]• {error}")
                return False

            console.print(f"[green]{syntax_result.message}")

            # Stage 2: Semantic
            status.update("[bold green]Stage 2/4: Semantic validation...")
            svd_file = get_svd_file_for(file_path)
            semantic_result = SemanticValidator.validate(file_path, svd_file)

            if not semantic_result.passed:
                console.print(f"[red]{semantic_result.message}")
                for error in semantic_result.errors:
                    console.print(f"  [red]• {error}")
                return False

            console.print(f"[green]{semantic_result.message}")

            # Stage 3: Compilation
            status.update("[bold green]Stage 3/4: Compilation test...")
            compile_result = CompileValidator.validate(file_path, board="auto")

            if not compile_result.passed:
                console.print(f"[red]{compile_result.message}")
                for error in compile_result.errors:
                    console.print(f"  [red]• {error}")
                return False

            console.print(f"[green]{compile_result.message}")

            # Stage 4: Tests
            status.update("[bold green]Stage 4/4: Generating tests...")
            test_file = TestGenerator.generate_tests(
                file_path,
                Path("tests/generated") / f"test_{file_path.stem}.cpp"
            )
            console.print(f"[green]✅ Generated test file: {test_file}")

        return True
```

**Alternatives Considered**:
- **No validation**: Rejected (fails quality goal)
- **Build-time only**: Rejected (errors found too late)
- **Manual validation**: Rejected (not scalable)

**Trade-offs**:
- ✅ Pro: Catches errors early
- ✅ Pro: Guarantees correctness
- ✅ Pro: Builds confidence
- ❌ Con: Slower generation (30s validation per file)
- ❌ Con: Requires clang and arm-gcc installed

---

### Decision 4: Typer + Rich for CLI Framework

**Decision**: Use Typer for CLI structure and Rich for terminal output.

**Rationale**:
- Typer provides modern Python CLI with type hints
- Automatic help generation
- Subcommand support
- Rich provides beautiful terminal output (tables, progress bars, colors)
- Both libraries are actively maintained
- Better UX than argparse + print()

**Implementation**:

```python
# tools/codegen/cli/main.py
import typer
from rich.console import Console
from rich.table import Table

app = typer.Typer(
    name="alloy",
    help="Alloy Embedded Framework CLI",
    add_completion=True
)

console = Console()

# Subcommands
@app.command()
def list(
    resource: str = typer.Argument(..., help="Resource type (mcus, boards)"),
    vendor: str = typer.Option(None, "--vendor", "-v", help="Filter by vendor"),
    min_flash: str = typer.Option(None, "--min-flash", help="Minimum flash (e.g., 512K)")
):
    """List available MCUs or boards"""

    if resource == "mcus":
        # Get MCUs
        flash_kb = parse_memory(min_flash) if min_flash else None
        mcus = MCUService.list(vendor=vendor, min_flash=flash_kb)

        # Create table
        table = Table(title="Available MCUs")
        table.add_column("Part Number", style="cyan")
        table.add_column("Core", style="magenta")
        table.add_column("Flash", style="green")
        table.add_column("RAM", style="green")
        table.add_column("Boards", style="yellow")

        for mcu in mcus:
            table.add_row(
                mcu.part_number,
                mcu.core,
                f"{mcu.flash_kb}KB",
                f"{mcu.sram_kb}KB",
                ", ".join(mcu.boards[:2])  # Show first 2 boards
            )

        console.print(table)
        console.print(f"\n[green]Found {len(mcus)} MCUs")

    elif resource == "boards":
        # Similar implementation for boards
        pass

@app.command()
def show(
    resource: str = typer.Argument(..., help="Resource type (mcu, board)"),
    name: str = typer.Argument(..., help="Resource name")
):
    """Show detailed information about MCU or board"""

    if resource == "mcu":
        mcu = MCUService.show(name)

        # Create rich panel
        from rich.panel import Panel
        from rich.text import Text

        content = Text()
        content.append(f"Part Number: ", style="bold")
        content.append(f"{mcu.part_number}\n")
        content.append(f"Core: ", style="bold")
        content.append(f"{mcu.core}\n")
        content.append(f"Max Frequency: ", style="bold")
        content.append(f"{mcu.max_freq_mhz} MHz\n")
        content.append(f"Flash: ", style="bold")
        content.append(f"{mcu.flash_kb} KB\n")
        content.append(f"SRAM: ", style="bold")
        content.append(f"{mcu.sram_kb} KB\n\n")

        content.append(f"Peripherals:\n", style="bold underline")
        for peripheral, info in mcu.peripherals.items():
            content.append(f"  • {peripheral.upper()}: {info['count']} instances\n")

        content.append(f"\nDatasheet: ", style="bold")
        content.append(f"{mcu.datasheet_url}\n", style="link")

        panel = Panel(content, title=f"[bold cyan]{mcu.display_name}", border_style="cyan")
        console.print(panel)

@app.command()
def init(
    board: str = typer.Option(None, "--board", "-b", help="Board name"),
    template: str = typer.Option(None, "--template", "-t", help="Project template"),
    build_system: str = typer.Option("cmake", "--build-system", help="Build system")
):
    """Initialize new project"""

    if not board:
        # Run interactive wizard
        board = run_board_wizard()

    if not template:
        template = run_template_wizard()

    # Create project
    with console.status("[bold green]Creating project..."):
        ProjectService.create(
            board=board,
            template=template,
            build_system=build_system,
            output_dir=Path.cwd()
        )

    console.print("[green]✓[/green] Project created successfully!")
    console.print("\nNext steps:")
    console.print("  1. cd <project-name>")
    console.print("  2. alloy build compile")
    console.print("  3. alloy build flash")

if __name__ == "__main__":
    app()
```

**Alternatives Considered**:
- **Click**: Rejected (Typer is built on Click with better type hints)
- **argparse**: Rejected (too verbose, poor UX)
- **Custom CLI**: Rejected (reinventing the wheel)

**Trade-offs**:
- ✅ Pro: Beautiful terminal output
- ✅ Pro: Type-safe commands
- ✅ Pro: Auto-generated help
- ❌ Con: Additional dependencies
- ❌ Con: Learning curve for contributors

---

## Component-by-Component Design

### Component 1: MCU Service

**File**: `tools/codegen/cli/services/mcu_service.py`

**Purpose**: MCU discovery, search, and detailed information

**Key Features**:
- Load MCU database from JSON
- Filter by vendor, family, flash, RAM, peripherals
- Search by features with AND logic
- Enrich with SVD data on demand
- Cache for performance

**API**:
```python
class MCUService:
    @classmethod
    def list(cls, vendor: Optional[str] = None,
             min_flash: Optional[int] = None,
             with_peripheral: Optional[str] = None) -> List[MCU]:
        """List MCUs with filtering"""

    @classmethod
    def show(cls, part_number: str) -> MCU:
        """Get detailed MCU information"""

    @classmethod
    def search(cls, query: str) -> List[MCU]:
        """Search MCUs by features (e.g., 'USB + 512KB')"""
```

**Implementation Notes**:
- Lazy loading: Database loaded on first access
- Caching: Parsed MCUs cached in class variable
- JSON schema validation on load (optional, for development)

---

### Component 2: Validation Service

**File**: `tools/codegen/cli/services/validation_service.py`

**Purpose**: Orchestrate 4-stage validation pipeline

**Key Features**:
- Run all stages sequentially
- Fail fast on first error
- Rich progress reporting
- Generate detailed error reports
- Save validation results to JSON

**API**:
```python
class ValidationService:
    @staticmethod
    def validate_all(file_path: Path) -> bool:
        """Run all validation stages"""

    @staticmethod
    def validate_stage(file_path: Path, stage: str) -> ValidationResult:
        """Run specific validation stage"""
```

**Implementation Notes**:
- Each stage is independent
- Can run stages in parallel (future optimization)
- Results cached to avoid re-validation

---

### Component 3: Project Service

**File**: `tools/codegen/cli/services/project_service.py`

**Purpose**: Project initialization and configuration

**Key Features**:
- Interactive wizard with InquirerPy
- Template-based project generation
- Smart pin recommendation
- Conflict detection
- CMake/Meson support

**API**:
```python
class ProjectService:
    @staticmethod
    def create(board: str, template: str,
               build_system: str, output_dir: Path):
        """Create new project from template"""

    @staticmethod
    def add_peripheral(peripheral: str):
        """Add peripheral to existing project"""
```

**Implementation Notes**:
- Uses Jinja2 for CMakeLists.txt generation
- Validates board/template combinations
- Creates directory structure atomically

---

### Component 4: Build Service

**File**: `tools/codegen/cli/services/build_service.py`

**Purpose**: Unified build/flash abstraction

**Key Features**:
- Auto-detect build system
- CMake and Meson support
- Progress reporting
- Size analysis
- Flash integration (OpenOCD, ST-Link)

**API**:
```python
class BuildService:
    @staticmethod
    def compile(project_dir: Path, build_type: str = "Release"):
        """Compile project"""

    @staticmethod
    def flash(project_dir: Path, programmer: str = "auto"):
        """Flash firmware to board"""

    @staticmethod
    def size(project_dir: Path):
        """Analyze binary size"""
```

**Implementation Notes**:
- Wraps CMake/Meson commands
- Parses build output for progress
- Detects programmer automatically

---

## Data Flow

### Flow 1: Project Initialization

```
User runs: alloy init

         │
         ▼
┌────────────────────┐
│ CLI (init command) │
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ Run wizard         │
│ • Select board     │
│ • Select template  │
│ • Configure pins   │
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ ProjectService     │
│ • Load template    │
│ • Render files     │
│ • Create structure │
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ File System        │
│ • project/         │
│   ├─ src/          │
│   ├─ CMakeLists.txt│
│   └─ boards/       │
└────────────────────┘
         │
         ▼
    ✓ Success!
```

### Flow 2: Code Validation

```
User runs: alloy codegen validate file.hpp

         │
         ▼
┌──────────────────────┐
│ ValidationService    │
└──────────────────────┘
         │
         ├───────────────────────────┐
         │                           │
         ▼                           ▼
┌──────────────────┐      ┌──────────────────┐
│ Stage 1: Syntax  │      │ Stage 2: Semantic│
│ (Clang)          │      │ (SVD Check)      │
└──────────────────┘      └──────────────────┘
         │                           │
         │                           │
         ▼                           ▼
       Pass?                       Pass?
         │                           │
         │                           │
         ├───────────────────────────┤
         │
         ▼
┌──────────────────────┐
│ Stage 3: Compilation │
│ (ARM GCC)            │
└──────────────────────┘
         │
         ▼
       Pass?
         │
         ▼
┌──────────────────────┐
│ Stage 4: Test Gen    │
│ (Catch2)             │
└──────────────────────┘
         │
         ▼
    ✓ All Passed!
```

---

## Testing Strategy

### Unit Tests (pytest)

**Coverage Target**: 80%

```python
# tools/codegen/tests/services/test_mcu_service.py
def test_list_mcus_filters_by_vendor():
    """Test MCU listing with vendor filter"""
    mcus = MCUService.list(vendor="st")

    assert len(mcus) > 0
    assert all("STM32" in m.part_number for m in mcus)

def test_search_mcus_with_and_logic():
    """Test MCU search with multiple terms"""
    mcus = MCUService.search("USB + 512KB + Cortex-M4")

    assert len(mcus) > 0
    for mcu in mcus:
        assert mcu.flash_kb >= 512
        assert "Cortex-M4" in mcu.core
        assert "usb" in mcu.peripherals
```

### Integration Tests

```python
# tools/codegen/tests/integration/test_init_workflow.py
def test_init_creates_working_project(tmp_path):
    """Test project initialization creates compilable project"""

    project_dir = tmp_path / "test-project"

    # Initialize
    ProjectService.create(
        board="nucleo_f401re",
        template="blinky",
        build_system="cmake",
        output_dir=project_dir
    )

    # Verify structure
    assert (project_dir / "CMakeLists.txt").exists()
    assert (project_dir / "src" / "main.cpp").exists()

    # Compile
    BuildService.compile(project_dir)

    # Verify binary
    assert (project_dir / "build" / "firmware.elf").exists()
```

### Validation Tests

```python
# tools/codegen/tests/validation/test_generated_code.py
def test_generated_gpio_passes_validation():
    """Test generated GPIO code passes all checks"""

    # Generate code
    codegen.generate_gpio("stm32f4")

    # Validate
    result = ValidationService.validate_all(
        Path("src/hal/vendors/st/stm32f4/gpio.hpp")
    )

    assert result.syntax.passed
    assert result.semantics.passed
    assert result.compilation.passed
```

---

## Performance Considerations

### Database Loading

**Optimization**: Lazy loading + caching

```python
class MCUService:
    _database: Optional[List[MCU]] = None  # Class-level cache

    @classmethod
    def load_database(cls) -> List[MCU]:
        if cls._database is not None:
            return cls._database  # Return cached

        # Load from JSON (only once)
        cls._database = parse_json_files()
        return cls._database
```

**Expected Performance**:
- First call: ~100ms (parse 20+ JSON files)
- Subsequent calls: <1ms (return cached)

### Validation Pipeline

**Current**: Sequential execution (30s per file)

**Future Optimization**: Parallel execution

```python
import concurrent.futures

def validate_all_parallel(file_path: Path):
    """Run validation stages in parallel where possible"""

    with concurrent.futures.ThreadPoolExecutor() as executor:
        # Stage 1 and 2 can run in parallel (independent)
        future_syntax = executor.submit(SyntaxValidator.validate, file_path)
        future_semantic = executor.submit(SemanticValidator.validate, file_path, svd)

        # Wait for both
        syntax_result = future_syntax.result()
        semantic_result = future_semantic.result()

        if not (syntax_result.passed and semantic_result.passed):
            return False

        # Stage 3 (depends on syntax check passing)
        compile_result = CompileValidator.validate(file_path)

        # ... continue
```

**Expected Improvement**: 30s → 15s (50% faster)

---

## Security Considerations

### Risk 1: Command Injection

**Vulnerable**:
```python
# DON'T DO THIS
os.system(f"clang++ {user_input}")
```

**Secure**:
```python
# DO THIS
subprocess.run(["clang++", user_input], check=True)
```

### Risk 2: Path Traversal

**Vulnerable**:
```python
# DON'T DO THIS
file_path = Path(user_input)
file_path.write_text(content)
```

**Secure**:
```python
# DO THIS
file_path = Path(user_input).resolve()
if not file_path.is_relative_to(project_dir):
    raise ValueError("Path outside project directory")
file_path.write_text(content)
```

---

## Migration/Rollback Plan

### Phase 1: Foundation (Week 1-2)
- Create database structure
- Implement MCUService and BoardService
- Add `alloy list` commands
- **Rollback**: Delete new files, no changes to existing code

### Phase 2: Validation (Week 3-4)
- Implement 4-stage validation
- Add `alloy codegen validate` command
- **Rollback**: Remove validation commands, keep database

### Phase 3: Initialization (Week 5-6)
- Implement project wizard
- Add `alloy init` command
- **Rollback**: Remove init command, keep validation

### Phase 4: Build Integration (Week 7)
- Implement build service
- Add `alloy build` commands
- **Rollback**: Remove build commands

### Phase 5: Documentation (Week 8)
- Add documentation commands
- Add pinout rendering
- **Rollback**: Remove docs commands

---

## Risk Analysis

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Database becomes stale | MEDIUM | MEDIUM | Automated SVD→JSON generation |
| Validation too slow | HIGH | LOW | Parallel execution, caching |
| False positives | MEDIUM | MEDIUM | Extensive testing, user feedback |
| Build system complexity | HIGH | MEDIUM | Clear abstraction layer |

---

## Success Metrics

- [ ] Project setup time: 30 min → 2 min (93% reduction)
- [ ] Generated code errors: 0 (100% validation pass rate)
- [ ] User satisfaction: >80% positive feedback
- [ ] Adding new MCU: <2 hours (vs 8+ hours)
- [ ] CLI test coverage: >80%
- [ ] Discovery commands respond in <1 second

---

## Open Questions

**Q1**: Should we support Windows natively or via WSL only?
**A**: Support Windows natively (use pathlib for cross-platform paths)

**Q2**: How to handle version conflicts between Clang on system?
**A**: Document required Clang version, error if incompatible

**Q3**: Should validation be mandatory or optional?
**A**: Optional by default, mandatory in CI/CD

---

## Future Enhancements (Out of Scope)

- Web UI for MCU/board browsing
- VS Code extension integration
- Graphical pinout editor
- Real-time debugging integration
- Code intelligence server (LSP)

---

## Conclusion

This design provides a comprehensive blueprint for transforming the Alloy CLI into a professional development tool. The modular architecture enables incremental implementation while maintaining backward compatibility. The 4-stage validation pipeline guarantees code correctness, addressing the user's key requirement.

**Next Steps**:
1. Review and approve this design
2. Begin Phase 1 implementation (Foundation & Discovery)
3. Validate with user testing after each phase
4. Iterate based on feedback
