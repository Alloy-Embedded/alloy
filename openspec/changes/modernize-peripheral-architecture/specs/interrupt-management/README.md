# Interrupt Management - Executive Summary

## Problem

Your interrupt implementation has **hardcoded IRQ numbers** that don't match real hardware:

```cpp
// ❌ WRONG - Generic IRQ numbers
enum class IrqNumber : u16 {
    UART0 = 10,   // Not the real IRQ for any MCU!
    UART1 = 11,   // SAME70 has UART0 at IRQ 7, STM32F4 at IRQ 37
};
```

**Reality:**
- **SAME70**: UART0 → IRQ 7, SPI0 → IRQ 21
- **STM32F4**: USART1 → IRQ 37, SPI1 → IRQ 35
- **Different for every MCU family!**

## Solution: Generate IRQ Tables from SVD

### 1. Parse SVD Files

```xml
<!-- STM32F429.svd -->
<peripheral>
  <name>USART1</name>
  <interrupt>
    <name>USART1</name>
    <value>37</value>  ← Extract this!
  </interrupt>
</peripheral>
```

### 2. Generate Platform-Specific IRQ Tables

```cpp
// hal/vendors/st/stm32f4/irq_table.hpp (GENERATED)
namespace alloy::hal::st::stm32f4 {

enum class IrqNumber : u16 {
    USART1 = 37,  // ✅ Correct for STM32F4
    USART2 = 38,
    SPI1 = 35,
    SPI2 = 36,
    // ... all peripherals
};

}  // namespace
```

```cpp
// hal/vendors/atmel/same70/irq_table.hpp (GENERATED)
namespace alloy::hal::atmel::same70 {

enum class IrqNumber : u16 {
    UART0 = 7,    // ✅ Correct for SAME70
    UART1 = 8,
    SPI0 = 21,
    SPI1 = 22,
    // ... all peripherals
};

}  // namespace
```

### 3. Create Interrupt Controller Policy (NVIC)

```cpp
// hal/vendors/arm/cortex_m/nvic_hardware_policy.hpp
struct NvicHardwarePolicy {
    static void enable_global() {
        __asm volatile("cpsie i" ::: "memory");
    }

    static void disable_global() {
        __asm volatile("cpsid i" ::: "memory");
    }

    static Result<void, ErrorCode> enable(uint16_t irq) {
        volatile uint32_t* NVIC_ISER = /* NVIC base + 0x000 */;
        NVIC_ISER[irq >> 5] = (1UL << (irq & 0x1F));
        return Ok();
    }

    static Result<void, ErrorCode> set_priority(uint16_t irq, uint8_t priority) {
        volatile uint8_t* NVIC_IPR = /* NVIC base + 0x300 */;
        NVIC_IPR[irq] = priority;
        return Ok();
    }

    // ... all NVIC operations
};
```

### 4. Link Peripherals to IRQs

```cpp
// hal/vendors/atmel/same70/uart_hardware_policy.hpp
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ, uint16_t IRQ_NUM>
struct Same70UartHardwarePolicy {
    static constexpr uint16_t irq_number = IRQ_NUM;  // ← NEW!
    // ... all existing methods
};

// Instances with IRQ numbers
using Uart0Hardware = Same70UartHardwarePolicy<0x400E0800, 150000000, 7>;   // IRQ 7
using Uart1Hardware = Same70UartHardwarePolicy<0x400E0A00, 150000000, 8>;   // IRQ 8
```

### 5. Update Generic Interrupt APIs

```cpp
// hal/api/interrupt_simple.hpp
class Interrupt {
public:
    static void enable_all() {
        InterruptControllerPolicy::enable_global();  // Uses NVIC policy
    }

    static Result<void, ErrorCode> enable(IrqNumber irq) {
        return InterruptControllerPolicy::enable(static_cast<uint16_t>(irq));
    }

    // ... other methods
};

class CriticalSection {
public:
    CriticalSection() {
        saved_state_ = InterruptControllerPolicy::save_and_disable();
    }

    ~CriticalSection() {
        InterruptControllerPolicy::restore(saved_state_);
    }

private:
    uint32_t saved_state_;
};
```

## Usage Examples

### Simple Interrupt Control

```cpp
using namespace alloy::platform::same70;

// Enable UART0 interrupts
constexpr auto uart0_irq = Uart0Hardware::irq_number;  // 7
Interrupt::enable(static_cast<IrqNumber>(uart0_irq));
Interrupt::set_priority(static_cast<IrqNumber>(uart0_irq), IrqPriority::High);
```

### Critical Section (RAII)

```cpp
{
    CriticalSection cs;  // Interrupts disabled
    // ... critical code ...
}  // Interrupts restored automatically
```

### UART with Interrupts

```cpp
auto uart = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
uart.initialize();

// Enable UART interrupt
Interrupt::enable(static_cast<IrqNumber>(Uart0Hardware::irq_number));

// In interrupt handler (defined elsewhere):
extern "C" void UART0_Handler() {
    // Handle UART interrupt
}
```

## Code Generation Flow

```
SVD File
  ↓
Parse <interrupt> tags
  ↓
Extract IRQ numbers
  ↓
Generate irq_table.hpp
  ↓
Update peripheral policies with IRQ numbers
  ↓
Link with interrupt controller policy
```

## Benefits

✅ **Correct IRQ numbers**: Extracted directly from SVD
✅ **Type-safe**: Platform-specific `IrqNumber` enums
✅ **Zero overhead**: All inline, compile-time resolved
✅ **Testable**: Mock interrupt controller policy
✅ **Portable**: Same API across platforms
✅ **Safe**: RAII critical sections prevent bugs

## What Gets Generated

### Per Platform

1. **IRQ Table** (`irq_table.hpp`)
   - Platform-specific `IrqNumber` enum
   - All peripheral IRQ numbers
   - Lookup table with names/descriptions

2. **Updated Peripheral Policies**
   - Each policy includes `irq_number` constant
   - Links peripheral to its IRQ

### Shared (Manual Implementation)

1. **Interrupt Controller Policy** (`nvic_hardware_policy.hpp`)
   - ARM Cortex-M NVIC operations
   - Assembly for global enable/disable
   - Register access for specific IRQs

2. **Generic Interrupt APIs** (`interrupt_simple.hpp`, `interrupt_expert.hpp`)
   - Uses interrupt controller policy
   - Platform-agnostic interface

## File Organization

```
src/hal/
├── interface/
│   └── interrupt.hpp              (Concepts only, no hardcoded IRQs)
│
├── api/
│   ├── interrupt_simple.hpp       (Uses InterruptControllerPolicy)
│   └── interrupt_expert.hpp       (Uses InterruptControllerPolicy)
│
└── vendors/
    ├── arm/cortex_m/
    │   └── nvic_hardware_policy.hpp   (NVIC implementation)
    │
    ├── atmel/same70/
    │   ├── irq_table.hpp              (Generated: SAME70 IRQ numbers)
    │   ├── uart_hardware_policy.hpp   (Includes irq_number = 7)
    │   └── spi_hardware_policy.hpp    (Includes irq_number = 21)
    │
    └── st/stm32f4/
        ├── irq_table.hpp              (Generated: STM32F4 IRQ numbers)
        ├── uart_hardware_policy.hpp   (Includes irq_number = 37)
        └── spi_hardware_policy.hpp    (Includes irq_number = 35)
```

## Implementation Checklist

- [ ] Create SVD interrupt parser
- [ ] Create IRQ table generator script
- [ ] Create `irq_table.hpp.j2` Jinja2 template
- [ ] Generate IRQ tables for SAME70 and STM32F4
- [ ] Implement NVIC hardware policy (ARM Cortex-M)
- [ ] Add `irq_number` to all peripheral policies
- [ ] Update `interrupt_simple.hpp` to use policy
- [ ] Update `interrupt_expert.hpp` to use policy
- [ ] Update `CriticalSection` to use policy
- [ ] Create unit tests for NVIC policy
- [ ] Create integration tests (UART + interrupts)
- [ ] Update documentation

## Timeline

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| SVD parser extension | 2 days | Parse `<interrupt>` tags |
| IRQ table generator | 2 days | Generate `irq_table.hpp` |
| NVIC policy | 3 days | ARM Cortex-M implementation |
| Update peripheral policies | 2 days | Add `irq_number` to all |
| Update generic APIs | 1 day | Use interrupt controller policy |
| Testing | 4 days | Unit + integration tests |
| Documentation | 1 day | Examples + migration guide |

**Total: ~3 weeks**

## Success Metrics

- ✅ IRQ tables match SVD exactly
- ✅ All platforms have correct IRQ numbers
- ✅ CriticalSection uses policy (no inline assembly in generic code)
- ✅ Peripheral policies expose IRQ numbers
- ✅ Unit tests achieve 100% coverage
- ✅ Integration tests pass on hardware

## Next Steps

1. **Read full spec**: `spec.md`
2. **Extend SVD parser**: Add interrupt extraction
3. **Create generator**: `interrupt_table_generator.py`
4. **Implement NVIC policy**: ARM Cortex-M
5. **Update peripherals**: Add IRQ numbers to policies
6. **Test**: Unit + integration + hardware

---

**Status**: 📝 Specification Complete - Ready for Implementation
**Priority**: P0 - Critical for interrupt-driven applications
**Dependencies**: Hardware policy infrastructure (Phase 8)
