# STM32 Common Code

This directory contains code **shared across all STM32 families** (F0, F1, F4, F7, G0, etc.).

## Purpose

STM32 microcontrollers share many common peripherals inherited from the ARM Cortex-M architecture and ST's design philosophy. Instead of duplicating identical code across each family, this directory provides:

1. **ARM Cortex-M Standard Peripherals** - SysTick, NVIC, MPU, FPU
2. **STM32 Common Utilities** - Shared helper functions and types
3. **Policy Base Classes** - Common hardware policy templates

## Files

### `systick_platform.hpp`

**Shared across**: STM32F4, STM32F7, STM32G0, and all other STM32 families

ARM Cortex-M SysTick timer implementation. Since all STM32 microcontrollers use ARM Cortex-M cores, the SysTick peripheral is **identical** across families.

**Usage**:
```cpp
#include "hal/vendors/st/common/systick_platform.hpp"

namespace alloy::hal::st::stm32f4 {
    // Use common implementation
    using SysTickPlatform = common::SysTickPlatform<84000000>;  // 84 MHz
}
```

## Design Principles

### 1. Namespace Organization

Common code lives in `alloy::hal::st::common` namespace. Family-specific code can:

- **Type alias** the common implementation:
  ```cpp
  using SysTickPlatform = common::SysTickPlatform<CLOCK_HZ>;
  ```

- **Inherit** from common base classes:
  ```cpp
  class Stm32f4SysTick : public common::SysTickPlatform<84000000> {};
  ```

### 2. Template-Based Configuration

Common code uses templates to handle family-specific parameters:

```cpp
template <uint32_t CLOCK_HZ>
class SysTickPlatform {
    static constexpr uint32_t clock_hz = CLOCK_HZ;
    // Implementation works for any clock frequency
};
```

### 3. Zero Runtime Overhead

- All abstractions are compile-time
- No virtual functions
- Fully inlined
- Same performance as hand-written code for each family

## Adding New Common Code

Before adding code to this directory, verify it meets these criteria:

1. ✅ **Truly Identical** - Works the same way on all STM32 families
2. ✅ **No Family-Specific Logic** - Uses only ARM Cortex-M standard or STM32-wide features
3. ✅ **Template Parameterizable** - Can handle family differences via template parameters
4. ✅ **Zero Overhead** - No runtime polymorphism

## Examples of Common vs Family-Specific

### Common Peripherals (Belong Here)

- **SysTick** - ARM Cortex-M standard, identical on all STM32
- **NVIC** - ARM Cortex-M standard interrupt controller
- **MPU** - ARM Cortex-M memory protection (when present)
- **SCB** - ARM System Control Block

### Family-Specific Peripherals (Stay in Family Directories)

- **RCC** - Reset and Clock Control (very different per family)
- **GPIO** - While similar, register layouts differ significantly
- **UART/USART** - Different feature sets per family
- **Timers** - Capabilities vary widely

## Migration Strategy

When you find duplicated code across families:

1. **Compare implementations** - Ensure they're truly identical
2. **Extract to common/** - Move to this directory with template parameters
3. **Update family code** - Use type aliases to the common implementation
4. **Test all families** - Ensure no regressions

## File Organization

```
src/hal/vendors/st/common/
├── README.md                 # This file
├── systick_platform.hpp      # ARM Cortex-M SysTick timer
└── (future additions)
```

## License

Same as the rest of the Alloy Framework.
