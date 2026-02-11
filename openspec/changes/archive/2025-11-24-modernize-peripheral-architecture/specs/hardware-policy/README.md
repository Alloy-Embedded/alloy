# Hardware Policy Layer - Executive Summary

## Problem

The new generic peripheral APIs (Simple, Fluent, Expert) are platform-agnostic but need to access MCU-specific hardware registers. Each MCU family has different register layouts, making it impossible to write a single implementation that works across all platforms.

**Example:**
- **SAME70** UART uses: `CR`, `MR`, `BRGR`, `THR`, `RHR`
- **STM32F4** UART uses: `CR1`, `CR2`, `BRR`, `DR`

## Solution: Policy-Based Design

Inject hardware-specific behavior into generic APIs using **Hardware Policies**.

### Architecture

```
┌─────────────────────────────────┐
│   Generic API (uart_simple.hpp) │
│   - Pin validation               │
│   - Signal routing               │
│   - Configuration logic          │
└───────────┬─────────────────────┘
            │ uses
            ▼
┌─────────────────────────────────┐
│   Hardware Policy (generated)   │
│   - Register access              │
│   - Bitfield operations          │
│   - Clock configuration          │
└─────────────────────────────────┘
```

### Key Benefits

✅ **Separation of Concerns**: Generic logic vs hardware access
✅ **Zero Overhead**: All inline, resolved at compile-time
✅ **Testability**: Mock policies enable testing without hardware
✅ **Maintainability**: Hardware changes isolated to policies
✅ **Scalability**: One generic API supports all MCU families

## Code Examples

### 1. Hardware Policy (Generated)

```cpp
// hal/vendors/atmel/same70/uart_hardware_policy.hpp
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70UartHardwarePolicy {
    static inline volatile UART0_Registers* hw() {
        #ifdef ALLOY_UART_MOCK_HW
            return ALLOY_UART_MOCK_HW();  // Test hook
        #else
            return reinterpret_cast<volatile UART0_Registers*>(BASE_ADDR);
        #endif
    }

    static void reset() {
        hw()->CR = uart::cr::RSTRX::mask | uart::cr::RSTTX::mask;
    }

    static void set_baudrate(uint32_t baud) {
        hw()->BRGR = PERIPH_CLOCK_HZ / (16 * baud);
    }

    // ... other methods
};
```

### 2. Generic API Using Policy

```cpp
// hal/api/uart_simple.hpp
template <PeripheralId PeriphId, typename HardwarePolicy>
class UartImpl {
public:
    template <typename TxPin, typename RxPin>
    static auto quick_setup(BaudRate baud) {
        // Generic pin validation
        static_assert(is_valid_tx_pin<TxPin>(), "Invalid TX pin");

        return SimpleUartConfig<TxPin, RxPin, HardwarePolicy>{
            PeriphId, baud, /* ... */
        };
    }
};

template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig {
    Result<void, ErrorCode> initialize() const {
        // Generic: Configure pins
        TxPin::configure_alternate_function(/* ... */);

        // Policy-specific: Configure hardware
        HardwarePolicy::reset();
        HardwarePolicy::set_baudrate(baudrate.value);
        HardwarePolicy::enable_tx();

        return Ok();
    }
};
```

### 3. User-Facing API (Platform-Specific Aliases)

```cpp
// platform/same70/peripherals.hpp
using Uart0 = hal::UartImpl<
    hal::PeripheralId::USART0,
    Same70UartHardwarePolicy<0x400E0800, 150000000>
>;

// User code:
auto config = Uart0::quick_setup<PinD3, PinD4>(BaudRate{115200});
config.initialize();
```

## Automatic Code Generation

Hardware policies are **auto-generated** from JSON metadata:

### Input: JSON Metadata

```json
{
  "family": "same70",
  "peripheral_name": "UART",
  "policy_methods": {
    "reset": {
      "return_type": "void",
      "code": "hw()->CR = uart::cr::RSTRX::mask;"
    },
    "set_baudrate": {
      "return_type": "void",
      "parameters": [{"name": "baud", "type": "uint32_t"}],
      "code": "hw()->BRGR = PERIPH_CLOCK_HZ / (16 * baud);"
    }
  }
}
```

### Output: Generated C++ Header

```cpp
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70UartHardwarePolicy {
    static inline void reset() {
        hw()->CR = uart::cr::RSTRX::mask;
    }

    static inline void set_baudrate(uint32_t baud) {
        hw()->BRGR = PERIPH_CLOCK_HZ / (16 * baud);
    }
};
```

## Testing Strategy

### 1. Unit Tests (Mock-Based)

```cpp
// Mock registers for testing
static volatile UART0_Registers mock_registers;
#define ALLOY_UART_MOCK_HW() (&mock_registers)

TEST_CASE("UART Policy - Set Baudrate") {
    TestPolicy::set_baudrate(115200);
    REQUIRE(mock_registers.BRGR == 81);  // 150MHz / (16 * 115200)
}
```

### 2. Integration Tests

```cpp
TEST_CASE("UART Simple API - Initialize") {
    auto config = TestUart::quick_setup<MockTxPin, MockRxPin>(BaudRate{115200});
    REQUIRE(config.initialize().is_ok());

    // Verify hardware was configured
    REQUIRE(mock_registers.BRGR != 0);
}
```

### 3. Hardware Tests

```cpp
TEST_CASE("UART Hardware - Loopback") {
    auto uart = Uart0::quick_setup<PinD3, PinD4>(BaudRate{115200});
    uart.initialize();

    uart.write("TEST", 4);
    char buffer[4];
    uart.read(buffer, 4);

    REQUIRE(memcmp(buffer, "TEST", 4) == 0);
}
```

## File Organization

### Before (Messy)

```
src/hal/
├── uart_simple.hpp
├── uart_fluent.hpp
├── spi_simple.hpp
├── gpio.hpp
└── ... (mixed files)
```

### After (Clean)

```
src/hal/
├── concepts.hpp
├── signals.hpp
├── interface/          (Platform-agnostic types)
│   ├── uart.hpp
│   ├── spi.hpp
│   └── ...
├── api/                (Generic implementations)
│   ├── uart_simple.hpp
│   ├── uart_fluent.hpp
│   └── ...
└── vendors/            (Hardware policies)
    ├── atmel/same70/
    │   ├── uart_hardware_policy.hpp
    │   └── spi_hardware_policy.hpp
    └── st/stm32f4/
        ├── uart_hardware_policy.hpp
        └── ...
```

## Peripheral Coverage

| Peripheral | Priority | Status |
|------------|----------|--------|
| UART       | P0       | 🔲 TODO |
| SPI        | P0       | 🔲 TODO |
| I2C        | P0       | 🔲 TODO |
| GPIO       | P0       | 🔲 TODO |
| ADC        | P1       | 🔲 TODO |
| Timer      | P1       | 🔲 TODO |
| PWM        | P2       | 🔲 TODO |
| DMA        | P1       | 🔲 TODO |
| DAC        | P3       | 🔲 TODO |
| CAN        | P3       | 🔲 TODO |

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| SAME70   | ✅ Primary | Development target |
| STM32F4  | 🔲 TODO | Discovery boards |
| STM32F1  | 🔲 TODO | Blue Pill |
| RP2040   | 🔲 TODO | Raspberry Pi Pico |
| ESP32    | 🔲 TODO | WiFi/BLE |

## Timeline

| Phase | Duration | Focus |
|-------|----------|-------|
| Phase 8 | Weeks 15-17 | UART policy + tests |
| Phase 9 | Week 18 | File cleanup |
| Phase 10 | Weeks 19-21 | Multi-platform |
| Phase 11 | Week 22 | Hardware testing |
| Phase 12 | Weeks 23-24 | Documentation |
| Phase 13 | Week 25 | Performance validation |

**Total: 11 weeks**

## Success Metrics

- ✅ **Zero runtime overhead** (verified via benchmarks)
- ✅ **< 15% compile time increase**
- ✅ **100% unit test coverage** for policies
- ✅ **All hardware tests pass** on all platforms
- ✅ **Clear file organization** (no duplicates)
- ✅ **Complete migration guide**

## Next Steps

1. **Read the full spec**: `spec.md`
2. **Review task breakdown**: `../../tasks.md` (Phases 8-13)
3. **Start with UART**: Implement first policy as proof of concept
4. **Iterate**: Extend to other peripherals once UART is validated

## Related Documents

- [Full Specification](spec.md) - Detailed requirements and implementation
- [Task Breakdown](../../tasks.md) - Phase-by-phase implementation tasks
- [Design Document](../../design.md) - Overall architecture
- [Proposal](../../proposal.md) - Original problem statement

---

**Status**: 📝 Specification Complete - Ready for Implementation
**Owner**: Architecture Team
**Last Updated**: 2025-01-10
