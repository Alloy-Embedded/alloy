# Enumeration Usage Guide

## Overview

The enum generator creates type-safe `enum class` definitions for register field enumerated values extracted from CMSIS-SVD files. This provides compile-time type safety and better IDE auto-completion.

## Generation

```bash
# Generate enumerations for all board MCUs
./codegen generate --enums

# Generate everything including enums
./codegen generate
```

## Generated Output

For each MCU with enumerated values in its SVD file, an `enums.hpp` file is generated:

```
src/hal/vendors/st/stm32f4/stm32f407vg/
└── enums.hpp          # All enumerated types for this MCU
```

## File Structure

```cpp
/// Auto-generated enumeration definitions
#pragma once

#include <cstdint>

namespace alloy::hal::st::stm32f4::stm32f407vg::enums {

// RCC - Reset and clock control
enum class RCC_CFGR_SW : uint32_t {
    HSI = 0x0,        // HSI selected as system clock
    HSE = 0x1,        // HSE selected as system clock
    PLL = 0x2,        // PLL selected as system clock
    PLL_R = 0x3,      // PLL_R selected as system clock
};

enum class RCC_CR_PLLSRC : uint32_t {
    HSI = 0x0,        // HSI clock selected
    HSE = 0x1,        // HSE oscillator selected
};

// ... more enumerations

}  // namespace alloy::hal::st::stm32f4::stm32f407vg::enums
```

## Usage Examples

### Example 1: System Clock Selection

```cpp
#include <hal/st/stm32f4/stm32f407vg/registers/rcc_registers.hpp>
#include <hal/st/stm32f4/stm32f407vg/bitfields/rcc_bitfields.hpp>
#include <hal/st/stm32f4/stm32f407vg/enums.hpp>

using namespace alloy::hal::st::stm32f4::stm32f407vg;

void select_system_clock() {
    // Type-safe clock source selection
    using ClockSource = enums::RCC_CFGR_SW;

    // Select PLL as system clock
    auto sw_value = static_cast<uint32_t>(ClockSource::PLL);
    rcc::RCC->CFGR = rcc::cfgr::SW::write(rcc::RCC->CFGR, sw_value);

    // Wait for PLL to be ready
    while (rcc::cfgr::SWS::read(rcc::RCC->CFGR) != sw_value) {
        // Waiting...
    }
}
```

### Example 2: ADC Resolution Configuration

```cpp
#include <hal/st/stm32f4/stm32f407vg/registers/adc_registers.hpp>
#include <hal/st/stm32f4/stm32f407vg/bitfields/adc_bitfields.hpp>
#include <hal/st/stm32f4/stm32f407vg/enums.hpp>

using namespace alloy::hal::st::stm32f4::stm32f407vg;

void configure_adc_resolution() {
    // Type-safe resolution selection
    using Resolution = enums::ADC_CR1_RES;

    // Configure for 12-bit resolution
    auto res_value = static_cast<uint32_t>(Resolution::BITS_12);
    adc::ADC1->CR1 = adc::cr1::RES::write(adc::ADC1->CR1, res_value);

    // ✅ Type-safe - won't compile with invalid enum
    // ❌ auto bad = Resolution::INVALID;  // Compile error
}
```

### Example 3: GPIO Mode Selection

```cpp
#include <hal/st/stm32f4/stm32f407vg/registers/gpio_registers.hpp>
#include <hal/st/stm32f4/stm32f407vg/bitfields/gpio_bitfields.hpp>
#include <hal/st/stm32f4/stm32f407vg/enums.hpp>

using namespace alloy::hal::st::stm32f4::stm32f407vg;

void configure_gpio_pin() {
    // Type-safe GPIO mode selection
    using Mode = enums::GPIO_MODER_MODE;

    // Set PA5 to output mode
    auto mode_value = static_cast<uint32_t>(Mode::OUTPUT);
    gpio::GPIOA->MODER = gpio::moder::MODE5::write(gpio::GPIOA->MODER, mode_value);
}
```

## Naming Convention

Enum class names follow the pattern: `{Peripheral}_{Register}_{Field}`

**Examples:**
- `RCC_CFGR_SW` - System clock switch (RCC peripheral, CFGR register, SW field)
- `ADC_CR1_RES` - ADC resolution (ADC peripheral, CR1 register, RES field)
- `GPIO_MODER_MODE` - GPIO mode (GPIO peripheral, MODER register, MODE field)

## Underlying Types

The enum underlying type is automatically selected based on register size:

- 8-bit registers → `uint8_t`
- 16-bit registers → `uint16_t`
- 32-bit registers → `uint32_t`

```cpp
// 32-bit register field
enum class RCC_CFGR_SW : uint32_t { ... };

// 16-bit register field
enum class TIM_CR1_CKD : uint16_t { ... };

// 8-bit register field
enum class I2C_OAR1_ADD : uint8_t { ... };
```

## Type Safety Benefits

### Compile-Time Validation

```cpp
// ✅ Type-safe - only valid enum values accepted
auto source = enums::RCC_CFGR_SW::PLL;

// ❌ Won't compile - type mismatch
enums::RCC_CFGR_SW bad = enums::ADC_CR1_RES::BITS_12;  // Error!

// ❌ Won't compile - invalid value
enums::RCC_CFGR_SW invalid = 99;  // Error!
```

### IDE Auto-Completion

Modern IDEs provide excellent auto-completion for enum classes:

```cpp
using ClockSource = enums::RCC_CFGR_SW;

// Type "ClockSource::" and IDE shows:
// - HSI
// - HSE
// - PLL
// - PLL_R
```

## Scoped Enumerations

All enums are scoped (enum class), preventing name collisions:

```cpp
// ✅ No collision - fully qualified names
enums::RCC_CFGR_SW::HSI
enums::RCC_PLLCFGR_PLLSRC::HSI

// Both can coexist without conflict
```

## Integration with BitFields

Enums work seamlessly with the BitField template system:

```cpp
#include <hal/st/stm32f4/stm32f407vg/registers/rcc_registers.hpp>
#include <hal/st/stm32f4/stm32f407vg/bitfields/rcc_bitfields.hpp>
#include <hal/st/stm32f4/stm32f407vg/enums.hpp>

using namespace alloy::hal::st::stm32f4::stm32f407vg;

void complete_example() {
    // 1. Use enum for type-safe value
    using ClockSource = enums::RCC_CFGR_SW;
    auto source = ClockSource::PLL;

    // 2. Cast to integer for bit field operation
    uint32_t value = static_cast<uint32_t>(source);

    // 3. Write using bit field template
    rcc::RCC->CFGR = rcc::cfgr::SW::write(rcc::RCC->CFGR, value);

    // 4. Read and compare
    uint32_t current = rcc::cfgr::SW::read(rcc::RCC->CFGR);
    if (current == value) {
        // Clock source switched successfully
    }
}
```

## SVD Files Without Enumerations

Some SVD files don't define enumerated values. In these cases, `enums.hpp` will be generated but contain no enum definitions:

```cpp
/// Auto-generated enumeration definitions
#pragma once

#include <cstdint>

namespace alloy::hal::st::stm32f1::stm32f103xx::enums {

// No enumerated values found in SVD file

}  // namespace alloy::hal::st::stm32f1::stm32f103xx::enums
```

This is normal and doesn't indicate an error. You can still use bit field templates with literal values.

## Performance

**Zero Runtime Overhead** ✅

Enum classes compile to the same assembly as literal integers:

```cpp
// Enum version
auto source = enums::RCC_CFGR_SW::PLL;  // 0x2

// Literal version
uint32_t source = 0x2;

// Both generate IDENTICAL assembly:
// mov r0, #2
```

The type safety is purely compile-time with no runtime cost.

## Best Practices

### 1. Use Type Aliases for Readability

```cpp
// ✅ Good - clear intent
using ClockSource = enums::RCC_CFGR_SW;
auto source = ClockSource::PLL;

// ❌ Harder to read
auto source = enums::RCC_CFGR_SW::PLL;
```

### 2. Always Cast When Writing to Hardware

```cpp
// ✅ Good - explicit cast
uint32_t value = static_cast<uint32_t>(ClockSource::PLL);
RCC->CFGR = rcc::cfgr::SW::write(RCC->CFGR, value);

// ❌ Won't compile - enum is not implicitly convertible
RCC->CFGR = rcc::cfgr::SW::write(RCC->CFGR, ClockSource::PLL);
```

### 3. Compare Enum Values Directly

```cpp
// ✅ Good - type-safe comparison
if (current_source == ClockSource::PLL) { ... }

// ❌ Unnecessary cast for comparison
if (static_cast<uint32_t>(current_source) == 0x2) { ... }
```

## Troubleshooting

### Issue: Enum class not found

**Cause**: SVD file doesn't define enumerated values for that field

**Solution**: Use literal values directly with bit field templates:

```cpp
// If no enum available, use literal
rcc::RCC->CFGR = rcc::cfgr::SW::write(rcc::RCC->CFGR, 0x2);  // PLL
```

### Issue: Type mismatch when writing

**Cause**: Forgot to cast enum to integer

**Solution**: Always cast enum values:

```cpp
// ✅ Correct
auto value = static_cast<uint32_t>(ClockSource::PLL);
RCC->CFGR = rcc::cfgr::SW::write(RCC->CFGR, value);
```

### Issue: Enum name collision

**Cause**: Two fields have same enum value name

**Solution**: All enums are scoped - use full qualified name:

```cpp
// ✅ No collision
enums::RCC_CFGR_SW::HSI
enums::RCC_PLLCFGR_PLLSRC::HSI
```

## Summary

Enumeration generation provides:

✅ **Type Safety** - Compile-time validation of values
✅ **IDE Support** - Auto-completion for all enum values
✅ **Zero Overhead** - Same performance as literal integers
✅ **Scoped Names** - No namespace pollution
✅ **Documentation** - Inline comments from SVD
✅ **Integration** - Works seamlessly with BitField templates

Use enums whenever available for safer, more maintainable embedded code!
