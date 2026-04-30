#pragma once

// irq_table.hpp — Tasks 3.1–3.2 (add-irq-vector-hal)
//
// RAM-based IRQ dispatch table.
//
// irq::set_handler<P>(handler) — install a handler for peripheral P's IRQ.
// irq::clear_handler<P>()      — remove handler (reset to default).
//
// Table size: ALLOY_IRQ_TABLE_SIZE (default: 128).
// Each entry is a function pointer or null (→ default_handler()).

#include <array>
#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/irq/irq_id.hpp"
#include "hal/irq/irq_vector_traits.hpp"
#include "device/runtime.hpp"

#ifndef ALLOY_IRQ_TABLE_SIZE
#  define ALLOY_IRQ_TABLE_SIZE 128
#endif

namespace alloy::hal::irq {

using IrqHandler = void (*)();

// ---------------------------------------------------------------------------
// RAM dispatch table
// ---------------------------------------------------------------------------

namespace detail {

inline auto& irq_table() noexcept {
    static std::array<IrqHandler, ALLOY_IRQ_TABLE_SIZE> table{};
    return table;
}

}  // namespace detail

// ---------------------------------------------------------------------------
// default_handler()
// ---------------------------------------------------------------------------

/// Default ISR: spins in a debug loop.
/// Overridden per-peripheral via set_handler<P>() or ALLOY_IRQ_HANDLER(P).
[[noreturn]] inline void default_handler() noexcept {
    while (true) { __asm volatile("nop"); }
}

/// Dispatch from a raw IRQ number → RAM table entry → default_handler.
inline void dispatch_irq(std::uint16_t irqn) noexcept {
    if (irqn >= ALLOY_IRQ_TABLE_SIZE) {
        default_handler();
    }
    auto* handler = detail::irq_table()[irqn];
    if (handler != nullptr) {
        handler();
    } else {
        default_handler();
    }
}

// ---------------------------------------------------------------------------
// set_handler<P> / clear_handler<P>
// ---------------------------------------------------------------------------

/// Install a function as the handler for peripheral P's IRQ.
/// The handler runs from the weak default ISR which dispatches to this table.
template <device::runtime::PeripheralId P>
[[nodiscard]] auto set_handler(IrqHandler handler) -> core::Result<void, core::ErrorCode> {
    if constexpr (!kHasIrq<P>) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        constexpr auto irqn = IrqVectorTraits<P>::kIrqId.value;
        if constexpr (irqn >= ALLOY_IRQ_TABLE_SIZE) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        detail::irq_table()[irqn] = handler;
        return core::Ok();
    }
}

/// Clear the handler for peripheral P's IRQ (falls back to default_handler).
template <device::runtime::PeripheralId P>
[[nodiscard]] auto clear_handler() -> core::Result<void, core::ErrorCode> {
    return set_handler<P>(nullptr);
}

}  // namespace alloy::hal::irq


// ---------------------------------------------------------------------------
// ALLOY_IRQ_HANDLER(P) macro — Task 3.3
// ---------------------------------------------------------------------------
//
// Usage:
//   ALLOY_IRQ_HANDLER(PeripheralId::Usart2) {
//       // handle UART2 interrupt
//   }
//
// Expands to:
//   extern "C" void USART2_IRQHandler();   // weak symbol override
//   void USART2_IRQHandler() { <body> }
//
// For devices without an IRQ, expands to a static_assert failure at
// compile-time if ALLOY_STRICT_IRQ_HANDLER is defined.
//
// The vector name is resolved via IrqVectorTraits<P>::kVectorName (a
// constexpr string set by the codegen template).  If no vector name is
// available, the macro falls back to the raw dispatch table entry.

#ifndef ALLOY_IRQ_HANDLER
#  define ALLOY_IRQ_HANDLER(PERIPHERAL_ID)                                    \
    _ALLOY_IRQ_HANDLER_IMPL(PERIPHERAL_ID)
#endif

// Internal macro — resolves vector name from traits if available.
// The lambda body follows immediately after the macro invocation.
#define _ALLOY_IRQ_HANDLER_IMPL(PERIPHERAL_ID)                                \
    /* Verify at compile time that this peripheral has an IRQ */               \
    static_assert(alloy::hal::irq::kHasIrq<PERIPHERAL_ID>,                   \
        "ALLOY_IRQ_HANDLER: " #PERIPHERAL_ID " has no IRQ in IrqVectorTraits"); \
    /* The handler body follows — user writes { ... } after the macro */       \
    void _alloy_irq_handler_for_##PERIPHERAL_ID##_impl();                     \
    extern "C" __attribute__((weak, alias("_alloy_irq_handler_for_" #PERIPHERAL_ID "_impl"))) \
    void _alloy_irq_default_for_##PERIPHERAL_ID();                            \
    void _alloy_irq_handler_for_##PERIPHERAL_ID##_impl()
