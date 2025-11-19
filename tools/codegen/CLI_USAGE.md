# Alloy CLI Usage Guide

**Last Updated**: 2025-11-19

This directory contains **two separate CLIs** for different purposes:

---

## 📋 Two CLIs Overview

### 1. Code Generation CLI (Legacy - `codegen.py`)

**Purpose**: Generate MCU HAL code from SVD files

**Entry Point**: `python3 codegen.py`

**Use Cases**:
- Generate startup code for MCUs
- Generate register definitions from SVD files
- Generate pin headers and alternate functions
- Generate peripheral enumerations
- Batch generate code for multiple vendors

**Status**: ✅ Active - Critical for code generation

---

### 2. Project Development CLI (New - `alloy`)

**Purpose**: Professional development tool for embedded projects

**Entry Point**: `alloy` (or `python3 -m cli.main`)

**Use Cases**:
- Discover and search MCUs/boards
- Initialize new projects with templates
- Build and flash firmware
- Validate generated code
- Access documentation

**Status**: ✅ Active - Primary development interface

---

## 🚀 Quick Start

### Code Generation (Legacy CLI)

```bash
# Generate all code (RECOMMENDED for first time)
python3 codegen.py generate-complete

# Generate specific components
python3 codegen.py generate --startup      # Startup code only
python3 codegen.py generate --registers    # Register definitions
python3 codegen.py generate --pins         # Pin headers

# Generate for specific vendor
python3 codegen.py generate --pins --vendor st
python3 codegen.py generate --pins --vendor atmel

# See status
python3 codegen.py status

# Clean generated files
python3 codegen.py clean --dry-run

# Test SVD parser
python3 codegen.py test-parser STMicro/STM32F103.svd
```

### Project Development (New CLI)

```bash
# Discover MCUs and boards
alloy list mcus
alloy list mcus --family stm32f4 --min-flash 256
alloy show mcu STM32F401RET6
alloy search mcu "USB + 512KB"

# List boards
alloy list boards
alloy show board nucleo_f401re
alloy show pinout nucleo_f401re

# Initialize project
alloy init                           # Interactive wizard
alloy init --board nucleo_f401re --template blinky
alloy init --list-templates

# Build and flash
alloy build configure
alloy build compile
alloy build flash
alloy build size
alloy build clean

# Access documentation
alloy docs datasheet STM32F4
alloy docs reference STM32F4
alloy docs examples
alloy docs examples --peripheral UART

# Validate code
alloy validate file src/main.cpp
alloy validate dir src/
```

---

## 🎯 When to Use Which CLI

### Use `codegen.py` (Legacy) when:
- ✅ Adding new MCU support to the library
- ✅ Regenerating HAL code from updated SVD files
- ✅ Generating vendor-specific code (ST, Atmel, etc.)
- ✅ Batch generating code for multiple MCU families
- ✅ Testing SVD file parsing

### Use `alloy` (New) when:
- ✅ Starting a new embedded project
- ✅ Discovering available MCUs and boards
- ✅ Building and flashing firmware
- ✅ Looking up MCU specs or documentation
- ✅ Validating generated or user code

---

## 📁 Directory Structure

```
tools/codegen/
├── codegen.py              # Legacy CLI entry point (code generation)
├── cli/
│   ├── main.py            # New CLI entry point (alloy)
│   ├── commands/          # CLI commands (both old and new)
│   │   ├── codegen.py     # Code generation command (old)
│   │   ├── status.py      # Status command (old)
│   │   ├── vendors.py     # Vendors command (old)
│   │   ├── clean.py       # Clean command (old)
│   │   ├── init_cmd.py    # Init command (new)
│   │   ├── build_cmd.py   # Build commands (new)
│   │   ├── docs_cmd.py    # Docs commands (new)
│   │   ├── list_cmd.py    # List commands (new)
│   │   ├── show_cmd.py    # Show commands (new)
│   │   └── ...
│   ├── vendors/           # Vendor-specific generators (old)
│   │   ├── st/            # STMicroelectronics
│   │   ├── atmel/         # Atmel/Microchip
│   │   ├── raspberrypi/   # Raspberry Pi (RP2040)
│   │   └── espressif/     # Espressif (ESP32)
│   ├── generators/        # Code generators (old + new)
│   │   ├── startup_generator.py        # Startup code (old)
│   │   ├── generate_registers.py       # Registers (old)
│   │   ├── generate_pin_functions.py   # Pin functions (old)
│   │   ├── project_generator.py        # Projects (new)
│   │   └── template_engine.py          # Templates (new)
│   ├── parsers/           # SVD parsers (old)
│   ├── services/          # Services (new)
│   │   ├── mcu_service.py
│   │   ├── board_service.py
│   │   ├── build_service.py
│   │   └── ...
│   ├── validators/        # Validators (new - improved)
│   └── models/            # Pydantic models (new)
└── tests/
    ├── unit/              # New test suite (Phase 3-5)
    └── test_*.py          # Old code generation tests
```

---

## 🔧 Code Generation Details (Legacy CLI)

### What Gets Generated

#### 1. Startup Code (`startup_*.cpp`)
```cpp
// Vector table
extern "C" {
    void Reset_Handler();
    void NMI_Handler() __attribute__((weak));
    // ... all interrupt handlers
}

// Reset handler
void Reset_Handler() {
    // Copy .data section
    // Zero .bss section
    // Call static constructors
    // Call main()
}
```

#### 2. Register Definitions (`*_registers.hpp`)
```cpp
// Peripheral base addresses
constexpr uintptr_t GPIOA_BASE = 0x40020000UL;

// Register structures
struct GPIO_TypeDef {
    volatile uint32_t MODER;      // 0x00
    volatile uint32_t OTYPER;     // 0x04
    // ... all registers with bitfields
};
```

#### 3. Pin Headers (`pin_functions.hpp`)
```cpp
// GPIO definitions
namespace gpio {
    constexpr Pin PA5 = {Port::A, 5};
    constexpr Pin PA6 = {Port::A, 6};
}

// Alternate functions
enum class AF {
    AF0_SYSTEM = 0,
    AF1_TIM1 = 1,
    AF2_TIM3 = 2,
    // ...
};
```

#### 4. Peripheral Enums (`*_enums.hpp`)
```cpp
enum class UART : uint8_t {
    UART1 = 0,
    UART2 = 1,
    UART3 = 2,
};

enum class SPI : uint8_t {
    SPI1 = 0,
    SPI2 = 1,
};
```

### Generate-Complete Pipeline

```bash
python3 codegen.py generate-complete
```

**Steps**:
1. Generate vendor code (pins, startup, registers, enums)
2. Generate platform HAL implementations
3. Format all code with `clang-format`
4. Validate with `clang-tidy`

**Options**:
- `--skip-format` - Skip formatting
- `--skip-validate` - Skip validation
- `--continue-on-error` - Don't stop on errors

---

## 🆕 Project Development Details (New CLI)

### Interactive Project Initialization

```bash
alloy init
```

**Wizard Steps**:
1. Select board (nucleo_f401re, same70_xplained, etc.)
2. Select template (blinky, uart_logger, rtos_tasks)
3. Enter project name
4. Choose build system (CMake/Meson)
5. Select optimization (debug/release/size)

**Generated Structure**:
```
my_project/
├── CMakeLists.txt
├── src/
│   └── main.cpp
├── include/
├── boards/
└── .gitignore
```

### Build Integration

```bash
# Configure (auto-detect CMake/Meson)
alloy build configure

# Compile with progress bar
alloy build compile

# Flash with auto-detected programmer
alloy build flash

# Show binary size analysis
alloy build size

# Clean build artifacts
alloy build clean
```

### Documentation Access

```bash
# Open datasheet in browser
alloy docs datasheet STM32F4

# Open reference manual
alloy docs reference STM32F4

# List examples
alloy docs examples
alloy docs examples --category basic
alloy docs examples --peripheral UART

# Show example details
alloy docs example uart_echo
```

---

## 🧪 Testing

### Test Code Generation (Legacy)
```bash
# Test individual generators
pytest tests/test_startup_generation.py
pytest tests/test_register_generation.py
pytest tests/test_pin_generation.py

# Test compilation
pytest tests/test_compilation.py
```

### Test Project Development (New)
```bash
# Test new CLI phases
pytest tests/unit/test_phase3.py  # Templates, wizard, pins
pytest tests/unit/test_phase4.py  # Build, flash
pytest tests/unit/test_phase5.py  # Docs, pinouts
```

---

## 🔮 Future Plans

### Phase 7: Unified Code Generation (Planned - 40h)

Goal: Merge code generation into `alloy` CLI

```bash
# Future commands (not yet implemented)
alloy generate startup --mcu STM32F401RET6
alloy generate registers --family stm32f4
alloy generate pins --vendor st
alloy generate-complete  # Full pipeline
```

**Status**: Not started - use `codegen.py` for now

---

## 📚 Additional Resources

- **CLI Feature Gap Analysis**: See `CLI_FEATURE_GAP_ANALYSIS.md`
- **Cleanup Plan**: See `CLEANUP_PLAN.md`
- **OpenSpec Tasks**: See `../openspec/changes/enhance-cli-professional-tool/tasks.md`

---

## ❓ FAQ

### Q: Why two CLIs?

**A**: The legacy `codegen.py` generates MCU HAL code from SVD files. The new `alloy` CLI is for project development. Code generation will be integrated into `alloy` in the future (Phase 7).

### Q: Which CLI should I use?

**A**:
- Use `codegen.py` for generating MCU HAL code
- Use `alloy` for day-to-day project development

### Q: Can I delete `codegen.py`?

**A**: ❌ NO - It's critical for code generation. The new CLI cannot generate code from SVD files yet.

### Q: When will they be unified?

**A**: When Phase 7 is implemented (estimated 40 hours of work). This is optional and not currently scheduled.

### Q: Which CLI is better?

**A**: They serve different purposes:
- `codegen.py` - Better for code generation (only option)
- `alloy` - Better for everything else (discovery, init, build, docs)

---

## 🤝 Contributing

When adding new features:

- **Code generation** → Add to `codegen.py` and `cli/generators/`
- **Project development** → Add to `alloy` (cli/commands/, cli/services/)
- **Validators** → Use new validators in `cli/validators/` (not `cli/core/validators/`)

---

**Last Updated**: 2025-11-19
**Maintained By**: Alloy Framework Team
