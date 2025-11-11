# Hardware Policy - Practical Examples

This document provides practical, end-to-end examples of the hardware policy pattern.

## Table of Contents

1. [Complete UART Example](#complete-uart-example)
2. [SPI Policy Example](#spi-policy-example)
3. [Cross-Platform Example](#cross-platform-example)
4. [Testing Example](#testing-example)
5. [Adding New Platform](#adding-new-platform)

---

## Complete UART Example

### Step 1: JSON Metadata

```json
// tools/codegen/metadata/platform/same70_uart.json
{
  "family": "same70",
  "vendor": "atmel",
  "peripheral_name": "UART",
  "register_include": "hal/vendors/atmel/same70/registers/uart0_registers.hpp",
  "bitfield_include": "hal/vendors/atmel/same70/bitfields/uart0_bitfields.hpp",
  "register_namespace": "atmel::same70::uart0",
  "register_type": "UART0_Registers",

  "template_params": [
    {"name": "BASE_ADDR", "type": "uint32_t", "description": "UART base address"},
    {"name": "PERIPH_CLOCK_HZ", "type": "uint32_t", "description": "Peripheral clock in Hz"}
  ],

  "policy_methods": {
    "reset": {
      "description": "Reset UART peripheral",
      "return_type": "void",
      "code": "hw()->CR = uart::cr::RSTRX::mask | uart::cr::RSTTX::mask | uart::cr::RXDIS::mask | uart::cr::TXDIS::mask;"
    },
    "configure_8n1": {
      "description": "Configure 8N1 mode",
      "return_type": "void",
      "code": "hw()->MR = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);"
    },
    "set_baudrate": {
      "description": "Set baud rate",
      "return_type": "void",
      "parameters": [{"name": "baud", "type": "uint32_t"}],
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
      "description": "Check TX ready",
      "return_type": "bool",
      "code": "return (hw()->SR & uart::sr::TXRDY::mask) != 0;"
    },
    "is_rx_ready": {
      "description": "Check RX ready",
      "return_type": "bool",
      "code": "return (hw()->SR & uart::sr::RXRDY::mask) != 0;"
    },
    "write_byte": {
      "description": "Write byte",
      "return_type": "void",
      "parameters": [{"name": "byte", "type": "uint8_t"}],
      "code": "hw()->THR = byte;"
    },
    "read_byte": {
      "description": "Read byte",
      "return_type": "uint8_t",
      "code": "return static_cast<uint8_t>(hw()->RHR);"
    },
    "wait_tx_ready": {
      "description": "Wait for TX ready",
      "return_type": "bool",
      "parameters": [
        {"name": "timeout", "type": "uint32_t", "default": "100000"}
      ],
      "code": "while (timeout-- > 0) {\n    if (is_tx_ready()) return true;\n}\nreturn false;"
    }
  },

  "instances": [
    {"name": "Uart0Hardware", "base": "0x400E0800", "clock": "150000000"},
    {"name": "Uart1Hardware", "base": "0x400E0A00", "clock": "150000000"}
  ]
}
```

### Step 2: Generated Policy Header

```cpp
// src/hal/vendors/atmel/same70/uart_hardware_policy.hpp (generated)
/**
 * @file uart_hardware_policy.hpp
 * @brief Hardware policy for SAME70 UART peripheral
 *
 * Auto-generated from: same70_uart.json
 */

#pragma once

#include "hal/vendors/atmel/same70/registers/uart0_registers.hpp"
#include "hal/vendors/atmel/same70/bitfields/uart0_bitfields.hpp"
#include "core/types.hpp"

namespace alloy::hal::atmel::same70 {

using namespace alloy::core;
namespace uart = atmel::same70::uart0;

template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70UartHardwarePolicy {
    // Type definitions
    using RegisterType = UART0_Registers;
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t periph_clock_hz = PERIPH_CLOCK_HZ;

    // Register access
    static inline volatile RegisterType* hw() {
    #ifdef ALLOY_UART_MOCK_HW
        return ALLOY_UART_MOCK_HW();
    #else
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
    #endif
    }

    // Hardware operations
    static inline void reset() {
        hw()->CR = uart::cr::RSTRX::mask | uart::cr::RSTTX::mask
                 | uart::cr::RXDIS::mask | uart::cr::TXDIS::mask;
    }

    static inline void configure_8n1() {
        hw()->MR = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);
    }

    static inline void set_baudrate(uint32_t baud) {
        uint32_t cd = PERIPH_CLOCK_HZ / (16 * baud);
        hw()->BRGR = uart::brgr::CD::write(0, cd);
    }

    static inline void enable_tx() {
        hw()->CR = uart::cr::TXEN::mask;
    }

    static inline void enable_rx() {
        hw()->CR = uart::cr::RXEN::mask;
    }

    static inline bool is_tx_ready() {
        return (hw()->SR & uart::sr::TXRDY::mask) != 0;
    }

    static inline bool is_rx_ready() {
        return (hw()->SR & uart::sr::RXRDY::mask) != 0;
    }

    static inline void write_byte(uint8_t byte) {
        hw()->THR = byte;
    }

    static inline uint8_t read_byte() {
        return static_cast<uint8_t>(hw()->RHR);
    }

    static inline bool wait_tx_ready(uint32_t timeout = 100000) {
        while (timeout-- > 0) {
            if (is_tx_ready()) return true;
        }
        return false;
    }
};

// Platform instances
using Uart0Hardware = Same70UartHardwarePolicy<0x400E0800, 150000000>;
using Uart1Hardware = Same70UartHardwarePolicy<0x400E0A00, 150000000>;

}  // namespace alloy::hal::atmel::same70
```

### Step 3: Generic API (Simple Level)

```cpp
// src/hal/api/uart_simple.hpp
#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "core/units.hpp"

namespace alloy::hal {

using namespace alloy::core;

// Forward declarations
template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig;

// Generic UART implementation
template <PeripheralId PeriphId, typename HardwarePolicy>
class UartImpl {
public:
    template <typename TxPin, typename RxPin>
    static constexpr auto quick_setup(BaudRate baudrate) {
        // Compile-time validation
        static_assert(is_valid_tx_pin<TxPin>(), "Invalid TX pin");
        static_assert(is_valid_rx_pin<RxPin>(), "Invalid RX pin");

        return SimpleUartConfig<TxPin, RxPin, HardwarePolicy>{
            PeriphId,
            baudrate
        };
    }

private:
    template <typename Pin>
    static constexpr bool is_valid_tx_pin() {
        // Check signal routing (implementation in signal_registry.hpp)
        return true;  // Simplified for example
    }

    template <typename Pin>
    static constexpr bool is_valid_rx_pin() {
        return true;  // Simplified for example
    }
};

// Configuration result
template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig {
    PeripheralId peripheral;
    BaudRate baudrate;

    Result<void, ErrorCode> initialize() const {
        // Configure pins
        TxPin::configure_alternate_function(/* AF from signal tables */);
        RxPin::configure_alternate_function(/* AF from signal tables */);

        // Configure hardware via policy
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

    Result<size_t, ErrorCode> read(uint8_t* buffer, size_t size) const {
        for (size_t i = 0; i < size; ++i) {
            if (!HardwarePolicy::wait_rx_ready()) {
                return Err(ErrorCode::Timeout);
            }
            buffer[i] = HardwarePolicy::read_byte();
        }
        return Ok(size);
    }
};

}  // namespace alloy::hal
```

### Step 4: Platform-Specific Aliases

```cpp
// src/platform/same70/peripherals.hpp
#pragma once

#include "hal/api/uart_simple.hpp"
#include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"

namespace alloy::platform::same70 {

using namespace alloy::hal;
using namespace alloy::hal::atmel::same70;

// UART type aliases
using Uart0 = UartImpl<PeripheralId::USART0, Uart0Hardware>;
using Uart1 = UartImpl<PeripheralId::USART1, Uart1Hardware>;

}  // namespace alloy::platform::same70
```

### Step 5: User Application

```cpp
// examples/same70_uart_simple/main.cpp
#include "platform/same70/peripherals.hpp"
#include "platform/same70/gpio.hpp"

using namespace alloy::platform::same70;
using namespace alloy::core;

// Pin definitions
using TxPin = GpioPin<PIOD_BASE, 3>;  // PD3 - USART0 TX
using RxPin = GpioPin<PIOD_BASE, 4>;  // PD4 - USART0 RX

int main() {
    // One-liner UART setup!
    auto uart = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});

    // Initialize
    if (auto result = uart.initialize(); result.is_error()) {
        // Handle error
        return -1;
    }

    // Write message
    const char* message = "Hello, World!\n";
    uart.write(
        reinterpret_cast<const uint8_t*>(message),
        std::strlen(message)
    );

    // Read response
    uint8_t buffer[32];
    if (auto result = uart.read(buffer, sizeof(buffer)); result.is_ok()) {
        // Process received data
        size_t bytes_read = result.value();
        // ...
    }

    while (true) {
        // Main loop
    }
}
```

---

## SPI Policy Example

### JSON Metadata

```json
{
  "family": "same70",
  "peripheral_name": "SPI",
  "register_type": "SPI0_Registers",

  "policy_methods": {
    "reset": {
      "return_type": "void",
      "code": "hw()->CR = spi::cr::SWRST::mask;"
    },
    "configure_mode": {
      "return_type": "void",
      "parameters": [
        {"name": "mode", "type": "SpiMode"},
        {"name": "clock_speed", "type": "uint32_t"}
      ],
      "code": "uint32_t mr = 0;\nmr = spi::mr::MSTR::write(mr, spi::mr::mstr::MASTER);\nmr = spi::mr::MODFDIS::set(mr);\nhw()->MR = mr;\n\n// Configure chip select 0\nuint32_t csr = 0;\ncsr = spi::csr::CPOL::write(csr, static_cast<uint32_t>(mode) & 0x2 ? 1 : 0);\ncsr = spi::csr::NCPHA::write(csr, static_cast<uint32_t>(mode) & 0x1 ? 1 : 0);\nhw()->CSR0 = csr;"
    },
    "enable": {
      "return_type": "void",
      "code": "hw()->CR = spi::cr::SPIEN::mask;"
    },
    "transfer_byte": {
      "return_type": "uint8_t",
      "parameters": [{"name": "tx_byte", "type": "uint8_t"}],
      "code": "while (!(hw()->SR & spi::sr::TDRE::mask));\nhw()->TDR = tx_byte;\nwhile (!(hw()->SR & spi::sr::RDRF::mask));\nreturn static_cast<uint8_t>(hw()->RDR);"
    },
    "is_busy": {
      "return_type": "bool",
      "code": "return (hw()->SR & spi::sr::TXEMPTY::mask) == 0;"
    }
  },

  "instances": [
    {"name": "Spi0Hardware", "base": "0x40008000", "clock": "150000000"},
    {"name": "Spi1Hardware", "base": "0x40058000", "clock": "150000000"}
  ]
}
```

### Usage

```cpp
// User code
using namespace alloy::platform::same70;

using SckPin = GpioPin<PIOD_BASE, 22>;
using MosiPin = GpioPin<PIOD_BASE, 21>;
using MisoPin = GpioPin<PIOD_BASE, 20>;
using CsPin = GpioPin<PIOD_BASE, 25>;

auto spi = Spi0::quick_setup<MosiPin, MisoPin, SckPin>(
    1000000,  // 1 MHz clock
    SpiMode::Mode0
);

spi.initialize();

// Transfer data
uint8_t tx_data[] = {0x01, 0x02, 0x03};
uint8_t rx_data[3];

{
    SpiChipSelect cs(CsPin{});  // CS low
    spi.transfer(tx_data, rx_data, 3);
}  // CS high (RAII)
```

---

## Cross-Platform Example

Same user code works on different MCUs!

### SAME70 Version

```cpp
// examples/uart_hello/main_same70.cpp
#include "platform/same70/peripherals.hpp"

using namespace alloy::platform::same70;

using TxPin = GpioPin<PIOD_BASE, 3>;
using RxPin = GpioPin<PIOD_BASE, 4>;

int main() {
    auto uart = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
    uart.initialize();
    uart.write("Hello from SAME70\n", 18);
}
```

### STM32F4 Version

```cpp
// examples/uart_hello/main_stm32f4.cpp
#include "platform/stm32f4/peripherals.hpp"

using namespace alloy::platform::stm32f4;

using TxPin = GpioPin<GPIOA_BASE, 9>;
using RxPin = GpioPin<GPIOA_BASE, 10>;

int main() {
    auto uart = Usart1::quick_setup<TxPin, RxPin>(BaudRate{115200});
    uart.initialize();
    uart.write("Hello from STM32F4\n", 19);
}
```

**Key Point**: Only the platform header and pin definitions change. The API is identical!

---

## Testing Example

### Unit Test with Mocks

```cpp
// tests/unit/test_uart_hardware_policy.cpp
#include "catch2/catch_test_macros.hpp"
#include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"

// Mock registers
namespace {
    volatile atmel::same70::uart0::UART0_Registers g_mock_registers;
}

// Mock hook
#define ALLOY_UART_MOCK_HW() (&g_mock_registers)

using namespace alloy::hal::atmel::same70;
using TestPolicy = Same70UartHardwarePolicy<0x400E0800, 150000000>;

TEST_CASE("UART Policy - Reset", "[uart][policy]") {
    // Arrange
    g_mock_registers = {};

    // Act
    TestPolicy::reset();

    // Assert
    namespace cr = atmel::same70::uart0::cr;
    uint32_t expected = cr::RSTRX::mask | cr::RSTTX::mask
                      | cr::RXDIS::mask | cr::TXDIS::mask;
    REQUIRE(g_mock_registers.CR == expected);
}

TEST_CASE("UART Policy - Set Baudrate 115200", "[uart][policy]") {
    // Arrange
    g_mock_registers = {};

    // Act
    TestPolicy::set_baudrate(115200);

    // Assert
    // 150MHz / (16 * 115200) = ~81
    namespace brgr = atmel::same70::uart0::brgr;
    uint32_t cd = brgr::CD::read(g_mock_registers.BRGR);
    REQUIRE(cd == 81);
}

TEST_CASE("UART Policy - Write Byte", "[uart][policy]") {
    // Arrange
    g_mock_registers = {};
    constexpr uint8_t test_byte = 0x42;

    // Act
    TestPolicy::write_byte(test_byte);

    // Assert
    REQUIRE(g_mock_registers.THR == test_byte);
}

TEST_CASE("UART Policy - Wait TX Ready Timeout", "[uart][policy]") {
    // Arrange
    g_mock_registers = {};
    g_mock_registers.SR = 0;  // Never ready

    // Act
    bool result = TestPolicy::wait_tx_ready(10);  // Small timeout

    // Assert
    REQUIRE_FALSE(result);
}

TEST_CASE("UART Policy - Wait TX Ready Immediate", "[uart][policy]") {
    // Arrange
    g_mock_registers = {};
    namespace sr = atmel::same70::uart0::sr;
    g_mock_registers.SR = sr::TXRDY::mask;  // Ready immediately

    // Act
    bool result = TestPolicy::wait_tx_ready(10);

    // Assert
    REQUIRE(result);
}
```

### Integration Test

```cpp
// tests/integration/test_uart_simple_api.cpp
#include "catch2/catch_test_macros.hpp"
#include "hal/api/uart_simple.hpp"
#include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"

// Mock registers (same as unit test)
namespace {
    volatile atmel::same70::uart0::UART0_Registers g_mock_registers;
}
#define ALLOY_UART_MOCK_HW() (&g_mock_registers)

// Mock GPIO pins
struct MockTxPin {
    static constexpr bool is_gpio_pin = true;
    static constexpr PinId get_pin_id() { return PinId::PD3; }
    static void configure_alternate_function(AlternateFunction af) {}
};

struct MockRxPin {
    static constexpr bool is_gpio_pin = true;
    static constexpr PinId get_pin_id() { return PinId::PD4; }
    static void configure_alternate_function(AlternateFunction af) {}
};

using namespace alloy::hal;
using namespace alloy::hal::atmel::same70;

using TestUart = UartImpl<PeripheralId::USART0, Uart0Hardware>;

TEST_CASE("UART Simple API - Quick Setup", "[uart][simple_api]") {
    auto config = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});

    REQUIRE(config.peripheral == PeripheralId::USART0);
    REQUIRE(config.baudrate.value == 115200);
}

TEST_CASE("UART Simple API - Initialize", "[uart][simple_api]") {
    // Arrange
    g_mock_registers = {};
    auto config = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});

    // Act
    auto result = config.initialize();

    // Assert
    REQUIRE(result.is_ok());
    REQUIRE(g_mock_registers.MR != 0);    // Mode register configured
    REQUIRE(g_mock_registers.BRGR != 0);  // Baud rate configured
}

TEST_CASE("UART Simple API - Write Success", "[uart][simple_api]") {
    // Arrange
    g_mock_registers = {};
    namespace sr = atmel::same70::uart0::sr;
    g_mock_registers.SR = sr::TXRDY::mask;  // Always ready

    auto config = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});
    config.initialize();

    const char* message = "TEST";

    // Act
    auto result = config.write(
        reinterpret_cast<const uint8_t*>(message),
        4
    );

    // Assert
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == 4);
}

TEST_CASE("UART Simple API - Write Timeout", "[uart][simple_api]") {
    // Arrange
    g_mock_registers = {};
    g_mock_registers.SR = 0;  // Never ready - timeout

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

---

## Adding New Platform

Let's add STM32F4 UART support.

### Step 1: Create Metadata

```json
// tools/codegen/metadata/platform/stm32f4_uart.json
{
  "family": "stm32f4",
  "vendor": "st",
  "peripheral_name": "UART",
  "register_include": "hal/vendors/st/stm32f4/registers/usart1_registers.hpp",
  "bitfield_include": "hal/vendors/st/stm32f4/bitfields/usart1_bitfields.hpp",
  "register_namespace": "st::stm32f4::usart1",
  "register_type": "USART1_Registers",

  "template_params": [
    {"name": "BASE_ADDR", "type": "uint32_t"},
    {"name": "PERIPH_CLOCK_HZ", "type": "uint32_t"}
  ],

  "policy_methods": {
    "reset": {
      "return_type": "void",
      "code": "hw()->CR1 &= ~usart::cr1::UE::mask;"
    },
    "configure_8n1": {
      "return_type": "void",
      "code": "uint32_t cr1 = hw()->CR1;\ncr1 = usart::cr1::M::clear(cr1);  // 8 data bits\nhw()->CR1 = cr1;\n\nuint32_t cr2 = hw()->CR2;\ncr2 = usart::cr2::STOP::write(cr2, 0);  // 1 stop bit\nhw()->CR2 = cr2;"
    },
    "set_baudrate": {
      "return_type": "void",
      "parameters": [{"name": "baud", "type": "uint32_t"}],
      "code": "uint32_t div = (PERIPH_CLOCK_HZ + baud/2) / baud;\nhw()->BRR = div;"
    },
    "enable_tx": {
      "return_type": "void",
      "code": "hw()->CR1 |= usart::cr1::UE::mask | usart::cr1::TE::mask;"
    },
    "enable_rx": {
      "return_type": "void",
      "code": "hw()->CR1 |= usart::cr1::RE::mask;"
    },
    "is_tx_ready": {
      "return_type": "bool",
      "code": "return (hw()->SR & usart::sr::TXE::mask) != 0;"
    },
    "is_rx_ready": {
      "return_type": "bool",
      "code": "return (hw()->SR & usart::sr::RXNE::mask) != 0;"
    },
    "write_byte": {
      "return_type": "void",
      "parameters": [{"name": "byte", "type": "uint8_t"}],
      "code": "hw()->DR = byte;"
    },
    "read_byte": {
      "return_type": "uint8_t",
      "code": "return static_cast<uint8_t>(hw()->DR);"
    },
    "wait_tx_ready": {
      "return_type": "bool",
      "parameters": [{"name": "timeout", "type": "uint32_t", "default": "100000"}],
      "code": "while (timeout-- > 0) {\n    if (is_tx_ready()) return true;\n}\nreturn false;"
    }
  },

  "instances": [
    {"name": "Usart1Hardware", "base": "0x40011000", "clock": "84000000"},
    {"name": "Usart2Hardware", "base": "0x40004400", "clock": "42000000"},
    {"name": "Usart6Hardware", "base": "0x40011400", "clock": "84000000"}
  ]
}
```

### Step 2: Generate Policy

```bash
$ python tools/codegen/generators/hardware_policy_generator.py
Generating policy for stm32f4 uart...
Generated: src/hal/vendors/st/stm32f4/uart_hardware_policy.hpp
```

### Step 3: Create Platform Aliases

```cpp
// src/platform/stm32f4/peripherals.hpp
#pragma once

#include "hal/api/uart_simple.hpp"
#include "hal/vendors/st/stm32f4/uart_hardware_policy.hpp"

namespace alloy::platform::stm32f4 {

using namespace alloy::hal;
using namespace alloy::hal::st::stm32f4;

using Usart1 = UartImpl<PeripheralId::USART1, Usart1Hardware>;
using Usart2 = UartImpl<PeripheralId::USART2, Usart2Hardware>;
using Usart6 = UartImpl<PeripheralId::USART6, Usart6Hardware>;

}  // namespace alloy::platform::stm32f4
```

### Step 4: Use It!

```cpp
// examples/stm32f4_uart/main.cpp
#include "platform/stm32f4/peripherals.hpp"

using namespace alloy::platform::stm32f4;

using TxPin = GpioPin<GPIOA_BASE, 9>;   // PA9 - USART1 TX
using RxPin = GpioPin<GPIOA_BASE, 10>;  // PA10 - USART1 RX

int main() {
    auto uart = Usart1::quick_setup<TxPin, RxPin>(BaudRate{115200});
    uart.initialize();
    uart.write("Hello from STM32F4!\n", 20);
}
```

**That's it!** The same generic API now works on STM32F4.

---

## Summary

The hardware policy pattern provides:

1. **Clean separation**: Generic APIs vs hardware-specific code
2. **Zero overhead**: Everything inline and compile-time resolved
3. **Testability**: Mock hooks enable unit testing
4. **Scalability**: Easy to add new platforms
5. **Maintainability**: Changes isolated to JSON metadata

All while maintaining the elegant, type-safe API users love!
