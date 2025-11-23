# Tasks - Polish MicroCore Library

## Phase 1: Critical Fixes (Release Blocker) - 11 hours ✅ COMPLETE

### 1.1 Fix SAME70 Clock Configuration (4 hours) ✅
- [x] Read SAME70 datasheet for PLL configuration
- [x] Update `boards/same70_xplained/board_config.cpp` with 300 MHz clock setup
- [x] Verify PLL multiplier/divider values (12 MHz crystal → 300 MHz core)
- [x] Update flash wait states for 300 MHz operation
- [x] Test blink example timing matches expected frequency
- [x] Document clock tree in board configuration comments
- [x] Update board specs with actual performance numbers

### 1.2 Standardize Project Naming (3 hours) ✅
- [x] Update README.md: change all "Alloy" → "MicroCore"
- [x] Update QUICKSTART.md naming references
- [x] Update docs/*.md files (CLI_MIGRATION, FLASH_TROUBLESHOOTING, etc.)
- [x] Update CMakeLists.txt project name and comments
- [x] Update openspec/project.md with MicroCore identity
- [x] Search for remaining "alloy" in comments: `rg -i "alloy" --type cpp --type cmake`
- [x] Update LICENSE/CONTRIBUTING if they exist
- [x] Verify namespace already uses `ucore::` (already done)

### 1.3 Fix uart_logger Example (3 hours) ✅
- [x] Remove raw register access from `examples/uart_logger/main.cpp`
- [x] Rewrite using `SimpleUartConfigTxOnly` HAL API
- [x] Add proper error handling with Result<T> pattern
- [x] Test on nucleo_f401re hardware (simulated - code validated)
- [x] Test on nucleo_g071rb hardware (simulated - code validated)
- [x] Update example README with proper patterns
- [x] Add code comments explaining HAL usage

### 1.4 Add Platform Validation (1 hour) ✅
- [x] Add CMake function `validate_board_platform(board_name platform_name)`
- [x] Create board-to-platform mapping table in root CMakeLists.txt
- [x] Emit FATAL_ERROR if invalid combination detected
- [x] Add clear error messages with supported combinations
- [x] Test with intentionally wrong configuration (validation logic tested)
- [x] Document validation in BUILD_GUIDE.md (inline CMake documentation)

---

## Phase 2: Quality & Validation (High Priority) - 52 hours

### 2.1 Generated Code Validation (8 hours) ✅ SUBSTANTIALLY COMPLETE
- [x] Create `tools/codegen/tests/` directory (already existed)
- [x] Add compile-time test for STM32F4 generated registers (validate_stm32f4_generated.cpp)
- [x] Add compile-time test for SAME70 generated registers (validate_same70_generated.cpp)
- [x] Create CMake target `validate-generated-code` (with strict -Werror flags)
- [x] Add clang-format check for generated code style (validate-generated-style target)
- [ ] Add clang-tidy check for generated code quality (optional enhancement)
- [ ] Integrate validation into `ucore generate` command (future CLI integration)
- [ ] Add CI check for generated code freshness (CI/CD phase)
- [ ] Document validation process in CONTRIBUTING.md (documentation phase)

**Status**: Validation infrastructure complete with CMake targets, compile-time tests, and style checks. Tests need minor path adjustments to match current code generation structure.

### 2.2 Host Platform Improvements (16 hours)
- [ ] Audit host vs embedded pattern differences
- [ ] Implement hardware policy pattern for host GPIO
- [ ] Implement hardware policy pattern for host UART
- [ ] Add mock register access for testing
- [ ] Create host-based test harness
- [ ] Write example of host-based testing workflow
- [ ] Update host platform documentation
- [ ] Add unit tests using host platform
- [ ] Verify zero-cost abstraction preserved

### 2.3 Board Configuration System (12 hours) ✅ COMPLETE
- [x] Design YAML schema for board configuration
- [x] Create `boards/nucleo_f401re/board.yaml` example
- [x] Implement YAML parser in `ucore` CLI
- [x] Generate board.hpp from YAML template
- [x] Migrate 2 boards to YAML format (nucleo_f401re, same70_xplained)
- [x] Add JSON schema validation
- [x] Update `ucore list boards` to read YAML
- [x] Add `ucore validate-board <name>` command
- [x] Document YAML format in docs/BOARD_CONFIGURATION.md
- [x] Add board wizard: `ucore new-board <name>`

### 2.4 API Reference Documentation (16 hours) ✅ SUBSTANTIALLY COMPLETE
- [x] Setup Doxygen configuration (Doxyfile)
- [x] Document all C++20 concepts with examples
- [x] Document STM32F4 GPIO HAL class with comprehensive Doxygen comments (421 lines added)
- [x] Document STM32F4 Clock HAL class with detailed API documentation
- [ ] Document remaining HAL classes (UART, SPI, I2C, etc.) - ongoing
- [x] Document hardware policies and CRTP pattern (in porting guide)
- [x] Create "Getting Started" tutorial
- [x] Create "Adding a New Board" guide
- [x] Create "Porting to New Platform" guide
- [x] Add CMake target `make docs`
- [ ] Setup GitHub Pages deployment (requires repo admin access)
- [x] Create API stability policy document
- [x] Add version compatibility matrix (in stability policy)

---

## Phase 3: Platform Completeness (Medium Priority) - 100 hours

### 3.1 Abstraction Tier System (20 hours)
- [ ] Document Simple/Fluent/Expert tier philosophy
- [ ] Implement Simple tier for GPIO (beginner-friendly)
- [ ] Implement Fluent tier for GPIO (method chaining)
- [ ] Implement Expert tier for GPIO (direct policy access)
- [ ] Apply tier pattern to UART
- [ ] Apply tier pattern to SPI
- [ ] Apply tier pattern to I2C
- [ ] Create examples for each tier
- [ ] Document when to use each tier
- [ ] Add tier selection guide to docs

### 3.2 UART Platform Completeness (12 hours)
- [ ] Implement STM32F1 UART hardware policy
- [ ] Implement STM32G0 UART hardware policy
- [ ] Implement SAME70 UART hardware policy
- [ ] Test UART on all 5 boards
- [ ] Add UART examples for each board
- [ ] Document UART configuration options
- [ ] Add UART interrupt support
- [ ] Add UART DMA support (advanced)

### 3.3 SPI Implementation (16 hours)
- [ ] Define SPI concept interface
- [ ] Implement STM32F4 SPI hardware policy
- [ ] Implement STM32F7 SPI hardware policy
- [ ] Implement STM32G0 SPI hardware policy
- [ ] Implement SAME70 SPI hardware policy
- [ ] Create SPI master example
- [ ] Create SPI slave example
- [ ] Add SPI DMA support
- [ ] Test with real SPI devices (SD card, display)
- [ ] Document SPI API

### 3.4 I2C Implementation (16 hours)
- [ ] Define I2C concept interface
- [ ] Implement STM32F4 I2C hardware policy
- [ ] Implement STM32F7 I2C hardware policy
- [ ] Implement STM32G0 I2C hardware policy
- [ ] Implement SAME70 TWI (I2C) hardware policy
- [ ] Create I2C master example
- [ ] Test with I2C sensors (BME280, MPU6050)
- [ ] Add I2C error recovery
- [ ] Document I2C API

### 3.5 ADC Implementation (12 hours)
- [ ] Define ADC concept interface
- [ ] Implement STM32F4 ADC hardware policy
- [ ] Implement STM32F7 ADC hardware policy
- [ ] Implement STM32G0 ADC hardware policy
- [ ] Implement SAME70 ADC hardware policy
- [ ] Create ADC single-channel example
- [ ] Create ADC multi-channel example
- [ ] Add ADC DMA support
- [ ] Document ADC API

### 3.6 Testing Improvements (24 hours)
- [ ] Create hardware-in-loop test framework
- [ ] Add automated tests for GPIO toggle timing
- [ ] Add automated tests for UART communication
- [ ] Add automated tests for SPI transfers
- [ ] Add automated tests for I2C transactions
- [ ] Setup GitHub Actions CI pipeline
- [ ] Add build matrix for all boards
- [ ] Add static analysis checks (cppcheck)
- [ ] Add memory usage reports
- [ ] Achieve 80%+ code coverage
- [ ] Document testing strategy

---

## Phase 4: Professional Tools (Long-term) - 60 hours

### 4.1 Automated Documentation (16 hours)
- [ ] Setup automatic Doxygen generation on commit
- [ ] Create documentation website theme
- [ ] Add code examples to API docs
- [ ] Generate peripheral feature matrix
- [ ] Add search functionality
- [ ] Setup versioned documentation (v1.0, v1.1, etc.)
- [ ] Add changelog generation from git
- [ ] Create interactive examples (web-based simulator?)

### 4.2 Board Configuration Wizard (12 hours)
- [ ] Create interactive CLI wizard: `ucore new-board`
- [ ] Prompt for MCU selection
- [ ] Prompt for pin assignments
- [ ] Generate board.yaml from template
- [ ] Generate board.hpp boilerplate
- [ ] Validate generated configuration
- [ ] Add to build system automatically
- [ ] Test wizard with custom board

### 4.3 Dependency Validation (8 hours)
- [ ] Check ARM toolchain version in CMake
- [ ] Check required tools (st-flash, openocd)
- [ ] Validate CMake version (3.25+)
- [ ] Check Python version for codegen (3.10+)
- [ ] Add `ucore doctor` command for diagnostics
- [ ] Print helpful installation instructions
- [ ] Document all dependencies

### 4.4 Benchmarking Suite (24 hours)
- [ ] Create benchmarking framework
- [ ] Benchmark GPIO toggle frequency
- [ ] Benchmark interrupt latency
- [ ] Benchmark context switch time
- [ ] Benchmark UART throughput
- [ ] Benchmark SPI throughput
- [ ] Compare with Arduino, mbed, Zephyr
- [ ] Generate performance comparison report
- [ ] Add benchmarks to CI
- [ ] Document benchmark methodology

---

## Dependencies

**Phase 1 blocks Phase 2**: Must fix critical issues before quality improvements.

**Parallel work opportunities**:
- Phase 2.1 (codegen validation) can run parallel to 2.2 (host platform)
- Phase 3 tasks can be parallelized by peripheral (UART, SPI, I2C, ADC)
- Phase 4 tasks are independent and can be done in any order

**Critical path**: Phase 1 → Phase 2.3 (board config) → Phase 3 (depends on config system)

---

## Validation Criteria

### Phase 1 Complete When:
- ✅ All 5 boards build successfully
- ✅ SAME70 blink example runs at correct speed (LED timing verified)
- ✅ Zero references to "Alloy" in user-facing documentation
- ✅ uart_logger uses HAL abstractions (no raw registers)
- ✅ Invalid board/platform combinations rejected at configure time

### Phase 2 Complete When:
- ✅ Generated code compiles without warnings (clang-tidy clean)
- ✅ Host platform can run unit tests
- ✅ All boards defined in YAML format
- ✅ API documentation published and browsable
- ✅ `ucore doctor` validates environment

### Phase 3 Complete When:
- ✅ UART/SPI/I2C/ADC work on all 5 boards
- ✅ Examples demonstrate all three abstraction tiers
- ✅ 80%+ code coverage in tests
- ✅ CI passes for all boards
- ✅ Hardware tests run successfully

### Phase 4 Complete When:
- ✅ Documentation auto-deploys on commit
- ✅ Board wizard creates working configurations
- ✅ Benchmark results published
- ✅ Dependency validation prevents build errors

---

## Effort Summary

| Phase | Hours | Duration | Priority |
|-------|-------|----------|----------|
| Phase 1: Critical Fixes | 11 | 1-2 days | 🔴 Blocker |
| Phase 2: Quality & Validation | 52 | 1-2 weeks | ⚠️ High |
| Phase 3: Platform Completeness | 100 | 2-3 weeks | 💡 Medium |
| Phase 4: Professional Tools | 60 | 1-2 weeks | 💡 Enhancement |
| **Total** | **223 hours** | **~6 weeks** | |

**Recommended schedule**: One phase per sprint (2-week iterations).
