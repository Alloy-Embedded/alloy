# Phase 4: Codegen Reorganization - Implementation Plan

**Date**: 2025-11-19
**Status**: IN PROGRESS
**Priority**: CRITICAL - Blocks CLI Development

---

## 🎯 Objective

Reorganize `tools/codegen/` to provide clear separation of concerns and unblock CLI development.

**Why Critical**: CLI needs:
- `core/validators/` to add ValidationService wrapper
- `generators/` structure for peripheral generators
- Separation of peripheral templates (library) from project templates (CLI)
- `core/schema_validator.py` foundation for YAML schemas

---

## 📋 Current State Analysis

### Current Structure (Flat, Unclear)
```
tools/codegen/
├── codegen.py                       # Main CLI (old)
├── cli/                            # New CLI (alloy)
│   ├── main.py
│   ├── commands/
│   ├── services/
│   ├── models/
│   ├── generators/
│   ├── validators/
│   ├── parsers/
│   ├── vendors/
│   └── core/
├── generate_*.py                    # Scattered generators
├── scripts/                         # Utility scripts
└── tests/                          # Test files
```

### Problems
1. ❌ No clear separation between core/generators/vendors
2. ❌ `cli/` directory contains BOTH old and new CLI logic
3. ❌ Validators scattered across multiple locations
4. ❌ Templates not separated by purpose (peripheral vs project)
5. ❌ Hard to find relevant code

---

## 🏗️ Target Structure

### New Organization
```
tools/codegen/
├── codegen.py                       # Old CLI entry point (code generation)
│
├── cli/                            # NEW CLI (alloy) - Project Development
│   ├── main.py                     # New CLI entry point
│   ├── commands/                   # CLI commands (list, show, init, build, etc.)
│   ├── services/                   # Services (MCU, Board, Build, Flash, etc.)
│   ├── models/                     # Pydantic models
│   ├── generators/                 # Project generators
│   │   ├── project_generator.py   # Project initialization
│   │   └── template_engine.py     # Jinja2 rendering (new)
│   └── validators/                 # Validators (new - better implementation)
│
├── core/                           # CORE - Shared functionality
│   ├── __init__.py
│   ├── svd_parser.py               # SVD XML → Python objects
│   ├── template_engine.py          # Jinja2 rendering (old)
│   ├── schema_validator.py         # JSON/YAML schema validation
│   ├── file_utils.py               # File I/O utilities
│   ├── config.py                   # Configuration
│   ├── logger.py                   # Logging
│   └── validators/                 # Core validators (for CLI integration)
│       ├── __init__.py
│       ├── syntax_validator.py     # C++ syntax validation
│       ├── semantic_validator.py   # SVD cross-reference
│       ├── compile_validator.py    # ARM GCC compilation
│       └── test_validator.py       # Test generation/execution
│
├── generators/                     # GENERATORS - Peripheral code generation
│   ├── __init__.py
│   ├── base_generator.py           # Abstract base class
│   ├── gpio_generator.py           # GPIO peripheral
│   ├── uart_generator.py           # UART peripheral
│   ├── spi_generator.py            # SPI peripheral
│   ├── i2c_generator.py            # I2C peripheral
│   ├── adc_generator.py            # ADC peripheral
│   ├── timer_generator.py          # Timer peripheral
│   ├── dma_generator.py            # DMA controller
│   ├── startup_generator.py        # Startup code
│   ├── peripheral_generator.py     # Peripheral addresses
│   ├── register_generator.py       # Register definitions
│   └── unified_generator.py        # Orchestrator
│
├── vendors/                        # VENDORS - Vendor-specific logic
│   ├── __init__.py
│   ├── st/                         # STMicroelectronics
│   │   ├── __init__.py
│   │   ├── generate_all_st_pins.py
│   │   ├── generate_stm32_pins.py
│   │   ├── stm32f1_pin_functions.py
│   │   ├── stm32f4_pin_functions.py
│   │   └── stm32f7_pin_functions.py
│   ├── atmel/                      # Atmel/Microchip
│   │   ├── __init__.py
│   │   ├── generate_all_atmel.py
│   │   ├── generate_same70_pins.py
│   │   ├── same70_pin_functions.py
│   │   └── samd21_pin_functions.py
│   ├── raspberrypi/                # Raspberry Pi
│   │   ├── __init__.py
│   │   ├── generate_rp2040_pins.py
│   │   └── rp2040_pin_functions.py
│   └── espressif/                  # Espressif
│       ├── __init__.py
│       ├── generate_esp32_pins.py
│       └── esp32_pin_functions.py
│
├── parsers/                        # PARSERS - SVD and metadata
│   ├── __init__.py
│   ├── generic_svd.py              # Generic SVD parser
│   ├── svd_discovery.py            # SVD file discovery
│   ├── svd_pin_extractor.py        # Pin extraction from SVD
│   └── list_svds.py                # SVD listing utility
│
├── templates/                      # TEMPLATES - Jinja2 templates
│   ├── peripheral/                 # Peripheral templates (library spec)
│   │   ├── gpio.hpp.j2
│   │   ├── uart.hpp.j2
│   │   ├── spi.hpp.j2
│   │   ├── i2c.hpp.j2
│   │   ├── adc.hpp.j2
│   │   ├── timer.hpp.j2
│   │   ├── startup.cpp.j2
│   │   └── peripherals.hpp.j2
│   └── project/                    # Project templates (CLI spec)
│       ├── blinky/
│       ├── uart_logger/
│       └── rtos_tasks/
│
├── metadata/                       # METADATA - Platform metadata
│   ├── schema/                     # JSON/YAML schemas (CLI spec)
│   │   ├── platform.schema.json
│   │   ├── board.schema.json
│   │   └── peripheral.schema.json
│   ├── mcus/                       # MCU metadata (YAML)
│   ├── boards/                     # Board metadata (YAML)
│   └── peripherals/                # Peripheral metadata (YAML)
│
├── tests/                          # TESTS - pytest test suite
│   ├── __init__.py
│   ├── conftest.py
│   ├── unit/                       # Unit tests
│   │   ├── test_phase3.py          # New CLI tests
│   │   ├── test_phase4.py
│   │   └── test_phase5.py
│   ├── test_svd_parser.py          # Old generation tests
│   ├── test_template_engine.py
│   ├── test_generators.py
│   └── fixtures/                   # Test fixtures
│       ├── test_svd_files/
│       └── expected_output/
│
├── scripts/                        # SCRIPTS - Utility scripts
│   ├── generate_from_svd.py
│   ├── migrate_json_to_yaml.py
│   ├── regenerate_all_startups.py
│   └── validate_metadata.py
│
├── docs/                           # DOCS - Generator documentation
│   ├── architecture.md
│   ├── adding_mcu.md
│   ├── template_reference.md
│   └── troubleshooting.md
│
├── pyproject.toml                  # Python project config
├── requirements.txt                # Dependencies
├── README.md                       # Quick start guide
├── CLI_USAGE.md                    # CLI separation guide
└── CLI_FEATURE_GAP_ANALYSIS.md     # Feature comparison
```

---

## 🔄 Migration Strategy

### Phase 1: Create New Structure (No Breaking Changes)
1. Create new directories
2. Copy (not move) files to new locations
3. Update imports in copied files
4. Keep old structure intact

### Phase 2: Validate New Structure Works
1. Test all generators with new paths
2. Run all existing tests
3. Verify builds still work

### Phase 3: Remove Old Files
1. Remove duplicates from old structure
2. Update references to point to new locations
3. Clean up empty directories

---

## 📝 Detailed Migration Map

### Core Files
```
OLD LOCATION                        → NEW LOCATION
────────────────────────────────────────────────────────────────
cli/core/svd_parser.py              → core/svd_parser.py
cli/core/template_engine.py         → core/template_engine.py
cli/core/schema_validator.py        → core/schema_validator.py
cli/core/file_utils.py              → core/file_utils.py
cli/core/config.py                  → core/config.py
cli/core/logger.py                  → core/logger.py
```

### Generators (Peripheral)
```
cli/generators/startup_generator.py → generators/startup_generator.py
cli/generators/generate_registers.py → generators/register_generator.py
cli/generators/generate_pin_functions.py → generators/pin_function_generator.py
cli/generators/generate_enums.py    → generators/enum_generator.py
cli/generators/unified_generator.py → generators/unified_generator.py
```

### Generators (Project) - Stay in CLI
```
cli/generators/project_generator.py → cli/generators/project_generator.py (KEEP)
cli/generators/template_engine.py   → cli/generators/template_engine.py (KEEP)
```

### Vendors
```
cli/vendors/st/*                    → vendors/st/* (KEEP IN PLACE)
cli/vendors/atmel/*                 → vendors/atmel/* (KEEP IN PLACE)
cli/vendors/raspberrypi/*           → vendors/raspberrypi/* (KEEP IN PLACE)
cli/vendors/espressif/*             → vendors/espressif/* (KEEP IN PLACE)
```

### Parsers
```
cli/parsers/generic_svd.py          → parsers/generic_svd.py (KEEP IN PLACE)
cli/parsers/svd_discovery.py        → parsers/svd_discovery.py (KEEP IN PLACE)
cli/parsers/svd_pin_extractor.py    → parsers/svd_pin_extractor.py (KEEP IN PLACE)
```

### Validators
```
cli/validators/*                    → cli/validators/* (KEEP - new implementation)

CREATE NEW:
core/validators/syntax_validator.py      (from old cli/core/validators/)
core/validators/semantic_validator.py    (from old cli/core/validators/)
core/validators/compile_validator.py     (from old cli/core/validators/)
core/validators/test_validator.py        (from old cli/core/validators/)
```

---

## ✅ Implementation Steps

### Step 1: Create Directory Structure (30min)
- [ ] Create `core/` directory
- [ ] Create `core/validators/` directory
- [ ] Create `generators/` directory
- [ ] Create `templates/peripheral/` directory
- [ ] Create `templates/project/` directory
- [ ] Create `metadata/` directory
- [ ] Create `docs/` directory

### Step 2: Move Core Files (1h)
- [ ] Move SVD parser to `core/`
- [ ] Move template engine (old) to `core/`
- [ ] Move schema validator to `core/`
- [ ] Move file utilities to `core/`
- [ ] Update imports in moved files

### Step 3: Create Core Validators (2h)
- [ ] Create `core/validators/__init__.py`
- [ ] Create `core/validators/syntax_validator.py`
- [ ] Create `core/validators/semantic_validator.py`
- [ ] Create `core/validators/compile_validator.py`
- [ ] Create `core/validators/test_validator.py`

### Step 4: Move Generators (1h)
- [ ] Move peripheral generators to `generators/`
- [ ] Keep project generators in `cli/generators/`
- [ ] Update imports
- [ ] Create base_generator.py

### Step 5: Organize Templates (30min)
- [ ] Move peripheral templates to `templates/peripheral/`
- [ ] Keep project templates in `templates/project/`
- [ ] Update template paths in generators

### Step 6: Update Imports (2h)
- [ ] Update imports in `codegen.py`
- [ ] Update imports in generators
- [ ] Update imports in tests
- [ ] Update imports in CLI commands

### Step 7: Test Migration (1h)
- [ ] Run old CLI: `python3 codegen.py --help`
- [ ] Run new CLI: `alloy --help`
- [ ] Test code generation
- [ ] Run all tests
- [ ] Verify builds work

### Step 8: Document Changes (1h)
- [ ] Update README.md
- [ ] Create architecture.md
- [ ] Update CLI_USAGE.md
- [ ] Document new structure

---

## 🎯 Success Criteria

- [ ] Both CLIs work (codegen.py and alloy)
- [ ] All tests pass
- [ ] Code generation works
- [ ] Builds complete successfully
- [ ] Clear separation of concerns
- [ ] Documentation updated
- [ ] CLI team can integrate ValidationService

---

## 📊 Timeline

**Total Estimated Time**: 9 hours

| Task | Duration | Status |
|------|----------|--------|
| Create directories | 30min | 🔄 In Progress |
| Move core files | 1h | ⏳ Pending |
| Create core validators | 2h | ⏳ Pending |
| Move generators | 1h | ⏳ Pending |
| Organize templates | 30min | ⏳ Pending |
| Update imports | 2h | ⏳ Pending |
| Test migration | 1h | ⏳ Pending |
| Document changes | 1h | ⏳ Pending |

---

## 🔗 CLI Integration Points

After Phase 4 completion, CLI can:

1. ✅ Import core validators:
   ```python
   from core.validators import (
       SyntaxValidator,
       SemanticValidator,
       CompileValidator,
       TestValidator
   )
   ```

2. ✅ Create ValidationService wrapper:
   ```python
   # cli/services/validation_service.py
   class ValidationService:
       def __init__(self):
           self.syntax = SyntaxValidator()
           self.semantic = SemanticValidator()
           # ...
   ```

3. ✅ Access peripheral generators for metadata:
   ```python
   from generators import GpioGenerator, UartGenerator
   ```

4. ✅ Use template engine for project templates:
   ```python
   from cli.generators.template_engine import TemplateEngine
   ```

---

**Status**: Ready to implement
**Next Step**: Create directory structure
