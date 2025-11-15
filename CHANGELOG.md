# Changelog

All notable changes to Alloy Framework will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Nothing yet

### Changed
- Nothing yet

### Deprecated
- Nothing yet

### Removed
- Nothing yet

### Fixed
- Nothing yet

### Security
- Nothing yet

---

## [1.0.0] - 2025-11-15

### Project Consolidation Release

This is the first official release after major project consolidation. The framework was renamed from CoreZero to Alloy and underwent extensive refactoring.

### Added

#### Core Features
- **Modern C++20 HAL**: Policy-based hardware abstraction layer with concepts
- **Multi-platform Support**: STM32F4, STM32F7, STM32G0, SAME70
- **Result<T,E> Type**: Rust-inspired error handling (zero exceptions)
- **C++20 Concepts**: Compile-time API validation

#### Supported Boards
- Nucleo-F401RE (STM32F4 @ 84 MHz)
- Nucleo-F722ZE (STM32F7 @ 216 MHz)
- Nucleo-G071RB (STM32G0 @ 64 MHz)
- Nucleo-G0B1RE (STM32G0 @ 64 MHz)

#### Code Generation
- Unified SVD-to-C++ code generator (`tools/codegen/codegen.py`)
- Multi-vendor support (ST, Atmel, Espressif, Raspberry Pi)
- Template-based generation with Jinja2
- Metadata tracking for generated files

#### Testing Infrastructure
- 49 unit tests (core functionality)
- 20 integration tests (component interaction)
- 31 regression tests (bug documentation)
- 3 hardware validation tests (bare-metal LED/button tests)
- Total: 103 tests with Catch2 v3

#### CI/CD
- Multi-OS builds (Ubuntu + macOS)
- Multi-compiler (GCC 11 + Clang 14)
- Embedded builds for all boards
- Code quality gates (clang-format, clang-tidy, cppcheck)
- Automated release workflow

#### Documentation
- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - 815 lines
- [PORTING_NEW_BOARD.md](docs/PORTING_NEW_BOARD.md) - 726 lines
- [PORTING_NEW_PLATFORM.md](docs/PORTING_NEW_PLATFORM.md) - 876 lines
- [CODE_GENERATION.md](docs/CODE_GENERATION.md) - 1181 lines
- [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) - Complete migration guide

### Changed

#### Directory Structure
- Moved generated files to `/generated/` subdirectories
- Consolidated `platform/` → `vendors/` directory structure
- Organized by vendor (st, arm, atmel, espressif, raspberrypi)

**Before**:
```
src/hal/platform/st/stm32f4/
├── registers/
└── bitfields/
```

**After**:
```
src/hal/vendors/st/stm32f4/
├── generated/
│   ├── registers/
│   └── bitfields/
├── gpio.hpp
└── clock_platform.hpp
```

#### API Standardization
- **GPIO API**: Added `write(bool)` method, `read()` returns `Result<bool, ErrorCode>`
- **Clock API**: All methods return `Result<void, ErrorCode>`
- **Clock API**: Added `enable_uart_clock()`, `enable_spi_clock()`, `enable_i2c_clock()`
- **Concepts**: Added `ClockPlatform` and `GpioPin` concepts for compile-time validation

#### Build System
- Platform selection: `ALLOY_PLATFORM` (linux, stm32f4, stm32f7, stm32g0)
- Board selection: `ALLOY_BOARD` (nucleo_f401re, nucleo_f722ze, etc.)
- Explicit source lists (removed excessive GLOB usage)
- Build system validation (`validate-build-system` target)

#### Testing
- Migrated from Google Test to Catch2 v3
- Organized tests: unit/ integration/ regression/ hardware/
- All tests use `Result<T,E>` pattern consistently

### Deprecated

- Old `COREZERO_*` CMake variables (use `ALLOY_*` instead)
- Platform name `host` (use `linux` instead)
- Direct include of generated files without `/generated/` path

### Removed

- `src/hal/platform/` directory (consolidated to `vendors/`)
- Old code generation scripts (replaced by unified `codegen.py`)
- Google Test framework (migrated to Catch2 v3)
- Manual test frameworks (all tests now use Catch2)

### Fixed

- **30+ bugs** documented and tested in regression test suite:
  - GPIO read/write edge cases
  - Clock initialization race conditions
  - UART buffer overflow handling
  - SPI/I2C timing issues
  - Result<T,E> error propagation

### Security

- No security vulnerabilities in this release
- Static analysis passes (clang-tidy + cppcheck)
- Zero dynamic allocation in core HAL
- No unsafe C-style casts

---

## Technical Details

### Build Artifacts

Each release includes:
- Binary packages for all supported boards (.elf, .bin, .hex, .map)
- Size reports (Flash/RAM usage)
- SHA256 checksums
- Version metadata
- Documentation package

### Test Coverage

| Category | Count | Coverage |
|----------|-------|----------|
| Unit Tests | 49 | Core functionality |
| Integration Tests | 20 | Component interaction |
| Regression Tests | 31 | Bug prevention |
| Hardware Tests | 3 | Bare-metal validation |
| **Total** | **103** | **Comprehensive** |

### Binary Sizes (MinSizeRel)

| Board | Blink Example | Flash | RAM |
|-------|---------------|-------|-----|
| Nucleo-F401RE | blink.elf | 3028 bytes | 112 bytes |
| Nucleo-F722ZE | blink.elf | 3028 bytes | 112 bytes |
| Nucleo-G071RB | blink.elf | 2632 bytes | 96 bytes |
| Nucleo-G0B1RE | blink.elf | 2632 bytes | 96 bytes |

### Platforms

| Platform | MCU Family | Clock Speed | Status |
|----------|------------|-------------|--------|
| STM32F4 | Cortex-M4 | up to 84 MHz | ✅ Tested |
| STM32F7 | Cortex-M7 | up to 216 MHz | ✅ Tested |
| STM32G0 | Cortex-M0+ | up to 64 MHz | ✅ Tested |
| SAME70 | Cortex-M7 | up to 300 MHz | ⏸️ Pending |

### Code Generation

- **759 generated files** across all platforms
- **677 hand-written files** (drivers, policies, boards)
- SVD sources: CMSIS-SVD repository
- Template engine: Jinja2
- Generation time: ~2 seconds for all platforms

---

## Migration from CoreZero

See [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) for complete migration instructions.

**Quick summary**:
1. Update includes: `hal/platform/` → `hal/vendors/` + `/generated/`
2. Update CMake variables: `COREZERO_*` → `ALLOY_*`
3. Update GPIO API: `read()` now returns `Result<bool, ErrorCode>`
4. Update Clock API: All methods return `Result<void, ErrorCode>`
5. Migrate tests: Google Test → Catch2 v3

---

## Contributors

- Primary development: Alloy Framework Team
- Code generation: Based on CMSIS-SVD
- Inspired by: Rust embedded ecosystem, Modern C++ best practices

---

## Links

- **Repository**: https://github.com/your-org/corezero
- **Documentation**: https://github.com/your-org/corezero/tree/main/docs
- **Issues**: https://github.com/your-org/corezero/issues
- **Releases**: https://github.com/your-org/corezero/releases

---

**Versioning**: We use [Semantic Versioning](https://semver.org/):
- **Major** version (X.0.0): Breaking API changes
- **Minor** version (0.X.0): New features, backward compatible
- **Patch** version (0.0.X): Bug fixes, backward compatible

---

[Unreleased]: https://github.com/your-org/corezero/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/your-org/corezero/releases/tag/v1.0.0
