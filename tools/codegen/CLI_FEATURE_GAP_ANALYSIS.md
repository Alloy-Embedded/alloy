# CLI Feature Gap Analysis - Old vs New

**Date**: 2025-11-19
**Purpose**: Identify missing features before retiring old CLI
**Status**: ANALYSIS COMPLETE

---

## Executive Summary

**Old CLI (`codegen.py`)**: Code generation-focused tool for MCU HAL
**New CLI (`alloy`)**: Project-focused development tool

**Critical Gaps Found**: ✅ 7 major features missing in new CLI
**Action Required**: YES - Need to add code generation capabilities

---

## Feature Comparison Matrix

| Feature | Old CLI | New CLI | Status | Priority |
|---------|---------|---------|--------|----------|
| **Discovery** |
| List MCUs | ❌ | ✅ | BETTER | - |
| Show MCU details | ❌ | ✅ | BETTER | - |
| List boards | ❌ | ✅ | BETTER | - |
| Search MCUs | ❌ | ✅ | BETTER | - |
| **Code Generation** |
| Generate startup code | ✅ | ❌ | MISSING | HIGH |
| Generate registers | ✅ | ❌ | MISSING | HIGH |
| Generate pin headers | ✅ | ❌ | MISSING | HIGH |
| Generate enums | ✅ | ❌ | MISSING | MEDIUM |
| Generate pin functions | ✅ | ❌ | MISSING | HIGH |
| Generate register map | ✅ | ❌ | MISSING | MEDIUM |
| Generate HAL wrappers | ✅ | ❌ | MISSING | LOW |
| **Vendor Support** |
| ST (STM32) generation | ✅ | ❌ | MISSING | HIGH |
| Atmel/Microchip | ✅ | ❌ | MISSING | MEDIUM |
| Raspberry Pi (RP2040) | ✅ | ❌ | MISSING | LOW |
| Espressif (ESP32) | ✅ | ❌ | MISSING | LOW |
| **Project Init** |
| Interactive wizard | ❌ | ✅ | BETTER | - |
| Template system | ❌ | ✅ | BETTER | - |
| Pin recommendation | ❌ | ✅ | BETTER | - |
| **Build & Flash** |
| Build integration | ❌ | ✅ | BETTER | - |
| Flash integration | ❌ | ✅ | BETTER | - |
| **Validation** |
| Syntax validation | ✅ | ✅ | SAME | - |
| Semantic validation | ✅ | ✅ | SAME | - |
| Compile validation | ✅ | ✅ | SAME | - |
| Test generation | ✅ | ✅ | SAME | - |
| **Utilities** |
| SVD parser testing | ✅ | ❌ | MISSING | LOW |
| Status/support matrix | ✅ | ❌ | MISSING | LOW |
| Clean generated files | ✅ | ✅ | SAME | - |
| **Complete Pipeline** |
| Generate-complete workflow | ✅ | ❌ | MISSING | MEDIUM |
| Format generated code | ✅ | ❌ | MISSING | MEDIUM |

---

## Detailed Feature Analysis

### 1. Code Generation (MISSING - HIGH PRIORITY)

#### Old CLI Capabilities:
```bash
# Generate all code for all MCUs
python3 codegen.py generate

# Generate specific components
python3 codegen.py generate --startup       # Startup code
python3 codegen.py generate --registers     # Register definitions
python3 codegen.py generate --pins          # Pin headers
python3 codegen.py generate --enums         # Peripheral enums
python3 codegen.py generate --pin-functions # Pin alternate functions
python3 codegen.py generate --register-map  # Complete register map

# Generate for specific vendor
python3 codegen.py generate --pins --vendor st
python3 codegen.py generate --pins --vendor atmel
```

#### What it generates:
1. **Startup Code** (`startup_*.cpp`):
   - Vector table
   - Reset handler
   - Interrupt handlers
   - Clock configuration
   - Memory initialization
   - C++ runtime initialization

2. **Register Definitions** (`*_registers.hpp`):
   - Peripheral base addresses
   - Register structures with bitfields
   - Register offsets
   - Bitfield masks and positions
   - Type-safe register access

3. **Pin Headers** (`pin_functions.hpp`):
   - GPIO pin definitions
   - Pin alternate functions
   - Port/pin mappings
   - Peripheral-to-pin mappings

4. **Enumerations** (`*_enums.hpp`):
   - Peripheral enums (GPIO, UART, SPI, I2C, etc.)
   - Interrupt enums
   - Clock source enums

5. **Pin Alternate Functions** (`pin_alternate_functions.hpp`):
   - Complete pin function mappings
   - AF (Alternate Function) tables
   - Pin mux configurations

6. **Register Map** (`register_map.hpp`):
   - Single include for all peripheral registers
   - Complete memory map

#### New CLI Status:
- ❌ No code generation capabilities
- ✅ Has validation for generated code
- ✅ Has project initialization (uses templates, not generation)

**Gap**: New CLI cannot generate MCU-specific HAL code from SVD files.

---

### 2. Complete Generation Pipeline (MISSING - MEDIUM PRIORITY)

#### Old CLI Workflow:
```bash
# Recommended workflow
python3 codegen.py generate-complete

# This does:
# 1. Generate all vendor code (pins, startup, registers, enums)
# 2. Generate platform HAL implementations
# 3. Format all code with clang-format
# 4. Validate with clang-tidy
```

#### Options:
- `--skip-format` - Skip clang-format step
- `--skip-validate` - Skip clang-tidy validation
- `--continue-on-error` - Don't stop on errors

#### New CLI Status:
- ❌ No complete pipeline
- ✅ Has build commands (configure, compile, flash)
- ✅ Has validation commands (validate file, validate dir)

**Gap**: New CLI cannot orchestrate complete code generation → format → validate pipeline.

---

### 3. Vendor-Specific Generators (MISSING - HIGH PRIORITY)

#### Old CLI Vendors:
1. **STMicroelectronics** (HIGH PRIORITY):
   - `cli/vendors/st/generate_all_st_pins.py`
   - `cli/vendors/st/generate_stm32_pins.py`
   - `cli/vendors/st/generate_pin_functions.py`
   - Supports: STM32F1, STM32F4, STM32F7, STM32G0

2. **Atmel/Microchip** (MEDIUM PRIORITY):
   - `cli/vendors/atmel/generate_all_atmel.py`
   - `cli/vendors/atmel/generate_same70_pins.py`
   - `cli/vendors/atmel/generate_samd21_pins.py`
   - Supports: SAME70, SAMV71, SAMD21

3. **Raspberry Pi** (LOW PRIORITY):
   - `cli/vendors/raspberrypi/generate_rp2040_pins.py`
   - Supports: RP2040

4. **Espressif** (LOW PRIORITY):
   - `cli/vendors/espressif/generate_esp32_pins.py`
   - Supports: ESP32, ESP32-S2, ESP32-S3, ESP32-C2, ESP32-C3, ESP32-C6, ESP32-H2, ESP32-P4

#### New CLI Status:
- ❌ No vendor-specific generators
- ✅ Has vendor metadata (MCU database, board database)
- ✅ Has MCU/board services for discovery

**Gap**: New CLI can discover MCUs but cannot generate code for them.

---

### 4. SVD Parser Utilities (MISSING - LOW PRIORITY)

#### Old CLI Capabilities:
```bash
# Test SVD parser
python3 codegen.py test-parser STMicro/STM32F103.svd --verbose

# Shows:
# - Device name, vendor, family
# - CPU info (core, FPU, MPU)
# - Peripheral count
# - Interrupt count
# - Memory regions
```

#### New CLI Status:
- ❌ No SVD parser testing command
- ✅ Has SVD parser in validators (for semantic validation)

**Gap**: Cannot quickly test SVD files without running full validation.

---

### 5. Generation Status/Support Matrix (MISSING - LOW PRIORITY)

#### Old CLI Capabilities:
```bash
# Show generation status
python3 codegen.py status

# Outputs:
# - MCU_STATUS.md (support matrix)
# - Which MCUs have generated code
# - Which MCUs are missing code
# - Generation coverage percentage
```

#### Options:
- `--output <file>` - Output file path
- `--vendor st|atmel|all` - Filter by vendor
- `--format markdown|json|text` - Output format

#### New CLI Status:
- ❌ No status/support matrix
- ✅ Has `alloy list mcus` (shows available MCUs)
- ✅ Has metadata validation

**Gap**: Cannot see which MCUs have generated code vs which are just metadata.

---

### 6. Manifest Tracking (PARTIALLY MISSING - LOW PRIORITY)

#### Old CLI Capabilities:
- Tracks all generated files in manifest
- Checksums for validation
- Generator attribution
- Clean command uses manifest

```bash
# Clean with manifest
python3 codegen.py clean --dry-run
python3 codegen.py clean --stats
python3 codegen.py clean --validate
python3 codegen.py clean --generator startup
```

#### New CLI Status:
- ❌ No manifest tracking
- ✅ Has `alloy build clean` (removes build dir only)

**Gap**: Cannot track what files were generated or validate they haven't been modified.

---

### 7. Configuration Display (MISSING - LOW PRIORITY)

#### Old CLI Capabilities:
```bash
# Show configuration
python3 codegen.py config --verbose

# Shows:
# - Board MCUs list
# - Vendor mappings
# - Family detection patterns
# - Test vendor/family detection
```

#### New CLI Status:
- ✅ Has `alloy config show` (shows user config)
- ❌ Does not show board MCUs or vendor mappings

**Gap**: Minor - mostly for debugging.

---

## Critical Missing Features Summary

### HIGH PRIORITY (Must Have):
1. **Code Generation from SVD**:
   - Generate startup code
   - Generate register definitions
   - Generate pin headers
   - Generate pin alternate functions
   - Generate for ST (STM32) at minimum

2. **Vendor Generators**:
   - ST generator (STM32F1, F4, F7, G0)
   - Atmel generator (SAME70, SAMD21) - optional but nice

### MEDIUM PRIORITY (Should Have):
3. **Complete Pipeline**:
   - `alloy generate-complete` workflow
   - Format generated code (clang-format integration)
   - Orchestrate multiple steps

4. **Enums & Register Map**:
   - Generate peripheral enums
   - Generate complete register map

### LOW PRIORITY (Nice to Have):
5. **Utilities**:
   - SVD parser testing
   - Generation status/support matrix
   - Manifest tracking for generated files

---

## Recommended Solution: New OpenSpec Phase

### Phase 7: Code Generation from SVD (NEW)

**Duration**: 32-40 hours

#### 7.1 SVD-Based Code Generation (16h)
- Implement `alloy generate` command
- SVD parser integration (reuse existing parser)
- Template-based code generation
- Generate startup, registers, pins, enums
- Support STM32 family (F1, F4, F7, G0)

#### 7.2 Vendor-Specific Generators (12h)
- ST generator (priority)
- Atmel generator (optional)
- Vendor plugin system for extensibility

#### 7.3 Complete Pipeline (4h)
- `alloy generate-complete` command
- Integrate with existing validators
- Format and validate generated code

#### 7.4 Testing (8h)
- Unit tests for generators
- Integration tests for complete pipeline
- Validate generated code compiles

**Total**: 40 hours (1 week)

---

## Migration Strategy

### Option A: Keep Old CLI Temporarily (RECOMMENDED)
1. Document that `codegen.py` is for code generation
2. Document that `alloy` is for project development
3. Add note: "Code generation will be integrated into `alloy` in future release"
4. Keep both CLIs for now
5. Implement Phase 7 later

### Option B: Implement Phase 7 Now
1. Add code generation to new CLI
2. Migrate all functionality
3. Retire old CLI
4. Update all documentation

### Option C: Hybrid Approach
1. Create `alloy codegen` command that wraps old `codegen.py`
2. Best of both worlds - unified interface
3. Minimal implementation time
4. Can refactor later

---

## Recommendation

**DO NOT DELETE OLD CLI YET**

Reasons:
1. **Code generation is critical** - generates actual MCU HAL code
2. **No replacement exists** - new CLI cannot generate code from SVD
3. **High complexity** - SVD parsing + code generation is complex
4. **Used in production** - library generation depends on it

**Recommended Actions**:
1. ✅ Keep `tools/codegen/codegen.py` (old CLI)
2. ✅ Keep `tools/codegen/cli/vendors/` (generators)
3. ✅ Keep `tools/codegen/cli/generators/` (generation logic)
4. ✅ Keep `tools/codegen/cli/parsers/` (SVD parsers)
5. ❌ Remove duplicated files only:
   - Old test files (`tests/_old/`)
   - Archived generators (`archive/`)
   - Duplicate validators (new ones are better)

**Future Work** (Phase 7 - Optional):
1. Implement `alloy generate` command
2. Port code generation logic to new CLI
3. Add vendor generators to new CLI
4. Retire old CLI when parity achieved

---

## Files to Keep (Code Generation)

### Critical Files (DO NOT DELETE):
```
tools/codegen/codegen.py                                    # Old CLI entry point
tools/codegen/cli/vendors/st/generate_all_st_pins.py       # ST generator
tools/codegen/cli/vendors/atmel/generate_all_atmel.py      # Atmel generator
tools/codegen/cli/generators/startup_generator.py          # Startup generation
tools/codegen/cli/generators/generate_registers.py         # Register generation
tools/codegen/cli/generators/generate_pin_functions.py     # Pin function generation
tools/codegen/cli/generators/generate_enums.py             # Enum generation
tools/codegen/cli/generators/generate_register_map.py      # Register map
tools/codegen/cli/parsers/generic_svd.py                   # SVD parser
tools/codegen/cli/core/svd_parser.py                       # SVD parsing core
tools/codegen/cli/core/template_engine.py                  # Jinja2 templates (old)
```

### Safe to Delete:
```
tools/codegen/tests/_old/                                   # Old tests
tools/codegen/archive/                                      # Archived code
tools/codegen/cli/core/validators/                         # Duplicate (new CLI has better ones)
```

---

## Conclusion

**Status**: ❌ **CANNOT DELETE OLD CLI YET**

The old CLI (`codegen.py`) provides **critical code generation** functionality that does not exist in the new CLI. It generates startup code, register definitions, and pin headers from SVD files - essential for the MCU HAL.

**Next Steps**:
1. Keep both CLIs for now
2. Clean up only duplicate/archived files
3. Document purpose of each CLI
4. Consider Phase 7 (Code Generation) for future

**Alternative**: Create OpenSpec for Phase 7 to add code generation to new CLI, then retire old CLI.
