# Peripheral Address Pattern

## Overview

This document describes the standard pattern for referencing peripheral base addresses and IRQ numbers in the HAL platform layer.

## Pattern: Use Generated Peripherals

**All platform layer files (GPIO, SPI, I2C, UART, etc.) MUST reference peripheral addresses from the auto-generated `peripherals.hpp` file instead of using hardcoded magic numbers.**

### Rationale

1. **Single Source of Truth**: All addresses come from the SVD file
2. **Maintainability**: Changing an address only requires regenerating from SVD
3. **Consistency**: Ensures all code uses the same addresses
4. **Scalability**: Easy to support new MCU variants

### Implementation Pattern

#### 1. Include the Generated Peripherals Header

```cpp
// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"
```

#### 2. Reference Peripheral Base Addresses

**❌ BAD - Hardcoded Magic Numbers:**
```cpp
constexpr uint32_t PIOA_BASE = 0x400E0E00;
constexpr uint32_t PIOB_BASE = 0x400E1000;
```

**✅ GOOD - Reference Generated Values:**
```cpp
constexpr uint32_t PIOA_BASE = alloy::generated::atsame70q21b::peripherals::PIOA;
constexpr uint32_t PIOB_BASE = alloy::generated::atsame70q21b::peripherals::PIOB;
```

#### 3. Reference Peripheral IDs (for IRQs and Clock Enable)

**❌ BAD - Hardcoded IRQ Numbers:**
```cpp
constexpr uint32_t SPI0_IRQ = 21;
constexpr uint32_t SPI1_IRQ = 42;
```

**✅ GOOD - Reference Generated IDs:**
```cpp
constexpr uint32_t SPI0_IRQ = alloy::generated::atsame70q21b::id::SPI0;
constexpr uint32_t SPI1_IRQ = alloy::generated::atsame70q21b::id::SPI1;
```

## Files Updated

The following files have been updated to follow this pattern:

### SAME70 Platform (2025-11-14)

1. **`src/hal/platform/same70/gpio.hpp`**
   - ✅ Uses `peripherals::PIOA/B/C/D/E` for base addresses
   - ✅ Updated `get_pin_id()` compile-time checks

2. **`src/hal/platform/same70/i2c.hpp`**
   - ✅ Uses `peripherals::TWIHS0/1/2` for base addresses
   - ✅ Uses `id::TWIHS0/1/2` for IRQ numbers

3. **`src/hal/platform/same70/spi.hpp`**
   - ✅ Uses `peripherals::SPI0/1` for base addresses
   - ✅ Uses `id::SPI0/1` for IRQ numbers

## Generated Peripherals Structure

The `peripherals.hpp` file (generated from SVD by `generate_from_svd.py`) contains:

```cpp
namespace alloy::generated::atsame70q21b {

// Peripheral base addresses
namespace peripherals {
    constexpr uintptr_t PIOA = 0x400E0E00;
    constexpr uintptr_t PIOB = 0x400E1000;
    constexpr uintptr_t SPI0 = 0x40008000;
    constexpr uintptr_t TWIHS0 = 0x40018000;
    // ... etc
}

// Peripheral IDs for clock management and interrupts
namespace id {
    constexpr uint8_t PIOA = 10;
    constexpr uint8_t SPI0 = 21;
    constexpr uint8_t TWIHS0 = 19;
    // ... etc
}

// Memory map
namespace memory {
    constexpr uintptr_t SDRAMC = 0x40084000;
}

}  // namespace alloy::generated::atsame70q21b
```

## Future Generator Requirements

When creating generators for platform layer files (GPIO, SPI, I2C, etc.), the generator MUST:

1. Add include for `peripherals.hpp`:
   ```cpp
   #include "hal/vendors/{vendor}/{family}/{mcu}/peripherals.hpp"
   ```

2. Generate peripheral constant definitions using generated namespace:
   ```cpp
   constexpr uint32_t {PERIPH}_BASE = alloy::generated::{mcu}::peripherals::{PERIPH};
   constexpr uint32_t {PERIPH}_IRQ = alloy::generated::{mcu}::id::{PERIPH};
   ```

3. Never emit hardcoded hex addresses directly in platform files

## Benefits

✅ **Automatic Updates**: Regenerate from SVD to update all addresses
✅ **No Magic Numbers**: All addresses have semantic names
✅ **Compile-Time Safety**: Typos caught at compile time
✅ **Documentation**: Generated file includes peripheral descriptions
✅ **Multi-MCU Support**: Easy to support device variants

---

*Last Updated: 2025-11-14*
