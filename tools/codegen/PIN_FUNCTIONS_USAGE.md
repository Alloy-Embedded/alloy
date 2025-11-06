# Pin Alternate Function Usage Guide

## Overview

The pin function generator creates compile-time type-safe alternate function (AF) mappings for MCU pins. This ensures that invalid pin/function combinations are caught at compile time, preventing runtime configuration errors.

## Generation

```bash
# Generate pin AF mappings for all board MCUs
./codegen generate --pin-functions

# Generate everything including pin functions
./codegen generate
```

## Generated Output

For each MCU, two files are generated:

```
src/hal/vendors/st/stm32f1/stm32f103c8/
├── pin_functions.hpp              # Type-safe AF template
└── pin_functions_template.json    # Template for adding more mappings
```

## File Structure

### pin_functions.hpp

```cpp
/// Auto-generated pin alternate function mappings
#pragma once

#include <cstdint>

namespace alloy::hal::st::stm32f1::stm32f103c8::pin_functions {

// Peripheral signal tags
struct USART1_TX {};
struct USART1_RX {};
struct SPI1_SCK {};
struct I2C1_SCL {};

// AF template
template<uint8_t Pin, typename Function>
struct AlternateFunction;

// Specializations
template<>
struct AlternateFunction<PA9, USART1_TX> {
    static constexpr uint8_t af_number = 7;
};

// Convenience alias
template<uint8_t Pin, typename Function>
constexpr uint8_t AF = AlternateFunction<Pin, Function>::af_number;

}  // namespace
```

## Usage Examples

### Example 1: UART Pin Configuration

```cpp
#include <hal/st/stm32f1/stm32f103c8/pins.hpp>
#include <hal/st/stm32f1/stm32f103c8/pin_functions.hpp>
#include <hal/st/stm32f1/stm32f103c8/registers/gpio_registers.hpp>

using namespace alloy::hal::st::stm32f1::stm32f103c8;

void configure_uart_pins() {
    // Get AF numbers at compile time
    constexpr uint8_t tx_af = pin_functions::AF<pins::PA9, pin_functions::USART1_TX>;
    constexpr uint8_t rx_af = pin_functions::AF<pins::PA10, pin_functions::USART1_RX>;

    // Configure PA9 as USART1_TX (AF7)
    // ... configure GPIO mode, speed, etc ...

    // ✅ Type-safe - only valid combinations compile
    // ❌ This won't compile:
    // auto bad = pin_functions::AF<pins::PA9, pin_functions::I2C1_SCL>;  // Error!
}
```

### Example 2: SPI Pin Configuration

```cpp
#include <hal/st/stm32f4/stm32f407vg/pins.hpp>
#include <hal/st/stm32f4/stm32f407vg/pin_functions.hpp>

using namespace alloy::hal::st::stm32f4::stm32f407vg;

void configure_spi_pins() {
    using namespace pin_functions;

    // Get all AF numbers for SPI1
    constexpr uint8_t sck_af = AF<pins::PA5, SPI1_SCK>;   // AF5
    constexpr uint8_t miso_af = AF<pins::PA6, SPI1_MISO>; // AF5
    constexpr uint8_t mosi_af = AF<pins::PA7, SPI1_MOSI>; // AF5

    // All three pins use the same AF number (5) for SPI1
    static_assert(sck_af == miso_af && miso_af == mosi_af,
                  "SPI1 pins should use same AF");

    // Configure pins...
}
```

### Example 3: Compile-Time Validation

```cpp
#include <hal/st/stm32f4/stm32f407vg/pin_functions.hpp>

using namespace alloy::hal::st::stm32f4::stm32f407vg;
using namespace pin_functions;

// Validate pin/function combinations at compile time
static_assert(HasAF<pins::PA9, USART1_TX>,
              "PA9 must support USART1_TX");

static_assert(HasAF<pins::PA10, USART1_RX>,
              "PA10 must support USART1_RX");

// This will fail at compile time if invalid:
// static_assert(HasAF<pins::PA9, I2C1_SCL>, "Invalid combination!");

void checked_uart_config() {
    // Only compiles if PA9 supports USART1_TX
    if constexpr (HasAF<pins::PA9, USART1_TX>) {
        constexpr uint8_t af = AF<pins::PA9, USART1_TX>;
        // Configure pin with AF number...
    }
}
```

### Example 4: Generic Pin Configuration Helper

```cpp
#include <hal/st/stm32f4/stm32f407vg/pins.hpp>
#include <hal/st/stm32f4/stm32f407vg/pin_functions.hpp>
#include <hal/st/stm32f4/stm32f407vg/registers/gpio_registers.hpp>

using namespace alloy::hal::st::stm32f4::stm32f407vg;

/// Configure a pin for alternate function use
///
/// @tparam Pin Pin number
/// @tparam Function Peripheral signal tag type
template<uint8_t Pin, typename Function>
void configure_pin_af() {
    // Get AF number at compile time
    constexpr uint8_t af = pin_functions::AF<Pin, Function>;

    // Determine which GPIO port based on pin
    constexpr uint8_t port = Pin / 16;  // PA=0, PB=1, etc.
    constexpr uint8_t pin_num = Pin % 16;

    // TODO: Configure GPIO for alternate function mode
    // - Set mode to AF
    // - Set AF number
    // - Configure speed, pull, etc.

    // Type safety enforced at compile time!
}

void example_usage() {
    // ✅ Valid combinations
    configure_pin_af<pins::PA9, pin_functions::USART1_TX>();
    configure_pin_af<pins::PA10, pin_functions::USART1_RX>();

    // ❌ Won't compile - invalid combination
    // configure_pin_af<pins::PA9, pin_functions::I2C1_SCL>();
}
```

## Adding Custom Pin Mappings

### Step 1: Edit the JSON Template

Each generated directory includes a `pin_functions_template.json`:

```json
{
  "mcu": "STM32F103C8",
  "vendor": "st",
  "family": "stm32f1",
  "pin_functions": [
    {
      "pin": "PA9",
      "peripheral": "USART1",
      "signal": "TX",
      "af": 7,
      "_comment": "STM32F1 USART1 TX on PA9"
    },
    {
      "pin": "PA10",
      "peripheral": "USART1",
      "signal": "RX",
      "af": 7
    },
    {
      "pin": "PB6",
      "peripheral": "I2C1",
      "signal": "SCL",
      "af": 4
    },
    {
      "pin": "PB7",
      "peripheral": "I2C1",
      "signal": "SDA",
      "af": 4
    }
  ]
}
```

### Step 2: Regenerate

```bash
# Regenerate with custom data
./codegen generate --pin-functions

# Or specify the JSON file explicitly
python3 cli/generators/generate_pin_functions.py \
  --svd STMicro/STM32F103xx.svd \
  --pin-data path/to/custom_pin_data.json
```

### Step 3: Use in Code

```cpp
// New mappings are now available
constexpr uint8_t af = pin_functions::AF<pins::PB6, pin_functions::I2C1_SCL>;
```

## Vendor-Specific Differences

### STM32 (ST)

STM32 uses AF numbers 0-15:

```cpp
// STM32F4 example
constexpr uint8_t af = AF<pins::PA9, USART1_TX>;  // AF7
```

### SAM (Atmel/Microchip)

Atmel SAM devices use SERCOM with pad numbers:

```cpp
// SAMD21 example
constexpr uint8_t af = AF<pins::PA22, SERCOM3_PAD0>;  // Function C
```

### RP2040 (Raspberry Pi)

RP2040 uses GPIO function select:

```cpp
// RP2040 example
constexpr uint8_t func = AF<pins::GPIO0, UART0_TX>;  // Function 2
```

## Type Safety Benefits

### Compile-Time Error Prevention

```cpp
// ✅ Valid - compiles successfully
constexpr uint8_t af1 = AF<pins::PA9, USART1_TX>;

// ❌ Compile error - PA9 doesn't support I2C
// constexpr uint8_t af2 = AF<pins::PA9, I2C1_SCL>;
// Error: incomplete type 'AlternateFunction<PA9, I2C1_SCL>'

// ❌ Compile error - wrong peripheral
// constexpr uint8_t af3 = AF<pins::PA9, USART2_TX>;
// Error: incomplete type 'AlternateFunction<PA9, USART2_TX>'
```

### IDE Support

Modern IDEs provide excellent auto-completion:

```cpp
using namespace pin_functions;

// Type "AF<pins::PA9, " and IDE shows only valid functions for PA9:
// - USART1_TX
// - TIM1_CH2
// - (other valid functions)
```

### Documentation in Code

```cpp
// Signal tag types are self-documenting
struct USART1_TX {};  // Clear what peripheral and signal
struct SPI1_MOSI {};  // More readable than "AF5" or "7"
```

## Performance

**Zero Runtime Overhead** ✅

AF lookup is purely compile-time:

```cpp
// This code:
constexpr uint8_t af = AF<pins::PA9, USART1_TX>;

// Compiles to:
// mov r0, #7

// No runtime lookup, no function call, just a constant.
```

## Integration with Existing Code

### With Pin Headers

```cpp
#include <hal/vendor/family/mcu/pins.hpp>
#include <hal/vendor/family/mcu/pin_functions.hpp>

// pins.hpp provides:
constexpr uint8_t PA9 = 9;

// pin_functions.hpp provides:
constexpr uint8_t af = pin_functions::AF<PA9, USART1_TX>;
```

### With Register Access

```cpp
#include <hal/vendor/family/mcu/registers/gpio_registers.hpp>
#include <hal/vendor/family/mcu/pin_functions.hpp>

void configure_af() {
    constexpr uint8_t af = pin_functions::AF<pins::PA9, USART1_TX>;

    // Write AF to GPIO AFR register
    gpio::GPIOA->AFRH = /* ... write AF value ... */;
}
```

## Best Practices

### 1. Use Descriptive Tag Types

```cpp
// ✅ Good - clear what peripheral and signal
using UartTx = pin_functions::USART1_TX;
constexpr uint8_t af = pin_functions::AF<pins::PA9, UartTx>;

// ❌ Less clear
constexpr uint8_t af = 7;  // What is 7? TX? RX? Which UART?
```

### 2. Validate at Compile Time

```cpp
// ✅ Good - catch errors early
template<uint8_t Pin, typename Function>
void configure_pin() {
    static_assert(pin_functions::HasAF<Pin, Function>,
                  "Invalid pin/function combination");
    constexpr uint8_t af = pin_functions::AF<Pin, Function>;
    // ...
}
```

### 3. Group Related Pins

```cpp
// ✅ Good - clear grouping
namespace uart1_pins {
    constexpr uint8_t tx = pins::PA9;
    constexpr uint8_t rx = pins::PA10;
    constexpr uint8_t tx_af = pin_functions::AF<tx, pin_functions::USART1_TX>;
    constexpr uint8_t rx_af = pin_functions::AF<rx, pin_functions::USART1_RX>;
}
```

## Troubleshooting

### Issue: Template incomplete type error

**Error**:
```
error: incomplete type 'AlternateFunction<PA9, I2C1_SCL>'
```

**Cause**: Pin doesn't support that function

**Solution**: Check datasheet for valid pin functions, or use different pin

### Issue: Missing signal tag

**Error**:
```
error: 'USART3_TX' was not declared in this scope
```

**Cause**: Pin function not in generated file

**Solution**: Add to `pin_functions_template.json` and regenerate

### Issue: Wrong AF number

**Cause**: Incorrect data in JSON template

**Solution**: Verify against MCU datasheet and correct JSON

## Summary

Pin alternate function generation provides:

✅ **Compile-Time Safety** - Invalid combinations won't compile
✅ **Zero Overhead** - Pure compile-time resolution
✅ **IDE Support** - Auto-completion for valid combinations
✅ **Self-Documenting** - Clear signal tag types
✅ **Vendor Agnostic** - Works with ST, Atmel, RP2040, etc.
✅ **Extensible** - Easy to add custom mappings via JSON

Use pin function templates to eliminate runtime pin configuration errors!
