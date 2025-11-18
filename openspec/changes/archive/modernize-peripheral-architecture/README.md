# Modernize Peripheral Architecture

> **Status**: 📝 Specification Complete - Ready for Implementation
> **Priority**: P0 - Critical architectural change
> **Timeline**: 25 weeks (Phases 1-13)

## Overview

Complete modernization of the Alloy HAL architecture using C++20 features, policy-based design, and auto-generated metadata.

**Key Improvements:**
- ✅ **10x better error messages** - C++20 concepts replace SFINAE
- ✅ **Zero overhead** - Policy-based design with static inline methods
- ✅ **Compile-time validation** - Pin/signal compatibility checked at compile-time
- ✅ **Testable** - Mock policies enable testing without hardware
- ✅ **Maintainable** - Auto-generated from JSON metadata
- ✅ **Three API levels** - Simple, Fluent, Expert (beginners → experts)

---

## 📖 Documentation

**NEW READER? START HERE:**

1. **[INDEX.md](INDEX.md)** - Complete documentation index and navigation guide
2. **[ARCHITECTURE.md](ARCHITECTURE.md)** - ⭐ **CANONICAL** - Complete rationale for policy-based design
3. **[REVIEW.md](REVIEW.md)** - Documentation consistency verification

**Core Documents:**

- [proposal.md](proposal.md) - Problem statement and solution overview
- [design.md](design.md) - Architecture details and design decisions
- [tasks.md](tasks.md) - Phase-by-phase implementation tasks

**Detailed Specifications:**

- [specs/hardware-policy/](specs/hardware-policy/) - **How to implement policies** (most important)
- [specs/interrupt-management/](specs/interrupt-management/) - IRQ tables and NVIC policy
- [specs/signal-routing/](specs/signal-routing/) - Pin/signal validation
- [specs/multi-level-api/](specs/multi-level-api/) - Simple/Fluent/Expert APIs
- [specs/concept-layer/](specs/concept-layer/) - C++20 concepts
- [specs/codegen-metadata/](specs/codegen-metadata/) - Code generation
- [specs/dma-integration/](specs/dma-integration/) - DMA configuration

---

## 🎯 Key Decision: Policy-Based Design ONLY

**THIS PROJECT USES EXCLUSIVELY POLICY-BASED DESIGN FOR HARDWARE ABSTRACTION.**

No other techniques (traits, CRTP, inheritance) will be used.

### Why Policies?

```cpp
// Generic API (platform-agnostic)
template <PeripheralId Id, typename HardwarePolicy>
class UartImpl {
    void initialize() {
        HardwarePolicy::reset();        // Zero overhead
        HardwarePolicy::set_baudrate(); // Fully inlined
    }
};

// Hardware Policy (platform-specific, auto-generated)
struct Same70UartHardwarePolicy {
    static inline void reset() { hw()->CR = uart::cr::RSTRX::mask; }
    static inline void set_baudrate(uint32_t b) { hw()->BRGR = CLOCK / (16 * b); }
};
```

**Benefits:**
- ✅ Zero overhead (all static inline)
- ✅ Testable (inject mock policies)
- ✅ Clear separation (generic vs hardware)
- ✅ Auto-generated (from JSON)

**See [ARCHITECTURE.md](ARCHITECTURE.md) for complete rationale and comparison with alternatives.**

---

## 🏗️ Architecture Layers

```
User Application
    ↓
Platform Aliases (using declarations)
    ↓
Generic APIs (template <typename HardwarePolicy>)
    ↓
Hardware Policies (static inline methods)
    ↓
Register/Bitfield Definitions (auto-generated)
    ↓
Hardware Registers
```

**Key Principle**: Generic APIs are **completely platform-agnostic**. All hardware access goes through **Hardware Policies**.

---

## 📋 Implementation Status

### ✅ Completed Phases

| Phase | Status | Description |
|-------|--------|-------------|
| Phase 1 | ✅ Done | Core concepts, signal types, validation helpers |
| Phase 2 | ✅ Done | Signal metadata generation from SVD |
| Phase 3 | ✅ Done | GPIO signal routing integration |
| Phase 4 | ✅ Done | Multi-level API (Simple, Fluent, Expert) |
| Phase 5 | ✅ Done | DMA integration and type-safe configuration |
| Phase 6 | ✅ Done | UART complete implementation with examples |

### 🔲 Pending Phases

| Phase | Duration | Description |
|-------|----------|-------------|
| Phase 8 | 3 weeks | **Hardware Policy Implementation** (UART, SPI, I2C, etc.) |
| Phase 9 | 1 week | File organization & cleanup |
| Phase 10 | 3 weeks | Multi-platform support (STM32F4, STM32F1, RP2040) |
| Phase 11 | 1 week | Hardware testing on real MCUs |
| Phase 12 | 2 weeks | Documentation & migration guide |
| Phase 13 | 1 week | Performance validation |

**Next Up**: Phase 8 - Hardware Policy Implementation

---

## 🚀 Quick Start

### For Architecture Review

```bash
# Read these in order:
1. INDEX.md           # Navigation guide
2. ARCHITECTURE.md    # Why policies? (15 min)
3. design.md          # Architecture overview (10 min)
4. REVIEW.md          # Consistency check
```

### For Implementation

```bash
# Read these in order:
1. specs/hardware-policy/README.md    # Executive summary (10 min)
2. specs/hardware-policy/spec.md      # Complete spec (45 min)
3. specs/hardware-policy/EXAMPLES.md  # Code examples (30 min)
4. tasks.md                           # Implementation tasks
```

### For Usage

```bash
# Read these in order:
1. specs/multi-level-api/spec.md      # API overview
2. specs/hardware-policy/EXAMPLES.md  # User code examples
```

---

## 🎓 Example: UART with Policy

### Platform-Agnostic Generic API

```cpp
// hal/api/uart_simple.hpp
template <PeripheralId PeriphId, typename HardwarePolicy>
class UartImpl {
public:
    template <typename TxPin, typename RxPin>
    static auto quick_setup(BaudRate baud) {
        // Generic validation
        static_assert(is_valid_tx_pin<TxPin>(), "Invalid TX pin");

        return SimpleUartConfig<TxPin, RxPin, HardwarePolicy>{
            PeriphId, baud
        };
    }
};
```

### Platform-Specific Hardware Policy (Auto-Generated)

```cpp
// hal/vendors/atmel/same70/uart_hardware_policy.hpp
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70UartHardwarePolicy {
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
};

using Uart0Hardware = Same70UartHardwarePolicy<0x400E0800, 150000000>;
```

### User Code (Clean and Simple)

```cpp
// User application
#include "platform/same70/peripherals.hpp"

using namespace alloy::platform::same70;

int main() {
    auto uart = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
    uart.initialize();
    uart.write("Hello World!\n", 13);
}
```

**Same generic API works on all platforms - just change the include!**

---

## 📊 Success Metrics

| Metric | Current | Target | Status |
|--------|---------|--------|--------|
| Error message lines | 50+ | < 10 | 🎯 Target defined |
| Compile time increase | - | < 15% | 🎯 Target defined |
| Binary size | Baseline | 0% increase | 🎯 Target defined |
| Lines per config | 10-15 | 5-8 | ✅ Achieved (Simple API) |
| Test coverage | - | 100% for policies | 🎯 Target defined |

---

## 🧪 Testing Strategy

### Unit Tests (Mock Policies)

```cpp
struct MockUartPolicy {
    static inline void reset() { /* record call */ }
    static inline void set_baudrate(uint32_t b) { /* verify b */ }
};

using TestUart = UartImpl<PeripheralId::USART0, MockUartPolicy>;

TEST_CASE("UART initialize") {
    auto uart = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});
    REQUIRE(uart.initialize().is_ok());
}
```

### Integration Tests (Real Policies)

```cpp
using RealUart = UartImpl<PeripheralId::USART0, Same70UartPolicy>;

TEST_CASE("UART loopback") {
    auto uart = RealUart::quick_setup<PinD3, PinD4>(BaudRate{115200});
    uart.initialize();
    // Test on real hardware
}
```

### Hardware Tests (Physical MCU)

```cpp
TEST_CASE("UART hardware loopback") {
    // Requires TX connected to RX
    auto uart = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
    uart.write("TEST", 4);
    char buffer[4];
    uart.read(buffer, 4);
    REQUIRE(memcmp(buffer, "TEST", 4) == 0);
}
```

---

## 🎯 Peripheral Coverage

| Peripheral | Priority | Status | Policy Spec |
|------------|----------|--------|-------------|
| **UART**   | P0 | 🔲 Policy TODO | [hardware-policy](specs/hardware-policy/) |
| **SPI**    | P0 | 🔲 Policy TODO | [hardware-policy](specs/hardware-policy/) |
| **I2C**    | P0 | 🔲 Policy TODO | [hardware-policy](specs/hardware-policy/) |
| **GPIO**   | P0 | 🔲 Policy TODO | [hardware-policy](specs/hardware-policy/) |
| **Interrupt** | P0 | 🔲 Policy TODO | [interrupt-management](specs/interrupt-management/) |
| **ADC**    | P1 | 🔲 Pending | - |
| **Timer**  | P1 | 🔲 Pending | - |
| **DMA**    | P1 | 🔲 Pending | - |
| **PWM**    | P2 | 🔲 Pending | - |

---

## 🌍 Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| **SAME70** | ✅ Primary | Development target |
| **STM32F4** | 🔲 Phase 10 | Discovery boards |
| **STM32F1** | 🔲 Phase 10 | Blue Pill |
| **RP2040** | 🔲 Phase 10 | Raspberry Pi Pico |
| **ESP32** | 🔲 Future | WiFi/BLE support |

---

## 📞 Getting Help

| Question | See Document |
|----------|--------------|
| "What is policy-based design?" | [ARCHITECTURE.md](ARCHITECTURE.md) |
| "How do I implement a policy?" | [specs/hardware-policy/spec.md](specs/hardware-policy/spec.md) |
| "Show me code examples!" | [specs/hardware-policy/EXAMPLES.md](specs/hardware-policy/EXAMPLES.md) |
| "What's the timeline?" | [tasks.md](tasks.md) |
| "How do I navigate?" | [INDEX.md](INDEX.md) |
| "Is it consistent?" | [REVIEW.md](REVIEW.md) |

---

## 🤝 Contributing

**Before implementing:**
1. Read [ARCHITECTURE.md](ARCHITECTURE.md) - Understand the decisions
2. Read [specs/hardware-policy/spec.md](specs/hardware-policy/spec.md) - Know the pattern
3. Follow [tasks.md](tasks.md) - Check phase dependencies

**Code review checklist:**
- [ ] Uses policy-based design (no traits/CRTP/inheritance)
- [ ] Generic API accepts `HardwarePolicy` template parameter
- [ ] All hardware access goes through policy methods
- [ ] Policy methods are `static inline`
- [ ] Mock hook available (`#ifdef ALLOY_*_MOCK_HW`)
- [ ] Unit tests with mock policy
- [ ] Integration tests with real policy

---

## 📝 Change Log

### 2025-01-10
- ✅ Created ARCHITECTURE.md (canonical reference)
- ✅ Updated design.md with policy-based design section
- ✅ Updated proposal.md to mention policies first
- ✅ Created REVIEW.md (consistency verification)
- ✅ Created INDEX.md (navigation guide)
- ✅ Created README.md (this file)
- ✅ Created specs/hardware-policy/ (complete spec)
- ✅ Created specs/interrupt-management/ (IRQ tables)
- ✅ Updated tasks.md (Phases 8-13)

### 2025-01-09
- ✅ Completed Phases 1-6 (concepts, signals, APIs, DMA, UART)

---

## 📄 License

Part of the Alloy embedded systems framework.

---

**Questions? Start with [INDEX.md](INDEX.md) for navigation guidance.**
**Ready to implement? Start with [specs/hardware-policy/spec.md](specs/hardware-policy/spec.md).**
