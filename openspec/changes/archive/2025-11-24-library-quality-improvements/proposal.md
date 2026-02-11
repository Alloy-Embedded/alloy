# Proposal: Library Quality Improvements

**Change ID**: `library-quality-improvements`
**Status**: PROPOSED
**Priority**: HIGH
**Complexity**: HIGH
**Estimated Duration**: 10 weeks (232 hours)
**⚠️ CRITICAL PATH**: Phase 4 (1 week) MUST complete first to unblock CLI development
**Last Updated**: 2025-01-17 (Integrated with Enhanced CLI)
**Coordination**: See `openspec/changes/INTEGRATION_LIBRARY_CLI.md`

---

## 🔗 Integration Notice

**This spec is coordinated with `enhance-cli-professional-tool`**.

**Ownership Boundaries**:
- **This Spec Owns**: Core validators, peripheral templates, CRTP refactoring, codegen reorganization
- **CLI Spec Owns**: YAML schemas, metadata commands, ValidationService wrapper, project templates
- **Shared**: Metadata database structure, template engine infrastructure

**Timeline Coordination**: This spec's **Phase 4 (Codegen Reorg) is CRITICAL and MUST complete before CLI implementation can begin**. See integration document for parallel execution plan (12 weeks total vs 21.5 sequential).

**⚠️ PREREQUISITE FOR CLI**: CLI development is blocked until Phase 4 completes. Phase 4 reorganizes the codegen system to provide the clean plugin architecture that CLI Phase 0 (YAML Migration) depends on.

---

## Executive Summary

This OpenSpec addresses **critical quality and scalability issues** identified in the Comprehensive Analysis 2024-11, focusing on improving code organization, eliminating duplication, enhancing test coverage, and completing the template system. The goal is to transform Alloy from a well-architected prototype (7.3/10) into a production-ready embedded framework (9.0+/10).

**Key Objectives**:
1. 🎯 **Eliminate API Duplication**: Reduce 268KB of duplicated code by 52% using CRTP pattern
2. 🧪 **Test Coverage**: Achieve 80% coverage for core systems and 60% for code generators
3. 📦 **Code Organization**: Restructure codegen system for maintainability
4. 🔧 **Template System**: Complete migration and create comprehensive peripheral templates
5. 📚 **Documentation**: Create unified documentation structure with guides
6. ⚡ **Performance**: Optimize startup code and reduce binary size

**Success Metrics**:
- **Code reduction**: 268KB → 129KB in API layer (52% reduction)
- **Test coverage**: Current 4/10 → 8/10 (80% core, 60% codegen)
- **Maintainability**: Adding new MCU from 8+ hours → 2 hours
- **Binary size**: 10-15% reduction through startup code optimization
- **Documentation**: Complete guides for all major workflows

**IMPORTANT DECISION**: Generated files stay in **source tree** (`src/hal/vendors/`), NOT in build directory. This differs from modm's approach but makes the codebase more transparent and easier to understand for users.

**Rationale for Generated Files in Source Tree**:
- ✅ **Transparency**: Users can see exactly what code is being compiled
- ✅ **Debugging**: Easier to inspect generated code when issues occur
- ✅ **IDE support**: Better code completion and navigation
- ✅ **Git tracking**: Explicit tracking of changes to generated code
- ✅ **Learning**: Users can study generated code to understand abstractions
- ❌ **Trade-off**: Larger repository size (acceptable for educational framework)

---

## 🆕 Metadata Format: YAML (Coordinated with CLI)

**This spec uses YAML for all metadata**, aligned with the Enhanced CLI proposal (`enhance-cli-professional-tool`).

**Rationale** (from CLI analysis):
- ✅ **25-30% smaller** files than JSON
- ✅ **Inline comments** for hardware quirks (critical for template development)
- ✅ **Clean code snippets** (multiline strings, no escaping)
- ✅ **<5% syntax error rate** (vs 30% with JSON)
- ✅ **Better git diffs** and merge resolution

**Example**:
```yaml
# tools/codegen/database/peripherals/gpio/stm32f4_gpio.yaml
schema_version: 1.0

platform:
  name: STM32F4
  vendor: st
  family: stm32f4

  # GPIO uses two-bit mode configuration (different from STM32F1)
  # Reference: RM0090 Rev 19, Section 8.4
  gpio_mode: two_bits

gpio_ports:
  - name: A
    base_address: 0x40020000
    pin_count: 16
    # Note: PA5 shared with LED on Nucleo boards
    quirks:
      pa5_led_conflict: true

operations:
  set:
    description: Set pin HIGH
    # Implementation uses atomic BSRR register
    # Single-cycle operation, no read-modify-write
    implementation: |
      hw()->BSRR = pin_mask;  // Set bit (atomic)
```

**Migration**: CLI Phase 0 provides migration tools. All JSON examples in this spec are shown in YAML format.

---

## Motivation

### Current State Assessment

**Overall Score**: 7.3/10 - GOOD with CRITICAL improvements needed

**Strengths**:
- ✅ Exemplary C++20/23 usage (concepts, Result<T,E>, zero-overhead abstractions)
- ✅ Comprehensive code generation pipeline with SVD support
- ✅ Excellent developer experience and API design
- ✅ Type-safe, compile-time validated peripherals

**Critical Issues** (from COMPREHENSIVE_ANALYSIS_2024-11.md):

1. **[CRITICAL] Incomplete Template System**
   - Only 1 active template (UART) found
   - 10+ templates in archive/ directory (old format)
   - Cannot scale to new peripherals without manual coding
   - Impact: Adding GPIO to new MCU requires 300+ lines of manual code

2. **[CRITICAL] Zero Test Coverage for Code Generators**
   - 91 Python files with 0 pytest tests
   - High regression risk when modifying generators
   - No validation of generated code correctness
   - Impact: Cannot safely refactor codegen system

3. **[HIGH] API Layer Duplication (268KB)**
   - 41 API files with 60-70% structural duplication
   - `uart_simple.hpp`, `uart_fluent.hpp`, `uart_expert.hpp` repeat same methods
   - Bug fixes require updating 3 files per peripheral
   - Impact: Maintenance burden, inconsistent behavior

4. **[HIGH] Codegen Directory Organization**
   - 91 Python files with unclear structure
   - No separation between core/generators/vendors
   - Difficult to find relevant code
   - Impact: High onboarding time, slow development

5. **[MEDIUM] Hardcoded Board Configuration**
   - CMakeLists.txt has if/else for all 12 boards
   - Adding board requires CMake + Python changes
   - No self-documenting board metadata
   - Impact: Adding board takes 4+ hours

6. **[MEDIUM] Documentation Fragmentation**
   - Docs split between `/docs/` and `/tools/codegen/docs/`
   - No step-by-step guides for common workflows
   - Missing error handling examples
   - Impact: Steep learning curve for new users

### Why This Matters

**For Library Developers**:
- Cannot safely refactor without breaking things
- Every new peripheral requires duplicate code in 3 API layers
- Adding new MCU is manual and error-prone

**For Library Users**:
- Difficult to understand how framework works
- Scattered documentation makes learning hard
- No examples for advanced patterns (error handling, RTOS integration)

**For Project Growth**:
- Cannot scale to 100+ supported MCUs without automation
- High maintenance burden discourages contributions
- Code quality issues undermine framework's credibility

---

## Goals

### Primary Goals (MUST HAVE)

**G1: Eliminate API Duplication** (MUST HAVE)
- [ ] Implement CRTP base classes for all peripherals
- [ ] Refactor Simple/Fluent/Expert to use inheritance
- [ ] Reduce API code from 268KB to ~129KB (52% reduction)
- [ ] Ensure zero runtime overhead (static assertions)
- [ ] Maintain backward compatibility

**G2: Complete Template System** (MUST HAVE)
- [ ] Migrate all templates from archive/ to active
- [ ] Modernize templates for C++20/23
- [ ] Create **peripheral templates** for all peripherals (GPIO, SPI, I2C, ADC, Timer)
- [ ] Document template variable schema (consumed by CLI spec's YAML schemas)
- [ ] Validate generated code compiles

**Note**: This spec owns **peripheral templates** (GPIO, UART, SPI, etc.). The CLI spec owns **project templates** (blinky, uart_logger, rtos). Both use the same template engine infrastructure.

**G3: Achieve Test Coverage** (MUST HAVE)
- [ ] 80% coverage for core systems (Result, concepts, error handling)
- [ ] 60% coverage for code generators (SVD parser, template engine)
- [ ] Hardware-in-loop tests for all supported boards
- [ ] Regression tests for all known issues
- [ ] CI/CD integration with coverage reporting

**Note**: Core validators (syntax, semantic, compile, test) are owned by **this spec**. The CLI spec (`enhance-cli-professional-tool`) wraps these validators in a ValidationService for command-line usage.

**G4: Reorganize Codegen System** (MUST HAVE)
- [ ] Separate core/generators/vendors directories
- [ ] Create clear plugin architecture for new vendors
- [ ] Document generator API
- [ ] Add YAML schema validation for all metadata (schemas owned by CLI spec)
- [ ] Create step-by-step guide for adding MCU

**Note**: This spec owns the core template engine and peripheral templates. The CLI spec owns YAML schemas and project templates (blinky, uart_logger, etc.).

**G5: Unified Documentation** (SHOULD HAVE)
- [ ] Consolidate docs/ and tools/codegen/docs/
- [ ] Create user guides (getting started, error handling, RTOS)
- [ ] Create developer guides (adding MCU, creating peripheral, testing)
- [ ] Add advanced examples (error recovery, DMA, interrupts)
- [ ] Generate API docs with Doxygen

**G6: Optimize Generated Code** (SHOULD HAVE)
- [ ] Template-based startup code (eliminate duplication)
- [ ] Reduce binary size by 10-15%
- [ ] Optimize clock initialization
- [ ] Add compile-time peripheral conflict detection

### Non-Goals (Out of Scope)

**Not in this OpenSpec**:
- ❌ New peripheral support (I2S, CAN, etc.) - separate OpenSpec
- ❌ RTOS enhancements - covered in separate OpenSpec
- ❌ CLI improvements - covered in enhance-cli-professional-tool
- ❌ Web-based configuration tool - future feature
- ❌ Real-time debugging - separate feature

---

## Technical Design

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    Alloy Library Improvements                           │
└─────────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────────┐
│ 1. API Layer Refactoring (CRTP Pattern)                               │
└────────────────────────────────────────────────────────────────────────┘

  BEFORE (DUPLICATED):                    AFTER (CRTP):

  uart_simple.hpp  (12KB)                 uart_base.hpp  (6KB)
  ├─ configure()                          ├─ configure_impl()
  ├─ send()         ┐                     ├─ send_impl()
  ├─ receive()      │ DUPLICATED          ├─ receive_impl()
  └─ ...20 methods  ┘                     └─ ...20 methods (ONCE)

  uart_fluent.hpp  (11KB)                 uart_simple.hpp  (2KB)
  ├─ configure()    ┐                     └─ inherits UartBase
  ├─ send()         │ DUPLICATED
  ├─ with_baud()    ┘                     uart_fluent.hpp  (4KB)
  └─ ...25 methods                        ├─ inherits UartBase
                                          └─ adds fluent methods
  uart_expert.hpp  (11KB)
  ├─ configure()    ┐                     uart_expert.hpp  (6KB)
  ├─ send()         │ DUPLICATED          ├─ inherits UartBase
  ├─ direct_access()┘                     └─ adds expert methods
  └─ ...28 methods

  Total: 34KB                             Total: 18KB (47% reduction)

┌────────────────────────────────────────────────────────────────────────┐
│ 2. Codegen System Reorganization                                      │
└────────────────────────────────────────────────────────────────────────┘

  BEFORE:                                 AFTER:

  tools/codegen/                          tools/codegen/
  ├─ 91 Python files (flat)               ├─ core/
  ├─ unclear organization                 │   ├─ svd_parser.py
  └─ hard to navigate                     │   ├─ template_engine.py
                                          │   └─ schema_validator.py
                                          ├─ generators/
                                          │   ├─ gpio_generator.py
                                          │   ├─ uart_generator.py
                                          │   └─ startup_generator.py
                                          ├─ vendors/
                                          │   ├─ st/
                                          │   ├─ atmel/
                                          │   └─ nordic/
                                          ├─ tests/
                                          │   ├─ test_svd_parser.py
                                          │   └─ test_generators.py
                                          └─ templates/
                                              ├─ platform/
                                              └─ board/

┌────────────────────────────────────────────────────────────────────────┐
│ 3. Template System Completion                                         │
└────────────────────────────────────────────────────────────────────────┘

  Current:                                Target:

  ✓ uart_hardware_policy.hpp.j2           ✓ gpio.hpp.j2
  ✗ gpio template (missing)               ✓ uart.hpp.j2
  ✗ spi template (missing)                ✓ spi.hpp.j2
  ✗ i2c template (missing)                ✓ i2c.hpp.j2
  ✗ adc template (missing)                ✓ adc.hpp.j2
  ✗ timer template (missing)              ✓ timer.hpp.j2
  ✗ dma template (missing)                ✓ dma.hpp.j2
  ✗ startup template (missing)            ✓ startup.cpp.j2
  ✗ peripherals template (missing)        ✓ peripherals.hpp.j2
                                          ✓ registers.hpp.j2

┌────────────────────────────────────────────────────────────────────────┐
│ 4. Test Coverage Strategy                                             │
└────────────────────────────────────────────────────────────────────────┘

  Current Coverage:                       Target Coverage:

  Core:        ~50% (estimated)           Core:        80%
  HAL:         ~30% (estimated)           HAL:         70%
  RTOS:        ~40% (estimated)           RTOS:        75%
  Codegen:      0%                        Codegen:     60%
  Integration: Low                        Integration: High
  Hardware:    3 tests                    Hardware:    15+ tests
```

---

## Implementation Details

### Phase 1: API Layer Refactoring (3 weeks, 72 hours)

#### 1.1 CRTP Base Class Design

**Goal**: Create single source of truth for peripheral implementations

**Implementation**:

```cpp
// src/hal/api/uart_base.hpp
namespace alloy::hal::api {

/**
 * @brief CRTP base class for UART implementations
 *
 * Provides common UART functionality to Simple/Fluent/Expert APIs.
 * Uses CRTP for zero-overhead static polymorphism.
 *
 * @tparam Derived The derived class (UartSimple, UartFluent, UartExpert)
 * @tparam HardwarePolicy Hardware-specific implementation policy
 */
template<typename Derived, typename HardwarePolicy>
class UartBase {
protected:
    // Hardware access
    static constexpr auto& hardware() {
        return HardwarePolicy::instance();
    }

    // Common implementation methods
    Result<void, ErrorCode> configure_impl(const UartConfig& config) {
        // Validate configuration
        if (config.baud_rate == 0) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Configure hardware
        ALLOY_TRY(hardware().set_baud_rate(config.baud_rate));
        ALLOY_TRY(hardware().set_data_bits(config.data_bits));
        ALLOY_TRY(hardware().set_parity(config.parity));
        ALLOY_TRY(hardware().set_stop_bits(config.stop_bits));

        return Ok();
    }

    Result<void, ErrorCode> send_impl(uint8_t byte) {
        // Wait for TX ready
        ALLOY_TRY(hardware().wait_tx_ready());

        // Send byte
        hardware().write_data(byte);

        return Ok();
    }

    Result<uint8_t, ErrorCode> receive_impl() {
        // Wait for RX ready
        ALLOY_TRY(hardware().wait_rx_ready());

        // Read byte
        return Ok(hardware().read_data());
    }

    Result<void, ErrorCode> send_buffer_impl(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; i++) {
            ALLOY_TRY(send_impl(data[i]));
        }
        return Ok();
    }

public:
    // Public interface (calls derived class)
    Result<void, ErrorCode> send(uint8_t byte) {
        return static_cast<Derived*>(this)->send_impl(byte);
    }

    Result<uint8_t, ErrorCode> receive() {
        return static_cast<Derived*>(this)->receive_impl();
    }

    // ... 20+ more common methods
};

// Ensure zero overhead
static_assert(sizeof(UartBase<class Dummy, class DummyPolicy>) == 1,
              "UartBase must have no runtime overhead");

} // namespace alloy::hal::api
```

**Simple API** (minimal code):

```cpp
// src/hal/api/uart_simple.hpp
namespace alloy::hal::api {

/**
 * @brief Simple UART API for basic use cases
 *
 * Provides straightforward UART operations without complexity.
 * All methods return Result<T, ErrorCode> for error handling.
 */
template<typename HardwarePolicy>
class UartSimple : public UartBase<UartSimple<HardwarePolicy>, HardwarePolicy> {
    using Base = UartBase<UartSimple<HardwarePolicy>, HardwarePolicy>;

public:
    // Simple API just forwards to base implementation
    Result<void, ErrorCode> configure(const UartConfig& config) {
        return this->configure_impl(config);
    }

    using Base::send;
    using Base::receive;
    using Base::send_buffer;

    // Simple-specific convenience methods
    Result<void, ErrorCode> println(const char* str) {
        ALLOY_TRY(this->send_buffer_impl(
            reinterpret_cast<const uint8_t*>(str),
            strlen(str)
        ));
        ALLOY_TRY(this->send_impl('\r'));
        ALLOY_TRY(this->send_impl('\n'));
        return Ok();
    }
};

} // namespace alloy::hal::api
```

**Fluent API** (method chaining):

```cpp
// src/hal/api/uart_fluent.hpp
namespace alloy::hal::api {

/**
 * @brief Fluent UART API for method chaining
 *
 * Allows configuration through builder pattern:
 * uart.with_baud(115200).with_parity(Parity::None).begin();
 */
template<typename HardwarePolicy>
class UartFluent : public UartBase<UartFluent<HardwarePolicy>, HardwarePolicy> {
    using Base = UartBase<UartFluent<HardwarePolicy>, HardwarePolicy>;

private:
    UartConfig config_;  // Accumulated configuration

public:
    // Fluent configuration methods
    UartFluent& with_baud(uint32_t baud) {
        config_.baud_rate = baud;
        return *this;
    }

    UartFluent& with_data_bits(DataBits bits) {
        config_.data_bits = bits;
        return *this;
    }

    UartFluent& with_parity(Parity parity) {
        config_.parity = parity;
        return *this;
    }

    UartFluent& with_stop_bits(StopBits stop) {
        config_.stop_bits = stop;
        return *this;
    }

    // Apply configuration
    Result<void, ErrorCode> begin() {
        return this->configure_impl(config_);
    }

    // Inherit common methods
    using Base::send;
    using Base::receive;
    using Base::send_buffer;
};

} // namespace alloy::hal::api
```

**Expert API** (direct register access):

```cpp
// src/hal/api/uart_expert.hpp
namespace alloy::hal::api {

/**
 * @brief Expert UART API for advanced users
 *
 * Provides direct register access and low-level control.
 * Use with caution - can bypass safety checks.
 */
template<typename HardwarePolicy>
class UartExpert : public UartBase<UartExpert<HardwarePolicy>, HardwarePolicy> {
    using Base = UartBase<UartExpert<HardwarePolicy>, HardwarePolicy>;

public:
    // Inherit common methods
    using Base::configure;
    using Base::send;
    using Base::receive;

    // Expert-only methods
    Result<void, ErrorCode> configure_register_direct(
        uint32_t baud_rate_register,
        uint32_t control_register
    ) {
        this->hardware().write_register(UART_BRR, baud_rate_register);
        this->hardware().write_register(UART_CR1, control_register);
        return Ok();
    }

    uint32_t read_status_register() const {
        return this->hardware().read_register(UART_SR);
    }

    void write_data_register_unsafe(uint8_t byte) {
        this->hardware().write_register(UART_DR, byte);
    }

    // DMA configuration
    Result<void, ErrorCode> configure_dma(
        DmaChannel tx_channel,
        DmaChannel rx_channel
    ) {
        ALLOY_TRY(this->hardware().enable_dma_tx(tx_channel));
        ALLOY_TRY(this->hardware().enable_dma_rx(rx_channel));
        return Ok();
    }

    // Interrupt configuration
    Result<void, ErrorCode> enable_interrupts(UartInterrupts interrupts) {
        return this->hardware().configure_interrupts(interrupts);
    }
};

} // namespace alloy::hal::api
```

**Benefits**:
- ✅ 268KB → ~129KB (52% reduction)
- ✅ Bug fixes in base class benefit all APIs
- ✅ Zero runtime overhead (CRTP + inlining)
- ✅ Easier to test (test base class once)
- ✅ Consistent behavior across APIs

**Validation**:

```cpp
// tests/unit/test_uart_crtp.cpp
TEST_CASE("UART CRTP has zero overhead", "[uart][crtp]") {
    using Simple = UartSimple<MockUartPolicy>;
    using Fluent = UartFluent<MockUartPolicy>;
    using Expert = UartExpert<MockUartPolicy>;

    // All APIs must be same size (no vtable, no extra data)
    STATIC_REQUIRE(sizeof(Simple) == sizeof(Fluent));
    STATIC_REQUIRE(sizeof(Fluent) == sizeof(Expert));

    // Must be trivially copyable
    STATIC_REQUIRE(std::is_trivially_copyable_v<Simple>);
}

TEST_CASE("UART base methods compile to same assembly", "[uart][asm]") {
    Simple simple;
    Fluent fluent;

    // Both should compile to identical assembly
    auto result1 = simple.send(0x42);
    auto result2 = fluent.send(0x42);

    // Verify both succeeded
    REQUIRE(result1.is_ok());
    REQUIRE(result2.is_ok());
}
```

---

### Phase 2: Template System Completion (2 weeks, 48 hours)

#### 2.1 Template Architecture

**Goal**: Create comprehensive templates for all peripherals

**Template Structure**:

```jinja2
{# templates/platform/gpio.hpp.j2 #}
/**
 * @file {{ output_file }}
 * @brief GPIO implementation for {{ platform.name }}
 *
 * Auto-generated from: {{ svd_file }}
 * Generator: {{ generator_version }}
 * Generated: {{ timestamp }}
 */

#pragma once

#include "core/result.hpp"
#include "core/error.hpp"
#include "hal/types.hpp"

// Register definitions
{% for port in gpio_ports %}
#include "{{ platform.vendor }}/{{ platform.family }}/generated/registers/gpio{{ port.name | lower }}_registers.hpp"
{% endfor %}

namespace alloy::hal::{{ platform.family }} {

using namespace alloy::core;

{% for port in gpio_ports %}
/**
 * @brief GPIO Port {{ port.name }}
 *
 * Base address: {{ "0x%08X" | format(port.base_address) }}
 * Pin count: {{ port.pin_count }}
 */
template <uint8_t PIN_NUM>
class GpioPin{{ port.name }} {
public:
    static constexpr uint32_t port_base = {{ "0x%08X" | format(port.base_address) }};
    static constexpr uint8_t pin_number = PIN_NUM;
    static constexpr uint32_t pin_mask = (1u << PIN_NUM);

    // Validate pin number at compile-time
    static_assert(PIN_NUM < {{ port.pin_count }}, "Pin number must be 0-{{ port.pin_count - 1 }}");

    /**
     * @brief Set pin HIGH
     */
    static Result<void, ErrorCode> set() {
        auto* port = reinterpret_cast<volatile gpio{{ port.name | lower }}::GPIO{{ port.name }}_Registers*>(port_base);
        {% if platform.has_set_clear_registers %}
        port->BSRR = pin_mask;  // Atomic set
        {% else %}
        port->ODR |= pin_mask;  // Read-modify-write
        {% endif %}
        return Ok();
    }

    /**
     * @brief Set pin LOW
     */
    static Result<void, ErrorCode> clear() {
        auto* port = reinterpret_cast<volatile gpio{{ port.name | lower }}::GPIO{{ port.name }}_Registers*>(port_base);
        {% if platform.has_set_clear_registers %}
        port->BSRR = (pin_mask << 16);  // Atomic clear
        {% else %}
        port->ODR &= ~pin_mask;  // Read-modify-write
        {% endif %}
        return Ok();
    }

    /**
     * @brief Toggle pin
     */
    static Result<void, ErrorCode> toggle() {
        auto* port = reinterpret_cast<volatile gpio{{ port.name | lower }}::GPIO{{ port.name }}_Registers*>(port_base);
        {% if platform.has_toggle_register %}
        port->TGL = pin_mask;  // Atomic toggle
        {% else %}
        if (port->ODR & pin_mask) {
            return clear();
        } else {
            return set();
        }
        {% endif %}
        return Ok();
    }

    /**
     * @brief Read pin value
     */
    static Result<bool, ErrorCode> read() {
        auto* port = reinterpret_cast<volatile gpio{{ port.name | lower }}::GPIO{{ port.name }}_Registers*>(port_base);
        bool value = (port->IDR & pin_mask) != 0;
        return Ok(value);
    }

    /**
     * @brief Configure pin direction
     */
    static Result<void, ErrorCode> set_direction(PinDirection direction) {
        auto* port = reinterpret_cast<volatile gpio{{ port.name | lower }}::GPIO{{ port.name }}_Registers*>(port_base);

        {% if platform.gpio_mode == "two_bits" %}
        // STM32-style: 2 bits per pin
        const uint32_t mode_pos = PIN_NUM * 2;
        const uint32_t mode_mask = 0x3 << mode_pos;
        const uint32_t mode_value = (direction == PinDirection::Output ? 0x1 : 0x0);

        port->MODER = (port->MODER & ~mode_mask) | (mode_value << mode_pos);
        {% elif platform.gpio_mode == "separate_registers" %}
        // SAME70-style: Separate enable/disable registers
        if (direction == PinDirection::Output) {
            port->OER = pin_mask;
        } else {
            port->ODR = pin_mask;
        }
        {% endif %}

        return Ok();
    }

    // ... more methods (setPull, setDrive, enableFilter, etc.)
};
{% endfor %}

// Port base address constants
{% for port in gpio_ports %}
constexpr uint32_t GPIO{{ port.name }}_BASE = {{ "0x%08X" | format(port.base_address) }};
{% endfor %}

} // namespace alloy::hal::{{ platform.family }}
```

**Template Metadata** (`metadata/stm32f4/gpio.json`):

```json
{
  "schema_version": "1.0",
  "platform": {
    "name": "STM32F4",
    "vendor": "st",
    "family": "stm32f4",
    "gpio_mode": "two_bits",
    "has_set_clear_registers": true,
    "has_toggle_register": false
  },
  "gpio_ports": [
    {
      "name": "A",
      "base_address": 1073872896,
      "pin_count": 16,
      "available_pins": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
    },
    {
      "name": "B",
      "base_address": 1073873920,
      "pin_count": 16,
      "available_pins": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
    },
    {
      "name": "C",
      "base_address": 1073874944,
      "pin_count": 16,
      "available_pins": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
    }
  ],
  "svd_file": "tools/codegen/svd/upstream/STMicro/STM32F401.svd",
  "output_file": "src/hal/vendors/st/stm32f4/gpio.hpp"
}
```

**Template Generation**:

```python
# tools/codegen/generators/gpio_generator.py
from pathlib import Path
from jinja2 import Environment, FileSystemLoader
from datetime import datetime

class GpioGenerator:
    """Generate GPIO implementation from template and metadata"""

    def __init__(self, template_dir: Path, metadata_dir: Path):
        self.env = Environment(loader=FileSystemLoader(template_dir))
        self.metadata_dir = metadata_dir

    def generate(self, platform: str, output_dir: Path) -> Path:
        """Generate GPIO code for platform"""

        # Load metadata
        metadata_file = self.metadata_dir / platform / "gpio.json"
        with open(metadata_file) as f:
            metadata = json.load(f)

        # Load template
        template = self.env.get_template("platform/gpio.hpp.j2")

        # Render template
        rendered = template.render(
            platform=metadata["platform"],
            gpio_ports=metadata["gpio_ports"],
            svd_file=metadata["svd_file"],
            output_file=metadata["output_file"],
            generator_version="2.0.0",
            timestamp=datetime.now().isoformat()
        )

        # Write output
        output_file = output_dir / metadata["output_file"]
        output_file.parent.mkdir(parents=True, exist_ok=True)
        output_file.write_text(rendered)

        return output_file
```

**Complete Template List**:

1. ✅ `gpio.hpp.j2` - GPIO peripheral
2. ✅ `uart.hpp.j2` - UART peripheral
3. ✅ `spi.hpp.j2` - SPI peripheral
4. ✅ `i2c.hpp.j2` - I2C peripheral
5. ✅ `adc.hpp.j2` - ADC peripheral
6. ✅ `timer.hpp.j2` - Timer peripheral
7. ✅ `dma.hpp.j2` - DMA controller
8. ✅ `startup.cpp.j2` - Startup code
9. ✅ `peripherals.hpp.j2` - Peripheral addresses
10. ✅ `registers.hpp.j2` - Register definitions

---

### Phase 3: Test Coverage (2 weeks, 48 hours)

#### 3.0 Validation Architecture (Ownership Clarification)

**This spec owns the core validators** used throughout the framework:

```python
# tools/codegen/core/validators/syntax_validator.py
"""
Validates generated C++ code syntax using Clang AST parser.
Owned by: library-quality-improvements spec
Consumed by: CLI ValidationService wrapper
"""

class SyntaxValidator:
    """Core syntax validator - checks C++ code compiles"""

    def validate(self, code: str, std: str = "c++23") -> ValidationResult:
        """Run clang syntax check"""
        # Implementation owned by this spec
        pass

# tools/codegen/core/validators/semantic_validator.py
"""
Cross-references generated code against SVD files.
Owned by: library-quality-improvements spec
Consumed by: CLI ValidationService wrapper
"""

class SemanticValidator:
    """Validates peripheral addresses, register offsets match SVD"""

    def validate(self, generated_file: Path, svd_file: Path) -> ValidationResult:
        """Cross-reference against SVD"""
        # Implementation owned by this spec
        pass

# tools/codegen/core/validators/compile_validator.py
"""
Full ARM GCC compilation test.
Owned by: library-quality-improvements spec
Consumed by: CLI ValidationService wrapper
"""

class CompileValidator:
    """Compiles generated code for target MCU"""

    def validate(self, source_files: list[Path], target: str) -> ValidationResult:
        """Compile with ARM GCC"""
        # Implementation owned by this spec
        pass

# tools/codegen/core/validators/test_validator.py
"""
Auto-generates and runs Catch2 unit tests.
Owned by: library-quality-improvements spec
Consumed by: CLI ValidationService wrapper
"""

class TestValidator:
    """Generates and runs unit tests for generated code"""

    def validate(self, peripheral_type: str, metadata: dict) -> ValidationResult:
        """Generate and run tests"""
        # Implementation owned by this spec
        pass
```

**CLI Spec Integration** (`enhance-cli-professional-tool`):

The CLI spec wraps these validators in a ValidationService:

```python
# tools/codegen/cli/services/validation_service.py (CLI spec owns this)
from core.validators import (
    SyntaxValidator,      # Owned by library-quality-improvements
    SemanticValidator,    # Owned by library-quality-improvements
    CompileValidator,     # Owned by library-quality-improvements
    TestValidator         # Owned by library-quality-improvements
)

class ValidationService:
    """
    CLI wrapper around core validators.
    Owned by: enhance-cli-professional-tool spec
    """

    def __init__(self):
        self.syntax = SyntaxValidator()
        self.semantic = SemanticValidator()
        self.compile = CompileValidator()
        self.test = TestValidator()

    def validate_all(self, files: list[Path], strict: bool = False):
        """Run all validators (CLI command: alloy metadata validate)"""
        # CLI-specific orchestration
        pass
```

#### 3.1 Core Systems Testing

**Goal**: Achieve 80% coverage for core library

**Test Structure**:

```cpp
// tests/unit/test_result.cpp
#include <catch2/catch_test_macros.hpp>
#include "core/result.hpp"

using namespace alloy::core;

TEST_CASE("Result<T, E> OK value", "[result]") {
    auto result = Ok(42);

    REQUIRE(result.is_ok());
    REQUIRE_FALSE(result.is_err());
    REQUIRE(result.unwrap() == 42);
}

TEST_CASE("Result<T, E> ERR value", "[result]") {
    auto result = Err(ErrorCode::InvalidParameter);

    REQUIRE(result.is_err());
    REQUIRE_FALSE(result.is_ok());
    REQUIRE(result.err() == ErrorCode::InvalidParameter);
}

TEST_CASE("Result<T, E> unwrap panics on error", "[result]") {
    auto result = Err(ErrorCode::Timeout);

    // Should trigger assertion
    REQUIRE_THROWS(result.unwrap());
}

TEST_CASE("Result<T, E> map transforms OK value", "[result]") {
    auto result = Ok(5);
    auto mapped = result.map([](int x) { return x * 2; });

    REQUIRE(mapped.is_ok());
    REQUIRE(mapped.unwrap() == 10);
}

TEST_CASE("Result<T, E> map leaves ERR unchanged", "[result]") {
    auto result = Err(ErrorCode::NotSupported);
    auto mapped = result.map([](int x) { return x * 2; });

    REQUIRE(mapped.is_err());
    REQUIRE(mapped.err() == ErrorCode::NotSupported);
}

TEST_CASE("Result<T, E> and_then chains operations", "[result]") {
    auto parse = [](const char* str) -> Result<int, ErrorCode> {
        int value = atoi(str);
        return value != 0 ? Ok(value) : Err(ErrorCode::InvalidParameter);
    };

    auto double_it = [](int x) -> Result<int, ErrorCode> {
        return Ok(x * 2);
    };

    auto result = parse("42").and_then(double_it);

    REQUIRE(result.is_ok());
    REQUIRE(result.unwrap() == 84);
}

TEST_CASE("Result<T, E> has zero overhead", "[result][performance]") {
    struct alignas(1) Tiny { char c; };

    // Result should only add error code (no vtable, no extra padding)
    STATIC_REQUIRE(sizeof(Result<Tiny, ErrorCode>) <= sizeof(Tiny) + sizeof(ErrorCode) + 1);

    // Must be trivially copyable
    STATIC_REQUIRE(std::is_trivially_copyable_v<Result<int, ErrorCode>>);
}
```

**Coverage Target**:

```
tests/unit/
├── test_result.cpp           # 100% coverage (critical)
├── test_error.cpp            # 90% coverage
├── test_concepts.cpp         # 80% coverage (compile-time)
├── test_types.cpp            # 85% coverage
└── test_gpio_api.cpp         # 80% coverage
```

#### 3.2 Code Generator Testing

**Goal**: Achieve 60% coverage for Python generators

```python
# tools/codegen/tests/test_svd_parser.py
import pytest
from pathlib import Path
from codegen.core.svd_parser import SVDParser, Peripheral, Register, BitField

@pytest.fixture
def stm32f4_svd():
    """Load STM32F4 SVD file"""
    svd_path = Path("tools/codegen/svd/upstream/STMicro/STM32F401.svd")
    return SVDParser.parse(svd_path)

def test_parse_peripheral_addresses(stm32f4_svd):
    """Test peripheral base addresses are parsed correctly"""
    peripherals = stm32f4_svd.peripherals

    assert "GPIOA" in peripherals
    assert peripherals["GPIOA"].base_address == 0x40020000

    assert "USART1" in peripherals
    assert peripherals["USART1"].base_address == 0x40011000

def test_parse_register_offsets(stm32f4_svd):
    """Test register offsets are parsed correctly"""
    gpioa = stm32f4_svd.peripherals["GPIOA"]

    assert "MODER" in gpioa.registers
    assert gpioa.registers["MODER"].offset == 0x00

    assert "ODR" in gpioa.registers
    assert gpioa.registers["ODR"].offset == 0x14

def test_parse_bitfields(stm32f4_svd):
    """Test bitfield positions and widths"""
    gpioa = stm32f4_svd.peripherals["GPIOA"]
    moder = gpioa.registers["MODER"]

    assert "MODER0" in moder.bitfields
    assert moder.bitfields["MODER0"].bit_offset == 0
    assert moder.bitfields["MODER0"].bit_width == 2

    assert "MODER15" in moder.bitfields
    assert moder.bitfields["MODER15"].bit_offset == 30
    assert moder.bitfields["MODER15"].bit_width == 2

def test_svd_validation():
    """Test SVD validation catches errors"""
    invalid_svd = """
    <device>
        <name>Invalid</name>
        <peripherals>
            <peripheral>
                <name>TEST</name>
                <!-- Missing base address -->
            </peripheral>
        </peripherals>
    </device>
    """

    with pytest.raises(ValueError, match="Missing base address"):
        SVDParser.parse_string(invalid_svd)

def test_duplicate_peripheral_detection(stm32f4_svd):
    """Test duplicate peripheral detection"""
    peripheral_names = list(stm32f4_svd.peripherals.keys())

    # Should have no duplicates
    assert len(peripheral_names) == len(set(peripheral_names))

# tools/codegen/tests/test_template_engine.py
from codegen.core.template_engine import TemplateEngine

def test_gpio_template_rendering():
    """Test GPIO template renders correctly"""
    engine = TemplateEngine(template_dir="tools/codegen/templates")

    metadata = {
        "platform": {
            "name": "STM32F4",
            "family": "stm32f4",
            "gpio_mode": "two_bits"
        },
        "gpio_ports": [
            {"name": "A", "base_address": 0x40020000, "pin_count": 16}
        ]
    }

    rendered = engine.render("platform/gpio.hpp.j2", metadata)

    # Verify generated code
    assert "class GpioPinA" in rendered
    assert "0x40020000" in rendered
    assert "pin_number < 16" in rendered

def test_template_syntax_errors():
    """Test template syntax error handling"""
    engine = TemplateEngine(template_dir="tools/codegen/templates")

    with pytest.raises(TemplateSyntaxError):
        engine.render("invalid_template.j2", {})

# tools/codegen/tests/test_generators.py
from codegen.generators.gpio_generator import GpioGenerator

def test_gpio_generator_output():
    """Test GPIO generator produces valid C++"""
    generator = GpioGenerator(
        template_dir="tools/codegen/templates",
        metadata_dir="tools/codegen/metadata"
    )

    output = generator.generate("stm32f4", output_dir=Path("build/test"))

    # Verify file was created
    assert output.exists()

    # Verify syntax (using clang)
    result = subprocess.run(
        ["clang++", "-std=c++23", "-fsyntax-only", str(output)],
        capture_output=True
    )
    assert result.returncode == 0, f"Syntax errors: {result.stderr}"

def test_all_platforms_generate():
    """Test code generation for all platforms"""
    platforms = ["stm32f4", "same70", "stm32g0", "nrf52"]

    for platform in platforms:
        generator = GpioGenerator(...)
        output = generator.generate(platform, ...)

        assert output.exists()
        assert output.stat().st_size > 0
```

**Coverage Target**:

```
tools/codegen/tests/
├── test_svd_parser.py          # 80% coverage
├── test_template_engine.py     # 70% coverage
├── test_gpio_generator.py      # 60% coverage
├── test_uart_generator.py      # 60% coverage
├── test_schema_validator.py    # 75% coverage
└── test_integration.py         # 50% coverage (end-to-end)
```

---

### Phase 4: Codegen Reorganization (1 week, 24 hours) ⚠️ CRITICAL - BLOCKS CLI

#### 4.1 Directory Restructure

**Goal**: Clear separation of concerns

**⚠️ WHY THIS IS CRITICAL FOR CLI**:
The Enhanced CLI (`enhance-cli-professional-tool`) requires a clean plugin architecture to implement YAML schemas, metadata commands, and project templates. Without this reorganization:
- CLI cannot safely add YAML schema validators (needs `core/schema_validator.py`)
- CLI metadata commands cannot locate generators (needs `generators/` structure)
- CLI project templates conflict with peripheral templates (needs separation)
- ValidationService wrapper cannot import validators (needs `core/validators/`)

**This phase MUST complete before CLI Phase 0 can begin.**

**New Structure**:

```
tools/codegen/
├── core/                          # Core functionality
│   ├── __init__.py
│   ├── svd_parser.py              # SVD XML → Python objects
│   ├── template_engine.py         # Jinja2 rendering
│   ├── schema_validator.py        # JSON schema validation
│   └── file_utils.py              # File I/O utilities
│
├── generators/                    # Peripheral generators
│   ├── __init__.py
│   ├── base_generator.py          # Abstract base class
│   ├── gpio_generator.py          # GPIO peripheral
│   ├── uart_generator.py          # UART peripheral
│   ├── spi_generator.py           # SPI peripheral
│   ├── i2c_generator.py           # I2C peripheral
│   ├── adc_generator.py           # ADC peripheral
│   ├── timer_generator.py         # Timer peripheral
│   ├── startup_generator.py       # Startup code
│   └── peripheral_generator.py    # Peripheral addresses
│
├── vendors/                       # Vendor-specific logic
│   ├── __init__.py
│   ├── st/
│   │   ├── __init__.py
│   │   ├── stm32f4.py             # STM32F4-specific quirks
│   │   ├── stm32g0.py             # STM32G0-specific quirks
│   │   └── clock_config.py        # ST clock tree
│   ├── atmel/
│   │   ├── __init__.py
│   │   ├── same70.py              # SAME70-specific quirks
│   │   └── clock_config.py        # Atmel clock tree
│   └── nordic/
│       ├── __init__.py
│       └── nrf52.py               # nRF52-specific quirks
│
├── templates/                     # Jinja2 templates
│   ├── platform/
│   │   ├── gpio.hpp.j2
│   │   ├── uart.hpp.j2
│   │   ├── spi.hpp.j2
│   │   ├── startup.cpp.j2
│   │   └── peripherals.hpp.j2
│   └── board/
│       └── board.hpp.j2
│
├── metadata/                      # Platform metadata
│   ├── schema/
│   │   ├── platform.schema.json   # Platform metadata schema
│   │   ├── board.schema.json      # Board metadata schema
│   │   └── peripheral.schema.json # Peripheral schema
│   ├── stm32f4/
│   │   ├── platform.json
│   │   ├── gpio.json
│   │   └── uart.json
│   └── same70/
│       ├── platform.json
│       └── gpio.json
│
├── tests/                         # pytest test suite
│   ├── __init__.py
│   ├── test_svd_parser.py
│   ├── test_template_engine.py
│   ├── test_generators.py
│   ├── test_schema_validator.py
│   ├── test_integration.py
│   └── fixtures/
│       ├── test_svd_files/
│       └── expected_output/
│
├── docs/                          # Generator documentation
│   ├── architecture.md            # System architecture
│   ├── adding_mcu.md              # Step-by-step MCU guide
│   ├── template_reference.md      # Template variable reference
│   └── troubleshooting.md         # Common issues
│
├── codegen.py                     # Main CLI entry point
├── pyproject.toml                 # Python project config
├── requirements.txt               # Dependencies
└── README.md                      # Quick start guide
```

**Migration Script**:

```python
# tools/codegen/migrate_structure.py
"""
Migrate codegen system to new structure.
This script moves files to new organization without breaking builds.
"""

import shutil
from pathlib import Path

def migrate():
    """Migrate to new structure"""

    moves = [
        # Core modules
        ("svd_parser.py", "core/svd_parser.py"),
        ("template_engine.py", "core/template_engine.py"),
        ("schema_validator.py", "core/schema_validator.py"),

        # Generators
        ("gpio_generator.py", "generators/gpio_generator.py"),
        ("uart_generator.py", "generators/uart_generator.py"),

        # Vendor-specific
        ("stm32_quirks.py", "vendors/st/stm32f4.py"),
        ("same70_quirks.py", "vendors/atmel/same70.py"),
    ]

    for src, dst in moves:
        src_path = Path("tools/codegen") / src
        dst_path = Path("tools/codegen") / dst

        if src_path.exists():
            dst_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(src_path, dst_path)
            print(f"Moved {src} → {dst}")

if __name__ == "__main__":
    migrate()
```

---

### Phase 5: Documentation Unification (1 week, 24 hours)

#### 5.1 Documentation Structure

**Goal**: Single source of truth for all documentation

**New Structure**:

```
docs/
├── README.md                      # Documentation index
│
├── user_guide/                    # For library users
│   ├── 01_getting_started.md      # Installation, first project
│   ├── 02_board_support.md        # Supported boards
│   ├── 03_peripherals.md          # Available peripherals
│   ├── 04_error_handling.md       # Result<T,E> patterns
│   ├── 05_rtos_integration.md     # Using RTOS
│   ├── 06_advanced_features.md    # DMA, interrupts, etc.
│   └── 07_troubleshooting.md      # Common issues
│
├── developer_guide/               # For library contributors
│   ├── 01_architecture.md         # System architecture
│   ├── 02_adding_mcu.md           # Step-by-step MCU guide
│   ├── 03_creating_peripheral.md  # New peripheral guide
│   ├── 04_template_reference.md   # Template variables
│   ├── 05_testing.md              # Testing guide
│   ├── 06_code_generation.md      # Codegen internals
│   └── 07_contributing.md         # Contribution guidelines
│
├── api_reference/                 # API documentation (Doxygen)
│   ├── core/
│   │   ├── result.md
│   │   └── error.md
│   ├── hal/
│   │   ├── gpio.md
│   │   ├── uart.md
│   │   └── spi.md
│   └── rtos/
│       ├── scheduler.md
│       └── queue.md
│
├── examples/                      # Detailed example walkthroughs
│   ├── blink_explained.md         # Blink example breakdown
│   ├── uart_logger.md             # UART logging example
│   ├── error_recovery.md          # Error handling example
│   └── rtos_tasks.md              # RTOS example
│
└── design_rationale/              # Design decisions
    ├── why_crtp.md                # Why use CRTP pattern
    ├── why_result.md              # Why Result<T,E> not exceptions
    ├── why_concepts.md            # Why C++20 concepts
    └── generated_in_source.md     # Why generated code in src/
```

**Example Documentation**:

```markdown
# Getting Started with Alloy

## Installation

### Prerequisites

- CMake 3.20+
- Python 3.11+
- ARM GCC 13.2.0+ (arm-none-eabi-gcc)
- OpenOCD (for flashing)

### Clone Repository

```bash
git clone https://github.com/user/alloy-embedded.git
cd alloy-embedded
```

### Install Python Dependencies

```bash
cd tools/codegen
pip install -r requirements.txt
```

## Your First Project

### 1. Choose a Board

Alloy supports these boards out of the box:

| Board | MCU | Architecture | Flash | RAM |
|-------|-----|--------------|-------|-----|
| Nucleo-F401RE | STM32F401RE | Cortex-M4F | 512KB | 96KB |
| SAME70-Xplained | ATSAME70Q21B | Cortex-M7 | 2MB | 384KB |
| Nucleo-G071RB | STM32G071RB | Cortex-M0+ | 128KB | 36KB |

For this guide, we'll use **Nucleo-F401RE**.

### 2. Create Project Directory

```bash
mkdir my-blink && cd my-blink
mkdir src
```

### 3. Write Code

Create `src/main.cpp`:

```cpp
#include "board.hpp"

int main() {
    // Initialize board (clocks, GPIO, etc.)
    if (auto result = board::init(); result.is_err()) {
        // Handle initialization error
        while(1) {}  // Halt on error
    }

    // Blink LED forever
    while (true) {
        board::led::on();
        board::delay_ms(500);

        board::led::off();
        board::delay_ms(500);
    }
}
```

### 4. Configure Build

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)

# Set board BEFORE project()
set(ALLOY_BOARD "nucleo_f401re")

# Include Alloy framework
include(/path/to/alloy-embedded/cmake/alloy.cmake)

# Define project
project(my-blink)

# Add executable
add_executable(firmware src/main.cpp)

# Link Alloy libraries
target_link_libraries(firmware
    alloy::core
    alloy::hal
    alloy::board
)
```

### 5. Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Output:
```
[  5%] Building CXX object CMakeFiles/firmware.dir/src/main.cpp.obj
[ 10%] Linking CXX executable firmware.elf
[100%] Built target firmware

   text    data     bss     dec     hex filename
   1247      12     128    1387     56b firmware.elf
```

### 6. Flash

```bash
make flash
```

Output:
```
Open On-Chip Debugger 0.12.0
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : clock speed 2000 kHz
Info : STLINK V2J37S7 (API v2) VID:PID 0483:374B
Info : Target voltage: 3.251835
Info : [stm32f4x.cpu] Cortex-M4 r0p1 processor detected
Info : [stm32f4x.cpu] target has 6 breakpoints, 4 watchpoints
Info : starting gdb server for stm32f4x.cpu on 3333
Info : Listening on port 3333 for gdb connections
[stm32f4x.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x01000000 pc: 0x08000194 msp: 0x20018000
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
shutdown command invoked
```

Your LED should now be blinking! 🎉

## Next Steps

- [Error Handling](04_error_handling.md) - Learn robust error handling patterns
- [RTOS Integration](05_rtos_integration.md) - Multi-tasking applications
- [Peripherals Guide](03_peripherals.md) - Using UART, SPI, I2C, etc.
```

---

### Phase 6: Startup Code Optimization (1 week, 16 hours)

#### 6.1 Template-Based Startup

**Goal**: Single startup implementation for all Cortex-M MCUs

**Current Problem**: 100+ duplicate startup files

**Solution**: Template-based startup with MCU-specific config

```cpp
// src/hal/vendors/arm/cortex_m/startup_template.hpp
namespace alloy::hal::cortex_m {

/**
 * @brief Generic Cortex-M startup code
 *
 * Uses MCU-specific configuration policy for customization.
 * Provides zero-overhead startup for all Cortex-M0/M0+/M3/M4/M7.
 *
 * @tparam MCU_CONFIG MCU-specific configuration policy
 */
template<typename MCU_CONFIG>
class Startup {
public:
    /**
     * @brief Reset handler (entry point)
     *
     * 1. Copy .data section from flash to RAM
     * 2. Zero-initialize .bss section
     * 3. Initialize hardware (clocks, FPU, etc.)
     * 4. Call static constructors
     * 5. Call main()
     */
    [[noreturn]] static void reset_handler() {
        // Copy .data section (initialized global variables)
        copy_data_section();

        // Zero .bss section (uninitialized global variables)
        zero_bss_section();

        // Initialize FPU (if available)
        if constexpr (MCU_CONFIG::has_fpu) {
            initialize_fpu();
        }

        // Initialize hardware
        MCU_CONFIG::system_init();

        // Call static constructors
        call_init_array();

        // Call main
        main();

        // Should never return
        while(1) {}
    }

private:
    static void copy_data_section() {
        extern uint32_t _sdata, _edata, _sidata;

        uint32_t* src = &_sidata;
        uint32_t* dst = &_sdata;
        uint32_t* end = &_edata;

        while (dst < end) {
            *dst++ = *src++;
        }
    }

    static void zero_bss_section() {
        extern uint32_t _sbss, _ebss;

        uint32_t* dst = &_sbss;
        uint32_t* end = &_ebss;

        while (dst < end) {
            *dst++ = 0;
        }
    }

    static void initialize_fpu() {
        // Enable FPU (Cortex-M4F/M7)
        constexpr uint32_t FPU_CPACR = 0xE000ED88;
        constexpr uint32_t FPU_CP10_CP11_FULL_ACCESS = (0x3 << 20) | (0x3 << 22);

        *reinterpret_cast<volatile uint32_t*>(FPU_CPACR) |= FPU_CP10_CP11_FULL_ACCESS;

        // DSB/ISB to ensure FPU is enabled before first FP instruction
        __asm__ volatile("dsb");
        __asm__ volatile("isb");
    }

    static void call_init_array() {
        extern void (*__init_array_start[])();
        extern void (*__init_array_end[])();

        size_t count = __init_array_end - __init_array_start;
        for (size_t i = 0; i < count; i++) {
            __init_array_start[i]();
        }
    }
};

} // namespace alloy::hal::cortex_m
```

**MCU-Specific Configuration**:

```cpp
// src/hal/vendors/st/stm32f4/startup_config.hpp (GENERATED)
namespace alloy::hal::stm32f4 {

struct STM32F401_StartupConfig {
    // MCU capabilities
    static constexpr bool has_fpu = true;
    static constexpr bool has_mpu = false;
    static constexpr bool has_cache = false;

    // System initialization
    static void system_init() {
        // Configure flash latency for 84 MHz
        configure_flash_latency();

        // Configure PLL for 84 MHz from 8 MHz HSE
        configure_pll();

        // Switch to PLL as system clock
        switch_to_pll();

        // Enable GPIO clocks
        enable_peripheral_clocks();
    }

private:
    static void configure_flash_latency() {
        constexpr uint32_t FLASH_ACR = 0x40023C00;
        constexpr uint32_t FLASH_ACR_LATENCY_2WS = 0x2;

        *reinterpret_cast<volatile uint32_t*>(FLASH_ACR) = FLASH_ACR_LATENCY_2WS;
    }

    static void configure_pll() {
        // PLL configuration for 84 MHz
        // Input: 8 MHz HSE
        // VCO: 8 MHz / 8 * 336 = 336 MHz
        // Output: 336 MHz / 4 = 84 MHz
        constexpr uint32_t RCC_PLLCFGR = 0x40023804;
        constexpr uint32_t PLLCFGR_VALUE =
            (8 << 0) |      // PLLM = 8
            (336 << 6) |    // PLLN = 336
            (0 << 16) |     // PLLP = 2 (div by 4)
            (1 << 22) |     // PLLSRC = HSE
            (7 << 24);      // PLLQ = 7

        *reinterpret_cast<volatile uint32_t*>(RCC_PLLCFGR) = PLLCFGR_VALUE;
    }

    // ... more init functions
};

} // namespace alloy::hal::stm32f4
```

**Instantiation** (generated per MCU):

```cpp
// src/hal/vendors/st/stm32f4/startup.cpp (GENERATED)
#include "hal/vendors/arm/cortex_m/startup_template.hpp"
#include "hal/vendors/st/stm32f4/startup_config.hpp"

using namespace alloy::hal;

// Instantiate startup for STM32F401
using STM32F401_Startup = cortex_m::Startup<stm32f4::STM32F401_StartupConfig>;

// Vector table
__attribute__((section(".isr_vector")))
const void* vector_table[] = {
    reinterpret_cast<void*>(0x20018000),  // Initial stack pointer
    reinterpret_cast<void*>(STM32F401_Startup::reset_handler),  // Reset handler
    // ... more interrupt vectors (generated)
};
```

**Benefits**:
- ✅ Single source of truth for startup logic
- ✅ 90% code reduction (10KB → 1KB per MCU)
- ✅ Bug fixes benefit all MCUs instantly
- ✅ Easier to add new MCUs (just config, not full startup)
- ✅ Zero runtime overhead (all inlined)

---

## Implementation Phases

### Phase 1: API Layer Refactoring (3 weeks, 72 hours)

**Tasks**:
1. Design CRTP base classes (8h)
2. Implement UartBase (8h)
3. Refactor Simple/Fluent/Expert UART (12h)
4. Repeat for GPIO, SPI, I2C, ADC (32h)
5. Write unit tests for all APIs (8h)
6. Validate zero overhead (4h)

**Deliverables**:
- ✅ 5 CRTP base classes (UART, GPIO, SPI, I2C, ADC)
- ✅ 15 refactored API files (5 peripherals × 3 APIs)
- ✅ 52% code reduction achieved
- ✅ 100% backward compatibility
- ✅ Test coverage >80%

---

### Phase 2: Template System Completion (2 weeks, 48 hours)

**Tasks**:
1. Design template architecture (4h)
2. Create GPIO template + metadata (8h)
3. Create UART template + metadata (8h)
4. Create SPI template + metadata (6h)
5. Create I2C template + metadata (6h)
6. Create ADC/Timer/DMA templates (12h)
7. Create startup template (4h)

**Deliverables**:
- ✅ 10 comprehensive Jinja2 templates
- ✅ Metadata schemas for all platforms
- ✅ Template generation tests
- ✅ Documentation for template variables

---

### Phase 3: Test Coverage (2 weeks, 48 hours)

**Tasks**:
1. Core systems tests (Result, concepts, error) (12h)
2. HAL tests (GPIO, UART, SPI) (16h)
3. Code generator tests (SVD, templates) (12h)
4. Integration tests (end-to-end) (4h)
5. Hardware-in-loop tests (4h)

**Deliverables**:
- ✅ 80% coverage for core
- ✅ 60% coverage for codegen
- ✅ 15+ hardware tests
- ✅ CI/CD integration

---

### Phase 4: Codegen Reorganization (1 week, 24 hours)

**Tasks**:
1. Design new directory structure (2h)
2. Create migration script (4h)
3. Move files to new structure (4h)
4. Update imports and paths (6h)
5. Test all generators still work (4h)
6. Update documentation (4h)

**Deliverables**:
- ✅ Clear core/generators/vendors separation
- ✅ All imports fixed
- ✅ Documentation updated
- ✅ No build breakage

---

### Phase 5: Documentation Unification (1 week, 24 hours)

**Tasks**:
1. Design documentation structure (2h)
2. Consolidate user guides (8h)
3. Create developer guides (8h)
4. Generate API reference (Doxygen) (4h)
5. Create advanced examples (2h)

**Deliverables**:
- ✅ Unified docs/ directory
- ✅ 7 user guides
- ✅ 7 developer guides
- ✅ Complete API reference
- ✅ 4 advanced examples

---

### Phase 6: Startup Code Optimization (1 week, 16 hours)

**Tasks**:
1. Design template-based startup (4h)
2. Implement Cortex-M startup template (6h)
3. Generate MCU-specific configs (4h)
4. Test on all platforms (2h)

**Deliverables**:
- ✅ Single startup template
- ✅ 100+ startup files → 1 template
- ✅ 10-15% binary size reduction
- ✅ All platforms still boot

---

## Success Criteria

### Functional Requirements

**FR1: API Duplication Eliminated** (MUST HAVE)
- [ ] CRTP base classes for all peripherals
- [ ] Code reduction from 268KB to ~129KB (52%)
- [ ] Zero runtime overhead validated
- [ ] Backward compatibility maintained

**FR2: Template System Complete** (MUST HAVE)
- [ ] 10 comprehensive templates created
- [ ] All templates generate valid C++23 code
- [ ] Metadata schemas validated
- [ ] Documentation complete

**FR3: Test Coverage Achieved** (MUST HAVE)
- [ ] Core: 80% coverage
- [ ] Codegen: 60% coverage
- [ ] Hardware-in-loop: 15+ tests
- [ ] CI/CD integration working

**FR4: Codegen Reorganized** (MUST HAVE)
- [ ] Clear directory structure
- [ ] All generators work correctly
- [ ] Documentation updated
- [ ] Adding MCU takes <2 hours

**FR5: Documentation Unified** (SHOULD HAVE)
- [ ] Single docs/ directory
- [ ] User + developer guides complete
- [ ] API reference generated
- [ ] Advanced examples created

**FR6: Startup Optimized** (SHOULD HAVE)
- [ ] Template-based startup implemented
- [ ] Binary size reduced 10-15%
- [ ] All platforms boot correctly

### Non-Functional Requirements

**NFR1: Performance**
- Code size reduced by 52% in API layer
- Binary size reduced by 10-15% overall
- Zero runtime overhead maintained
- Build time not increased

**NFR2: Maintainability**
- Adding new MCU: 8h → 2h
- Adding new peripheral: Template-driven
- Bug fixes: Single location (base class)
- Clear code organization

**NFR3: Quality**
- Test coverage: Core 80%, Codegen 60%
- Zero regressions in existing functionality
- All platforms build and run
- Documentation complete and accurate

**NFR4: Backward Compatibility**
- All existing examples work unchanged
- API remains identical
- Build process unchanged
- No breaking changes

---

## Risk Mitigation

### Risk 1: CRTP Refactoring Breaks Existing Code

**Likelihood**: MEDIUM
**Impact**: HIGH

**Mitigation**:
- Extensive unit tests before refactoring
- Maintain parallel implementation during migration
- Use compiler warnings to catch issues
- Validate on all platforms before merging
- Keep old code in deprecated/ until stable

### Risk 2: Template System Too Complex

**Likelihood**: MEDIUM
**Impact**: MEDIUM

**Mitigation**:
- Start with simple templates (GPIO)
- Comprehensive documentation
- Example-driven development
- User feedback after each template
- Simplify if adoption is low

### Risk 3: Test Coverage Takes Too Long

**Likelihood**: HIGH
**Impact**: LOW

**Mitigation**:
- Prioritize critical paths first
- Use coverage tools to find gaps
- Automate test generation where possible
- Accept 60% as minimum viable
- Improve coverage incrementally

### Risk 4: Binary Size Increases

**Likelihood**: LOW
**Impact**: MEDIUM

**Mitigation**:
- Validate size after each change
- Use -Os optimization for templates
- Profile binary size continuously
- Remove unused code sections
- Benchmark against baseline

---

## Dependencies

### External Dependencies

**Build Tools**:
- CMake 3.20+
- arm-none-eabi-gcc 13.2.0+
- clang-format 14+
- clang-tidy 14+

**Python (for codegen)**:
- Python 3.11+
- Jinja2 3.1+
- lxml 4.9+ (SVD parsing)
- jsonschema 4.17+ (validation)
- pytest 7.0+ (testing)
- pytest-cov 4.0+ (coverage)

**Testing**:
- Catch2 3.0+
- OpenOCD (for hardware tests)

### Internal Dependencies

**Blockers**:
- None - this OpenSpec is independent

**Prerequisites**:
- Git repository clean
- All existing tests passing
- Build system working

---

## Migration Path

### For Library Users

**Phase 1-2 (API Refactoring + Templates)**:
- ✅ No changes required
- ✅ Existing code works unchanged
- ✅ Can opt into new templates if desired

**Phase 3 (Test Coverage)**:
- ✅ No user impact
- ✅ Improved reliability

**Phase 4-6 (Codegen, Docs, Startup)**:
- ✅ Better documentation available
- ✅ Faster code generation
- ✅ Smaller binaries

### For Library Contributors

**Phase 1 (API Refactoring)**:
- Learn CRTP pattern
- Follow new API structure for new peripherals

**Phase 2 (Templates)**:
- Use templates for new peripherals
- Refer to template documentation

**Phase 4 (Codegen Reorganization)**:
- Update import paths in custom generators
- Follow new directory structure

---

## Conclusion

This OpenSpec transforms Alloy from a well-architected prototype (7.3/10) into a production-ready embedded framework (9.0+/10) by addressing critical quality and scalability issues.

**Key Improvements**:
1. ✅ **52% code reduction** in API layer through CRTP
2. ✅ **Complete template system** for all peripherals
3. ✅ **80% test coverage** for core systems
4. ✅ **Clear code organization** for maintainability
5. ✅ **Unified documentation** for users and developers
6. ✅ **Optimized startup** reducing binary size 10-15%

**Timeline**: 10 weeks, 232 hours

**Success Metrics**:
- Code: 268KB → 129KB
- Test coverage: 4/10 → 8/10
- Adding MCU: 8h → 2h
- Binary size: -10-15%
- Documentation: Complete

**IMPORTANT**: Generated files remain in `src/` tree for transparency and ease of understanding, unlike modm which hides them in `build/`.

**Next Steps**:
1. Review and approve this OpenSpec
2. Begin Phase 1 (API Refactoring)
3. Validate after each phase
4. Iterate based on feedback
