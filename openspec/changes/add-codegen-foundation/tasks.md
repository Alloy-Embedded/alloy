## 1. SVD Infrastructure

- [ ] 1.1 Create `tools/codegen/` directory structure
- [ ] 1.2 Add `.gitmodules` for CMSIS-SVD submodule
- [ ] 1.3 Create `tools/codegen/upstream/.gitignore` (ignore submodule contents)
- [ ] 1.4 Initialize git submodule for cmsis-svd repository
- [ ] 1.5 Create `tools/codegen/database/svd/` directory

## 2. SVD Sync Script

- [ ] 2.1 Create `tools/codegen/sync_svd.py` main script
- [ ] 2.2 Implement `--init` command (initial setup of submodules)
- [ ] 2.3 Implement `--update` command (pull latest from upstream)
- [ ] 2.4 Implement `--vendor` filter (e.g., `--vendor STM32,nRF`)
- [ ] 2.5 Create organized symlink structure by vendor
- [ ] 2.6 Generate `database/svd/INDEX.md` listing all available SVDs
- [ ] 2.7 Add `--list-vendors` command showing available vendors
- [ ] 2.8 Add `--list-mcus <vendor>` showing MCUs per vendor
- [ ] 2.9 Add dry-run mode `--dry-run` to preview changes
- [ ] 2.10 Add progress reporting for large syncs

## 3. Database Structure

- [ ] 3.1 Create `tools/codegen/database/families/` directory
- [ ] 3.2 Create `database_schema.json` defining JSON structure
- [ ] 3.3 Create `validate_database.py` schema validator
- [ ] 3.4 Add example database `database/families/example.json`
- [ ] 3.5 Document database format in `database/README.md`

## 4. SVD Parser

- [ ] 4.1 Create `tools/codegen/svd_parser.py` skeleton
- [ ] 4.2 Add XML parsing with lxml
- [ ] 4.3 Implement `parse_device_info()` (name, vendor, flash, ram)
- [ ] 4.4 Implement `parse_peripherals()` (instances and registers)
- [ ] 4.5 Implement `parse_interrupts()` (vector table)
- [ ] 4.6 Implement JSON output formatter
- [ ] 4.7 Add `--merge` mode (merge into existing family JSON)
- [ ] 4.8 Add vendor-specific normalizations (STM32 quirks)
- [ ] 4.9 Add validation of parsed data
- [ ] 4.10 Handle SVD derivedFrom inheritance
- [ ] 4.11 Add CLI with argparse (--input, --output, --merge)
- [ ] 4.12 Test with STM32F103 SVD file

## 5. Code Generator Core

- [ ] 5.1 Create `tools/codegen/generator.py` skeleton
- [ ] 5.2 Add Jinja2 environment setup
- [ ] 5.3 Implement database loader (JSON → dict)
- [ ] 5.4 Implement MCU selector (find MCU in database)
- [ ] 5.5 Add template renderer (Jinja2 → string)
- [ ] 5.6 Add file writer (safe atomic writes)
- [ ] 5.7 Create `CodeGenerator` class
- [ ] 5.8 Add `--verbose` mode with progress output
- [ ] 5.9 Add `--validate` mode (check but don't write)
- [ ] 5.10 Add CLI with argparse

## 6. Jinja2 Templates

- [ ] 6.1 Create `tools/codegen/templates/` directory structure
- [ ] 6.2 Create `templates/common/header.j2` (file header)
- [ ] 6.3 Create `templates/common/macros.j2` (reusable macros)
- [ ] 6.4 Create `templates/startup/cortex_m_startup.cpp.j2`
- [ ] 6.5 Implement .data copy in startup template
- [ ] 6.6 Implement .bss zeroing in startup template
- [ ] 6.7 Implement C++ static constructors call
- [ ] 6.8 Add weak default handlers
- [ ] 6.9 Test startup template renders valid C++

## 7. Initial Generator Functions

- [ ] 7.1 Implement `generate_startup()` method
- [ ] 7.2 Implement `generate_header()` method
- [ ] 7.3 Add output directory creation
- [ ] 7.4 Add generation timestamp to files
- [ ] 7.5 Test generation produces valid files

## 8. CMake Integration

- [ ] 8.1 Create `cmake/codegen.cmake` module
- [ ] 8.2 Implement `alloy_generate_code()` function
- [ ] 8.3 Add database file detection (find JSON for MCU)
- [ ] 8.4 Add marker file logic (.generated timestamp)
- [ ] 8.5 Add dependency tracking (regenerate if database newer)
- [ ] 8.6 Add Python detection (find_package(Python3))
- [ ] 8.7 Add generator invocation (execute_process)
- [ ] 8.8 Add error handling and messaging
- [ ] 8.9 Export variables (ALLOY_GENERATED_DIR, etc.)
- [ ] 8.10 Test integration with existing build system

## 9. STM32F103 Test Case

- [ ] 9.1 Download STM32F103 SVD file
- [ ] 9.2 Parse STM32F103 SVD → JSON database
- [ ] 9.3 Create `database/families/stm32f1xx.json`
- [ ] 9.4 Generate startup code for STM32F103C8
- [ ] 9.5 Verify generated startup.cpp compiles
- [ ] 9.6 Verify generated code is readable/navigable
- [ ] 9.7 Compare generated vs manual STM32F103 startup

## 10. Testing Infrastructure

- [ ] 10.1 Create `tools/codegen/tests/` directory
- [ ] 10.2 Add pytest configuration
- [ ] 10.3 Create `tests/test_svd_parser.py`
- [ ] 10.4 Create fixture SVD file for testing
- [ ] 10.5 Test SVD parsing with assertions
- [ ] 10.6 Create `tests/test_generator.py`
- [ ] 10.7 Test template rendering
- [ ] 10.8 Test generated file validity
- [ ] 10.9 Add integration test (full pipeline)
- [ ] 10.10 Add test runner script `run_tests.sh`

## 11. Documentation

- [ ] 11.1 Create `tools/codegen/README.md` overview
- [ ] 11.2 Document `sync_svd.py` usage and commands
- [ ] 11.3 Document `svd_parser.py` usage
- [ ] 11.4 Document `generator.py` usage
- [ ] 11.5 Document database JSON schema
- [ ] 11.6 Create tutorial: "Adding a new ARM MCU"
- [ ] 11.7 Document CMake integration
- [ ] 11.8 Add troubleshooting guide
- [ ] 11.9 Document template customization
- [ ] 11.10 Update main project README

## 12. Validation and Polish

- [ ] 12.1 Run database schema validation on all JSONs
- [ ] 12.2 Verify all generated code compiles
- [ ] 12.3 Check Python code style (black, mypy)
- [ ] 12.4 Add type hints to Python code
- [ ] 12.5 Add docstrings to all functions
- [ ] 12.6 Verify sync_svd.py works on clean checkout
- [ ] 12.7 Test complete workflow end-to-end
- [ ] 12.8 Measure generation time (should be < 5s)
- [ ] 12.9 Update IMPLEMENTATION_ROADMAP.md
- [ ] 12.10 Create demo video/GIF of code generation

## Dependencies Between Tasks

**Critical path:**
1. SVD Infrastructure (1) → SVD Sync Script (2)
2. Database Structure (3) → SVD Parser (4)
3. SVD Parser (4) → STM32F103 Test Case (9)
4. Templates (6) + Generator Core (5) → Initial Generator Functions (7)
5. Generator Functions (7) + STM32F103 Test (9) → CMake Integration (8)

**Parallelizable:**
- Templates (6) can start early
- Documentation (11) can progress throughout
- Testing infrastructure (10) can build alongside implementation

## Validation Checklist

After completing all tasks:
- [ ] `python tools/codegen/sync_svd.py --init` works
- [ ] `python tools/codegen/sync_svd.py --update` syncs successfully
- [ ] `python tools/codegen/svd_parser.py --input STM32F103.svd` produces JSON
- [ ] `python tools/codegen/generator.py --mcu STM32F103C8` generates files
- [ ] Generated startup.cpp compiles with arm-none-eabi-g++
- [ ] CMake integration generates code transparently
- [ ] All Python tests pass: `pytest tools/codegen/tests/`
- [ ] Database validation passes: `python tools/codegen/validate_database.py`
