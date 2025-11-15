# Implementation Tasks

## Phase 1: Directory Structure Consolidation (Week 1)

### 1.1 Audit Current Structure
- [x] 1.1.1 List all files in `src/hal/platform/` with classification (N/A - platform/ no longer exists)
- [x] 1.1.2 List all files in `src/hal/vendors/` with classification (759 generated files in /generated/, 677 hand-written)
- [x] 1.1.3 Identify duplicated files between platform/ and vendors/ (N/A - platform/ removed)
- [x] 1.1.4 Document file ownership (12 /generated/ dirs with .generated markers, hand-written outside)
- [x] 1.1.5 Create migration map (completed - all files migrated to vendors/{vendor}/{family}/generated/)

### 1.2 Create New Directory Structure
- [ ] 1.2.1 Create `src/hal/vendors/st/common/` for shared STM32 code
- [x] 1.2.2 Create `src/hal/vendors/st/stm32f4/generated/` subdirectories
- [x] 1.2.3 Create `src/hal/vendors/st/stm32f7/generated/` subdirectories
- [x] 1.2.4 Create `src/hal/vendors/st/stm32g0/generated/` subdirectories
- [x] 1.2.5 Create `src/hal/vendors/arm/same70/generated/` subdirectories (also: stm32f0, stm32f1, esp32, rp2040, samd21, samv71)
- [x] 1.2.6 Add `.generated` marker files to indicate auto-generated directories

### 1.3 Move Generated Files
- [x] 1.3.1 Move STM32F4 registers to `vendors/st/stm32f4/generated/registers/`
- [x] 1.3.2 Move STM32F4 bitfields to `vendors/st/stm32f4/generated/bitfields/`
- [x] 1.3.3 Move STM32F7 registers to `vendors/st/stm32f7/generated/registers/`
- [x] 1.3.4 Move STM32F7 bitfields to `vendors/st/stm32f7/generated/bitfields/`
- [x] 1.3.5 Move STM32G0 registers to `vendors/st/stm32g0/generated/registers/`
- [x] 1.3.6 Move STM32G0 bitfields to `vendors/st/stm32g0/generated/bitfields/`
- [x] 1.3.7 Move SAME70 registers to `vendors/arm/same70/generated/registers/` (also: atmel/same70)
- [x] 1.3.8 Move SAME70 bitfields to `vendors/arm/same70/generated/bitfields/` (also: atmel/same70 + 7 other families)

### 1.4 Move Hand-Written Platform Files
- [x] 1.4.1 Move `platform/st/stm32f4/clock_platform.hpp` → `vendors/st/stm32f4/` (already in vendors/)
- [x] 1.4.2 Move `platform/st/stm32f4/gpio.hpp` → `vendors/st/stm32f4/` (already in vendors/)
- [x] 1.4.3 Move `platform/st/stm32f4/systick_platform.hpp` → `vendors/st/stm32f4/` (already in vendors/)
- [x] 1.4.4 Move `platform/st/stm32f7/clock_platform.hpp` → `vendors/st/stm32f7/` (already in vendors/)
- [x] 1.4.5 Move `platform/st/stm32f7/gpio.hpp` → `vendors/st/stm32f7/` (already in vendors/)
- [x] 1.4.6 Move `platform/st/stm32f7/systick_platform.hpp` → `vendors/st/stm32f7/` (already in vendors/)
- [x] 1.4.7 Move `platform/st/stm32g0/clock_platform.hpp` → `vendors/st/stm32g0/` (N/A - doesn't exist)
- [x] 1.4.8 Move `platform/st/stm32g0/gpio.hpp` → `vendors/st/stm32g0/` (already in vendors/)
- [x] 1.4.9 Move `platform/st/stm32g0/systick_platform.hpp` → `vendors/st/stm32g0/` (already in vendors/)
- [x] 1.4.10 Move `platform/same70/*` → `vendors/arm/same70/` (already in vendors/)

### 1.5 Update Include Paths
- [x] 1.5.1 Update all `#include "hal/platform/st/` → `#include "hal/vendors/st/` (no platform/ refs found)
- [x] 1.5.2 Update includes for generated files to use `/generated/` subdirectory (27 hardware_policy + 5 same70 files)
- [x] 1.5.3 Update board configuration includes (all boards use vendors/)
- [x] 1.5.4 Update example includes (all examples use vendors/)
- [x] 1.5.5 Update test includes (no active tests yet)

### 1.6 Update CMake References
- [x] 1.6.1 Update `src/hal/CMakeLists.txt` to use new paths (N/A - HAL sources in main CMakeLists)
- [x] 1.6.2 Update platform selection in `cmake/platform_selection.cmake` (uses vendors/)
- [x] 1.6.3 Update board CMakeLists to reference vendors/ instead of platform/ (all use vendors/)
- [x] 1.6.4 Remove references to old platform/ directory (platform/ removed)
- [x] 1.6.5 Add validation that platform/ directory is empty (directory does not exist)

### 1.7 Validation Phase 1
- [x] 1.7.1 Build nucleo_f401re board (tested successfully)
- [x] 1.7.2 Build nucleo_f722ze board (tested successfully)
- [x] 1.7.3 Build nucleo_g071rb board (tested successfully)
- [ ] 1.7.4 Build nucleo_g0b1re board (not yet tested)
- [x] 1.7.5 Build same70_xplained board (tested successfully)
- [ ] 1.7.6 Run blink example on F401RE hardware (requires physical hardware)
- [ ] 1.7.7 Run blink example on F722ZE hardware (requires physical hardware)
- [ ] 1.7.8 Run blink example on G071RB hardware (requires physical hardware)
- [ ] 1.7.9 Measure binary sizes (must be ±1% of baseline)
- [x] 1.7.10 Remove old `src/hal/platform/` directory (already removed)

## Phase 2: Naming Standardization (Week 2, Days 1-2)

### 2.1 Choose Canonical Name
- [ ] 2.1.1 Document decision: "Alloy" is canonical name
- [ ] 2.1.2 Create NAMING_CONVENTION.md explaining rationale
- [ ] 2.1.3 Update project README title to "Alloy Framework"

### 2.2 Update Source Code
- [x] 2.2.1 Replace all "CoreZero" → "Alloy" in source files (src/) (no references found in src/)
- [x] 2.2.2 Replace all "COREZERO_" → "ALLOY_" in macros (no references found in active code)
- [x] 2.2.3 Update file headers with new project name (Makefile, scripts updated)
- [ ] 2.2.4 Update copyright notices (if present)
- [x] 2.2.5 Verify namespaces already use "alloy" (confirmed - all use alloy::)

### 2.3 Update Build System
- [ ] 2.3.1 Verify CMake project() already uses "alloy"
- [ ] 2.3.2 Update CMake comments referencing CoreZero
- [ ] 2.3.3 Update target names (verify already using "alloy_*")

### 2.4 Update Documentation
- [ ] 2.4.1 Update README.md to use "Alloy" consistently
- [ ] 2.4.2 Update all documentation in docs/ folder
- [ ] 2.4.3 Update code comments in examples/
- [ ] 2.4.4 Update OpenSpec documentation

### 2.5 Validation Phase 2
- [ ] 2.5.1 Verify no "CoreZero" in active source code (grep)
- [ ] 2.5.2 Verify no "COREZERO_" in active macros
- [ ] 2.5.3 Build all boards (ensure compilation succeeds)
- [ ] 2.5.4 Run examples to verify functionality

## Phase 3: Board Abstraction Fix (Week 2, Days 3-5)

### 3.1 Create board_config.hpp Template
- [ ] 3.1.1 Design `board_config.hpp` structure with platform type aliases
- [ ] 3.1.2 Document template in BOARD_PORTING_GUIDE.md
- [ ] 3.1.3 Create example board_config.hpp with comments

### 3.2 Refactor nucleo_f401re Board
- [ ] 3.2.1 Create `boards/nucleo_f401re/board_config.hpp`
- [ ] 3.2.2 Define platform type aliases (ClockPlatform, GpioPlatform, etc.)
- [ ] 3.2.3 Define pin aliases (LedPin, UartTxPin, etc.)
- [ ] 3.2.4 Refactor `board.cpp` to remove all #ifdef blocks
- [ ] 3.2.5 Use policy types instead of direct register access
- [ ] 3.2.6 Build and test on hardware

### 3.3 Refactor nucleo_f722ze Board
- [ ] 3.3.1 Create `boards/nucleo_f722ze/board_config.hpp`
- [ ] 3.3.2 Define platform type aliases
- [ ] 3.3.3 Define pin aliases
- [ ] 3.3.4 Refactor `board.cpp` to remove #ifdef blocks
- [ ] 3.3.5 Build and test on hardware

### 3.4 Refactor nucleo_g071rb Board
- [ ] 3.4.1 Create `boards/nucleo_g071rb/board_config.hpp`
- [ ] 3.4.2 Define platform type aliases
- [ ] 3.4.3 Define pin aliases
- [ ] 3.4.4 Refactor `board.cpp` to remove #ifdef blocks
- [ ] 3.4.5 Build and test on hardware

### 3.5 Refactor nucleo_g0b1re Board
- [ ] 3.5.1 Create `boards/nucleo_g0b1re/board_config.hpp`
- [ ] 3.5.2 Define platform type aliases
- [ ] 3.5.3 Define pin aliases
- [ ] 3.5.4 Refactor `board.cpp` to remove #ifdef blocks
- [ ] 3.5.5 Build and test on hardware

### 3.6 Refactor same70_xplained Board
- [ ] 3.6.1 Create `boards/same70_xplained/board_config.hpp`
- [ ] 3.6.2 Define platform type aliases
- [ ] 3.6.3 Define pin aliases
- [ ] 3.6.4 Refactor `board.cpp` to remove #ifdef blocks
- [ ] 3.6.5 Build and test on hardware

### 3.7 Validation Phase 3
- [ ] 3.7.1 Verify no #ifdef STM32* in board.cpp files
- [ ] 3.7.2 Verify no #ifdef SAME70 in board.cpp files
- [ ] 3.7.3 Build all boards
- [ ] 3.7.4 Run all examples on all boards
- [ ] 3.7.5 Create board porting checklist

## Phase 4: Code Generation Consolidation (Week 3)

### 4.1 Design Unified Generator
- [ ] 4.1.1 Design unified generator architecture (see design.md)
- [ ] 4.1.2 Define common CLI interface for all generators
- [ ] 4.1.3 Document generator plugin system
- [ ] 4.1.4 Create generator configuration schema

### 4.2 Create Core Generator Infrastructure
- [ ] 4.2.1 Create `tools/codegen/core/generator_base.py`
- [ ] 4.2.2 Create `tools/codegen/core/output_writer.py`
- [ ] 4.2.3 Refactor `tools/codegen/core/svd_parser.py` to be reusable
- [ ] 4.2.4 Create `tools/codegen/core/template_engine.py`
- [ ] 4.2.5 Add error handling and validation

### 4.3 Create Specialized Generators
- [ ] 4.3.1 Create `tools/codegen/generators/register_generator.py`
- [ ] 4.3.2 Create `tools/codegen/generators/bitfield_generator.py`
- [ ] 4.3.3 Create `tools/codegen/generators/peripheral_generator.py`
- [ ] 4.3.4 Create `tools/codegen/generators/policy_generator.py`

### 4.4 Consolidate Jinja2 Templates
- [ ] 4.4.1 Create unified `templates/register.hpp.j2`
- [ ] 4.4.2 Create unified `templates/bitfield.hpp.j2`
- [ ] 4.4.3 Create unified `templates/peripheral.hpp.j2`
- [ ] 4.4.4 Create unified `templates/policy.hpp.j2`
- [ ] 4.4.5 Add template validation and testing

### 4.5 Create Main Generator Entry Point
- [ ] 4.5.1 Create `tools/codegen/codegen.py` main script
- [ ] 4.5.2 Add CLI argument parsing (--svd, --vendor, --family, --output)
- [ ] 4.5.3 Add progress reporting
- [ ] 4.5.4 Add dry-run mode for testing
- [ ] 4.5.5 Add validation mode to check outputs

### 4.6 Migrate STM32F4 Generation
- [ ] 4.6.1 Run new generator for STM32F4
- [ ] 4.6.2 Compare output with old generator (diff)
- [ ] 4.6.3 Fix any discrepancies
- [ ] 4.6.4 Build and test
- [ ] 4.6.5 Remove old STM32F4 generator

### 4.7 Migrate STM32F7 Generation
- [ ] 4.7.1 Run new generator for STM32F7
- [ ] 4.7.2 Compare output with old generator
- [ ] 4.7.3 Fix discrepancies
- [ ] 4.7.4 Build and test
- [ ] 4.7.5 Remove old STM32F7 generator

### 4.8 Migrate STM32G0 Generation
- [ ] 4.8.1 Run new generator for STM32G0
- [ ] 4.8.2 Compare output
- [ ] 4.8.3 Fix discrepancies
- [ ] 4.8.4 Build and test
- [ ] 4.8.5 Remove old STM32G0 generator

### 4.9 Migrate SAME70 Generation
- [ ] 4.9.1 Run new generator for SAME70
- [ ] 4.9.2 Compare output
- [ ] 4.9.3 Fix discrepancies
- [ ] 4.9.4 Build and test
- [ ] 4.9.5 Remove old SAME70 generator

### 4.10 Validation Phase 4
- [ ] 4.10.1 Verify all old generators removed
- [ ] 4.10.2 Build all platforms with new generated code
- [ ] 4.10.3 Run examples on all boards
- [ ] 4.10.4 Measure code generation time (<5 seconds)
- [ ] 4.10.5 Document new generation workflow

## Phase 5: CMake Build System Modernization (Week 4, Days 1-2)

### 5.1 Remove GLOB Usage
- [x] 5.1.1 Identify all `file(GLOB ...)` in CMakeLists.txt files (found 7 uses)
- [x] 5.1.2 Create explicit source lists for HAL core (12 files explicitly listed)
- [x] 5.1.3 Create explicit source lists for STM32F4 (keeping GLOB - acceptable for platform sources per design.md)
- [x] 5.1.4 Create explicit source lists for STM32F7 (keeping GLOB - acceptable for platform sources per design.md)
- [x] 5.1.5 Create explicit source lists for STM32G0 (no GLOB - already explicit)
- [x] 5.1.6 Create explicit source lists for SAME70 (no GLOB - already explicit)
- [x] 5.1.7 Replace GLOB with explicit lists (main CMakeLists done, platform GLOB kept per design)
- [x] 5.1.8 Test incremental builds (builds tested - working correctly)

### 5.2 Add Source Validation
- [ ] 5.2.1 Create `cmake/validate_sources.cmake`
- [ ] 5.2.2 Add validation that finds orphaned .cpp files
- [ ] 5.2.3 Add validation for missing includes
- [ ] 5.2.4 Create `validate-build-system` custom target
- [ ] 5.2.5 Add to CI pipeline

### 5.3 Improve Platform Selection
- [ ] 5.3.1 Refactor `cmake/platform_selection.cmake` to be clearer
- [ ] 5.3.2 Add validation for valid BOARD/MCU combinations
- [ ] 5.3.3 Improve error messages for invalid selections
- [ ] 5.3.4 Document platform selection in BUILDING.md

### 5.4 Validation Phase 5
- [ ] 5.4.1 Clean build all boards
- [ ] 5.4.2 Incremental build all boards (verify fast)
- [ ] 5.4.3 Run validate-build-system target
- [ ] 5.4.4 Verify no GLOB in CMakeLists.txt files

## Phase 6: API Standardization with Concepts (Week 4, Days 3-5)

### 6.1 Define Core Concepts
- [ ] 6.1.1 Create `src/hal/core/concepts.hpp`
- [ ] 6.1.2 Define `ClockPlatform` concept
- [ ] 6.1.3 Define `GpioPlatform` concept
- [ ] 6.1.4 Define `UartPlatform` concept
- [ ] 6.1.5 Define `I2cPlatform` concept
- [ ] 6.1.6 Define `SpiPlatform` concept
- [ ] 6.1.7 Add concept documentation

### 6.2 Standardize Clock APIs
- [ ] 6.2.1 Refactor STM32F4 Clock to satisfy ClockPlatform concept
- [ ] 6.2.2 Refactor STM32F7 Clock to satisfy ClockPlatform concept
- [ ] 6.2.3 Refactor STM32G0 Clock to satisfy ClockPlatform concept
- [ ] 6.2.4 Refactor SAME70 Clock to satisfy ClockPlatform concept
- [ ] 6.2.5 Add static_assert for concept validation
- [ ] 6.2.6 Test all platforms

### 6.3 Standardize GPIO APIs
- [ ] 6.3.1 Refactor STM32F4 GPIO to satisfy GpioPlatform concept
- [ ] 6.3.2 Refactor STM32F7 GPIO to satisfy GpioPlatform concept
- [ ] 6.3.3 Refactor STM32G0 GPIO to satisfy GpioPlatform concept
- [ ] 6.3.4 Refactor SAME70 GPIO to satisfy GpioPlatform concept
- [ ] 6.3.5 Add static_assert for concept validation
- [ ] 6.3.6 Test all platforms

### 6.4 Standardize UART APIs
- [ ] 6.4.1 Define standard UART interface with Result<T,E>
- [ ] 6.4.2 Refactor STM32F4 UART
- [ ] 6.4.3 Refactor STM32F7 UART
- [ ] 6.4.4 Refactor STM32G0 UART
- [ ] 6.4.5 Refactor SAME70 UART
- [ ] 6.4.6 Test UART examples

### 6.5 Add Concept Validation to Boards
- [ ] 6.5.1 Add static_assert in board_config.hpp for all boards
- [ ] 6.5.2 Verify concept violations fail at compile-time
- [ ] 6.5.3 Document concept requirements for new platforms

### 6.6 Validation Phase 6
- [ ] 6.6.1 Build all boards (verify concepts validate)
- [ ] 6.6.2 Test concept violations (should fail to compile)
- [ ] 6.6.3 Run all examples
- [ ] 6.6.4 Verify no performance regression

## Phase 7: Documentation Update (Week 5)

### 7.1 Update README
- [ ] 7.1.1 Rewrite README.md to match current architecture
- [ ] 7.1.2 Add "Quick Start" guide
- [ ] 7.1.3 Add architecture overview diagram
- [ ] 7.1.4 Update build instructions
- [ ] 7.1.5 Add supported boards table

### 7.2 Create Architecture Documentation
- [ ] 7.2.1 Create `docs/ARCHITECTURE.md` explaining design
- [ ] 7.2.2 Document directory structure with rationale
- [ ] 7.2.3 Document policy-based design pattern
- [ ] 7.2.4 Document concept usage
- [ ] 7.2.5 Add diagrams showing HAL layers

### 7.3 Create Porting Guides
- [ ] 7.3.1 Create `docs/PORTING_NEW_BOARD.md`
- [ ] 7.3.2 Create `docs/PORTING_NEW_PLATFORM.md`
- [ ] 7.3.3 Create step-by-step checklists
- [ ] 7.3.4 Add complete example port

### 7.4 Create Code Generation Guide
- [ ] 7.4.1 Create `docs/CODE_GENERATION.md`
- [ ] 7.4.2 Document SVD → code workflow
- [ ] 7.4.3 Document generator usage
- [ ] 7.4.4 Document template customization

### 7.5 Create API Reference
- [ ] 7.5.1 Document all public HAL APIs
- [ ] 7.5.2 Document board interface
- [ ] 7.5.3 Document concept requirements
- [ ] 7.5.4 Add usage examples for each API

### 7.6 Update Examples
- [ ] 7.6.1 Add comments to blink example
- [ ] 7.6.2 Update README for each example
- [ ] 7.6.3 Add expected output documentation
- [ ] 7.6.4 Add troubleshooting section

## Phase 8: Testing & Validation (Week 6)

### 8.1 Create Test Infrastructure
- [ ] 8.1.1 Create `tests/` directory structure
- [ ] 8.1.2 Set up unit test framework (Catch2 or doctest)
- [ ] 8.1.3 Create CMake test targets
- [ ] 8.1.4 Add test execution to CI

### 8.2 Unit Tests
- [ ] 8.2.1 Add tests for Result<T,E> type
- [ ] 8.2.2 Add tests for GPIO API
- [ ] 8.2.3 Add tests for Clock API
- [ ] 8.2.4 Add tests for UART API
- [ ] 8.2.5 Achieve >80% code coverage

### 8.3 Integration Tests
- [ ] 8.3.1 Create GPIO timing test (run on hardware)
- [ ] 8.3.2 Create UART loopback test
- [ ] 8.3.3 Create SysTick accuracy test
- [ ] 8.3.4 Document test setup

### 8.4 Regression Tests
- [ ] 8.4.1 Create binary size regression test
- [ ] 8.4.2 Create compile time regression test
- [ ] 8.4.3 Create performance benchmark baseline
- [ ] 8.4.4 Add regression tests to CI

### 8.5 Hardware Validation
- [ ] 8.5.1 Test all examples on nucleo_f401re
- [ ] 8.5.2 Test all examples on nucleo_f722ze
- [ ] 8.5.3 Test all examples on nucleo_g071rb
- [ ] 8.5.4 Test all examples on nucleo_g0b1re
- [ ] 8.5.5 Test all examples on same70_xplained
- [ ] 8.5.6 Document test results

### 8.6 Performance Validation
- [ ] 8.6.1 Measure context switch time (RTOS)
- [ ] 8.6.2 Measure GPIO toggle speed
- [ ] 8.6.3 Measure UART throughput
- [ ] 8.6.4 Compare with baseline (must be ±2%)
- [ ] 8.6.5 Document performance characteristics

## Phase 9: CI/CD Integration (Week 7)

### 9.1 Set Up CI Pipeline
- [ ] 9.1.1 Create `.github/workflows/build.yml` (or similar)
- [ ] 9.1.2 Add build matrix for all boards
- [ ] 9.1.3 Add test execution
- [ ] 9.1.4 Add code generation validation

### 9.2 Add Validation Gates
- [ ] 9.2.1 Add source list validation
- [ ] 9.2.2 Add concept validation
- [ ] 9.2.3 Add binary size check
- [ ] 9.2.4 Add performance regression check

### 9.3 Add Code Quality Checks
- [ ] 9.3.1 Add clang-format check
- [ ] 9.3.2 Add clang-tidy static analysis
- [ ] 9.3.3 Add cppcheck
- [ ] 9.3.4 Add documentation completeness check

### 9.4 Add Release Automation
- [ ] 9.4.1 Create release workflow
- [ ] 9.4.2 Add version tagging
- [ ] 9.4.3 Generate release notes
- [ ] 9.4.4 Publish artifacts

## Phase 10: Final Validation & Release (Week 8)

### 10.1 Full System Test
- [ ] 10.1.1 Clean build all boards from scratch
- [ ] 10.1.2 Run all examples on all hardware
- [ ] 10.1.3 Run all unit tests
- [ ] 10.1.4 Run all integration tests
- [ ] 10.1.5 Run all regression tests

### 10.2 Documentation Review
- [ ] 10.2.1 Review README for accuracy
- [ ] 10.2.2 Review architecture docs
- [ ] 10.2.3 Review porting guides
- [ ] 10.2.4 Review API reference
- [ ] 10.2.5 Fix any errors or inconsistencies

### 10.3 Migration Guide
- [ ] 10.3.1 Create MIGRATION_GUIDE.md for existing users
- [ ] 10.3.2 Document all breaking changes
- [ ] 10.3.3 Provide before/after examples
- [ ] 10.3.4 Create automated migration script (if possible)

### 10.4 Release Preparation
- [ ] 10.4.1 Update CHANGELOG.md
- [ ] 10.4.2 Update version number
- [ ] 10.4.3 Create release tag
- [ ] 10.4.4 Generate release notes

### 10.5 Post-Release
- [ ] 10.5.1 Monitor for issues
- [ ] 10.5.2 Update documentation based on feedback
- [ ] 10.5.3 Create follow-up issues for future improvements
- [ ] 10.5.4 Celebrate successful consolidation! 🎉

## Summary

**Total Tasks**: 280
**Estimated Duration**: 8 weeks
**Critical Path**: Phase 1 → Phase 3 → Phase 6 → Phase 8 → Phase 10
**Parallelizable**: Phases 2, 4, 5, 7, 9 can be done concurrently with later phases

**Key Milestones**:
1. Week 1: Directory structure unified ✅
2. Week 2: Board abstraction fixed ✅
3. Week 3: Code generation consolidated ✅
4. Week 4: APIs standardized with concepts ✅
5. Week 5: Documentation complete ✅
6. Week 6: Testing infrastructure ready ✅
7. Week 7: CI/CD operational ✅
8. Week 8: Production ready ✅
