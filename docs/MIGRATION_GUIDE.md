# Migration Guide: Old Architecture → Policy-Based Design

**Version**: 1.0
**Last Updated**: 2025-11-11
**Target Audience**: Developers migrating existing code to the new policy-based peripheral architecture

---

## Table of Contents

1. [Overview](#overview)
2. [Why Migrate?](#why-migrate)
3. [Breaking Changes](#breaking-changes)
4. [Migration Strategy](#migration-strategy)
5. [Step-by-Step Migration](#step-by-step-migration)
6. [Common Patterns](#common-patterns)
7. [Platform-Specific Changes](#platform-specific-changes)
8. [Troubleshooting](#troubleshooting)
9. [FAQ](#faq)

---

## Overview

The new **Policy-Based Design** architecture provides:
- **Zero-overhead abstraction** - All methods are `static inline`
- **Compile-time configuration** - No runtime overhead
- **Multi-platform support** - Same API across SAME70, STM32F4, STM32F1
- **Testable hardware** - Mock register system for unit tests
- **Auto-generated policies** - Consistent, error-free code generation

This guide will help you migrate from the old template-based architecture to the new policy-based design.

---

## Why Migrate?

### Problems with Old Architecture

```cpp
// ❌ OLD: Runtime overhead, hard to test
class Uart {
    volatile uint32_t* base_;  // Runtime pointer

    void set_baudrate(uint32_t baud) {
        // Pointer dereference + virtual call overhead
        auto* regs = reinterpret_cast<UartRegs*>(base_);
        regs->BRR = calculate_brr(baud);
    }
};
```

**Issues**:
- Runtime overhead (pointer dereference, virtual calls)
- Hard to test (requires hardware or complex mocking)
- Platform-specific code scattered across files
- Inconsistent register access patterns

### Benefits of New Architecture

```cpp
// ✅ NEW: Zero overhead, testable
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70UartHardwarePolicy {
    static inline void set_baudrate(uint32_t baud) {
        // Direct register access, inlined at compile-time
        hw()->BRGR = (PERIPH_CLOCK_HZ / baud / 16);
    }
};
```

**Benefits**:
- **Zero runtime overhead** - Compiles to same assembly as hand-written code
- **Testable** - Mock register system for unit tests
- **Type-safe** - Template parameters checked at compile-time
- **Consistent** - All platforms use same pattern

---

## Breaking Changes

### 1. Namespace Changes

```cpp
// ❌ OLD
#include "hal/uart.hpp"
using namespace alloy::hal;

Uart0 uart;
uart.initialize();

// ✅ NEW
#include "hal/api/uart_simple.hpp"
#include "hal/platform/same70/uart.hpp"
using namespace alloy::hal::same70;

Usart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
```

### 2. Include Paths

| Old Path | New Path |
|----------|----------|
| `hal/uart.hpp` | `hal/api/uart_simple.hpp` |
| `hal/spi.hpp` | `hal/api/spi_simple.hpp` |
| `hal/i2c.hpp` | `hal/api/i2c_simple.hpp` |
| `hal/gpio.hpp` | `hal/api/gpio_simple.hpp` |

### 3. Initialization Pattern

```cpp
// ❌ OLD: Object-oriented
Uart0 uart;
uart.set_baudrate(115200);
uart.configure_8n1();
uart.enable_tx();
uart.enable_rx();

// ✅ NEW: Static policy-based
auto config = Usart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
config.initialize();
```

### 4. API Levels

The new architecture provides **three API levels**:

| Level | Old Equivalent | New API | Use Case |
|-------|----------------|---------|----------|
| **Level 1** | Simple API | `Uart` | Beginners, quick prototypes |
| **Level 2** | Fluent API | `UartBuilder` | Common tasks, readable code |
| **Level 3** | Expert API | `UartExpertConfig` | Advanced, performance-critical |

---

## Migration Strategy

### Phase 1: Understand Your Current Code

1. **Identify all peripheral usage**
   ```bash
   grep -r "hal/uart.hpp" src/
   grep -r "hal/spi.hpp" src/
   ```

2. **Check platform dependencies**
   ```bash
   grep -r "same70::" src/
   grep -r "stm32f4::" src/
   ```

3. **Find compile-time vs runtime configuration**
   ```bash
   # Old code with runtime config
   grep -r "uart.set_baudrate" src/
   ```

### Phase 2: Choose Migration Approach

#### Option A: Incremental Migration (Recommended)
- Migrate one peripheral at a time
- Keep old code working during transition
- Test each migration step

#### Option B: Full Migration
- Migrate entire codebase at once
- Requires comprehensive testing
- Faster but riskier

### Phase 3: Update Build System

```cmake
# CMakeLists.txt - Add new include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/hal/api         # NEW
    ${CMAKE_SOURCE_DIR}/src/hal/platform    # NEW
    ${CMAKE_SOURCE_DIR}/src/hal/vendors     # NEW
)
```

---

## Step-by-Step Migration

### Example: Migrating UART Code

#### Step 1: Old UART Code

```cpp
// ❌ OLD: src/app/serial_logger.cpp
#include "hal/uart.hpp"

using namespace alloy::hal;

class SerialLogger {
    Uart0 uart_;

public:
    void initialize() {
        uart_.set_baudrate(115200);
        uart_.configure_8n1();
        uart_.enable_tx();
        uart_.enable_rx();
    }

    void log(const char* msg) {
        while (*msg) {
            uart_.write_byte(*msg++);
        }
    }
};
```

#### Step 2: New UART Code (Simple API)

```cpp
// ✅ NEW: src/app/serial_logger.cpp
#include "hal/api/uart_simple.hpp"
#include "hal/platform/same70/uart.hpp"

using namespace alloy::hal;
using namespace alloy::hal::same70;

class SerialLogger {
    // No member variable needed - static API

public:
    void initialize() {
        // One-line setup
        auto config = Usart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
        config.initialize();
    }

    void log(const char* msg) {
        Usart0::write(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
    }
};
```

**Changes**:
- ✅ Changed include paths
- ✅ Used static API (no member variable)
- ✅ Simplified initialization
- ✅ Zero runtime overhead

#### Step 3: New UART Code (Fluent API)

```cpp
// ✅ BETTER: Using Fluent API for readable configuration
#include "hal/api/uart_fluent.hpp"
#include "hal/platform/same70/uart.hpp"

using namespace alloy::hal;
using namespace alloy::hal::same70;

class SerialLogger {
public:
    void initialize() {
        auto config = Usart0Builder{}
            .with_baudrate(BaudRate{115200})
            .with_tx_pin<TxPin>()
            .with_rx_pin<RxPin>()
            .with_8n1()
            .build();
        config.initialize();
    }

    void log(const char* msg) {
        Usart0::write(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
    }
};
```

**Benefits**:
- ✅ More readable configuration
- ✅ Same zero-overhead guarantee
- ✅ Compile-time validation

---

## Common Patterns

### Pattern 1: Simple Read/Write

```cpp
// ❌ OLD
Uart0 uart;
uart.write_byte('A');
uint8_t data = uart.read_byte();

// ✅ NEW (Simple API)
Usart0::write_byte('A');
uint8_t data = Usart0::read_byte();
```

### Pattern 2: Buffered Write

```cpp
// ❌ OLD
const char* msg = "Hello\n";
for (size_t i = 0; i < strlen(msg); i++) {
    uart.write_byte(msg[i]);
}

// ✅ NEW
const char* msg = "Hello\n";
Usart0::write(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
```

### Pattern 3: Interrupt Configuration

```cpp
// ❌ OLD
uart.enable_interrupt(UartInterrupt::RX);
uart.set_interrupt_handler([](){ /* handler */ });

// ✅ NEW (Expert API)
#include "hal/api/uart_expert.hpp"

auto config = Usart0ExpertConfig{}
    .with_rx_interrupt_enabled()
    .with_interrupt_handler([](){ /* handler */ });
config.initialize();
```

### Pattern 4: DMA Transfer

```cpp
// ❌ OLD
uart.enable_dma_tx();
uart.start_dma_transfer(buffer, size);

// ✅ NEW (DMA API)
#include "hal/api/uart_dma.hpp"

auto config = Usart0DmaConfig{}
    .with_tx_dma_channel<DMA_CH0>()
    .with_buffer(buffer, size);
config.start_transfer();
```

### Pattern 5: Multi-Platform Code

```cpp
// ❌ OLD: Platform-specific ifdefs
#ifdef PLATFORM_SAME70
    #include "hal/same70/uart.hpp"
    using Uart = Same70Uart;
#elif PLATFORM_STM32F4
    #include "hal/stm32f4/uart.hpp"
    using Uart = Stm32f4Uart;
#endif

// ✅ NEW: Conditional includes
#ifdef PLATFORM_SAME70
    #include "hal/platform/same70/uart.hpp"
    using namespace alloy::hal::same70;
#elif PLATFORM_STM32F4
    #include "hal/platform/stm32f4/uart.hpp"
    using namespace alloy::hal::stm32f4;
#endif

// Same API on all platforms!
auto config = Usart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
```

---

## Platform-Specific Changes

### SAME70 (Atmel Cortex-M7)

```cpp
// ❌ OLD
#include "hal/same70/uart.hpp"
Same70::Uart0 uart;

// ✅ NEW
#include "hal/platform/same70/uart.hpp"
using namespace alloy::hal::same70;
auto config = Usart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
```

**Platform Aliases**:
- `Usart0`, `Usart1`, `Usart2` - UART instances
- `Uart0`, `Uart1`, `Uart2`, `Uart3`, `Uart4` - Additional UART instances
- Clock: 150 MHz peripheral clock

### STM32F4 (ARM Cortex-M4)

```cpp
// ❌ OLD
#include "hal/stm32f4/uart.hpp"
Stm32f4::Usart1 uart;

// ✅ NEW
#include "hal/platform/stm32f4/uart.hpp"
using namespace alloy::hal::stm32f4;
auto config = Usart1::quick_setup<TxPin, RxPin>(BaudRate{115200});
```

**Platform Aliases**:
- `Usart1`, `Usart2`, `Usart3` - APB2 @ 84MHz
- `Uart4`, `Uart5` - APB1 @ 42MHz
- `Usart6` - APB2 @ 84MHz

### STM32F1 (ARM Cortex-M3, Blue Pill)

```cpp
// ❌ OLD
#include "hal/stm32f1/uart.hpp"
Stm32f1::Usart1 uart;

// ✅ NEW
#include "hal/platform/stm32f1/uart.hpp"
using namespace alloy::hal::stm32f1;
auto config = Usart1::quick_setup<TxPin, RxPin>(BaudRate{115200});
```

**Platform Aliases**:
- `Usart1` - APB2 @ 72MHz (PA9/PA10 on Blue Pill)
- `Usart2` - APB1 @ 36MHz
- `Usart3` - APB1 @ 36MHz

---

## Troubleshooting

### Error: "No such file or directory"

```
fatal error: hal/uart.hpp: No such file or directory
```

**Solution**: Update include paths
```cpp
// Change from:
#include "hal/uart.hpp"

// To:
#include "hal/api/uart_simple.hpp"
#include "hal/platform/same70/uart.hpp"
```

---

### Error: "Undefined reference to..."

```
undefined reference to `Uart0::initialize()'
```

**Cause**: Trying to use instance methods on static API

**Solution**: Use static methods
```cpp
// ❌ Wrong:
Uart0 uart;
uart.initialize();

// ✅ Correct:
auto config = Usart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
config.initialize();
```

---

### Error: "Template parameter not specified"

```
error: template argument required for 'Uart'
```

**Cause**: Old API expected non-templated class

**Solution**: Use platform-specific alias
```cpp
// ❌ Wrong:
Uart uart;

// ✅ Correct:
using namespace alloy::hal::same70;
auto config = Usart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
```

---

### Error: "No matching function for call"

```
error: no matching function for call to 'Uart::set_baudrate(int)'
```

**Cause**: Old API method doesn't exist in new API

**Solution**: Use configuration in setup
```cpp
// ❌ Old:
uart.set_baudrate(115200);

// ✅ New:
auto config = Usart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
```

---

### Performance Regression

**Symptom**: Code is slower after migration

**Cause**: Not using static inline methods correctly

**Solution**: Ensure compiler optimizations are enabled
```cmake
# CMakeLists.txt
target_compile_options(${TARGET_NAME} PRIVATE -O2)
```

**Verify zero overhead**:
```bash
# Check assembly output
arm-none-eabi-objdump -d build/app.elf | grep uart
```

Should show direct register access, no function calls.

---

## FAQ

### Q: Do I need to migrate all at once?

**A**: No. You can migrate incrementally, one peripheral at a time. The old and new APIs can coexist during transition.

### Q: Will my code be slower?

**A**: No. The new policy-based design has **zero runtime overhead**. All methods are `static inline` and compile to the same assembly as hand-written register access.

### Q: Can I test without hardware?

**A**: Yes! The new architecture includes a mock register system. See [HARDWARE_POLICY_GUIDE.md](HARDWARE_POLICY_GUIDE.md#testing-your-policy) for details.

### Q: What if I need custom behavior?

**A**: Use the **Expert API** (`UartExpertConfig`) which provides full control over all register settings while maintaining zero overhead.

### Q: How do I add a new platform?

**A**: See [HARDWARE_POLICY_GUIDE.md](HARDWARE_POLICY_GUIDE.md#creating-a-new-hardware-policy) for step-by-step instructions on adding new platform support.

### Q: Are there performance benchmarks?

**A**: Yes. See `openspec/changes/modernize-peripheral-architecture/PHASE_8_SUMMARY.md` for detailed benchmarks showing zero overhead vs old architecture.

### Q: What about backwards compatibility?

**A**: The old API is deprecated but still available. You can use both during migration. Plan to remove old API in next major version.

---

## Next Steps

1. **Read the [HARDWARE_POLICY_GUIDE.md](HARDWARE_POLICY_GUIDE.md)** to understand the new architecture
2. **Identify peripheral usage** in your codebase
3. **Start with UART migration** (simplest peripheral)
4. **Test thoroughly** with mock registers
5. **Migrate other peripherals** (SPI, I2C, GPIO, etc.)
6. **Remove old API usage** once migration is complete

---

## Additional Resources

- [HARDWARE_POLICY_GUIDE.md](HARDWARE_POLICY_GUIDE.md) - Comprehensive policy implementation guide
- [ARCHITECTURE.md](../openspec/changes/modernize-peripheral-architecture/ARCHITECTURE.md) - Policy-based design rationale
- [PHASE_8_SUMMARY.md](../openspec/changes/modernize-peripheral-architecture/PHASE_8_SUMMARY.md) - Implementation details and benchmarks
- [PHASE_10_SUMMARY.md](../openspec/changes/modernize-peripheral-architecture/PHASE_10_SUMMARY.md) - Multi-platform support details

---

**Questions?** Open an issue or contact the maintainers.
