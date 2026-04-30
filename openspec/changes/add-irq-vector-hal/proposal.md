# Proposal: IRQ Vector HAL

## Status
`open` — required for async HAL coroutine scheduler.

## Problem

Interrupt handling in alloy is completely unabstracted. Users write `extern "C"`
ISR functions by name, hardcode NVIC priority registers, and manually clear
pending flags. There is no type-safe way to:

- Enable/disable a specific peripheral interrupt without knowing the NVIC IRQn number.
- Register a C++ callable as an ISR without a raw extern "C" shim.
- Set interrupt priority using the alloy type system.

```cpp
// Today: raw CMSIS, no alloy abstraction
NVIC_SetPriority(USART2_IRQn, 3);
NVIC_EnableIRQ(USART2_IRQn);
extern "C" void USART2_IRQHandler() { /* ... */ }
```

This blocks the coroutine scheduler and async HAL (which need `enable<Pin>()` /
`on_event(Pin, Callback)` idioms).

## Proposed Solution

### Typed IrqId — codegen side

The code generator emits `IrqVectorTraits<PeripheralId>` from IR interrupt data:

```cpp
// Generated for STM32G0:
template <>
struct IrqVectorTraits<PeripheralId::Usart2> {
    static constexpr IrqId kIrqId = IrqId{38};  // USART2_IRQn
    static constexpr bool  kPresent = true;
};

template <>
struct IrqVectorTraits<PeripheralId::Exti0> {
    static constexpr IrqId kIrqId = IrqId{6};   // EXTI0_1_IRQn (G0 combines EXTI0+1)
    static constexpr bool  kPresent = true;
};
```

`IrqId` is a strong typedef over `uint16_t`.

### `hal::irq::enable<PeripheralId>()` / `disable<PeripheralId>()`

```cpp
// src/hal/irq/irq_handle.hpp
namespace alloy::hal::irq {

template <device::PeripheralId P>
auto enable(uint8_t priority = 0) -> core::Result<void, core::ErrorCode>
{
    static_assert(device::IrqVectorTraits<P>::kPresent,
        "No IRQ vector for this peripheral — check device IR");
    constexpr auto irqn = device::IrqVectorTraits<P>::kIrqId;
    detail::nvic_set_priority(irqn, priority);
    detail::nvic_enable(irqn);
    return core::Ok();
}

template <device::PeripheralId P>
auto disable() -> void
{
    detail::nvic_disable(device::IrqVectorTraits<P>::kIrqId);
}

}  // namespace alloy::hal::irq
```

`detail::nvic_*` functions wrap CMSIS `NVIC_SetPriority` / `NVIC_EnableIRQ` /
`NVIC_DisableIRQ` — or the equivalent on non-Cortex-M (RISC-V CLINT/PLIC
behind an abstraction).

### IRQ handler registration without extern "C"

```cpp
// src/hal/irq/irq_table.hpp
namespace alloy::hal::irq {

using IrqHandler = void(*)();

/// Register a C++ function as the ISR for a peripheral.
/// The linker-section dispatch table routes the vector to the registered fn.
template <device::PeripheralId P>
auto set_handler(IrqHandler fn) -> void;

}
```

Implementation: a RAM-based vector table (relocated from ROM on startup) with
`irq_table[irqn] = fn`. The weak default handler in `src/hal/irq/default_handler.cpp`
calls `set_handler` dispatch.

For static registration (no RAM table):

```cpp
// Macro for zero-overhead static ISR binding (generates the extern "C" symbol):
ALLOY_IRQ_HANDLER(PeripheralId::Usart2) {
    // called by USART2_IRQHandler weak override
}
```

### Coroutine integration

The async HAL (`add-coroutine-scheduler` spec) uses `set_handler` internally:

```cpp
template <device::PeripheralId P>
auto uart_handle<P>::async_read(span<byte> buf) -> Task<size_t> {
    irq::set_handler<P>([this]{ _rx_waker.wake(); });
    irq::enable<P>(4);
    co_await _rx_waker;
    irq::disable<P>();
    // ...
}
```

### Architecture abstraction

| Arch       | NVIC equivalent   | detail:: implementation       |
|------------|-------------------|-------------------------------|
| Cortex-M   | NVIC              | `nvic_set_priority/enable/disable` |
| RISC-V     | PLIC + CLINT      | `plic_set_priority/enable`    |
| Xtensa     | XTHAL_SET_INTMASK | `xthal_intena/intdis`         |
| AVR        | sei/cli + mask    | `avr_irq_enable/disable`      |

Each arch provides `src/hal/irq/detail/<arch>/nvic_impl.hpp`.

## Non-goals

- Dynamic IRQ routing (NVIC mux on Cortex-M55 is out of scope).
- Priority grouping (NVIC_SetPriorityGrouping) — single-group only in v1.
