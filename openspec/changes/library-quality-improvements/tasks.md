# Tasks: Library Quality Improvements

**Change ID**: `library-quality-improvements`
**Status**: PROPOSED
**Last Updated**: 2025-01-17 (Integrated with Enhanced CLI)

---

## 🔗 Coordination Notice

**This spec is coordinated with `enhance-cli-professional-tool`**.

**Critical Dependency**: Phase 4 (Codegen Reorganization) MUST complete before CLI Phase 0 (YAML Migration) begins.

**Coordination Checkpoints**:
- ✅ **Before Phase 4.2**: Notify CLI team that validator interfaces are stable
- ✅ **After Phase 4.3**: Provide template engine API to CLI team
- ✅ **Before Phase 4.4**: Coordinate YAML metadata structure with CLI team
- ✅ **After Phase 4**: CLI team can begin Phase 0 (YAML Migration)

**Weekly Sync**: Required during parallel execution (weeks 4-12)

See `openspec/changes/INTEGRATION_LIBRARY_CLI.md` for full coordination plan.

---

## Overview

**Total Tasks**: 180+
**Total Estimated Time**: 232 hours (10 weeks)
**Complexity**: HIGH
**Priority**: HIGH

**Goal**: Transform Alloy from 7.3/10 to 9.0+/10 through systematic quality improvements

**⚠️ IMPLEMENTATION ORDER**:
1. **START HERE**: Phase 4 (Codegen Reorganization) - BLOCKS CLI development
2. After Phase 4 completes, CLI can start Phase 0 (YAML Migration)
3. Then Phases 1-3, 5-6 can run in parallel with CLI development

---

## Phase 1: API Layer Refactoring (3 weeks, 72 hours)

### 1.1 Design CRTP Base Classes (8h) ✅ COMPLETE

- [x] Research CRTP pattern for zero-overhead abstraction
  - [x] Study CRTP examples in modern C++
  - [x] Analyze performance characteristics
  - [x] Validate zero-overhead with assembly inspection
- [x] Design UartBase class structure
  - [x] Define protected implementation methods
  - [x] Define public interface methods
  - [x] Add compile-time checks
- [x] Design GpioBase class structure
  - [x] Support different GPIO architectures (STM32, SAME70)
  - [x] Handle pin configuration variations
  - [x] Add static assertions
- [x] Design SpiBase class structure
  - [x] Handle full-duplex vs half-duplex
  - [x] Support DMA integration
- [x] Design I2cBase class structure
  - [x] Handle master vs slave modes
  - [x] Support multi-master scenarios
- [x] Design AdcBase class structure
  - [x] Handle single vs multi-channel
  - [x] Support DMA and interrupts
- [x] Create base class template documentation
  - [x] Document template parameters
  - [x] Document usage patterns
  - [x] Add examples

**Deliverables**: `docs/architecture/CRTP_PATTERN.md` (480 lines)

### 1.2 Implement UartBase (8h) ✅ COMPLETE

- [x] Create `src/hal/api/uart_base.hpp`
  - [x] Implement CRTP base class
  - [x] Add send_impl() method
  - [x] Add receive_impl() method
  - [x] Add send_buffer_impl() method
  - [x] Add receive_buffer_impl() method
  - [x] Add flush_impl() method
  - [x] Add set_baud_rate_impl() method
  - [x] Add available_impl() method
- [x] Add comprehensive documentation
  - [x] Method descriptions
  - [x] Usage examples
  - [x] Performance notes
- [x] Add compile-time checks
  - [x] Static assert for zero size overhead
  - [x] Concept validation (UartImplementation)

**Deliverables**:
- `src/hal/api/uart_base.hpp` (400 lines)
- `docs/architecture/UART_CRTP_INTEGRATION.md` (464 lines)

### 1.3 Refactor UartSimple (4h) ✅ COMPLETE

- [x] Refactor `src/hal/api/uart_simple.hpp`
  - [x] Make SimpleUartConfig inherit from UartBase
  - [x] Make SimpleUartConfigTxOnly inherit from UartBase
  - [x] Implement all *_impl() methods
  - [x] Keep simple-specific methods (initialize, write_byte)
  - [x] Forward to base implementation
- [x] Validate API compatibility
  - [x] Check all methods still exist
  - [x] Verify CRTP inheritance works
  - [x] Test TX-only mode error handling
- [x] Create compile tests
  - [x] Test all simple API methods
  - [x] Test error handling
  - [x] Test CRTP zero-overhead
- [ ] Measure code size reduction (pending actual binary comparison)

**Deliverables**:
- `src/hal/api/uart_simple.hpp` (refactored with CRTP)
- `tests/compile_tests/test_uart_simple_crtp.cpp` (130 lines)
- `docs/architecture/UART_SIMPLE_REFACTORING_PLAN.md` (276 lines)

### 1.4 Refactor UartFluent (4h) ✅ COMPLETE

- [x] Refactor `src/hal/api/uart_fluent.hpp`
  - [x] Make FluentUartConfig inherit from UartBase
  - [x] Implement all *_impl() methods
  - [x] Keep fluent-specific methods (with_*, apply)
  - [x] Keep builder pattern (UartBuilder)
  - [x] Maintain method chaining
- [x] Validate fluent API works
  - [x] Test method chaining
  - [x] Test configuration accumulation
  - [x] Test preset methods (8n1, 8e1, 8o1)
  - [x] Test TX-only and RX-only modes
- [x] Create compile tests
  - [x] Test fluent pattern
  - [x] Test error handling
  - [x] Test CRTP inheritance
- [ ] Measure code size reduction (pending actual binary comparison)

**Deliverables**:
- `src/hal/api/uart_fluent.hpp` (refactored with CRTP)
- `tests/compile_tests/test_uart_fluent_crtp.cpp` (175 lines)

### 1.5 Refactor UartExpert (4h) ✅ COMPLETE

- [x] Refactor `src/hal/api/uart_expert.hpp`
  - [x] Create ExpertUartInstance inheriting from UartBase
  - [x] Implement all *_impl() methods
  - [x] Keep expert-specific methods (config validation)
  - [x] Maintain UartExpertConfig struct with constexpr validation
- [x] Validate expert API works
  - [x] Test compile-time validation
  - [x] Test preset configurations (standard_115200, logger_config, dma_config)
  - [x] Test error message generation
  - [x] Test TX-only mode
- [x] Create compile tests
  - [x] Test CRTP inheritance
  - [x] Test constexpr validation
  - [x] Test all preset methods
  - [x] Test validation helper functions
- [ ] Measure code size reduction (pending actual binary comparison)

**Deliverables**:
- `src/hal/api/uart_expert.hpp` (refactored with CRTP)
- `tests/compile_tests/test_uart_expert_crtp.cpp` (280 lines)

### 1.6 Implement GpioBase (8h) ✅ COMPLETE

- [x] Create `src/hal/api/gpio_base.hpp`
  - [x] Implement CRTP base class
  - [x] Add on_impl(), off_impl(), toggle_impl() methods (logical operations)
  - [x] Add is_on_impl() method for logical state
  - [x] Add set_impl(), clear_impl() methods (physical operations)
  - [x] Add read_impl() method
  - [x] Add set_direction_impl() method
  - [x] Add set_pull_impl() method
  - [x] Add set_drive_impl() method
- [x] Add convenience configuration methods
  - [x] configure_push_pull_output()
  - [x] configure_open_drain_output()
  - [x] configure_input_pullup/pulldown/floating()
- [x] Create compile tests
  - [x] Test CRTP inheritance
  - [x] Test logical vs physical operations
  - [x] Test active-high and active-low behavior
  - [x] Test configuration methods
  - [x] Test zero-overhead guarantee
  - [x] Test GpioImplementation concept

**Deliverables**:
- `src/hal/api/gpio_base.hpp` (450 lines)
- `tests/compile_tests/test_gpio_base_crtp.cpp` (410 lines)

**Note**: Platform differences are handled via HardwarePolicy pattern (injected at derived class level), not in base class.

### 1.7 Refactor GPIO APIs (12h) ✅ PARTIAL (Simple & Fluent complete)

- [x] Refactor GpioSimple
  - [x] Inherit from GpioBase via CRTP
  - [x] Implement all *_impl() methods
  - [x] Keep active-high/active-low abstraction
  - [x] Maintain factory methods (output, input, etc.)
  - [x] Create compile test
- [x] Refactor GpioFluent
  - [x] Make FluentGpioConfig inherit from GpioBase
  - [x] Implement all *_impl() methods (delegate to SimpleGpioPin)
  - [x] Keep fluent builder pattern (GpioBuilder)
  - [x] Maintain method chaining
  - [x] Fix initialize() method to work with CRTP
- [ ] Refactor GpioExpert (deferred)
  - [ ] Inherit from GpioBase
  - [ ] Keep expert-specific methods
  - [ ] Maintain compile-time validation
- [ ] Test on all boards (deferred to later phase)

**Deliverables**:
- `src/hal/api/gpio_simple.hpp` (refactored with CRTP, 280 lines)
- `src/hal/api/gpio_fluent.hpp` (refactored with CRTP, 405 lines)
- `tests/compile_tests/test_gpio_simple_crtp.cpp` (320 lines)

**Benefits**:
- Eliminates code duplication between Simple and Fluent APIs
- Both APIs share common implementation from GpioBase
- Zero runtime overhead maintained via CRTP
- Consistent API across GPIO levels

### 1.8 Implement SpiBase (6h) ✅ COMPLETE

- [x] Create `src/hal/api/spi_base.hpp`
  - [x] Implement CRTP base class
  - [x] Add transfer_impl() method (full-duplex)
  - [x] Add transmit_impl() method (TX-only)
  - [x] Add receive_impl() method (RX-only)
  - [x] Add configure_impl() method
  - [x] Add is_busy_impl() method
- [x] Add convenience single-byte methods
  - [x] transfer_byte(), transmit_byte(), receive_byte()
- [x] Add configuration convenience methods
  - [x] set_mode(), set_speed()
- [x] Handle full-duplex vs half-duplex (transfer vs transmit/receive)
- [x] Create compile tests
  - [x] Test CRTP inheritance
  - [x] Test all transfer operations
  - [x] Test single-byte operations
  - [x] Test configuration methods
  - [x] Test zero-overhead guarantee
  - [x] Test SpiImplementation concept

**Deliverables**:
- `src/hal/api/spi_base.hpp` (370 lines)
- `tests/compile_tests/test_spi_base_crtp.cpp` (300 lines)

**Benefits**:
- Provides common SPI interface for all API levels
- Zero runtime overhead via CRTP
- Type-safe via C++20 concepts
- Comprehensive error handling with Result<T, E>
- Ready for SPI API refactoring

**Note**: DMA support hooks deferred to individual implementations

### 1.9 Refactor SPI APIs (8h)

- [x] Refactor SpiSimple (3h) ✅ COMPLETE
  - [x] Make SimpleSpiConfig inherit from SpiBase via CRTP
  - [x] Make SimpleSpiTxConfig inherit from SpiBase via CRTP
  - [x] Implement all *_impl() methods for both configs
  - [x] Add constructors (protected base prevents aggregate init)
  - [x] Update quick_setup() to use constructors
  - [x] Update quick_setup_master_tx() to use constructors
  - [x] Handle TX-only limitations (transfer/receive return NotSupported)
  - [x] Create compile test
  - [x] Validate zero-overhead guarantee
- [ ] Refactor SpiFluent (2h)
- [ ] Refactor SpiExpert (3h)
- [ ] Test on all platforms

**Deliverables (Phase 1.9.1)**:
- `src/hal/api/spi_simple.hpp` (refactored with CRTP, 425 lines)
- `tests/compile_tests/test_spi_simple_crtp.cpp` (390 lines)

**Benefits**:
- SimpleSpiConfig and SimpleSpiTxConfig now share common SPI interface from SpiBase
- Zero runtime overhead maintained via CRTP
- TX-only configuration properly restricts unsupported operations
- Consistent API across SPI configuration types

### 1.10 Implement I2cBase (6h)

- [ ] Create `src/hal/api/i2c_base.hpp`
  - [ ] Implement CRTP base class
  - [ ] Add configure_impl() method
  - [ ] Add write_impl() method
  - [ ] Add read_impl() method
  - [ ] Add write_read_impl() method (repeated start)
  - [ ] Handle master vs slave
- [ ] Add 7-bit and 10-bit address support
- [ ] Add clock stretching support

### 1.11 Refactor I2C APIs (8h)

- [ ] Refactor I2cSimple (3h)
- [ ] Refactor I2cFluent (2h)
- [ ] Refactor I2cExpert (3h)
- [ ] Test on all platforms

### 1.12 Validation and Testing (8h)

- [ ] Write comprehensive unit tests
  - [ ] Test all base classes (16 tests)
  - [ ] Test all derived classes (48 tests)
  - [ ] Test cross-platform compatibility
  - [ ] Test error handling
- [ ] Validate zero overhead
  - [ ] Compile and inspect assembly
  - [ ] Verify no vtables generated
  - [ ] Verify same code size as handwritten
  - [ ] Benchmark performance
- [ ] Measure code reduction
  - [ ] Calculate per-peripheral reduction
  - [ ] Calculate total reduction
  - [ ] Target: 52% reduction (268KB → 129KB)
- [ ] Update documentation
  - [ ] API reference
  - [ ] Migration guide
  - [ ] Examples

**Phase 1 Deliverables**:
- ✅ 5 CRTP base classes (UART, GPIO, SPI, I2C, ADC)
- ✅ 15 refactored API files (5 peripherals × 3 APIs)
- ✅ 52% code reduction achieved
- ✅ 100% backward compatibility
- ✅ Test coverage >80%

---

## Phase 2: Template System Completion (2 weeks, 48 hours)

### 2.1 Template Architecture Design (4h)

- [ ] Define template variable schema
  - [ ] Platform metadata (name, vendor, family)
  - [ ] Peripheral metadata (base_address, registers)
  - [ ] Board metadata (pins, peripherals)
- [ ] Design template hierarchy
  - [ ] Base templates (common patterns)
  - [ ] Platform templates (peripheral-specific)
  - [ ] Board templates (board-specific)
- [ ] Create JSON schemas for validation
  - [ ] platform.schema.json
  - [ ] peripheral.schema.json
  - [ ] board.schema.json
- [ ] Document template conventions
  - [ ] Naming conventions
  - [ ] Directory structure
  - [ ] Variable naming

### 2.2 Create GPIO Template (8h)

- [ ] Design GPIO template structure
  - [ ] Support STM32-style (MODER, BSRR)
  - [ ] Support SAME70-style (PER, OER, SODR)
  - [ ] Support generic patterns
- [ ] Create `templates/platform/gpio.hpp.j2`
  - [ ] Port definitions
  - [ ] Pin class template
  - [ ] Set/clear/toggle methods
  - [ ] Direction configuration
  - [ ] Pull resistor configuration
  - [ ] Drive mode configuration
  - [ ] Filter enable/disable
- [ ] Create GPIO metadata schemas
  - [ ] `metadata/schema/gpio.schema.json`
  - [ ] Validate required fields
- [ ] Create GPIO metadata for all platforms
  - [ ] `metadata/stm32f4/gpio.json`
  - [ ] `metadata/same70/gpio.json`
  - [ ] `metadata/stm32g0/gpio.json`
- [ ] Test template generation
  - [ ] Generate for STM32F4
  - [ ] Generate for SAME70
  - [ ] Validate syntax with clang
  - [ ] Validate semantics with SVD
- [ ] Document GPIO template
  - [ ] Available variables
  - [ ] Customization points
  - [ ] Examples

### 2.3 Create UART Template (8h)

- [ ] Design UART template structure
  - [ ] Support different register layouts
  - [ ] Handle baud rate calculation differences
  - [ ] Support FIFO vs no-FIFO
- [ ] Create `templates/platform/uart.hpp.j2`
  - [ ] UART class template
  - [ ] Configure method
  - [ ] Send/receive methods
  - [ ] Baud rate calculation
  - [ ] Parity/stop bits configuration
  - [ ] FIFO configuration (if available)
  - [ ] DMA support hooks
  - [ ] Interrupt support hooks
- [ ] Create UART metadata for all platforms
  - [ ] `metadata/stm32f4/uart.json`
  - [ ] `metadata/same70/uart.json`
- [ ] Test template generation
  - [ ] Generate and validate
  - [ ] Test on real hardware
- [ ] Document UART template

### 2.4 Create SPI Template (6h)

- [ ] Design SPI template structure
  - [ ] Full-duplex vs half-duplex
  - [ ] Master vs slave
  - [ ] NSS handling (hardware vs software)
- [ ] Create `templates/platform/spi.hpp.j2`
  - [ ] SPI class template
  - [ ] Configure method
  - [ ] Transfer method
  - [ ] Mode configuration (CPOL, CPHA)
  - [ ] Speed configuration
  - [ ] Data size configuration (8/16 bit)
- [ ] Create SPI metadata
- [ ] Test template generation
- [ ] Document SPI template

### 2.5 Create I2C Template (6h)

- [ ] Design I2C template structure
  - [ ] Master vs slave modes
  - [ ] 7-bit vs 10-bit addressing
  - [ ] Clock stretching
- [ ] Create `templates/platform/i2c.hpp.j2`
  - [ ] I2C class template
  - [ ] Configure method
  - [ ] Write/read methods
  - [ ] Repeated start support
  - [ ] Multi-master support
- [ ] Create I2C metadata
- [ ] Test template generation
- [ ] Document I2C template

### 2.6 Create ADC/Timer/DMA Templates (12h)

- [ ] Create ADC template (4h)
  - [ ] Single vs multi-channel
  - [ ] Polling vs interrupt vs DMA
  - [ ] Resolution configuration
  - [ ] Sample time configuration
- [ ] Create Timer template (4h)
  - [ ] Basic timer
  - [ ] PWM mode
  - [ ] Input capture
  - [ ] Output compare
- [ ] Create DMA template (4h)
  - [ ] Channel configuration
  - [ ] Memory-to-memory
  - [ ] Peripheral-to-memory
  - [ ] Memory-to-peripheral
- [ ] Test all templates
- [ ] Document all templates

### 2.7 Create Startup Template (4h)

- [ ] Design startup template
  - [ ] Support Cortex-M0/M0+/M3/M4/M7
  - [ ] Configurable FPU initialization
  - [ ] Configurable MPU initialization
  - [ ] Configurable cache initialization
- [ ] Create `templates/platform/startup.cpp.j2`
  - [ ] Reset handler
  - [ ] Data section copy
  - [ ] BSS zero initialization
  - [ ] FPU initialization (if has_fpu)
  - [ ] Static constructor calls
  - [ ] Main call
- [ ] Create startup metadata
  - [ ] MCU capabilities (has_fpu, has_mpu, etc.)
  - [ ] Memory layout (RAM, flash addresses)
  - [ ] Stack size
- [ ] Test template generation
  - [ ] Generate for all platforms
  - [ ] Validate boot sequence
  - [ ] Measure binary size

**Phase 2 Deliverables**:
- ✅ 10 comprehensive Jinja2 templates
- ✅ Metadata schemas for all platforms
- ✅ Template generation tests
- ✅ Documentation for template variables

---

## Phase 3: Test Coverage (2 weeks, 48 hours)

### 3.1 Core Systems Testing (12h)

- [ ] Test Result<T, E> (3h)
  - [ ] Test Ok() construction
  - [ ] Test Err() construction
  - [ ] Test is_ok() / is_err()
  - [ ] Test unwrap() / expect()
  - [ ] Test map() / and_then()
  - [ ] Test zero overhead (sizeof checks)
  - [ ] Test move semantics
  - [ ] Test copy semantics
- [ ] Test error handling (2h)
  - [ ] Test all ErrorCode values
  - [ ] Test error messages
  - [ ] Test error propagation
- [ ] Test concepts (3h)
  - [ ] Test GpioPin concept
  - [ ] Test UartInterface concept
  - [ ] Test SpiInterface concept
  - [ ] Test ClockPlatform concept
  - [ ] Verify concept failures with bad types
- [ ] Test types (2h)
  - [ ] Test PinDirection
  - [ ] Test PinPull
  - [ ] Test PinDrive
  - [ ] Test BaudRate
  - [ ] Test SpiMode
- [ ] Test utilities (2h)
  - [ ] Test bit manipulation helpers
  - [ ] Test register access helpers
  - [ ] Test constexpr helpers

### 3.2 HAL Testing (16h)

- [ ] Test GPIO implementation (4h)
  - [ ] Test set/clear/toggle
  - [ ] Test read/write
  - [ ] Test direction configuration
  - [ ] Test pull resistor configuration
  - [ ] Test drive mode configuration
  - [ ] Test on all platforms (STM32F4, SAME70, STM32G0)
- [ ] Test UART implementation (4h)
  - [ ] Test configure
  - [ ] Test send/receive
  - [ ] Test baud rate calculation
  - [ ] Test parity configuration
  - [ ] Test stop bits configuration
  - [ ] Test on all platforms
- [ ] Test SPI implementation (4h)
  - [ ] Test configure
  - [ ] Test transfer
  - [ ] Test mode configuration
  - [ ] Test speed configuration
  - [ ] Test full-duplex operation
  - [ ] Test on all platforms
- [ ] Test I2C implementation (2h)
  - [ ] Test configure
  - [ ] Test write/read
  - [ ] Test repeated start
  - [ ] Test on available platforms
- [ ] Test clock configuration (2h)
  - [ ] Test PLL configuration
  - [ ] Test system clock switching
  - [ ] Test peripheral clock enable
  - [ ] Test on all platforms

### 3.3 Code Generator Testing (12h)

- [ ] Set up pytest framework (2h)
  - [ ] Install pytest and pytest-cov
  - [ ] Configure pytest.ini
  - [ ] Set up fixtures directory
  - [ ] Create test helper utilities
- [ ] Test SVD parser (4h)
  - [ ] Test peripheral parsing
  - [ ] Test register parsing
  - [ ] Test bitfield parsing
  - [ ] Test address calculation
  - [ ] Test error handling
  - [ ] Test with real SVD files (STM32, SAME70)
- [ ] Test template engine (3h)
  - [ ] Test template loading
  - [ ] Test variable substitution
  - [ ] Test conditionals
  - [ ] Test loops
  - [ ] Test filters
  - [ ] Test error handling
- [ ] Test generators (3h)
  - [ ] Test GPIO generator
  - [ ] Test UART generator
  - [ ] Test SPI generator
  - [ ] Test startup generator
  - [ ] Validate generated code compiles

### 3.4 Integration Testing (4h)

- [ ] Test end-to-end workflows (2h)
  - [ ] Test: Generate → Compile → Flash → Run
  - [ ] Test: Modify metadata → Regenerate → Build
  - [ ] Test: Add new MCU → Generate → Build
- [ ] Test cross-platform builds (2h)
  - [ ] Build for all supported boards
  - [ ] Verify binary sizes
  - [ ] Verify no warnings
  - [ ] Verify no errors

### 3.5 Hardware-in-Loop Testing (4h)

- [ ] Expand hardware test suite
  - [ ] Test GPIO on real hardware (1h)
    - [ ] Test LED blink
    - [ ] Test button input
    - [ ] Test interrupt handling
  - [ ] Test UART on real hardware (1h)
    - [ ] Test echo server
    - [ ] Test data integrity
    - [ ] Test error detection
  - [ ] Test SPI on real hardware (1h)
    - [ ] Test with SPI flash
    - [ ] Test data transfer
    - [ ] Test speed
  - [ ] Test I2C on real hardware (1h)
    - [ ] Test with I2C sensor
    - [ ] Test read/write
    - [ ] Test error handling
- [ ] Create automated test runner
  - [ ] Test framework setup
  - [ ] Automated flash and execute
  - [ ] Result collection
  - [ ] Report generation

**Phase 3 Deliverables**:
- ✅ 80% coverage for core
- ✅ 60% coverage for codegen
- ✅ 15+ hardware tests
- ✅ CI/CD integration

---

## ⚠️ Phase 4: Codegen Reorganization (1 week, 24 hours) - START HERE

**🚨 CRITICAL - BLOCKS CLI DEVELOPMENT 🚨**

**This is the FIRST phase to implement**. All other phases can wait, but CLI development cannot start until this completes.

**Why this blocks CLI**:
- CLI needs `core/validators/` structure to add ValidationService wrapper
- CLI needs `generators/` structure to locate peripheral generators for metadata commands
- CLI needs separation of peripheral templates (this spec) from project templates (CLI spec)
- CLI needs `core/schema_validator.py` foundation to add YAML schema validation

**After Phase 4 completion**: CLI team can immediately begin Phase 0 (YAML Migration), while Library Quality continues with Phases 1-3, 5-6 in parallel.

### 4.1 Design New Structure (2h)

- [ ] Design directory layout
  - [ ] core/ for parsing and rendering
  - [ ] core/validators/ for validation infrastructure (owned by this spec)
  - [ ] generators/ for peripheral generators
  - [ ] vendors/ for vendor-specific logic
  - [ ] tests/ for pytest suite
  - [ ] templates/ for Jinja2 templates (peripheral templates owned by this spec)
  - [ ] metadata/ for platform metadata (structure coordinated with CLI)
- [ ] Document new structure
  - [ ] Create architecture.md
  - [ ] Explain each directory purpose
  - [ ] Document import conventions
  - [ ] **Coordinate with CLI team on metadata/ structure**
- [ ] Plan migration strategy
  - [ ] Identify all files to move
  - [ ] Plan import path updates
  - [ ] Plan test updates

### 4.2 Create Migration Script (4h)

**🔗 Checkpoint**: Notify CLI team that validator interfaces are stable

- [ ] Create `tools/codegen/migrate_structure.py`
  - [ ] Map old paths to new paths
  - [ ] Handle file moves
  - [ ] Update imports automatically
  - [ ] Validate moved files
  - [ ] **Create core/validators/ structure for CLI integration**
- [ ] Test migration script (dry-run)
  - [ ] Verify all files mapped
  - [ ] Check for conflicts
  - [ ] Validate import updates
- [ ] Add rollback capability
  - [ ] Backup original structure
  - [ ] Implement undo functionality

### 4.3 Execute Migration (4h)

**🔗 Checkpoint**: Provide template engine API documentation to CLI team

- [ ] Run migration script
  - [ ] Move core modules
  - [ ] Move generators (peripheral generators owned by this spec)
  - [ ] Move vendor-specific code
  - [ ] Create new directories
  - [ ] **Document template engine API for CLI consumption**
- [ ] Update import paths
  - [ ] Update in Python files
  - [ ] Update in tests
  - [ ] Update in documentation
- [ ] Verify migration
  - [ ] Check all files moved
  - [ ] Check no broken imports
  - [ ] Check no duplicate files

### 4.4 Update Imports and Paths (6h)

**🔗 Checkpoint**: Coordinate YAML metadata structure with CLI team

- [ ] Update all Python imports
  - [ ] Update in codegen.py
  - [ ] Update in generators
  - [ ] Update in tests
  - [ ] Update in utilities
- [ ] Update configuration files
  - [ ] Update pyproject.toml
  - [ ] Update requirements.txt (PyYAML will be added by CLI spec)
  - [ ] Update .gitignore
- [ ] Update CMake references
  - [ ] Update codegen paths in CMakeLists.txt
  - [ ] Update in board configs
  - [ ] Update in platform configs
- [ ] **Define metadata structure for YAML migration**
  - [ ] Document expected metadata fields
  - [ ] Share with CLI team for schema creation
  - [ ] Agree on peripheral template variables

### 4.5 Validate All Generators Work (4h)

- [ ] Test each generator individually
  - [ ] GPIO generator
  - [ ] UART generator
  - [ ] SPI generator
  - [ ] I2C generator
  - [ ] Startup generator
  - [ ] Peripherals generator
- [ ] Test end-to-end generation
  - [ ] Generate for STM32F4
  - [ ] Generate for SAME70
  - [ ] Generate for STM32G0
- [ ] Verify build still works
  - [ ] Build all examples
  - [ ] Run all tests
  - [ ] Check no errors

### 4.6 Update Documentation (4h)

- [ ] Update architecture documentation
  - [ ] Reflect new structure
  - [ ] Update diagrams
  - [ ] Update examples
- [ ] Update developer guides
  - [ ] Update "Adding MCU" guide
  - [ ] Update "Creating Peripheral" guide
  - [ ] Update contribution guidelines
- [ ] Update README files
  - [ ] tools/codegen/README.md
  - [ ] Update quick start
  - [ ] Update examples

**Phase 4 Deliverables**:
- ✅ Clear core/generators/vendors separation
- ✅ Core validators ready for CLI integration
- ✅ Template engine API documented
- ✅ Metadata structure coordinated with CLI
- ✅ All imports fixed
- ✅ Documentation updated
- ✅ No build breakage

**🔗 After Phase 4**: CLI team can begin Phase 0 (YAML Migration)

---

## Phase 5: Documentation Unification (1 week, 24 hours)

### 5.1 Design Documentation Structure (2h)

- [ ] Plan unified structure
  - [ ] User guides
  - [ ] Developer guides
  - [ ] API reference
  - [ ] Examples
  - [ ] Design rationale
- [ ] Create directory layout
  - [ ] docs/user_guide/
  - [ ] docs/developer_guide/
  - [ ] docs/api_reference/
  - [ ] docs/examples/
  - [ ] docs/design_rationale/
- [ ] Plan content migration
  - [ ] Identify docs to consolidate
  - [ ] Plan what to keep/remove
  - [ ] Plan new content to create

### 5.2 Create User Guides (8h)

- [ ] Write Getting Started (2h)
  - [ ] Installation instructions
  - [ ] Prerequisites
  - [ ] First project walkthrough
  - [ ] Build and flash instructions
- [ ] Write Board Support (1h)
  - [ ] List all supported boards
  - [ ] Board specifications
  - [ ] Board pinouts
  - [ ] Example projects per board
- [ ] Write Peripherals Guide (2h)
  - [ ] GPIO usage
  - [ ] UART usage
  - [ ] SPI usage
  - [ ] I2C usage
  - [ ] ADC usage
  - [ ] Timer usage
- [ ] Write Error Handling (1h)
  - [ ] Result<T,E> patterns
  - [ ] ALLOY_TRY macro
  - [ ] Error recovery examples
  - [ ] Best practices
- [ ] Write RTOS Integration (1h)
  - [ ] Using scheduler
  - [ ] Creating tasks
  - [ ] Using queues
  - [ ] Using notifications
- [ ] Write Troubleshooting (1h)
  - [ ] Common build errors
  - [ ] Common runtime errors
  - [ ] Debugging tips
  - [ ] FAQ

### 5.3 Create Developer Guides (8h)

- [ ] Write Architecture Overview (2h)
  - [ ] System architecture
  - [ ] Component relationships
  - [ ] Design patterns used
  - [ ] Code organization
- [ ] Write Adding MCU Guide (2h)
  - [ ] Step-by-step instructions
  - [ ] Required files
  - [ ] Metadata format
  - [ ] Testing checklist
- [ ] Write Creating Peripheral (2h)
  - [ ] Template creation
  - [ ] Metadata schema
  - [ ] Generator implementation
  - [ ] Testing
- [ ] Write Template Reference (1h)
  - [ ] Available variables
  - [ ] Filters and functions
  - [ ] Conditionals
  - [ ] Loops
  - [ ] Best practices
- [ ] Write Contribution Guide (1h)
  - [ ] Code style
  - [ ] Pull request process
  - [ ] Testing requirements
  - [ ] Documentation requirements

### 5.4 Generate API Reference (4h)

- [ ] Set up Doxygen
  - [ ] Install Doxygen
  - [ ] Configure Doxyfile
  - [ ] Set output directory
- [ ] Improve code documentation
  - [ ] Add missing doxygen comments
  - [ ] Fix formatting issues
  - [ ] Add usage examples
- [ ] Generate API docs
  - [ ] Generate HTML
  - [ ] Generate search index
  - [ ] Validate links
- [ ] Organize API reference
  - [ ] Core (Result, error, types)
  - [ ] HAL (GPIO, UART, SPI, I2C)
  - [ ] RTOS (Scheduler, tasks, IPC)

### 5.5 Create Advanced Examples (2h)

- [ ] Write Error Recovery example (30min)
  - [ ] Demonstrate error handling
  - [ ] Show recovery strategies
  - [ ] Document best practices
- [ ] Write DMA Usage example (30min)
  - [ ] UART with DMA
  - [ ] SPI with DMA
  - [ ] Performance comparison
- [ ] Write Interrupt Handling (30min)
  - [ ] GPIO interrupt
  - [ ] UART interrupt
  - [ ] Best practices
- [ ] Write RTOS Integration (30min)
  - [ ] Multi-task example
  - [ ] Task communication
  - [ ] Synchronization

**Phase 5 Deliverables**:
- ✅ Unified docs/ directory
- ✅ 7 user guides
- ✅ 7 developer guides
- ✅ Complete API reference
- ✅ 4 advanced examples

---

## Phase 6: Startup Code Optimization (1 week, 16 hours)

### 6.1 Design Template-Based Startup (4h)

- [ ] Research Cortex-M startup requirements
  - [ ] Study ARM documentation
  - [ ] Compare Cortex-M0/M0+/M3/M4/M7
  - [ ] Identify common patterns
  - [ ] Identify differences
- [ ] Design startup template architecture
  - [ ] Define MCU_CONFIG policy
  - [ ] Define startup sequence
  - [ ] Plan FPU initialization
  - [ ] Plan MPU initialization
  - [ ] Plan cache initialization
- [ ] Design configuration schema
  - [ ] MCU capabilities (has_fpu, has_mpu, etc.)
  - [ ] Memory layout (flash, RAM)
  - [ ] Stack size
  - [ ] Heap size

### 6.2 Implement Cortex-M Startup Template (6h)

- [ ] Create `src/hal/vendors/arm/cortex_m/startup_template.hpp`
  - [ ] Implement reset_handler()
  - [ ] Implement copy_data_section()
  - [ ] Implement zero_bss_section()
  - [ ] Implement initialize_fpu() (conditional)
  - [ ] Implement initialize_mpu() (conditional)
  - [ ] Implement initialize_cache() (conditional)
  - [ ] Implement call_init_array()
- [ ] Add comprehensive documentation
  - [ ] Explain startup sequence
  - [ ] Document template parameters
  - [ ] Add usage examples
- [ ] Add compile-time validation
  - [ ] Static assert for required methods
  - [ ] Concept validation for MCU_CONFIG
  - [ ] Size optimization checks

### 6.3 Generate MCU-Specific Configs (4h)

- [ ] Create config template
  - [ ] `templates/platform/startup_config.hpp.j2`
  - [ ] Generate system_init() method
  - [ ] Generate clock configuration
  - [ ] Generate peripheral initialization
- [ ] Create configs for all platforms
  - [ ] STM32F4 startup config
  - [ ] SAME70 startup config
  - [ ] STM32G0 startup config
- [ ] Create startup instantiation
  - [ ] `templates/platform/startup.cpp.j2`
  - [ ] Instantiate startup template
  - [ ] Generate vector table
  - [ ] Generate interrupt handlers

### 6.4 Test on All Platforms (2h)

- [ ] Test STM32F4 startup
  - [ ] Verify boot sequence
  - [ ] Verify FPU initialization
  - [ ] Verify clock configuration
  - [ ] Measure binary size
- [ ] Test SAME70 startup
  - [ ] Verify boot sequence
  - [ ] Verify FPU initialization
  - [ ] Verify clock configuration
  - [ ] Measure binary size
- [ ] Test STM32G0 startup
  - [ ] Verify boot sequence
  - [ ] Verify clock configuration
  - [ ] Measure binary size
- [ ] Compare binary sizes
  - [ ] Before optimization
  - [ ] After optimization
  - [ ] Calculate % reduction
  - [ ] Target: 10-15% reduction

**Phase 6 Deliverables**:
- ✅ Single startup template
- ✅ 100+ startup files → 1 template
- ✅ 10-15% binary size reduction
- ✅ All platforms still boot

---

## Summary

**Total Tasks**: 180+
**Total Estimated Time**: 232 hours (10 weeks)

### Phase Breakdown

| Phase | Duration | Hours | Key Deliverables |
|-------|----------|-------|------------------|
| Phase 1: API Refactoring | 3 weeks | 72h | CRTP base classes, 52% code reduction |
| Phase 2: Template System | 2 weeks | 48h | 10 comprehensive templates |
| Phase 3: Test Coverage | 2 weeks | 48h | 80% core, 60% codegen coverage |
| Phase 4: Codegen Reorg | 1 week | 24h | Clear directory structure |
| Phase 5: Documentation | 1 week | 24h | Unified docs, 14 guides |
| Phase 6: Startup Optim | 1 week | 16h | Template-based startup |

### Critical Path

1. Phase 1 (API Refactoring) must complete before Phase 3 (Testing)
2. Phase 2 (Templates) can partially overlap with Phase 1
3. Phase 4 (Codegen Reorg) should complete before Phase 5 (Documentation)
4. Phase 6 (Startup) can be done independently

### Success Criteria

- [ ] Code reduction: 268KB → 129KB (52%)
- [ ] Test coverage: Core 80%, Codegen 60%
- [ ] Adding MCU: 8h → 2h
- [ ] Binary size: -10-15%
- [ ] Documentation: Complete and comprehensive
- [ ] Zero runtime overhead maintained
- [ ] Backward compatibility preserved
- [ ] All platforms build and run

### Risk Management

**High Risk**:
- CRTP refactoring breaks existing code
  - Mitigation: Extensive testing, parallel implementation

**Medium Risk**:
- Template system too complex
  - Mitigation: Start simple, iterate based on feedback
- Test coverage takes too long
  - Mitigation: Prioritize critical paths, accept 60% minimum

**Low Risk**:
- Binary size increases
  - Mitigation: Continuous size monitoring, optimization flags
