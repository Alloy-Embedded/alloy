# Spec: MCU-Specific Pin Generation

## ADDED Requirements

### Requirement: Per-MCU Pin Header Generation
**ID**: PIN-GEN-001
**Priority**: P0 (Critical)

The system SHALL generate per-MCU pin definition headers with constexpr pin constants for each supported MCU variant.

#### Scenario: Generate pin header for STM32F103C8
```cpp
// Given pin database with STM32F103C8 information:
{
  "stm32f103c8": {
    "package": "LQFP48",
    "pins": [
      {"name": "PA0", "number": 0},
      {"name": "PC13", "number": 45}
    ]
  }
}

// When running code generation:
python tools/codegen/generate_mcu_pins.py

// Then file is created:
//   src/hal/st/stm32f1/generated/stm32f103c8/pins.hpp

// And contains constexpr definitions:
namespace alloy::hal::stm32f1::stm32f103c8::pins {
    constexpr uint8_t PA0 = 0;
    constexpr uint8_t PC13 = 45;
}
```

#### Scenario: Pin header includes all ports
```cpp
// Given STM32F103C8 with ports A, B, C, D
// When pin header is generated
#include <hal/st/stm32f1/generated/stm32f103c8/pins.hpp>

// Then all port pins are available:
using namespace alloy::hal::stm32f1::stm32f103c8::pins;
constexpr auto pin_a0 = PA0;    // Port A
constexpr auto pin_b5 = PB5;    // Port B
constexpr auto pin_c13 = PC13;  // Port C
constexpr auto pin_d0 = PD0;    // Port D

// And all are constexpr (zero runtime cost)
static_assert(std::is_const_v<decltype(PA0)>);
```

---

### Requirement: Compile-Time Pin Validation
**ID**: PIN-GEN-002
**Priority**: P0 (Critical)

The system SHALL provide C++20 concepts to validate pin numbers at compile time, preventing use of non-existent pins.

#### Scenario: Valid pin compiles successfully
```cpp
// Given generated pins for STM32F103C8
#include <hal/st/stm32f1/generated/stm32f103c8/pins.hpp>
using namespace alloy::hal::stm32f1::stm32f103c8::pins;

// When using valid pin
template<uint8_t Pin>
void configure_gpio() {
    validate_pin<Pin>();  // Compile-time check
    // GPIO configuration code...
}

// Then code compiles without error
configure_gpio<PC13>();  // ✅ Compiles (PC13 exists)
```

#### Scenario: Invalid pin causes compile error
```cpp
// Given STM32F103C8 (LQFP48 package)
// When attempting to use pin not available on this package
constexpr auto bad_pin = 100;  // PG4 - doesn't exist on LQFP48

template<uint8_t Pin>
void configure() {
    validate_pin<Pin>();  // Static assertion
}

configure<bad_pin>();

// Then compilation fails with clear error:
// ❌ static_assert failed: "Invalid pin for STM32F103C8 (LQFP48)!
//    Available pins: PA0-PA15, PB0-PB15, PC13-PC15, PD0-PD1"
```

#### Scenario: Concept-based pin validation
```cpp
// Given ValidPin concept
template<uint8_t Pin>
concept ValidPin = is_valid_pin_v<Pin>;

// When using concept in template
template<uint8_t Pin> requires ValidPin<Pin>
class GpioPin {
    // Implementation
};

// Then invalid pins are rejected at compile time
GpioPin<PC13> valid_pin;   // ✅ Compiles
GpioPin<100> invalid_pin;  // ❌ Compile error: concept not satisfied
```

---

### Requirement: MCU Traits Generation
**ID**: PIN-GEN-003
**Priority**: P1 (High)

The system SHALL generate MCU trait headers containing device characteristics (flash, RAM, peripherals).

#### Scenario: Traits provide memory information
```cpp
// Given STM32F103C8 traits generated
#include <hal/st/stm32f1/generated/stm32f103c8/traits.hpp>
using Traits = alloy::hal::stm32f1::stm32f103c8::Traits;

// When accessing memory constants
constexpr uint32_t flash_size = Traits::FLASH_SIZE;
constexpr uint32_t sram_size = Traits::SRAM_SIZE;

// Then correct values are provided
static_assert(flash_size == 64 * 1024);  // 64 KB
static_assert(sram_size == 20 * 1024);   // 20 KB

// And all are constexpr (compile-time)
static_assert(Traits::FLASH_BASE == 0x08000000);
```

#### Scenario: Traits show peripheral availability
```cpp
// Given STM32F103C8 peripheral configuration
using Peripherals = alloy::hal::stm32f1::stm32f103c8::Traits::Peripherals;

// When checking peripheral counts
constexpr uint8_t uart_count = Peripherals::UART_COUNT;
constexpr uint8_t i2c_count = Peripherals::I2C_COUNT;
constexpr bool has_usb = Peripherals::HAS_USB;

// Then accurate counts are provided
static_assert(uart_count == 3);  // USART1, USART2, USART3
static_assert(i2c_count == 2);   // I2C1, I2C2
static_assert(has_usb == true);  // USB device available
static_assert(Peripherals::HAS_DAC == false);  // No DAC
```

---

### Requirement: Board Configuration Simplification
**ID**: PIN-GEN-004
**Priority**: P0 (Critical)

The system SHALL enable board files to use generated pins directly without manual pin number mapping.

#### Scenario: Board uses generated pins
```cpp
// Given Bluepill board with STM32F103C8
// Before (manual pin numbers):
namespace board {
    constexpr auto LED = 45;  // Is this correct?
}

// After (generated pins):
#include <hal/st/stm32f1/generated/stm32f103c8/pins.hpp>

namespace board {
    using namespace alloy::hal::stm32f1::stm32f103c8::pins;
    constexpr auto LED = PC13;      // ✅ Type-safe
    constexpr auto UART_TX = PA9;   // ✅ Self-documenting
    constexpr auto UART_RX = PA10;  // ✅ Compile-time checked
}

// Then pins are clear, type-safe, and validated
```

#### Scenario: IDE auto-completion for pins
```cpp
// Given generated pin namespace imported
using namespace alloy::hal::stm32f1::stm32f103c8::pins;

// When typing "P" in IDE
// Then auto-complete shows:
//   PA0, PA1, PA2, ..., PA15
//   PB0, PB1, PB2, ..., PB15
//   PC13, PC14, PC15
//   PD0, PD1

// And user can discover available pins without datasheet
constexpr auto my_pin = P[TAB]  // Shows all options
```

---

### Requirement: Zero Runtime Overhead
**ID**: PIN-GEN-005
**Priority**: P0 (Critical)

The system SHALL ensure all pin definitions and validation occur at compile time with zero runtime cost.

#### Scenario: Pin constants are constexpr
```cpp
// Given generated pins
using namespace alloy::hal::stm32f1::stm32f103c8::pins;

// When using pins
constexpr uint8_t led_pin = PC13;
constexpr bool is_valid = is_valid_pin_v<PC13>;

// Then all evaluation happens at compile time
static_assert(led_pin == 45);
static_assert(is_valid == true);

// And no runtime code is generated for pin lookup
// (verify with: objdump -d build/firmware.elf | grep "PC13")
```

#### Scenario: Validation has no runtime cost
```cpp
// Given pin validation function
template<uint8_t Pin>
void configure() {
    validate_pin<Pin>();  // Compile-time only
    // GPIO code here
}

// When compiled with optimization
configure<PC13>();

// Then static_assert is evaluated at compile time
// And no validation code exists in binary
// And function body contains only GPIO code
```

---

### Requirement: Code Generation Pipeline Integration
**ID**: PIN-GEN-006
**Priority**: P0 (Critical)

The system SHALL integrate pin generation into existing codegen pipeline seamlessly.

#### Scenario: Makefile triggers pin generation
```bash
# Given codegen pipeline
# When running make codegen
make codegen

# Then pin generation runs automatically:
# 1. Update SVD files
# 2. Generate pin database (NEW)
# 3. Generate pin headers (NEW)
# 4. Generate peripheral code (existing)

# And output shows progress:
# 🔍 Scanning SVD files...
# 📝 Generating pin database...
# 🎯 Generating pins for STM32F103C8... ✓
# 🎯 Generating pins for STM32F103CB... ✓
```

#### Scenario: Pin headers regenerate on SVD changes
```bash
# Given existing generated pins
# When custom SVD is added:
cp NewDevice.svd tools/codegen/custom-svd/vendors/STMicro/

# And codegen is run:
make codegen

# Then new pin header is created:
# ✅ Generated: src/hal/st/stm32f1/generated/newdevice/pins.hpp
# ✅ Generated: src/hal/st/stm32f1/generated/newdevice/traits.hpp
```

---

### Requirement: Package-Specific Pin Availability
**ID**: PIN-GEN-007
**Priority**: P2 (Medium)

The system SHALL generate different pin sets for different package variants of the same MCU.

#### Scenario: LQFP48 vs LQFP64 packages
```cpp
// Given STM32F103C8 (LQFP48) - 37 GPIO pins
#include <hal/st/stm32f1/generated/stm32f103c8/pins.hpp>
namespace c8 = alloy::hal::stm32f1::stm32f103c8::pins;

// Given STM32F103RC (LQFP64) - 51 GPIO pins
#include <hal/st/stm32f1/generated/stm32f103rc/pins.hpp>
namespace rc = alloy::hal::stm32f1::stm32f103rc::pins;

// When comparing pin availability
static_assert(c8::TOTAL_PIN_COUNT == 37);
static_assert(rc::TOTAL_PIN_COUNT == 51);

// Then RC variant has additional pins:
constexpr auto pin = rc::PE15;  // ✅ Available on LQFP64
// constexpr auto bad = c8::PE15;  // ❌ Not available on LQFP48
```
