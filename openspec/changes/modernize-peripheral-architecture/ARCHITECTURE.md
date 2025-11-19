# Architecture Decision: Policy-Based Design for Hardware Abstraction

## Executive Decision

**THIS PROJECT USES EXCLUSIVELY POLICY-BASED DESIGN FOR HARDWARE ABSTRACTION.**

No other techniques (traits, CRTP, inheritance, virtual functions) will be used for connecting generic APIs to platform-specific hardware.

## Why Policy-Based Design?

### What is Policy-Based Design?

A generic class accepts a **policy** (another class/struct) as a template parameter. The policy provides platform-specific implementation details without inheritance or runtime polymorphism.

```cpp
// Generic API accepts Policy as template parameter
template <typename HardwarePolicy>
class UartImpl {
    void initialize() {
        HardwarePolicy::reset();        // Calls policy method
        HardwarePolicy::set_baudrate(); // Calls policy method
    }
};

// Platform-specific policy (no inheritance!)
struct Same70UartPolicy {
    static void reset() { /* SAME70-specific code */ }
    static void set_baudrate() { /* SAME70-specific code */ }
};

// User-facing alias
using Uart0 = UartImpl<Same70UartPolicy>;
```

### Comparison with Other Techniques

| Technique | Pros | Cons | Decision |
|-----------|------|------|----------|
| **Policy-Based Design** | ✅ Zero overhead<br>✅ All compile-time<br>✅ Testable (mock policies)<br>✅ Clear separation | Slightly more template code | ✅ **CHOSEN** |
| **Traits** | Compile-time | Requires specialization for each type | ❌ **REJECTED** - Less flexible than policies |
| **CRTP** | No vtable | Still requires inheritance hierarchy | ❌ **REJECTED** - More complex, less clear |
| **Inheritance + Virtual** | Familiar OOP | Runtime overhead, vtable | ❌ **REJECTED** - Violates zero-overhead principle |
| **Template Specialization** | Compile-time | Lots of duplicate code | ❌ **REJECTED** - Hard to maintain |

### Why Not Traits?

```cpp
// ❌ Trait-based approach (REJECTED)
template <PeripheralId Id>
struct UartTraits;  // Forward declaration

// Requires explicit specialization for EACH peripheral
template <>
struct UartTraits<PeripheralId::USART0> {
    static void reset() { /* ... */ }
};

template <>
struct UartTraits<PeripheralId::USART1> {
    static void reset() { /* ... */ }
};
// ... repeat for every UART instance!

// Generic API
template <PeripheralId Id>
class Uart {
    void initialize() {
        UartTraits<Id>::reset();  // Uses trait
    }
};
```

**Problems:**
- Must specialize for EVERY peripheral instance (USART0, USART1, USART2...)
- Code duplication if instances share logic
- Less flexible than policies (can't easily compose or test)

### Why Not CRTP?

```cpp
// ❌ CRTP approach (REJECTED)
template <typename Derived>
class UartBase {
    void initialize() {
        auto* hw = static_cast<Derived*>(this);
        hw->reset_impl();  // Calls derived method
    }
};

class Same70Uart : public UartBase<Same70Uart> {
    void reset_impl() { /* SAME70-specific */ }
};
```

**Problems:**
- Still requires inheritance (even if static)
- More complex to understand
- Harder to test (need to instantiate derived class)
- Generic API is base class, feels backwards

### Why Not Inheritance?

```cpp
// ❌ Inheritance approach (REJECTED)
class UartInterface {
    virtual void reset() = 0;  // Pure virtual
    virtual void set_baudrate(uint32_t) = 0;
};

class Same70Uart : public UartInterface {
    void reset() override { /* ... */ }
    void set_baudrate(uint32_t) override { /* ... */ }
};
```

**Problems:**
- **Runtime overhead**: vtable lookup, can't inline
- **Binary size**: vtable in flash
- **Not zero-cost**: Violates fundamental design principle

## Policy-Based Design: The Right Choice

### Benefits

1. **Zero Overhead**
   ```cpp
   // All methods are static inline
   struct Policy {
       static inline void reset() { /* direct register access */ }
   };

   // Compiles to same assembly as hand-written code
   Policy::reset();  // No function call, just register write
   ```

2. **Testability**
   ```cpp
   // Mock policy for testing
   struct MockUartPolicy {
       static inline void reset() { mock_registers.CR = 0; }
   };

   using TestUart = UartImpl<MockUartPolicy>;
   ```

3. **Composability**
   ```cpp
   // Can easily compose policies
   template <typename UartPolicy, typename DmaPolicy>
   class UartWithDma {
       void setup() {
           UartPolicy::reset();
           DmaPolicy::configure();
       }
   };
   ```

4. **Clear Separation**
   - **Generic API**: Business logic, validation, error handling
   - **Policy**: Hardware access, register manipulation
   - No mixing of concerns

5. **Auto-Generation Friendly**
   ```cpp
   // Policies are simple structs - easy to generate from JSON
   struct Same70UartPolicy {
       static void reset() { hw()->CR = uart::cr::RSTRX::mask; }
       static void enable() { hw()->CR = uart::cr::TXEN::mask; }
   };
   ```

## Architecture Layers

```
User Code
    ↓
Platform Aliases (using declarations)
    ↓
Generic APIs (template <typename Policy>)
    ↓
Hardware Policies (static methods, inline)
    ↓
Register/Bitfield Definitions (generated)
    ↓
Hardware Registers
```

### Layer 1: Generic APIs

**Location**: `src/hal/api/`

```cpp
// hal/api/uart_simple.hpp
template <PeripheralId PeriphId, typename HardwarePolicy>
class UartImpl {
public:
    template <typename TxPin, typename RxPin>
    static auto quick_setup(BaudRate baud) {
        // Generic logic: validation, pin checking
        static_assert(is_valid_tx_pin<TxPin>(), "Invalid TX pin");

        return SimpleUartConfig<TxPin, RxPin, HardwarePolicy>{
            PeriphId, baud
        };
    }
};

template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig {
    Result<void, ErrorCode> initialize() const {
        // Generic: Configure pins
        TxPin::configure_alternate_function();

        // Policy: Configure hardware
        HardwarePolicy::reset();
        HardwarePolicy::set_baudrate(baudrate.value);
        HardwarePolicy::enable();

        return Ok();
    }
};
```

**Key Points:**
- ✅ No platform-specific code
- ✅ No `#ifdef` for different MCUs
- ✅ All hardware access goes through `HardwarePolicy`
- ✅ Testable with mock policies

### Layer 2: Hardware Policies

**Location**: `src/hal/vendors/{vendor}/{family}/`

```cpp
// hal/vendors/atmel/same70/uart_hardware_policy.hpp
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70UartHardwarePolicy {
    using RegisterType = atmel::same70::uart0::UART0_Registers;
    static constexpr uint32_t base_address = BASE_ADDR;

    static inline volatile RegisterType* hw() {
        #ifdef ALLOY_UART_MOCK_HW
            return ALLOY_UART_MOCK_HW();  // Test hook
        #else
            return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
        #endif
    }

    static inline void reset() {
        hw()->CR = uart::cr::RSTRX::mask | uart::cr::RSTTX::mask;
    }

    static inline void set_baudrate(uint32_t baud) {
        uint32_t cd = PERIPH_CLOCK_HZ / (16 * baud);
        hw()->BRGR = uart::brgr::CD::write(0, cd);
    }

    static inline void enable() {
        hw()->CR = uart::cr::TXEN::mask | uart::cr::RXEN::mask;
    }

    // ... all hardware operations
};

// Instances
using Uart0Hardware = Same70UartHardwarePolicy<0x400E0800, 150000000>;
using Uart1Hardware = Same70UartHardwarePolicy<0x400E0A00, 150000000>;
```

**Key Points:**
- ✅ All methods `static inline` - zero overhead
- ✅ Direct register access
- ✅ Mock hooks for testing
- ✅ Auto-generated from JSON metadata

### Layer 3: Platform Aliases

**Location**: `src/platform/{family}/peripherals.hpp`

```cpp
// platform/same70/peripherals.hpp
#include "hal/api/uart_simple.hpp"
#include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"

namespace alloy::platform::same70 {

using Uart0 = hal::UartImpl<hal::PeripheralId::USART0, Uart0Hardware>;
using Uart1 = hal::UartImpl<hal::PeripheralId::USART1, Uart1Hardware>;

}  // namespace
```

**Key Points:**
- ✅ User-facing API
- ✅ Hides template complexity
- ✅ One line per peripheral instance

### Layer 4: User Code

```cpp
// User application
#include "platform/same70/peripherals.hpp"

using namespace alloy::platform::same70;

int main() {
    // Simple, clean API
    auto uart = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
    uart.initialize();
    uart.write("Hello!\n", 7);
}
```

**Key Points:**
- ✅ No templates visible to user
- ✅ No platform-specific knowledge needed
- ✅ Same code works on all platforms (just change include)

## Comparison: Same Code, Different Platforms

### SAME70

```cpp
#include "platform/same70/peripherals.hpp"
using namespace alloy::platform::same70;

auto uart = Uart0::quick_setup<PinD3, PinD4>(BaudRate{115200});
```

**Behind the scenes:**
- Uses `Same70UartHardwarePolicy`
- Accesses registers at `0x400E0800`
- Uses SAME70-specific bitfields
- IRQ 7

### STM32F4

```cpp
#include "platform/stm32f4/peripherals.hpp"
using namespace alloy::platform::stm32f4;

auto uart = Usart1::quick_setup<PinA9, PinA10>(BaudRate{115200});
```

**Behind the scenes:**
- Uses `Stm32F4UartHardwarePolicy`
- Accesses registers at `0x40011000`
- Uses STM32F4-specific bitfields
- IRQ 37

**Same generic API, different policies!**

## Why This is Better Than Alternatives

### vs. Traits

| Aspect | Policies | Traits |
|--------|----------|--------|
| Code reuse | ✅ Template parameter | ❌ Must specialize each |
| Flexibility | ✅ Can swap easily | ❌ Fixed at compile-time |
| Testing | ✅ Inject mock policy | ❌ Hard to mock traits |
| Maintenance | ✅ One policy per family | ❌ One trait per instance |

### vs. CRTP

| Aspect | Policies | CRTP |
|--------|----------|------|
| Clarity | ✅ Explicit injection | ⚠️ Inheritance confusing |
| Direction | ✅ Generic → Specific | ⚠️ Specific → Generic |
| Testing | ✅ Mock policy | ⚠️ Must derive mock |
| Complexity | ✅ Simple template | ⚠️ Recursive templates |

### vs. Inheritance

| Aspect | Policies | Inheritance |
|--------|----------|-------------|
| Overhead | ✅ Zero (inline) | ❌ Vtable lookup |
| Binary size | ✅ No vtable | ❌ Vtable in flash |
| Flexibility | ✅ Compile-time swap | ⚠️ Runtime polymorphism |
| Performance | ✅ Inlined | ❌ Indirect call |

## Consistency Across Project

**ALL peripheral implementations SHALL use policy-based design:**

- ✅ UART: `UartImpl<PeriphId, HardwarePolicy>`
- ✅ SPI: `SpiImpl<PeriphId, HardwarePolicy>`
- ✅ I2C: `I2cImpl<PeriphId, HardwarePolicy>`
- ✅ Interrupt: `Interrupt` uses `InterruptControllerPolicy`
- ✅ GPIO: `GpioImpl<...>`
- ✅ DMA: `DmaImpl<...>`
- ✅ ADC: `AdcImpl<...>`
- ✅ Timer: `TimerImpl<...>`

**No exceptions. No mixing of techniques.**

## Code Generation

Policies are **auto-generated** from JSON metadata:

```json
{
  "family": "same70",
  "peripheral": "UART",
  "policy_methods": {
    "reset": {
      "code": "hw()->CR = uart::cr::RSTRX::mask;"
    }
  }
}
```

↓ **Generator** ↓

```cpp
struct Same70UartHardwarePolicy {
    static inline void reset() {
        hw()->CR = uart::cr::RSTRX::mask;
    }
};
```

**This approach enables:**
- ✅ Consistent code structure
- ✅ Easy to add new platforms
- ✅ Reduced human error
- ✅ Maintainable at scale

## Testing Strategy

### Unit Tests (Mock Policy)

```cpp
struct MockUartPolicy {
    static inline void reset() { /* record call */ }
    static inline void set_baudrate(uint32_t baud) { /* check baud */ }
};

using TestUart = UartImpl<PeripheralId::USART0, MockUartPolicy>;

TEST_CASE("UART initialize") {
    auto uart = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});
    REQUIRE(uart.initialize().is_ok());
}
```

### Integration Tests (Real Policy)

```cpp
using RealUart = UartImpl<PeripheralId::USART0, Same70UartPolicy>;

TEST_CASE("UART loopback") {
    auto uart = RealUart::quick_setup<PinD3, PinD4>(BaudRate{115200});
    uart.initialize();

    // Test on real hardware
    uart.write("TEST", 4);
    // ...
}
```

## Migration from Old Code

### Old (Platform-Specific Classes)

```cpp
// ❌ OLD - One class per family, lots of duplication
namespace alloy::hal::same70 {
    class Uart {
        void reset() { /* SAME70 code */ }
        void set_baudrate(uint32_t) { /* SAME70 code */ }
    };
}

namespace alloy::hal::stm32f4 {
    class Uart {
        void reset() { /* STM32F4 code - duplicated logic! */ }
        void set_baudrate(uint32_t) { /* STM32F4 code - duplicated logic! */ }
    };
}
```

### New (Policy-Based)

```cpp
// ✅ NEW - One generic API, multiple policies
template <typename HardwarePolicy>
class UartImpl {
    void initialize() {
        HardwarePolicy::reset();          // Generic logic
        HardwarePolicy::set_baudrate();   // Same for all platforms
    }
};

// Platform-specific policies (only hardware access)
struct Same70UartPolicy { static void reset() { /* ... */ } };
struct Stm32F4UartPolicy { static void reset() { /* ... */ } };
```

## Summary

| Decision | Rationale |
|----------|-----------|
| **Policy-Based Design ONLY** | Zero overhead, testable, clear separation |
| **No Traits** | Less flexible, more specializations needed |
| **No CRTP** | More complex, inheritance-based |
| **No Virtual Functions** | Runtime overhead, violates zero-cost principle |
| **No Template Specialization** | Code duplication, hard to maintain |

**This decision is final and applies to ALL peripheral implementations.**

Any deviation must be approved by architecture review and documented as an exception.

---

**Approved**: Architecture Team
**Date**: 2025-01-10
**Status**: ✅ **CANONICAL** - This is the source of truth for hardware abstraction design
