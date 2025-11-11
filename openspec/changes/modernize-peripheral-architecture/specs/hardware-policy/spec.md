# Spec: Hardware Policy Layer for Peripheral Implementation

## Overview

Define a policy-based architecture for connecting generic peripheral APIs to MCU-specific register implementations. This spec covers implementation for all peripherals, code generation, testing strategy, and cleanup of legacy code.

## Problem Statement

The new generic APIs (Simple, Fluent, Expert) are platform-agnostic, but need to access MCU-specific hardware registers. Each MCU family has different:

- **Register layouts**: SAME70 uses `CR`, `MR`, `BRGR` while STM32 uses `CR1`, `CR2`, `BRR`
- **Bitfield positions**: Parity bits at different offsets
- **Clock configurations**: Different peripheral clock frequencies
- **Initialization sequences**: Varying reset/enable procedures

**Current Problem**:
- Generic APIs in `src/hal/uart_simple.hpp` have no way to access hardware
- No clear separation between generic logic and hardware access
- Cannot test generic APIs without hardware

**Solution**:
- Use **Policy-Based Design** to inject hardware-specific behavior
- Generate policies automatically from existing JSON metadata
- Enable testing with mock policies

---

## ADDED Requirements

### Requirement: Hardware Policy Interface

All peripherals SHALL define a Hardware Policy interface that encapsulates MCU-specific register access.

**Rationale**: Separates generic API logic from hardware implementation, enabling testing and multi-platform support.

#### Policy Interface Contract

Each Hardware Policy MUST provide:

1. **Register Access**: `hw()` returns pointer to register block
2. **Initialization**: Methods to reset, configure, and enable peripheral
3. **Data Transfer**: Methods to read/write data
4. **Status Checking**: Methods to query peripheral state
5. **Type Definitions**: Register type, address constants

#### Scenario: UART Hardware Policy

```cpp
// hal/vendors/atmel/same70/uart_hardware_policy.hpp (generated)
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70UartHardwarePolicy {
    // Register types
    using RegisterType = atmel::same70::uart0::UART0_Registers;
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock = PERIPH_CLOCK_HZ;

    // Register access
    static inline volatile RegisterType* hw() {
        #ifdef ALLOY_UART_MOCK_HW
            return ALLOY_UART_MOCK_HW();
        #else
            return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
        #endif
    }

    // Initialization
    static void reset() {
        hw()->CR = uart::cr::RSTRX::mask | uart::cr::RSTTX::mask
                 | uart::cr::RXDIS::mask | uart::cr::TXDIS::mask;
    }

    static void configure_8n1() {
        hw()->MR = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);
    }

    static void set_baudrate(uint32_t baud) {
        uint32_t cd = PERIPH_CLOCK_HZ / (16 * baud);
        hw()->BRGR = uart::brgr::CD::write(0, cd);
    }

    static void enable_tx() {
        hw()->CR = uart::cr::TXEN::mask;
    }

    static void enable_rx() {
        hw()->CR = uart::cr::RXEN::mask;
    }

    // Data transfer
    static bool is_tx_ready() {
        return (hw()->SR & uart::sr::TXRDY::mask) != 0;
    }

    static bool is_rx_ready() {
        return (hw()->SR & uart::sr::RXRDY::mask) != 0;
    }

    static void write_byte(uint8_t byte) {
        hw()->THR = byte;
    }

    static uint8_t read_byte() {
        return static_cast<uint8_t>(hw()->RHR);
    }

    static bool wait_tx_ready(uint32_t timeout = 100000) {
        while (timeout-- > 0) {
            if (is_tx_ready()) return true;
        }
        return false;
    }

    static bool wait_rx_ready(uint32_t timeout = 100000) {
        while (timeout-- > 0) {
            if (is_rx_ready()) return true;
        }
        return false;
    }
};

// Platform-specific instances
using Uart0Hardware = Same70UartHardwarePolicy<0x400E0800, 150000000>;
using Uart1Hardware = Same70UartHardwarePolicy<0x400E0A00, 150000000>;
using Uart2Hardware = Same70UartHardwarePolicy<0x400E1A00, 150000000>;
using Uart3Hardware = Same70UartHardwarePolicy<0x400E1C00, 150000000>;
using Uart4Hardware = Same70UartHardwarePolicy<0x400E1E00, 150000000>;
```

**Success Criteria**:
- ✅ Policy provides all methods needed by generic API
- ✅ All register access goes through policy
- ✅ Mock hook available for testing (`ALLOY_UART_MOCK_HW`)
- ✅ Zero runtime overhead (all inline)

---

### Requirement: Generic API Integration

Generic APIs SHALL accept Hardware Policy as template parameter.

#### Scenario: Simple API with Policy

```cpp
// hal/uart_simple.hpp (generic)
template <PeripheralId PeriphId, typename HardwarePolicy>
class UartImpl {
public:
    template <typename TxPin, typename RxPin>
    static constexpr auto quick_setup(BaudRate baudrate) {
        // Validate pins at compile-time (generic)
        static_assert(is_valid_tx_pin<TxPin>(),
                     "TX pin is not compatible with this UART peripheral");
        static_assert(is_valid_rx_pin<RxPin>(),
                     "RX pin is not compatible with this UART peripheral");

        return SimpleUartConfig<TxPin, RxPin, HardwarePolicy>{
            PeriphId,
            baudrate,
            UartDefaults::data_bits,
            UartDefaults::parity,
            UartDefaults::stop_bits,
            UartDefaults::flow_control
        };
    }
};

// hal/uart_simple.hpp (implementation)
template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig {
    PeripheralId peripheral;
    BaudRate baudrate;
    u8 data_bits;
    UartParity parity;
    u8 stop_bits;
    bool flow_control;

    Result<void, ErrorCode> initialize() const {
        // Configure pins (generic)
        TxPin::configure_alternate_function(get_tx_af<TxPin>());
        RxPin::configure_alternate_function(get_rx_af<RxPin>());

        // Configure hardware (policy-specific)
        HardwarePolicy::reset();
        HardwarePolicy::configure_8n1();
        HardwarePolicy::set_baudrate(baudrate.value);
        HardwarePolicy::enable_tx();
        HardwarePolicy::enable_rx();

        return Ok();
    }

    Result<size_t, ErrorCode> write(const uint8_t* data, size_t size) const {
        for (size_t i = 0; i < size; ++i) {
            if (!HardwarePolicy::wait_tx_ready()) {
                return Err(ErrorCode::Timeout);
            }
            HardwarePolicy::write_byte(data[i]);
        }
        return Ok(size);
    }
};
```

**Success Criteria**:
- ✅ Generic API independent of hardware details
- ✅ Policy injected via template parameter
- ✅ All hardware access goes through policy methods

---

### Requirement: Platform-Specific Type Aliases

Each platform SHALL provide convenient type aliases combining peripheral ID with hardware policy.

#### Scenario: Platform Aliases

```cpp
// platform/same70/peripherals.hpp
#include "hal/uart_simple.hpp"
#include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"

namespace alloy::platform::same70 {

// UART instances
using Uart0 = hal::UartImpl<hal::PeripheralId::USART0, Uart0Hardware>;
using Uart1 = hal::UartImpl<hal::PeripheralId::USART1, Uart1Hardware>;
using Uart2 = hal::UartImpl<hal::PeripheralId::USART2, Uart2Hardware>;
using Uart3 = hal::UartImpl<hal::PeripheralId::USART3, Uart3Hardware>;
using Uart4 = hal::UartImpl<hal::PeripheralId::USART4, Uart4Hardware>;

// SPI instances (when implemented)
using Spi0 = hal::SpiImpl<hal::PeripheralId::SPI0, Spi0Hardware>;
using Spi1 = hal::SpiImpl<hal::PeripheralId::SPI1, Spi1Hardware>;

// I2C instances (when implemented)
using I2c0 = hal::I2cImpl<hal::PeripheralId::I2C0, I2c0Hardware>;
using I2c1 = hal::I2cImpl<hal::PeripheralId::I2C1, I2c1Hardware>;

}  // namespace alloy::platform::same70
```

**Success Criteria**:
- ✅ One file per platform with all peripheral aliases
- ✅ Easy to use: `same70::Uart0::quick_setup(...)`
- ✅ Clear naming convention

---

## Policy Implementation for All Peripherals

### Peripheral Coverage

The following peripherals SHALL have hardware policy implementations:

| Peripheral | Priority | Policy Methods | Status |
|------------|----------|----------------|--------|
| **UART**   | P0 (Critical) | reset, configure_8n1, set_baudrate, enable_tx/rx, write_byte, read_byte, is_tx_ready, is_rx_ready | ✅ Specified |
| **SPI**    | P0 (Critical) | reset, configure_mode, set_clock_speed, enable, transfer_byte, is_busy | 🔲 TODO |
| **I2C**    | P0 (Critical) | reset, configure, set_speed, start, stop, write_byte, read_byte, wait_ready | 🔲 TODO |
| **GPIO**   | P0 (Critical) | set_mode, set_output, read_input, set_alternate_function, set_pull | 🔲 TODO |
| **ADC**    | P1 (High) | reset, configure_channel, start_conversion, read_value, is_conversion_complete | 🔲 TODO |
| **Timer**  | P1 (High) | reset, configure_period, enable, start, stop, get_count | 🔲 TODO |
| **PWM**    | P2 (Medium) | reset, configure_channel, set_duty_cycle, enable_channel | 🔲 TODO |
| **DMA**    | P1 (High) | configure_transfer, start_transfer, is_complete, abort_transfer | 🔲 TODO |
| **DAC**    | P3 (Low) | reset, configure_channel, write_value, enable | 🔲 TODO |
| **CAN**    | P3 (Low) | reset, configure, send_frame, receive_frame, is_tx_ready | 🔲 TODO |

### Common Policy Interface Pattern

All policies SHALL follow this structure:

```cpp
template <uint32_t BASE_ADDR, /* other compile-time params */>
struct {Family}{Peripheral}HardwarePolicy {
    // Type definitions
    using RegisterType = /* vendor-specific register struct */;
    static constexpr uint32_t base_address = BASE_ADDR;

    // Register access with mock hook
    static inline volatile RegisterType* hw() {
        #ifdef ALLOY_{PERIPHERAL}_MOCK_HW
            return ALLOY_{PERIPHERAL}_MOCK_HW();
        #else
            return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
        #endif
    }

    // Peripheral-specific methods (varies by peripheral)
    // All methods must be static inline for zero overhead
};
```

---

## Code Generation

### Requirement: Automatic Policy Generation

Hardware policies SHALL be auto-generated from existing JSON metadata files.

**Rationale**: Avoid manual coding errors, ensure consistency across families, enable rapid platform addition.

#### Code Generator Architecture

```
tools/codegen/
├── generators/
│   └── hardware_policy_generator.py  (NEW)
├── templates/
│   ├── uart_hardware_policy.hpp.j2   (NEW)
│   ├── spi_hardware_policy.hpp.j2    (NEW)
│   ├── i2c_hardware_policy.hpp.j2    (NEW)
│   └── ... (one per peripheral)
└── metadata/
    └── platform/
        ├── same70_uart.json          (EXISTS - extend)
        ├── same70_spi.json           (EXISTS)
        ├── stm32f4_uart.json         (NEW)
        └── ...
```

#### JSON Metadata Extension

Extend existing peripheral JSON files to include policy method definitions:

```json
{
  "family": "same70",
  "vendor": "atmel",
  "peripheral_name": "UART",
  "register_type": "UART0_Registers",
  "register_namespace": "atmel::same70::uart0",

  "template_params": [
    {"name": "BASE_ADDR", "type": "uint32_t"},
    {"name": "PERIPH_CLOCK_HZ", "type": "uint32_t"}
  ],

  "policy_methods": {
    "reset": {
      "description": "Reset UART peripheral",
      "return_type": "void",
      "code": "hw()->CR = uart::cr::RSTRX::mask | uart::cr::RSTTX::mask | uart::cr::RXDIS::mask | uart::cr::TXDIS::mask;"
    },
    "configure_8n1": {
      "description": "Configure 8 data bits, no parity, 1 stop bit",
      "return_type": "void",
      "code": "hw()->MR = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);"
    },
    "set_baudrate": {
      "description": "Set baud rate",
      "return_type": "void",
      "parameters": [
        {"name": "baud", "type": "uint32_t"}
      ],
      "code": "uint32_t cd = PERIPH_CLOCK_HZ / (16 * baud);\nhw()->BRGR = uart::brgr::CD::write(0, cd);"
    },
    "enable_tx": {
      "description": "Enable transmitter",
      "return_type": "void",
      "code": "hw()->CR = uart::cr::TXEN::mask;"
    },
    "enable_rx": {
      "description": "Enable receiver",
      "return_type": "void",
      "code": "hw()->CR = uart::cr::RXEN::mask;"
    },
    "is_tx_ready": {
      "description": "Check if transmit buffer is ready",
      "return_type": "bool",
      "code": "return (hw()->SR & uart::sr::TXRDY::mask) != 0;"
    },
    "is_rx_ready": {
      "description": "Check if receive data is available",
      "return_type": "bool",
      "code": "return (hw()->SR & uart::sr::RXRDY::mask) != 0;"
    },
    "write_byte": {
      "description": "Write byte to transmit buffer",
      "return_type": "void",
      "parameters": [
        {"name": "byte", "type": "uint8_t"}
      ],
      "code": "hw()->THR = byte;"
    },
    "read_byte": {
      "description": "Read byte from receive buffer",
      "return_type": "uint8_t",
      "code": "return static_cast<uint8_t>(hw()->RHR);"
    },
    "wait_tx_ready": {
      "description": "Wait for transmit ready with timeout",
      "return_type": "bool",
      "parameters": [
        {"name": "timeout", "type": "uint32_t", "default": "100000"}
      ],
      "code": "while (timeout-- > 0) {\n    if (is_tx_ready()) return true;\n}\nreturn false;"
    },
    "wait_rx_ready": {
      "description": "Wait for receive ready with timeout",
      "return_type": "bool",
      "parameters": [
        {"name": "timeout", "type": "uint32_t", "default": "100000"}
      ],
      "code": "while (timeout-- > 0) {\n    if (is_rx_ready()) return true;\n}\nreturn false;"
    }
  },

  "instances": [
    {"name": "Uart0Hardware", "base": "0x400E0800", "clock": "150000000"},
    {"name": "Uart1Hardware", "base": "0x400E0A00", "clock": "150000000"},
    {"name": "Uart2Hardware", "base": "0x400E1A00", "clock": "150000000"},
    {"name": "Uart3Hardware", "base": "0x400E1C00", "clock": "150000000"},
    {"name": "Uart4Hardware", "base": "0x400E1E00", "clock": "150000000"}
  ]
}
```

#### Jinja2 Template

```jinja2
{# templates/uart_hardware_policy.hpp.j2 #}
/**
 * @file {{ family }}_uart_hardware_policy.hpp
 * @brief Hardware policy for {{ family|upper }} UART peripheral
 *
 * Auto-generated from: {{ metadata_file }}
 * Generator: hardware_policy_generator.py
 *
 * @note Part of modernize-peripheral-architecture OpenSpec change
 */

#pragma once

#include "{{ register_include }}"
#include "{{ bitfield_include }}"
#include "core/types.hpp"

namespace alloy::hal::{{ vendor }}::{{ family }} {

using namespace alloy::core;
namespace uart = {{ register_namespace }};

/**
 * @brief Hardware policy for {{ family|upper }} UART
 *
 * Encapsulates all register-level operations for UART peripheral.
 * Uses static methods for zero overhead abstraction.
 *
{% for param in template_params %}
 * @tparam {{ param.name }} {{ param.description }}
{% endfor %}
 */
template <{% for param in template_params %}{{ param.type }} {{ param.name }}{% if not loop.last %}, {% endif %}{% endfor %}>
struct {{ family|capitalize }}UartHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = {{ register_type }};
    {% for param in template_params %}
    static constexpr {{ param.type }} {{ param.name|lower }} = {{ param.name }};
    {% endfor %}

    // ========================================================================
    // Register Access
    // ========================================================================

    /**
     * @brief Get pointer to hardware registers
     *
     * Supports mock injection via ALLOY_UART_MOCK_HW preprocessor hook.
     *
     * @return Pointer to UART register block
     */
    static inline volatile RegisterType* hw() {
    #ifdef ALLOY_UART_MOCK_HW
        return ALLOY_UART_MOCK_HW();
    #else
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
    #endif
    }

    // ========================================================================
    // Hardware Operations
    // ========================================================================

{% for method_name, method in policy_methods.items() %}
    /**
     * @brief {{ method.description }}
     *
{% if method.parameters %}
{% for param in method.parameters %}
     * @param {{ param.name }} {{ param.description|default("") }}
{% endfor %}
{% endif %}
{% if method.return_type != "void" %}
     * @return {{ method.return_type }}
{% endif %}
     */
    static inline {{ method.return_type }} {{ method_name }}(
        {%- for param in method.parameters -%}
        {{ param.type }} {{ param.name }}{% if param.default %} = {{ param.default }}{% endif %}
        {%- if not loop.last %}, {% endif -%}
        {%- endfor -%}
    ) {
        {{ method.code|indent(8) }}
    }

{% endfor %}
};

// ============================================================================
// Platform-Specific Instances
// ============================================================================

{% for instance in instances %}
/**
 * @brief {{ instance.name }} - {{ instance.description|default("UART instance") }}
 */
using {{ instance.name }} = {{ family|capitalize }}UartHardwarePolicy<{{ instance.base }}, {{ instance.clock }}>;
{% endfor %}

}  // namespace alloy::hal::{{ vendor }}::{{ family }}
```

#### Generator Script

```python
# tools/codegen/generators/hardware_policy_generator.py
#!/usr/bin/env python3
"""
Hardware Policy Generator

Generates hardware policy headers from peripheral JSON metadata.
"""

import json
from pathlib import Path
from jinja2 import Environment, FileSystemLoader

class HardwarePolicyGenerator:
    def __init__(self, template_dir: Path, output_dir: Path):
        self.env = Environment(loader=FileSystemLoader(template_dir))
        self.output_dir = output_dir

    def generate_policy(self, metadata_file: Path, peripheral_type: str):
        """Generate hardware policy from JSON metadata."""

        # Load metadata
        with open(metadata_file) as f:
            metadata = json.load(f)

        # Load template
        template = self.env.get_template(f"{peripheral_type}_hardware_policy.hpp.j2")

        # Add metadata file name for documentation
        metadata['metadata_file'] = metadata_file.name

        # Render template
        output = template.render(**metadata)

        # Determine output path
        vendor = metadata['vendor']
        family = metadata['family']
        output_file = self.output_dir / vendor / family / f"{peripheral_type}_hardware_policy.hpp"

        # Ensure directory exists
        output_file.parent.mkdir(parents=True, exist_ok=True)

        # Write output
        output_file.write_text(output)

        print(f"Generated: {output_file}")
        return output_file

def main():
    template_dir = Path("tools/codegen/templates")
    metadata_dir = Path("tools/codegen/metadata/platform")
    output_dir = Path("src/hal/vendors")

    generator = HardwarePolicyGenerator(template_dir, output_dir)

    # Find all peripheral metadata files
    for metadata_file in metadata_dir.glob("*_*.json"):
        # Parse filename: {family}_{peripheral}.json
        stem = metadata_file.stem
        parts = stem.split("_")
        if len(parts) >= 2:
            family = parts[0]
            peripheral = "_".join(parts[1:])

            print(f"Generating policy for {family} {peripheral}...")
            generator.generate_policy(metadata_file, peripheral)

if __name__ == "__main__":
    main()
```

**Success Criteria**:
- ✅ Generator produces valid C++ code
- ✅ All peripheral types supported
- ✅ Integrated into CMake build system
- ✅ Regeneration on metadata change

---

## Testing Strategy

### Requirement: Comprehensive Testing

Hardware policies SHALL be tested at multiple levels: unit tests, integration tests, and hardware tests.

#### Level 1: Unit Tests (Mock-Based)

Test policy methods in isolation using mocks.

```cpp
// tests/unit/test_uart_hardware_policy.cpp
#include "catch2/catch_test_macros.hpp"
#include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"

// Mock registers
static volatile atmel::same70::uart0::UART0_Registers mock_registers;

// Mock hook
#define ALLOY_UART_MOCK_HW() (&mock_registers)

using TestPolicy = Same70UartHardwarePolicy<0x400E0800, 150000000>;

TEST_CASE("UART Hardware Policy - Reset", "[uart][policy]") {
    // Arrange
    mock_registers = {};

    // Act
    TestPolicy::reset();

    // Assert
    REQUIRE(mock_registers.CR == (uart::cr::RSTRX::mask | uart::cr::RSTTX::mask
                                  | uart::cr::RXDIS::mask | uart::cr::TXDIS::mask));
}

TEST_CASE("UART Hardware Policy - Set Baudrate", "[uart][policy]") {
    // Arrange
    mock_registers = {};
    constexpr uint32_t baud = 115200;
    constexpr uint32_t expected_cd = 150000000 / (16 * 115200);  // ~81

    // Act
    TestPolicy::set_baudrate(baud);

    // Assert
    uint32_t actual_cd = uart::brgr::CD::read(mock_registers.BRGR);
    REQUIRE(actual_cd == expected_cd);
}

TEST_CASE("UART Hardware Policy - TX Ready Status", "[uart][policy]") {
    // Arrange
    mock_registers = {};
    mock_registers.SR = 0;

    // Act & Assert
    REQUIRE_FALSE(TestPolicy::is_tx_ready());

    mock_registers.SR = uart::sr::TXRDY::mask;
    REQUIRE(TestPolicy::is_tx_ready());
}

TEST_CASE("UART Hardware Policy - Write Byte", "[uart][policy]") {
    // Arrange
    mock_registers = {};
    constexpr uint8_t test_byte = 0x42;

    // Act
    TestPolicy::write_byte(test_byte);

    // Assert
    REQUIRE(mock_registers.THR == test_byte);
}

TEST_CASE("UART Hardware Policy - Wait TX Ready Timeout", "[uart][policy]") {
    // Arrange
    mock_registers = {};
    mock_registers.SR = 0;  // Never ready

    // Act
    bool result = TestPolicy::wait_tx_ready(10);  // Small timeout

    // Assert
    REQUIRE_FALSE(result);
}

TEST_CASE("UART Hardware Policy - Wait TX Ready Success", "[uart][policy]") {
    // Arrange
    mock_registers = {};
    mock_registers.SR = uart::sr::TXRDY::mask;  // Ready immediately

    // Act
    bool result = TestPolicy::wait_tx_ready(10);

    // Assert
    REQUIRE(result);
}
```

**Coverage Targets**:
- ✅ All policy methods tested
- ✅ Edge cases covered (timeouts, invalid states)
- ✅ Register values verified with assertions
- ✅ 100% line coverage for generated policies

---

#### Level 2: Integration Tests (Generic API + Policy)

Test that generic APIs work correctly with policies.

```cpp
// tests/integration/test_uart_simple_api.cpp
#include "catch2/catch_test_macros.hpp"
#include "hal/uart_simple.hpp"
#include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"

// Mock registers
static volatile atmel::same70::uart0::UART0_Registers mock_registers;
#define ALLOY_UART_MOCK_HW() (&mock_registers)

using namespace alloy::hal;

// Mock GPIO pins
struct MockTxPin {
    static constexpr bool is_gpio_pin = true;
    static constexpr PinId get_pin_id() { return PinId::PD3; }
    static void configure_alternate_function(AlternateFunction af) {
        // Mock implementation
    }
};

struct MockRxPin {
    static constexpr bool is_gpio_pin = true;
    static constexpr PinId get_pin_id() { return PinId::PD4; }
    static void configure_alternate_function(AlternateFunction af) {
        // Mock implementation
    }
};

using TestUart = UartImpl<PeripheralId::USART0, Uart0Hardware>;

TEST_CASE("UART Simple API - Quick Setup", "[uart][simple_api]") {
    // Arrange
    mock_registers = {};

    // Act
    auto config = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});

    // Assert - config is compile-time validated
    REQUIRE(config.peripheral == PeripheralId::USART0);
    REQUIRE(config.baudrate.value == 115200);
}

TEST_CASE("UART Simple API - Initialize", "[uart][simple_api]") {
    // Arrange
    mock_registers = {};
    auto config = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});

    // Act
    auto result = config.initialize();

    // Assert
    REQUIRE(result.is_ok());

    // Verify hardware was configured
    REQUIRE(mock_registers.MR != 0);  // Mode register set
    REQUIRE(mock_registers.BRGR != 0);  // Baud rate set
}

TEST_CASE("UART Simple API - Write Data", "[uart][simple_api]") {
    // Arrange
    mock_registers = {};
    mock_registers.SR = uart::sr::TXRDY::mask;  // Always ready

    auto config = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});
    config.initialize();

    const char* message = "Hello";

    // Act
    auto result = config.write(reinterpret_cast<const uint8_t*>(message), 5);

    // Assert
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == 5);
}

TEST_CASE("UART Simple API - Write Timeout", "[uart][simple_api]") {
    // Arrange
    mock_registers = {};
    mock_registers.SR = 0;  // Never ready - simulate timeout

    auto config = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});
    config.initialize();

    const uint8_t byte = 0x42;

    // Act
    auto result = config.write(&byte, 1);

    // Assert
    REQUIRE(result.is_error());
    REQUIRE(result.error() == ErrorCode::Timeout);
}
```

**Coverage Targets**:
- ✅ All API levels tested (Simple, Fluent, Expert)
- ✅ Success and error paths covered
- ✅ Policy methods called correctly
- ✅ Pin validation integration tested

---

#### Level 3: Hardware Tests (Real Hardware)

Test on actual hardware to verify register operations.

```cpp
// tests/hardware/same70/test_uart_hardware.cpp
#include "catch2/catch_test_macros.hpp"
#include "platform/same70/peripherals.hpp"

using namespace alloy::platform::same70;

TEST_CASE("UART Hardware - Loopback Test", "[uart][hardware][same70]") {
    // This test requires UART TX connected to RX (loopback)

    // Arrange
    auto config = Uart0::quick_setup<PinD3, PinD4>(BaudRate{115200});
    REQUIRE(config.initialize().is_ok());

    const char* test_message = "LOOPBACK";
    uint8_t receive_buffer[16] = {0};

    // Act
    auto write_result = config.write(
        reinterpret_cast<const uint8_t*>(test_message),
        8
    );

    // Small delay for transmission
    for (volatile int i = 0; i < 10000; ++i);

    auto read_result = config.read(receive_buffer, 8);

    // Assert
    REQUIRE(write_result.is_ok());
    REQUIRE(read_result.is_ok());
    REQUIRE(std::memcmp(test_message, receive_buffer, 8) == 0);
}

TEST_CASE("UART Hardware - Baud Rate Accuracy", "[uart][hardware][same70]") {
    // Test that baud rate is set correctly by measuring timing
    // (requires oscilloscope or logic analyzer)

    auto config = Uart0::quick_setup<PinD3, PinD4>(BaudRate{9600});
    REQUIRE(config.initialize().is_ok());

    // Send single byte and measure timing
    const uint8_t byte = 0xAA;  // 10101010 - easy to measure
    auto result = config.write(&byte, 1);

    REQUIRE(result.is_ok());
    // Actual timing verification would require external measurement
}
```

**Hardware Test Requirements**:
- ✅ Tests run on CI hardware test farm (if available)
- ✅ Tests can be skipped on platforms without hardware
- ✅ Loopback tests for data integrity
- ✅ Timing tests for baud rate accuracy

---

### Test Organization

```
tests/
├── unit/
│   ├── test_uart_hardware_policy.cpp
│   ├── test_spi_hardware_policy.cpp
│   ├── test_i2c_hardware_policy.cpp
│   └── ... (one per peripheral)
├── integration/
│   ├── test_uart_simple_api.cpp
│   ├── test_uart_fluent_api.cpp
│   ├── test_uart_expert_api.cpp
│   ├── test_spi_simple_api.cpp
│   └── ... (one per peripheral API level)
└── hardware/
    ├── same70/
    │   ├── test_uart_hardware.cpp
    │   ├── test_spi_hardware.cpp
    │   └── ...
    ├── stm32f4/
    │   └── ...
    └── README.md (hardware test setup instructions)
```

---

## File Organization and Cleanup

### Requirement: Restructure HAL Directory

The `src/hal` directory SHALL be reorganized for clarity and consistency.

**Current Problem**:
- Files scattered inconsistently
- Mix of old and new implementations
- No clear separation of concerns
- Duplicate/obsolete code

**Target Structure**:

```
src/hal/
├── concepts.hpp                    (Core concepts)
├── signals.hpp                     (Signal type definitions)
├── signal_registry.hpp             (Signal allocation tracking)
├── validation.hpp                  (consteval validation helpers)
│
├── interface/                      (Platform-agnostic interfaces)
│   ├── gpio.hpp                   (GPIO concept & types)
│   ├── uart.hpp                   (UART concept & types)
│   ├── spi.hpp                    (SPI concept & types)
│   ├── i2c.hpp                    (I2C concept & types)
│   ├── adc.hpp                    (ADC concept & types)
│   ├── timer.hpp                  (Timer concept & types)
│   ├── pwm.hpp                    (PWM concept & types)
│   ├── dma.hpp                    (DMA concept & types)
│   ├── dac.hpp                    (DAC concept & types)
│   └── can.hpp                    (CAN concept & types)
│
├── api/                            (Generic API implementations)
│   ├── uart_simple.hpp            (Level 1: Simple API)
│   ├── uart_fluent.hpp            (Level 2: Fluent API)
│   ├── uart_expert.hpp            (Level 3: Expert API)
│   ├── uart_dma.hpp               (DMA integration)
│   ├── spi_simple.hpp
│   ├── spi_fluent.hpp
│   ├── spi_expert.hpp
│   ├── spi_dma.hpp
│   └── ... (one set per peripheral)
│
└── vendors/                        (Vendor-specific implementations)
    ├── atmel/
    │   └── same70/
    │       ├── same70_signals.hpp          (Generated signal tables)
    │       ├── uart_hardware_policy.hpp    (Generated UART policy)
    │       ├── spi_hardware_policy.hpp     (Generated SPI policy)
    │       ├── i2c_hardware_policy.hpp     (Generated I2C policy)
    │       └── ... (one policy per peripheral)
    │
    ├── st/
    │   ├── stm32f1/
    │   │   ├── stm32f1_signals.hpp
    │   │   ├── uart_hardware_policy.hpp
    │   │   └── ...
    │   ├── stm32f4/
    │   │   ├── stm32f4_signals.hpp
    │   │   ├── uart_hardware_policy.hpp
    │   │   └── ...
    │   └── stm32h7/
    │       └── ...
    │
    ├── espressif/
    │   └── esp32/
    │       └── ...
    │
    └── raspberrypi/
        └── rp2040/
            └── ...
```

### Files to Remove

The following legacy files SHALL be deleted:

```
src/hal/
├── uart_simple.hpp         → MOVE to api/uart_simple.hpp
├── uart_fluent.hpp         → MOVE to api/uart_fluent.hpp
├── uart_expert.hpp         → MOVE to api/uart_expert.hpp
├── uart_dma.hpp            → MOVE to api/uart_dma.hpp
├── spi_simple.hpp          → MOVE to api/spi_simple.hpp
├── spi_fluent.hpp          → MOVE to api/spi_fluent.hpp
├── spi_expert.hpp          → MOVE to api/spi_expert.hpp
├── spi_dma.hpp             → MOVE to api/spi_dma.hpp
└── dma_base.hpp            → MOVE to api/dma_base.hpp (if needed)
```

### Migration Checklist

- [ ] Create new directory structure
- [ ] Move files to appropriate locations
- [ ] Update `#include` paths in all files
- [ ] Update CMakeLists.txt to reflect new structure
- [ ] Verify all examples still compile
- [ ] Update documentation with new paths
- [ ] Remove legacy files after verification

---

## Legacy Code Generator Cleanup

### Requirement: Remove Obsolete Generators

Old code generators that are replaced by the policy generator SHALL be removed.

#### Generators to Remove

```
tools/codegen/
├── generators/
│   ├── platform_generator.py          (REMOVE - replaced by policy generator)
│   ├── peripheral_generator.py        (REMOVE - replaced by policy generator)
│   └── old_uart_generator.py          (REMOVE - obsolete)
│
└── templates/
    ├── platform/
    │   ├── uart.hpp.j2                (REMOVE - replaced by policy template)
    │   ├── spi.hpp.j2                 (REMOVE)
    │   └── i2c.hpp.j2                 (REMOVE)
    └── peripheral/
        └── ... (REMOVE if obsolete)
```

#### Keep These Generators

```
tools/codegen/
├── generators/
│   ├── hardware_policy_generator.py   (NEW - primary generator)
│   ├── signal_table_generator.py      (KEEP - generates signal routing)
│   ├── register_generator.py          (KEEP - generates register structs)
│   └── bitfield_generator.py          (KEEP - generates bitfield helpers)
│
└── templates/
    ├── uart_hardware_policy.hpp.j2    (NEW)
    ├── spi_hardware_policy.hpp.j2     (NEW)
    ├── registers.hpp.j2               (KEEP)
    └── bitfields.hpp.j2               (KEEP)
```

#### Cleanup Procedure

1. **Identify dependencies**: Ensure removed generators are not used
2. **Run final generation**: Generate all files with old generators one last time
3. **Switch to new generators**: Update build scripts to use policy generator
4. **Verify output**: Compare old vs new generated files
5. **Archive old generators**: Move to `tools/codegen/archive/` for reference
6. **Delete after verification**: Remove after 1 sprint of stability

---

## Platform Coverage

### Requirement: Multi-Platform Support

Hardware policies SHALL be generated for all supported platforms.

#### Supported Platforms

| Platform | Family | Status | Notes |
|----------|--------|--------|-------|
| **ATMEL** | SAME70 | ✅ Complete | Primary development target |
| **ATMEL** | SAMD21 | 🔲 TODO | Arduino Zero compatible |
| **ATMEL** | SAMV71 | 🔲 TODO | Similar to SAME70 |
| **ST** | STM32F1 | 🔲 TODO | Blue Pill |
| **ST** | STM32F4 | 🔲 TODO | Discovery boards |
| **ST** | STM32H7 | 🔲 TODO | High performance |
| **Espressif** | ESP32 | 🔲 TODO | WiFi/BLE support |
| **Raspberry Pi** | RP2040 | 🔲 TODO | Pico boards |
| **Host** | Linux | 🔲 TODO | Testing/simulation |

#### Priority Order

1. **Phase 1**: SAME70 (already in progress)
2. **Phase 2**: STM32F4 (popular platform)
3. **Phase 3**: STM32F1 (Blue Pill)
4. **Phase 4**: RP2040 (education market)
5. **Phase 5**: ESP32 (IoT applications)
6. **Phase 6**: Remaining platforms

---

## Success Criteria

### Compilation

- [ ] All generated policies compile without warnings
- [ ] Generic APIs compile with all policies
- [ ] No increase in binary size vs old implementation
- [ ] Compile time increase < 15%

### Testing

- [ ] 100% unit test coverage for policies
- [ ] All integration tests pass
- [ ] Hardware tests pass on available platforms
- [ ] Mock-based tests enable CI without hardware

### Code Quality

- [ ] All policies follow consistent naming convention
- [ ] Generated code matches coding standards (clang-format)
- [ ] Documentation complete for all public interfaces
- [ ] No TODO/FIXME comments in generated code

### Organization

- [ ] HAL directory structure clear and consistent
- [ ] No duplicate or obsolete files
- [ ] All files in correct locations
- [ ] CMakeLists.txt updated correctly

### Migration

- [ ] All examples updated to use new structure
- [ ] Old code generators archived or removed
- [ ] Documentation reflects new architecture
- [ ] Migration guide complete

---

## Dependencies

- ✅ Concept layer implementation (Phase 1)
- ✅ Signal routing implementation (Phase 2)
- ✅ Multi-level API implementation (Phase 4)
- 🔲 Code generator framework extension
- 🔲 Test infrastructure (mocks, hardware test rig)

---

## Timeline

| Phase | Duration | Deliverables |
|-------|----------|--------------|
| **Phase 1** | Week 1 | UART policy template + generator |
| **Phase 2** | Week 2 | UART unit & integration tests |
| **Phase 3** | Week 3 | SPI, I2C policies + tests |
| **Phase 4** | Week 4 | GPIO, ADC, Timer policies + tests |
| **Phase 5** | Week 5 | File reorganization + cleanup |
| **Phase 6** | Week 6 | Legacy generator removal |
| **Phase 7** | Week 7-8 | Multi-platform support (STM32F4, STM32F1) |
| **Phase 8** | Week 9 | Hardware tests on all platforms |
| **Phase 9** | Week 10 | Documentation + migration guide |

---

## Open Questions

1. **Q**: Should policies support runtime configuration?
   **A**: No, keep compile-time only for Phase 1. Can revisit if needed.

2. **Q**: How to handle peripherals with multiple register layouts (e.g., UART vs USART)?
   **A**: Separate policies for each variant, shared interface where possible.

3. **Q**: Should we support partial mocking (some methods real, some mocked)?
   **A**: Not in Phase 1. Full mock or full hardware for simplicity.

4. **Q**: How to version generated policies?
   **A**: Include generator version + metadata hash in generated file header.

5. **Q**: What to do with platform-specific features (e.g., ESP32 UART FIFO)?
   **A**: Add as optional methods in policy, generic APIs can check with `requires` or `if constexpr`.
