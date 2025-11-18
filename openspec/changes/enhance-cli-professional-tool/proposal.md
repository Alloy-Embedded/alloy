# Proposal: Enhanced CLI - Professional Development Tool

**Change ID**: `enhance-cli-professional-tool`
**Status**: PROPOSED
**Priority**: HIGH
**Complexity**: HIGH
**Estimated Duration**: 11.5 weeks (~230 hours)
**Last Updated**: 2025-01-17 (consolidated with usability improvements + integrated with Library Quality)
**Coordination**: See `openspec/changes/INTEGRATION_LIBRARY_CLI.md`

---

## 🔗 Integration Notice

**This spec is coordinated with `library-quality-improvements`**.

**Ownership Boundaries**:
- **This Spec Owns**: YAML schemas, metadata CLI commands, ValidationService wrapper, project templates, configuration system
- **Library Quality Owns**: Core validators, peripheral templates, CRTP refactoring, template engine core
- **Shared**: Metadata database structure, template engine infrastructure

**Critical Dependency**: Library Quality Phase 4 (Codegen Reorganization) MUST complete before Phase 0 (YAML Migration) begins.

**Timeline Coordination**: 12 weeks parallel execution vs 21.5 sequential (40% faster). See integration document for detailed coordination plan.

---

## Executive Summary

Transform Alloy's code generation CLI into a **comprehensive embedded development assistant** that rivals industry tools like modm while maintaining simplicity. The CLI will provide instant access to MCU information, intelligent peripheral discovery, automated project initialization, and guaranteed code correctness through integrated validation.

**Key Objectives**:
1. 🔍 **Discovery**: Browse MCUs, boards, peripherals with zero documentation lookup
2. 🚀 **Initialization**: Interactive wizard creates projects in 2 minutes (vs 30+ minutes manually)
3. ✅ **Validation**: Automated checks ensure generated code is correct and compiles
4. 🔨 **Integration**: Unified build/flash/debug workflow
5. 📚 **Documentation**: Instant access to datasheets, pinouts, and examples

**Success Metrics** (Updated):
- **Time to create project**: 30 min → 2 min (93% reduction)
- **Generated code errors**: Manual → 0 (automated validation)
- **Metadata file size**: 8,500 lines → 6,400 lines (25% reduction via YAML)
- **Metadata syntax errors**: 30% → <5% (improved format + validation)
- **Iteration speed**: 5s → 0.5s (10x faster with incremental generation)
- **Developer Experience Score**: 5.0/10 → 8.6/10 (72% improvement)
- **Learning curve**: Steep → None (guided wizard)

---

## 🆕 Consolidated Improvements (2025-01-17)

This proposal has been **enhanced** with critical usability improvements identified through comprehensive CLI analysis:

### Added Features (Not in Original Proposal)

1. **🔥 YAML Metadata Format** (Phase 0 - Week 1)
   - Replace JSON with YAML for all database files
   - 25-30% size reduction (604 lines → ~450 for GPIO)
   - Inline comments for hardware quirks documentation
   - Cleaner code snippets (multiline strings, no escaping)
   - Better git diffs and merge conflict resolution

2. **👁️ Preview/Diff Capability** (Phase 2 - Enhanced)
   - `alloy codegen generate --dry-run --diff`
   - See exact changes before applying
   - Colorized unified diff output
   - Prevents destructive changes, builds confidence

3. **⚡ Incremental Generation** (Phase 2 - Enhanced)
   - Generate only files with changed metadata
   - 10x faster iteration (5s → 0.5s)
   - Checksum-based change detection
   - Dependency cascade tracking

4. **⚙️ Configuration File System** (Phase 1 - Enhanced)
   - `.alloy.yaml` for project/user settings
   - Hierarchy: CLI args > env vars > project > user > defaults
   - Customizable paths, defaults, formatting preferences
   - Environment variable support (`ALLOY_*`)

5. **🔍 Enhanced Metadata Management** (Phase 1 - Enhanced)
   - `alloy metadata validate [--strict]`
   - `alloy metadata create --template <type>`
   - `alloy metadata diff <name>`
   - Better error messages with suggestions

### Why These Additions Matter

**JSON Pain Points Identified**:
- ❌ 604-line GPIO metadata file (verbose)
- ❌ No comments → hardware quirks undocumented
- ❌ Code snippets require `\n` escaping
- ❌ Trailing comma errors common
- ❌ 30% of manual edits cause syntax errors

**YAML Solution**:
- ✅ 25-30% smaller files
- ✅ Inline comments preserve knowledge
- ✅ Clean multiline code (no escaping)
- ✅ No trailing comma issues
- ✅ <5% syntax error rate

**See `CONSOLIDATED_IMPROVEMENTS.md` for detailed analysis and integration plan.**

---

## Motivation

### Current Pain Points

**1. Project Setup is Manual and Error-Prone**
```bash
# Current process (30+ minutes):
mkdir project && cd project
mkdir src include boards cmake
cp -r ~/alloy/boards/nucleo_f401re boards/
vim CMakeLists.txt  # Copy 50+ lines from example, modify paths...
vim src/main.cpp
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake \
      -DALLOY_BOARD=nucleo_f401re \
      -DCMAKE_BUILD_TYPE=Release ..
cd ../tools/codegen && python3 codegen.py generate
cd ../../build && make
```

**Issues**:
- ❌ Requires deep knowledge of CMake
- ❌ Easy to make typos in paths/flags
- ❌ No validation until build fails
- ❌ Time-consuming and frustrating

**2. MCU/Board Discovery Requires External Documentation**
```bash
# Current process (5-10 minutes per lookup):
1. Google "STM32F401RE datasheet"
2. Download 200-page PDF
3. Search for "UART" pins
4. Cross-reference with board schematic
5. Check for conflicts manually
```

**Issues**:
- ❌ Slow context switching (code → browser → PDF → code)
- ❌ Error-prone (miss conflicts, wrong pins)
- ❌ Requires multiple documents open

**3. Generated Code Has No Automated Validation**
```bash
# Current process:
python3 codegen.py generate
# Hope it's correct... find out during compilation
make
# ERROR: undefined reference to 'GPIO_PORT_A'
# Manually debug generated code...
```

**Issues**:
- ❌ No pre-compilation checks
- ❌ Syntax errors found late in cycle
- ❌ No semantic validation (correct but wrong)

**4. Metadata Format is Verbose and Error-Prone** (🆕 Added 2025-01-17)
```bash
# Current metadata format (JSON):
# - 604 lines for same70_gpio.json
# - No comments allowed (can't document quirks)
# - Escape sequences required for code (\n, \", etc.)
# - 30% of manual edits cause syntax errors
# - Trailing commas forbidden (common mistake)

# Example pain point:
{
  "svd_quirks": {
    "array_dimension_fixes": {
      "PIO_ABCDSR": 2  // Why 2? Can't document!
    }
  },
  "implementation": "uint32_t x = port->ODSR;\nif (x) {\n    ...\n}"  // Hard to read
}
```

**Issues**:
- ❌ No inline documentation for hardware quirks
- ❌ Code snippets unreadable (escaped strings)
- ❌ High syntax error rate (trailing commas, quotes)
- ❌ Difficult git merges
- ❌ Large file sizes (verbose format)

### Why This Matters

**For Beginners**:
- Steep learning curve blocks adoption
- Too many manual steps create frustration
- Errors discourage continued use

**For Experts**:
- Repetitive setup wastes time
- Context switching breaks flow
- Manual validation is tedious

**For Teams**:
- Inconsistent project structures
- Knowledge silos (only one person knows setup)
- Hard to onboard new members

---

## Goals

### Primary Goals

**G1: Discovery System** (MUST HAVE)
- [ ] Browse all supported MCUs with specs (flash, RAM, peripherals)
- [ ] Search MCUs by features ("USB + 512KB flash")
- [ ] List supported boards with pinouts
- [ ] Find pins for peripherals ("UART TX on nucleo_f401re")
- [ ] Visual pinout display (ASCII art)
- [ ] Access datasheets directly from CLI

**G2: Interactive Project Initialization** (MUST HAVE)
- [ ] Guided wizard for project setup (5 questions, 2 minutes)
- [ ] Board selection with visual specs
- [ ] Peripheral configuration with pin recommendations
- [ ] Conflict detection and auto-resolution
- [ ] **Project template support** (blinky, uart, rtos, etc.) - owned by this spec
- [ ] CMake + Meson support

**Note**: This spec owns **project templates** (blinky, uart_logger, rtos). Library Quality spec owns **peripheral templates** (GPIO, UART, SPI). Both use shared template engine.

**G3: Code Generation Validation** (MUST HAVE)
- [ ] **ValidationService wrapper** around core validators (owned by this spec)
- [ ] CLI commands for validation (`alloy metadata validate`)
- [ ] Integration with code generation workflow
- [ ] CI/CD integration for validation
- [ ] Rich CLI output with suggestions

**Note**: Core validators (syntax, semantic, compile, test) are **owned by library-quality-improvements spec**. This spec wraps them in ValidationService for CLI usage.

**G4: Build System Integration** (MUST HAVE)
- [ ] Unified build commands (`alloy build compile`)
- [ ] Flash integration (`alloy build flash`)
- [ ] Size analysis (`alloy build size`)
- [ ] Clean abstraction over CMake/Meson

**G5: Documentation Integration** (SHOULD HAVE)
- [ ] Open datasheets in browser
- [ ] API documentation access
- [ ] Example code browser
- [ ] Interactive pinout explorer

**G6: YAML Metadata Migration** (🆕 MUST HAVE - Added 2025-01-17)
- [ ] **Define YAML schemas** for all metadata types (owned by this spec)
- [ ] Migration tools (JSON → YAML converter)
- [ ] Support inline comments for hardware quirks
- [ ] Reduce metadata size by 25-30%
- [ ] Improve readability with multiline code snippets
- [ ] Maintain backward compatibility during transition

**Note**: This spec owns **YAML schemas** (mcu.schema.yaml, board.schema.yaml, peripheral.schema.yaml). Library Quality spec **consumes** these schemas for peripheral template generation.

**Dependency**: Requires Library Quality Phase 4 (Codegen Reorganization) to complete first.

**G7: Preview and Incremental Generation** (🆕 MUST HAVE - Added 2025-01-17)
- [ ] Preview changes before applying (`--dry-run --diff`)
- [ ] Show exact unified diffs with colorization
- [ ] Incremental generation (only changed metadata)
- [ ] 10x faster iteration cycles (5s → 0.5s)
- [ ] Checksum-based change detection

**G8: Configuration and User Customization** (🆕 SHOULD HAVE - Added 2025-01-17)
- [ ] `.alloy.yaml` configuration file support
- [ ] Configuration hierarchy (CLI > env > project > user > defaults)
- [ ] Environment variable overrides (`ALLOY_*`)
- [ ] Per-project and per-user settings
- [ ] Customizable paths, formatting, defaults

### Non-Goals (Out of Scope)

**Not in this OpenSpec**:
- ❌ Web UI (future feature)
- ❌ VS Code extension (separate OpenSpec)
- ❌ Graphical pinout editor (future)
- ❌ Real-time debugging integration (separate)
- ❌ Code intelligence/autocomplete (IDE feature)

---

## Technical Design

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        Alloy CLI Architecture                           │
└─────────────────────────────────────────────────────────────────────────┘

┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   Commands   │────▶│   Services   │────▶│   Database   │
└──────────────┘     └──────────────┘     └──────────────┘
      │                    │                     │
      │                    │                     │
┌─────▼─────┐      ┌──────▼──────┐      ┌──────▼──────┐
│ list mcus │      │ MCU Service │      │ mcus.json   │
│ show mcu  │      │ Board Svc   │      │ boards.json │
│ search    │      │ Codegen Svc │      │ periph.json │
│ init      │      │ Build Svc   │      │ svd/        │
│ build     │      │ Validate Svc│      └─────────────┘
└───────────┘      └─────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                          Validation Pipeline                            │
└─────────────────────────────────────────────────────────────────────────┘

Generate Code ──▶ Syntax Check ──▶ Semantic Check ──▶ Compile Test ──▶ ✅
     │                 │                 │                 │
     │                 │                 │                 │
  Templates        Clang AST          SVD Cross        Catch2 Tests
  + Metadata       Parser             Reference        Generated
```

### Component Breakdown

#### 1. Command Layer (User Interface)

**Technology**: Typer (modern Python CLI framework)

**Commands**:
```python
# tools/codegen/cli/commands/list.py
@app.command()
def list_mcus(
    vendor: Optional[str] = None,
    family: Optional[str] = None,
    min_flash: Optional[str] = None,
    with_peripheral: Optional[str] = None
):
    """List available MCUs with filtering"""
    mcus = MCUService.list(
        vendor=vendor,
        family=family,
        min_flash=parse_memory(min_flash),
        with_peripheral=with_peripheral
    )

    # Rich table output
    table = create_mcu_table(mcus)
    console.print(table)

# tools/codegen/cli/commands/init.py
@app.command()
def init(
    board: Optional[str] = None,
    template: Optional[str] = None,
    build_system: Literal["cmake", "meson"] = "cmake"
):
    """Initialize new project"""
    if not board:
        # Interactive wizard
        board = run_board_wizard()

    if not template:
        template = run_template_wizard()

    # Generate project
    ProjectService.create(
        board=board,
        template=template,
        build_system=build_system
    )
```

#### 2. Service Layer (Business Logic)

**Location**: `tools/codegen/cli/services/`

**MCU Service** (`mcu_service.py`):
```python
class MCUService:
    """MCU discovery and information"""

    @staticmethod
    def list(vendor=None, family=None, **filters) -> List[MCU]:
        """List MCUs with filtering"""
        db = Database.load("mcus")
        mcus = db.filter(vendor=vendor, family=family)

        # Apply additional filters
        if filters.get("min_flash"):
            mcus = [m for m in mcus if m.flash >= filters["min_flash"]]

        return mcus

    @staticmethod
    def show(part_number: str) -> MCU:
        """Get detailed MCU information"""
        db = Database.load("mcus")
        mcu = db.get(part_number)

        # Enrich with SVD data
        svd = SVDParser.parse(mcu.svd_file)
        mcu.peripherals = svd.peripherals

        return mcu
```

**Board Service** (`board_service.py`):
```python
class BoardService:
    """Board discovery and configuration"""

    @staticmethod
    def list() -> List[Board]:
        """List supported boards"""
        return Database.load("boards").all()

    @staticmethod
    def show_pinout(board_name: str) -> Pinout:
        """Get visual pinout"""
        board = Database.load("boards").get(board_name)
        return PinoutRenderer.render(board)
```

**Validation Service** (`validation_service.py`):
```python
class ValidationService:
    """Code validation pipeline"""

    @staticmethod
    def validate_generated_code(file_path: Path) -> ValidationResult:
        """Multi-stage validation"""
        result = ValidationResult()

        # Stage 1: Syntax check (clang)
        syntax_ok = validate_syntax(file_path)
        result.add_check("syntax", syntax_ok)

        # Stage 2: Semantic check (SVD cross-reference)
        semantic_ok = validate_semantics(file_path)
        result.add_check("semantics", semantic_ok)

        # Stage 3: Compilation test
        compile_ok = validate_compilation(file_path)
        result.add_check("compilation", compile_ok)

        # Stage 4: Unit tests (if exist)
        if has_tests(file_path):
            tests_ok = run_unit_tests(file_path)
            result.add_check("tests", tests_ok)

        return result
```

#### 3. Database Layer (Data Storage)

**Structure** (🆕 Updated to YAML):
```
tools/codegen/database/
├── mcus/
│   ├── stm32f4.yaml         # MCU definitions (YAML format)
│   ├── same70.yaml
│   └── index.yaml           # Fast lookup
├── boards/
│   ├── nucleo_f401re.yaml   # Board configurations
│   ├── same70_xplained.yaml
│   └── index.yaml
├── peripherals/
│   ├── uart.yaml            # Peripheral implementations
│   ├── spi.yaml
│   └── index.yaml
├── templates/
│   ├── blinky.yaml          # Project templates
│   ├── rtos.yaml
│   └── index.yaml
└── datasheets/
    └── urls.yaml            # Documentation URLs
```

**MCU Database Schema** (`mcus/stm32f4.yaml`) - 🆕 YAML Format:
```yaml
schema_version: 1.0

family:
  id: stm32f4
  vendor: st
  display_name: STM32F4 Series

  # High-performance ARM Cortex-M4 MCUs with FPU and DSP instructions
  # Target: General-purpose applications requiring floating-point performance
  description: |
    The STM32F4 series offers a balance of performance and power efficiency,
    featuring a Cortex-M4F core with FPU and DSP instructions. Perfect for
    motor control, digital audio, and sensor fusion applications.

  core: Cortex-M4F
  features:
    - FPU              # Hardware floating-point unit
    - DSP Instructions # Digital signal processing extensions

mcus:
  - part_number: STM32F401RET6
    display_name: STM32F401RE
    core: Cortex-M4F
    max_freq_mhz: 84

    memory:
      flash_kb: 512
      sram_kb: 96
      eeprom_kb: 0

    package:
      type: LQFP64
      pins: 64
      io_pins: 50

    peripherals:
      uart:
        count: 3
        instances: [USART1, USART2, USART6]
        # Note: USART1/USART6 support higher baud rates (5.25 Mbps max)
        # USART2 limited to 2.625 Mbps

      spi:
        count: 4
        max_speed_mbps: 42  # Up to 42 MHz SPI clock

      i2c:
        count: 3
        max_speed_khz: 1000  # Fast-mode Plus

      adc:
        count: 1
        resolution_bits: 12
        channels: 16

      timers:
        count: 11
        types: [2x 32-bit, 9x 16-bit]

      usb:
        count: 1
        type: OTG Full-Speed

    documentation:
      datasheet: https://st.com/resource/en/datasheet/stm32f401re.pdf
      reference_manual: https://st.com/resource/en/reference_manual/dm00096844.pdf
      errata: https://st.com/resource/en/errata_sheet/dm00105230.pdf
      svd_file: tools/codegen/svd/upstream/STMicro/STM32F401.svd

    boards: [nucleo_f401re]
    status: production
    tags: [beginner-friendly, arduino-compatible]
```

**Benefits of YAML** (vs original JSON):
- ✅ **25% smaller** (67 lines vs 90+ lines JSON)
- ✅ **Inline comments** document why 3 UARTs, speed limits, etc.
- ✅ **Multiline description** (clean, no `\n` escaping)
- ✅ **No trailing commas** to worry about
- ✅ **Better readability** for humans

**Board Database Schema** (`boards/nucleo_f401re.yaml`) - 🆕 YAML Format:
```yaml
schema_version: 1.0

board:
  id: nucleo_f401re
  display_name: Nucleo-F401RE
  vendor: st
  url: https://www.st.com/en/evaluation-tools/nucleo-f401re.html

mcu:
  part_number: STM32F401RET6
  family: stm32f4

clock:
  system_freq_hz: 84000000
  xtal_freq_hz: 8000000  # External 8 MHz crystal
  has_pll: true

pinout:
  leds:
    - name: LD2
      color: green
      gpio: PA5
      active: high
      description: User LED

  buttons:
    - name: B1
      gpio: PC13
      active: low
      description: User button

  debugger:
    type: ST-LINK/V2-1
    uart:
      tx: PA2
      rx: PA3
      instance: USART2
      description: Virtual COM Port (VCP)

peripherals:
  uart:
    - instance: USART1
      pins:
        - {tx: PA9, rx: PA10, af: 7, recommended: true}
        - {tx: PB6, rx: PB7, af: 7}

    - instance: USART2
      pins:
        - tx: PA2
          rx: PA3
          af: 7
          # Connected to ST-LINK VCP for debugging output
          note: ST-LINK Virtual COM Port

  spi:
    - instance: SPI1
      pins:
        - sck: PA5
          miso: PA6
          mosi: PA7
          af: 5
          # SCK on PA5 is shared with LED LD2 - will cause LED flicker
          note: SCK shared with LED (LD2)

        - sck: PB3  # Recommended - no conflicts
          miso: PA6
          mosi: PA7
          af: 5
          recommended: true
          note: Avoids LED conflict

  i2c:
    - instance: I2C1
      pins:
        - {scl: PB8, sda: PB9, af: 4, recommended: true}

examples: [blink, uart_echo, systick_demo]
status: supported
tags: [beginner-friendly, arduino-compatible, nucleo]
```

**Again, YAML wins**:
- ✅ **Inline comments** explain quirks (LED/SCK conflict, VCP connection)
- ✅ **Cleaner structure** - easier to scan
- ✅ **~30% smaller** than JSON equivalent

### Validation Pipeline Design

#### Stage 1: Syntax Validation

**Tool**: Clang AST parser

**Check**: Generated C++ code is syntactically correct

**Implementation**:
```python
# tools/codegen/cli/validators/syntax_validator.py
import subprocess
from pathlib import Path

class SyntaxValidator:
    """Validate C++ syntax using Clang"""

    @staticmethod
    def validate(file_path: Path) -> ValidationResult:
        """Parse file with Clang and check for syntax errors"""

        # Run clang with AST dump
        cmd = [
            "clang++",
            "-std=c++23",
            "-fsyntax-only",  # Don't generate code
            "-Xclang", "-ast-dump",
            "-I", "include/",
            str(file_path)
        ]

        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True
        )

        if result.returncode == 0:
            return ValidationResult(
                passed=True,
                stage="syntax",
                message="✅ Syntax valid"
            )
        else:
            # Parse errors from stderr
            errors = parse_clang_errors(result.stderr)
            return ValidationResult(
                passed=False,
                stage="syntax",
                message=f"❌ {len(errors)} syntax error(s)",
                errors=errors
            )
```

**Example Output**:
```bash
$ alloy codegen validate src/hal/vendors/st/stm32f4/gpio.hpp

┌──────────────────────────────────────────────────────────────────┐
│                    Syntax Validation                             │
└──────────────────────────────────────────────────────────────────┘

✅ Parsing with Clang++... OK
✅ No syntax errors found
✅ AST structure valid

File: src/hal/vendors/st/stm32f4/gpio.hpp
Lines: 247
Classes: 3 (GpioPin, GpioPort, GpioConfig)
Templates: 5
```

#### Stage 2: Semantic Validation

**Tool**: Custom validator + SVD cross-reference

**Check**: Generated code matches hardware specification

**Implementation**:
```python
# tools/codegen/cli/validators/semantic_validator.py
from xml.etree import ElementTree as ET

class SemanticValidator:
    """Validate generated code against SVD specification"""

    @staticmethod
    def validate(file_path: Path, svd_file: Path) -> ValidationResult:
        """Cross-reference generated code with SVD"""

        # Parse generated code
        generated = parse_generated_code(file_path)

        # Parse SVD file
        svd = SVDParser.parse(svd_file)

        errors = []

        # Check peripheral base addresses
        for peripheral in generated.peripherals:
            svd_peripheral = svd.get_peripheral(peripheral.name)

            if not svd_peripheral:
                errors.append(f"Peripheral {peripheral.name} not in SVD")
                continue

            if peripheral.base_address != svd_peripheral.base_address:
                errors.append(
                    f"{peripheral.name}: Base address mismatch "
                    f"(generated: {hex(peripheral.base_address)}, "
                    f"SVD: {hex(svd_peripheral.base_address)})"
                )

        # Check register offsets
        for register in generated.registers:
            svd_register = svd.get_register(
                register.peripheral,
                register.name
            )

            if svd_register and register.offset != svd_register.offset:
                errors.append(
                    f"{register.name}: Offset mismatch "
                    f"(generated: {hex(register.offset)}, "
                    f"SVD: {hex(svd_register.offset)})"
                )

        # Check bit fields
        for bitfield in generated.bitfields:
            svd_field = svd.get_bitfield(
                bitfield.peripheral,
                bitfield.register,
                bitfield.name
            )

            if svd_field:
                if bitfield.bit_offset != svd_field.bit_offset:
                    errors.append(f"{bitfield.name}: Bit offset mismatch")

                if bitfield.bit_width != svd_field.bit_width:
                    errors.append(f"{bitfield.name}: Bit width mismatch")

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

**Example Output**:
```bash
$ alloy codegen validate src/hal/vendors/st/stm32f4/peripherals.hpp

┌──────────────────────────────────────────────────────────────────┐
│                  Semantic Validation                             │
└──────────────────────────────────────────────────────────────────┘

✅ Cross-referencing with SVD... OK
✅ Checked 15 peripherals: All base addresses match
✅ Checked 247 registers: All offsets correct
✅ Checked 1,203 bitfields: All positions valid

SVD File: tools/codegen/svd/upstream/STMicro/STM32F401.svd
Generated: src/hal/vendors/st/stm32f4/peripherals.hpp

No semantic errors found!
```

#### Stage 3: Compilation Test

**Tool**: ARM GCC compiler

**Check**: Generated code compiles for target architecture

**Implementation**:
```python
# tools/codegen/cli/validators/compile_validator.py
import tempfile
import subprocess
from pathlib import Path

class CompileValidator:
    """Validate generated code compiles"""

    @staticmethod
    def validate(
        file_path: Path,
        board: str,
        toolchain: str = "arm-none-eabi-gcc"
    ) -> ValidationResult:
        """Attempt to compile generated code"""

        # Create temporary test file
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
                f"{toolchain}",
                "-std=c++23",
                "-mcpu=cortex-m4",
                "-mthumb",
                "-c",  # Compile only, don't link
                "-I", "include/",
                "-I", "src/",
                str(test_file),
                "-o", f"{tmpdir}/test.o"
            ]

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True
            )

            if result.returncode == 0:
                # Get object file size
                obj_size = Path(f"{tmpdir}/test.o").stat().st_size

                return ValidationResult(
                    passed=True,
                    stage="compilation",
                    message=f"✅ Compiles successfully ({obj_size} bytes)",
                    metadata={"object_size": obj_size}
                )
            else:
                errors = parse_gcc_errors(result.stderr)
                return ValidationResult(
                    passed=False,
                    stage="compilation",
                    message=f"❌ Compilation failed: {len(errors)} error(s)",
                    errors=errors
                )
```

**Example Output**:
```bash
$ alloy codegen validate src/hal/vendors/st/stm32f4/gpio.hpp --compile

┌──────────────────────────────────────────────────────────────────┐
│                  Compilation Test                                │
└──────────────────────────────────────────────────────────────────┘

✅ Creating test program... OK
✅ Compiling with arm-none-eabi-gcc... OK
✅ Object size: 1,247 bytes

Compiler: arm-none-eabi-gcc 13.2.0
Target: Cortex-M4F
Flags: -std=c++23 -mcpu=cortex-m4 -mthumb -Os

Code compiles successfully!
```

#### Stage 4: Automated Test Generation

**Tool**: Catch2 + Python codegen

**Check**: Generated code passes unit tests

**Implementation**:
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

        // Mock hardware registers
        struct Mock{{ peripheral_name }}Registers {
            uint32_t registers[1024] = {0};
        };

        TEST_CASE("{{ peripheral_name }}: Base address is correct", "[{{ peripheral_name }}]") {
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

        {% for bitfield in bitfields %}
        TEST_CASE("{{ peripheral_name }}: {{ bitfield.name }} bitfield", "[{{ peripheral_name }}]") {
            constexpr uint32_t pos = {{ bitfield.bit_offset }};
            constexpr uint32_t width = {{ bitfield.bit_width }};
            constexpr uint32_t mask = ((1U << width) - 1) << pos;

            REQUIRE({{ peripheral_name }}::{{ bitfield.name }}_POS == pos);
            REQUIRE({{ peripheral_name }}::{{ bitfield.name }}_MASK == mask);
        }
        {% endfor %}
        """)

        # Render test file
        test_code = test_template.render(
            header_file=file_path.name,
            peripheral_name=code.peripheral_name,
            base_address=hex(code.base_address),
            registers=code.registers,
            bitfields=code.bitfields
        )

        output_path.write_text(test_code)

        return output_path
```

**Example Generated Test**:
```cpp
// tests/generated/test_gpio_stm32f4.cpp (AUTO-GENERATED)
#include <catch2/catch_test_macros.hpp>
#include "hal/vendors/st/stm32f4/gpio.hpp"

TEST_CASE("GPIO: Base address is correct", "[gpio]") {
    constexpr uint32_t expected = 0x40020000;
    constexpr uint32_t actual = GPIOA::base_address;
    STATIC_REQUIRE(expected == actual);
}

TEST_CASE("GPIO: MODER offset", "[gpio]") {
    constexpr uint32_t expected = 0x00;
    constexpr uint32_t actual = GPIOA::MODER_OFFSET;
    STATIC_REQUIRE(expected == actual);
}

TEST_CASE("GPIO: ODR offset", "[gpio]") {
    constexpr uint32_t expected = 0x14;
    constexpr uint32_t actual = GPIOA::ODR_OFFSET;
    STATIC_REQUIRE(expected == actual);
}

TEST_CASE("GPIO: MODER0 bitfield", "[gpio]") {
    constexpr uint32_t pos = 0;
    constexpr uint32_t width = 2;
    constexpr uint32_t mask = ((1U << width) - 1) << pos;

    REQUIRE(GPIOA::MODER0_POS == pos);
    REQUIRE(GPIOA::MODER0_MASK == mask);
}
```

**Running Generated Tests**:
```bash
$ alloy test generated

┌──────────────────────────────────────────────────────────────────┐
│                  Generated Code Tests                            │
└──────────────────────────────────────────────────────────────────┘

Running tests from tests/generated/...

[==========] Running 1,247 tests from 15 test suites

[----------] GPIO Tests (87 tests)
[ RUN      ] GPIO: Base address is correct
[       OK ] GPIO: Base address is correct (0 ms)
[ RUN      ] GPIO: MODER offset
[       OK ] GPIO: MODER offset (0 ms)
...
[  PASSED  ] 87 tests

[----------] UART Tests (142 tests)
[  PASSED  ] 142 tests

...

[==========] 1,247 tests run (2.3 seconds)
[  PASSED  ] 1,247 tests ✅

All generated code tests passed!
```

---

## Implementation Phases

**Updated Timeline**: 11.5 weeks (original 8 weeks + 3.5 weeks for usability improvements)

### Phase 0: YAML Migration 🆕 (1 week, 20 hours)

**Goal**: Migrate all metadata from JSON to YAML format

**Tasks**:
1. **YAML Infrastructure** (6h)
   - Add `PyYAML>=6.0` to dependencies
   - Create `YAMLDatabaseLoader` class
   - Implement auto-detection (`.yaml` vs `.json`)
   - Support both formats in parallel
   - Write YAML schema validators

2. **JSON→YAML Migration Script** (4h)
   - Create conversion script (`migrate_json_to_yaml.py`)
   - Preserve structure and semantics
   - Add comment placeholders for quirks
   - Format multiline code snippets
   - Validate converted output

3. **Database Migration** (8h)
   - Convert `mcus/*.json` → `.yaml` (add hardware quirk comments)
   - Convert `boards/*.json` → `.yaml` (document pin conflicts)
   - Convert `peripherals/*.json` → `.yaml`
   - Convert `templates/*.json` → `.yaml`
   - Validate all converted files
   - Verify generated code identical

4. **Documentation Update** (2h)
   - Update `METADATA.md` with YAML format
   - Create `YAML_METADATA_GUIDE.md`
   - Add migration guide

**Deliverables**:
- ✅ YAML loader with auto-detection
- ✅ All 45 metadata files in YAML format
- ✅ Comments documenting hardware quirks
- ✅ Migration script for future use
- ✅ Updated documentation

**Validation**:
```bash
# YAML files load correctly
$ alloy list mcus  # Should work with YAML metadata

# Generated code identical
$ diff old-json-output/ new-yaml-output/
# No differences

# Metadata is 25-30% smaller
$ du -sh database/mcus/*.yaml vs *.json
# 6.4KB vs 8.5KB (25% reduction)
```

**Benefits Achieved**:
- ✅ 25-30% size reduction
- ✅ Inline comments preserve tribal knowledge
- ✅ Clean multiline code snippets
- ✅ <5% syntax error rate (vs 30% with JSON)

---

### Phase 1: Foundation & Discovery (3 weeks, 52 hours) 🆕 +12h

**Goal**: Build discovery system and database

**Tasks**:
1. **Database Schema Design** (4h)
   - Define JSON schemas for MCUs, boards, peripherals
   - Create validation schemas (JSON Schema)
   - Set up index files for fast lookup

2. **Database Population** (8h)
   - Extract MCU data from existing SVD files
   - Parse board.hpp files to JSON
   - Create peripheral metadata
   - Add datasheet URLs

3. **MCU Service** (8h)
   - Implement `MCUService.list()` with filtering
   - Implement `MCUService.show()` with SVD enrichment
   - Add search functionality

4. **Board Service** (6h)
   - Implement `BoardService.list()`
   - Implement `BoardService.show()`
   - Parse pinout data from board configs

5. **CLI Commands** (10h)
   - `alloy list mcus` with Rich tables
   - `alloy list boards`
   - `alloy show mcu <name>`
   - `alloy show board <name>`
   - `alloy search mcu <query>`

6. **Configuration System** 🆕 (6h)
   - Create `.alloy.yaml` schema
   - Implement `ConfigLoader` class
   - Support project and user configs
   - Environment variable overrides
   - Configuration hierarchy (CLI > env > project > user > defaults)

7. **Enhanced Metadata Commands** 🆕 (6h)
   - `alloy metadata validate [--strict]`
   - `alloy metadata create --template <type>`
   - `alloy metadata diff <name>`
   - Better error messages with suggestions

8. **Testing** (4h)
   - Unit tests for services
   - Integration tests for commands
   - Database validation tests
   - Config system tests

**Deliverables**:
- ✅ MCU/board database (YAML files) 🆕
- ✅ Discovery services (Python)
- ✅ CLI commands (5 discovery + 3 metadata) 🆕
- ✅ Configuration system (`.alloy.yaml`) 🆕
- ✅ Tests (pytest, 80% coverage)

**Validation**:
```bash
# User can discover MCUs without documentation
$ alloy list mcus --vendor st --min-flash 512K
# Shows table with 15 STM32 MCUs

$ alloy show mcu STM32F401RE
# Shows detailed specs + datasheet link

$ alloy search mcu "USB + Cortex-M4"
# Finds 23 matching MCUs
```

---

### Phase 2: Validation Pipeline (2 weeks, 40 hours)

**Goal**: Automated validation of generated code

**Tasks**:
1. **Syntax Validator** (8h)
   - Implement Clang integration
   - Parse AST for syntax errors
   - Report errors with line numbers

2. **Semantic Validator** (12h)
   - SVD parser integration
   - Cross-reference base addresses
   - Cross-reference register offsets
   - Cross-reference bitfield positions

3. **Compile Validator** (8h)
   - Create test program generator
   - Integrate ARM GCC
   - Parse compilation errors
   - Report object code size

4. **Test Generator** (8h)
   - Jinja2 template for Catch2 tests
   - Generate tests for peripherals
   - Generate tests for registers
   - Generate tests for bitfields

5. **Validation CLI** (4h)
   - `alloy codegen validate <file>`
   - `alloy codegen validate --all`
   - Progress reporting

**Deliverables**:
- ✅ 4-stage validation pipeline
- ✅ Automated test generation
- ✅ CLI validation command
- ✅ CI/CD integration

**Validation**:
```bash
# Validate single file
$ alloy codegen validate src/hal/vendors/st/stm32f4/gpio.hpp
✅ Syntax: OK
✅ Semantics: OK (247 registers checked)
✅ Compilation: OK (1,247 bytes)
✅ Tests: OK (87/87 passed)

# Validate all generated code
$ alloy codegen validate --all
Checking 156 generated files...
✅ 156/156 files passed all checks
```

---

### Phase 3: Interactive Initialization (2 weeks, 48 hours)

**Goal**: Wizard-based project creation

**Tasks**:
1. **Project Templates** (8h)
   - Create blinky template
   - Create uart_logger template
   - Create rtos_tasks template
   - Create template metadata

2. **Wizard Framework** (8h)
   - Implement InquirerPy integration
   - Board selection with specs
   - Peripheral selection (checkboxes)
   - Pin configuration wizard

3. **Smart Pin Recommendation** (12h)
   - Analyze available pins for peripheral
   - Detect conflicts (LED, ST-LINK, etc.)
   - Recommend optimal configuration
   - Show alternatives

4. **Project Generator** (12h)
   - Generate directory structure
   - Generate CMakeLists.txt from template
   - Generate src/main.cpp
   - Generate board configuration
   - Generate peripheral drivers

5. **CLI Commands** (4h)
   - `alloy init` wizard
   - `alloy init --template <name>`
   - `alloy config peripheral add`

6. **Testing** (4h)
   - Integration tests for wizard
   - Template validation tests

**Deliverables**:
- ✅ Interactive wizard
- ✅ 3 project templates
- ✅ Smart pin recommendation
- ✅ Project generator
- ✅ Tests

**Validation**:
```bash
# Interactive wizard
$ alloy init
# Wizard asks 5 questions, creates project in 2 minutes

# From template
$ alloy init --template rtos --board nucleo_f401re
Created project in 10 seconds!

# Add peripheral to existing project
$ cd my-project
$ alloy config peripheral add
# Wizard configures UART with pin recommendations
```

---

### Phase 4: Build Integration (1 week, 24 hours)

**Goal**: Unified build/flash/debug commands

**Tasks**:
1. **Build Service** (8h)
   - Abstract CMake/Meson
   - Detect build system from project
   - Progress tracking

2. **Flash Integration** (8h)
   - OpenOCD integration
   - ST-Link support
   - J-Link support (if available)
   - Progress reporting

3. **CLI Commands** (6h)
   - `alloy build configure`
   - `alloy build compile`
   - `alloy build flash`
   - `alloy build size`
   - `alloy build clean`

4. **Testing** (2h)
   - Mock build tests
   - Integration tests

**Deliverables**:
- ✅ Build abstraction layer
- ✅ 5 build commands
- ✅ Flash integration
- ✅ Tests

**Validation**:
```bash
$ alloy build configure
✅ CMake configured

$ alloy build compile
[12/12] Linking firmware.elf
✅ Build successful (14.7 KB)

$ alloy build flash
✅ Flashed to nucleo_f401re
```

---

### Phase 5: Documentation & Pinouts (1 week, 24 hours)

**Goal**: Integrated documentation access

**Tasks**:
1. **Pinout Renderer** (12h)
   - ASCII art pinout generator
   - Interactive terminal UI
   - Pin highlighting
   - Function search

2. **Documentation Service** (6h)
   - Datasheet URL database
   - Browser integration
   - API doc generation

3. **CLI Commands** (4h)
   - `alloy show pinout <board>`
   - `alloy docs datasheet <mcu>`
   - `alloy docs api <peripheral>`

4. **Testing** (2h)
   - Pinout rendering tests
   - URL validation tests

**Deliverables**:
- ✅ ASCII art pinout display
- ✅ Documentation integration
- ✅ 3 documentation commands
- ✅ Tests

**Validation**:
```bash
$ alloy show pinout nucleo_f401re
# Displays ASCII art pinout with all pins

$ alloy docs datasheet STM32F401RE
# Opens PDF in browser

$ alloy docs api uart
# Opens API documentation
```

---

### Phase 6: Polish & Optimization (Optional, 16 hours)

**Goal**: UX improvements and performance

**Tasks**:
1. **Performance Optimization** (6h)
   - Database indexing
   - Lazy loading
   - Caching

2. **Error Messages** (4h)
   - Helpful error messages
   - Troubleshooting hints
   - Recovery suggestions

3. **Documentation** (4h)
   - CLI usage guide
   - Video tutorials
   - Migration guide

4. **Final Testing** (2h)
   - End-to-end tests
   - User acceptance testing

**Deliverables**:
- ✅ Performance optimizations
- ✅ Better error messages
- ✅ Complete documentation

---

## Success Criteria

### Functional Requirements

**FR1: Discovery** (MUST HAVE)
- [ ] User can list all MCUs with filtering (vendor, family, flash, RAM)
- [ ] User can search MCUs by features ("USB + 512KB")
- [ ] User can view MCU details (specs, peripherals, datasheet)
- [ ] User can list boards with pinouts
- [ ] User can find pins for peripherals ("UART TX")

**FR2: Initialization** (MUST HAVE)
- [ ] User can create project in <2 minutes via wizard
- [ ] Wizard recommends pins and detects conflicts
- [ ] Templates work for common patterns (blinky, uart, rtos)
- [ ] Generated project compiles without modification

**FR3: Validation** (MUST HAVE)
- [ ] Generated code passes syntax check (Clang)
- [ ] Generated code passes semantic check (SVD cross-ref)
- [ ] Generated code compiles for target (ARM GCC)
- [ ] Generated code passes unit tests (Catch2)
- [ ] Validation runs in <30 seconds per file

**FR4: Build Integration** (MUST HAVE)
- [ ] `alloy build compile` works for CMake projects
- [ ] `alloy build flash` works with ST-Link
- [ ] `alloy build size` shows memory usage
- [ ] Build commands have progress tracking

**FR5: Documentation** (SHOULD HAVE)
- [ ] Visual pinout display in terminal
- [ ] Datasheet opens in browser
- [ ] API docs accessible from CLI

### Non-Functional Requirements

**NFR1: Performance**
- Discovery commands respond in <1 second
- Validation completes in <30 seconds per file
- Build commands match native CMake performance

**NFR2: Reliability**
- All CLI commands have error handling
- Validation has 100% success rate for correct code
- No false positives in validation

**NFR3: Usability**
- Wizard requires <5 questions
- Error messages are actionable
- Help text is comprehensive

**NFR4: Maintainability**
- Database schema is versioned
- CLI has 80% test coverage
- Code follows Python best practices

---

## Testing Strategy

### Unit Tests (pytest)

**Coverage Target**: 80%

**Test Files**:
```
tools/codegen/tests/
├── services/
│   ├── test_mcu_service.py
│   ├── test_board_service.py
│   ├── test_validation_service.py
│   └── test_project_service.py
├── validators/
│   ├── test_syntax_validator.py
│   ├── test_semantic_validator.py
│   ├── test_compile_validator.py
│   └── test_test_generator.py
├── commands/
│   ├── test_list_commands.py
│   ├── test_show_commands.py
│   ├── test_init_command.py
│   └── test_build_commands.py
└── database/
    ├── test_schema_validation.py
    └── test_data_integrity.py
```

**Example Test**:
```python
# tools/codegen/tests/services/test_mcu_service.py
import pytest
from cli.services.mcu_service import MCUService

def test_list_mcus_filters_by_vendor():
    """Test MCU listing with vendor filter"""
    mcus = MCUService.list(vendor="st")

    assert len(mcus) > 0
    assert all(m.vendor == "st" for m in mcus)

def test_list_mcus_filters_by_min_flash():
    """Test MCU listing with flash filter"""
    mcus = MCUService.list(min_flash=512*1024)  # 512KB

    assert len(mcus) > 0
    assert all(m.flash_kb >= 512 for m in mcus)

def test_show_mcu_enriches_with_svd():
    """Test MCU details include SVD data"""
    mcu = MCUService.show("STM32F401RE")

    assert mcu.part_number == "STM32F401RE"
    assert mcu.peripherals is not None
    assert "GPIOA" in mcu.peripherals
```

### Integration Tests

**Test Full Workflows**:
```python
# tools/codegen/tests/integration/test_init_workflow.py
import pytest
from cli.commands.init import init_project

def test_init_creates_working_project(tmp_path):
    """Test project initialization creates compilable project"""

    # Initialize project
    project_dir = tmp_path / "test-project"
    init_project(
        output_dir=project_dir,
        board="nucleo_f401re",
        template="blinky",
        build_system="cmake"
    )

    # Verify structure
    assert (project_dir / "CMakeLists.txt").exists()
    assert (project_dir / "src" / "main.cpp").exists()

    # Configure build
    configure_build(project_dir)

    # Compile
    result = compile_project(project_dir)
    assert result.returncode == 0

    # Verify binary
    elf_file = project_dir / "build" / "firmware.elf"
    assert elf_file.exists()
    assert elf_file.stat().st_size > 0
```

### Validation Tests

**Test Generated Code**:
```python
# tools/codegen/tests/validation/test_generated_code.py
import pytest
from cli.validators.validation_service import ValidationService

def test_generated_gpio_passes_validation():
    """Test generated GPIO code passes all checks"""

    # Generate code
    codegen.generate_gpio("stm32f4")

    # Validate
    result = ValidationService.validate_all(
        file_path="src/hal/vendors/st/stm32f4/gpio.hpp"
    )

    assert result.syntax.passed
    assert result.semantics.passed
    assert result.compilation.passed
    assert result.tests.passed

@pytest.mark.parametrize("vendor,family", [
    ("st", "stm32f4"),
    ("atmel", "same70"),
    ("st", "stm32g0"),
])
def test_all_vendors_pass_validation(vendor, family):
    """Test all vendor code passes validation"""

    codegen.generate(vendor=vendor, family=family)

    result = ValidationService.validate_all(
        vendor=vendor,
        family=family
    )

    assert result.all_passed()
```

### CI/CD Integration

**GitHub Actions Workflow**:
```yaml
# .github/workflows/cli-validation.yml
name: CLI Validation

on: [push, pull_request]

jobs:
  test-cli:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v3

      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'

      - name: Install dependencies
        run: |
          pip install -r tools/codegen/requirements.txt
          pip install pytest pytest-cov

      - name: Run unit tests
        run: |
          cd tools/codegen
          pytest tests/ --cov=cli --cov-report=xml

      - name: Upload coverage
        uses: codecov/codecov-action@v3
        with:
          files: ./tools/codegen/coverage.xml

  validate-generated-code:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v3

      - name: Set up ARM GCC
        uses: carlosperate/arm-none-eabi-gcc-action@v1
        with:
          release: '13.2.0'

      - name: Generate code
        run: |
          cd tools/codegen
          python3 codegen.py generate --all

      - name: Validate generated code
        run: |
          python3 codegen.py validate --all

      - name: Compile test programs
        run: |
          python3 codegen.py test generated
```

---

## Risk Mitigation

### Risk 1: Database Maintenance Overhead

**Risk**: Keeping MCU/board database up to date is time-consuming

**Mitigation**:
- Automate database generation from SVD files
- Community contributions (PR template for new boards)
- Quarterly update schedule
- Validation scripts catch outdated data

### Risk 2: Validation False Positives

**Risk**: Correct code fails validation checks

**Mitigation**:
- Extensive testing with known-good code
- Manual review of all validation rules
- User feedback mechanism
- Option to skip specific checks

### Risk 3: Build System Complexity

**Risk**: Supporting both CMake and Meson is complex

**Mitigation**:
- Start with CMake only (Phase 1-5)
- Add Meson as experimental (Phase 6)
- Unified abstraction layer
- Separate test suites

### Risk 4: Performance with Large Databases

**Risk**: Listing 1000+ MCUs is slow

**Mitigation**:
- Index files for fast lookup
- Lazy loading (load details on demand)
- Pagination for large result sets
- Caching frequently accessed data

---

## Migration Path

### For Existing Projects

**Step 1**: Install new CLI
```bash
cd tools/codegen
pip install -r requirements.txt --upgrade
```

**Step 2**: Validate existing generated code
```bash
alloy codegen validate --all
# Fix any issues found
```

**Step 3**: Adopt new workflow gradually
```bash
# Continue using old commands
python3 codegen.py generate

# Try new discovery
alloy list mcus

# Eventually migrate fully
alloy build compile
```

### Backward Compatibility

**Guarantee**: Old `codegen.py` commands still work
```bash
# Old (still works)
python3 codegen.py generate

# New (equivalent)
alloy codegen generate
```

**Timeline**: Deprecate old commands in 6 months after new CLI is stable

---

## Dependencies

### Python Packages

```toml
# tools/codegen/pyproject.toml
[project]
name = "alloy-cli"
version = "2.0.0"
dependencies = [
    "typer>=0.9.0",        # Modern CLI framework
    "rich>=13.0.0",        # Beautiful terminal output
    "inquirerpy>=0.3.4",   # Interactive prompts
    "jinja2>=3.1.0",       # Template engine
    "pydantic>=2.0.0",     # Data validation
    "jsonschema>=4.17.0",  # JSON schema validation
    "lxml>=4.9.0",         # XML/SVD parsing
    "click>=8.1.0",        # CLI utilities
    "pyyaml>=6.0",         # YAML parsing
]

[project.optional-dependencies]
dev = [
    "pytest>=7.0.0",
    "pytest-cov>=4.0.0",
    "pytest-mock>=3.10.0",
    "mypy>=1.0.0",
    "ruff>=0.1.0",
]

[project.scripts]
alloy = "cli.main:app"
```

### External Tools

**Required**:
- Python 3.11+
- arm-none-eabi-gcc (for validation)
- clang (for syntax checking)

**Optional**:
- OpenOCD (for flashing)
- Meson (for alternative build system)

---

## Documentation

### User Documentation

**CLI Reference** (`docs/cli/reference.md`):
- Complete command reference
- Examples for each command
- Common workflows

**Getting Started** (`docs/cli/getting-started.md`):
- Installation
- First project (5-minute tutorial)
- Troubleshooting

**Advanced Usage** (`docs/cli/advanced.md`):
- Custom templates
- Validation customization
- Database contributions

### Developer Documentation

**Architecture** (`docs/cli/architecture.md`):
- Component design
- Service layer patterns
- Database schema

**Contributing** (`docs/cli/contributing.md`):
- Adding new commands
- Adding new validators
- Database updates

---

## Success Metrics

### Quantitative Metrics

- **Adoption**: 80% of new projects use `alloy init`
- **Time savings**: 93% reduction in project setup time (30 min → 2 min)
- **Error reduction**: 0 syntax/semantic errors in generated code
- **Test coverage**: 80% CLI code coverage
- **Performance**: <1 second for discovery commands

### Qualitative Metrics

- **User feedback**: Positive sentiment in surveys
- **GitHub stars**: Increase in stars after CLI launch
- **Community contributions**: New board definitions via PRs
- **Support requests**: Decrease in setup-related issues

---

## Conclusion

This OpenSpec defines a comprehensive transformation of Alloy's CLI into a professional development tool. The phased approach allows incremental delivery with validation at each step. The integrated validation pipeline ensures generated code is always correct, eliminating a major source of user frustration.

**Key Innovations**:
1. **Discovery without docs**: Instant MCU/board information
2. **2-minute project setup**: Interactive wizard eliminates manual configuration
3. **Guaranteed correctness**: 4-stage validation catches all errors
4. **Professional UX**: Rich terminal UI rivals commercial tools

**Next Steps**:
1. Review and approve this OpenSpec
2. Begin Phase 1 implementation (Foundation & Discovery)
3. Validate with user testing after each phase
4. Iterate based on feedback

**The goal is to make Alloy the easiest embedded framework to use, with the most powerful development tools.**

---

## 🆕 Change Log - Consolidated Improvements (2025-01-17)

This proposal was enhanced with critical usability improvements from comprehensive CLI analysis. **Key changes integrated**:

### Timeline Updates
- **Original**: 8 weeks (176 hours)
- **Updated**: 11.5 weeks (230 hours)
- **Added**: +3.5 weeks for YAML migration, config system, preview/diff, incremental generation

### New Phases
- **Phase 0** (NEW): YAML Migration (1 week, 20 hours)
  - Migrate 45 JSON files → YAML
  - 25-30% size reduction
  - Inline comments for hardware quirks
  - Auto-detection during transition

### Enhanced Phases
- **Phase 1**: +12 hours for:
  - Configuration system (`.alloy.yaml`)
  - Enhanced metadata commands (`validate`, `create`, `diff`)

- **Phase 2**: +10 hours for:
  - Preview/diff capability (`--dry-run --diff`)
  - Incremental generation (10x faster iteration)
  - Checksum-based change detection

### New Goals Added
- **G6**: YAML Metadata Migration (MUST HAVE)
- **G7**: Preview & Incremental Generation (MUST HAVE)
- **G8**: Configuration System (SHOULD HAVE)

### Success Metrics Enhanced
Added metrics:
- Metadata file size: -25% reduction
- Syntax errors: 30% → <5%
- Iteration speed: 10x faster
- Developer Experience: 5.0 → 8.6 (+72%)

### Database Format Changed
- **All schemas converted** from JSON → YAML in examples
- **Inline comments** demonstrate value (hardware quirks, pin conflicts)
- **Multiline descriptions** showcase cleaner format

### Reference Documents
- **`CONSOLIDATED_IMPROVEMENTS.md`** - Detailed integration guide
- **`CLI_IMPROVEMENT_ANALYSIS.md`** - Original analysis (1,750 lines)

**Result**: The proposal now delivers a **significantly more polished and usable CLI** with professional-grade features while maintaining the original vision of a comprehensive embedded development assistant.
