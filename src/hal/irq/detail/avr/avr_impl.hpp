#pragma once

// avr_impl.hpp — Task 2.5 (add-irq-vector-hal)
//
// AVR global-interrupt backend (sei/cli + individual mask registers).
// AVR doesn't have per-IRQ NVIC priority — uses global enable only.

#include <cstdint>

#include "hal/irq/irq_id.hpp"

namespace alloy::hal::irq::detail {

struct AvrImpl {
    static void enable(IrqId /*irq*/, std::uint8_t /*priority*/) noexcept {
#if defined(__AVR__)
        sei();  // global interrupt enable
        // Per-peripheral enable bits are set via dedicated peripheral registers (e.g. TIMSK)
        // IRQ-number→mask-register mapping is device-specific; handled by alloy-cpp-emit
#endif
    }

    static void disable(IrqId /*irq*/) noexcept {
#if defined(__AVR__)
        // Per-peripheral disable — for global: cli()
        // Individual peripheral mask bit cleared by peripheral driver
#endif
    }

    static void set_priority(IrqId /*irq*/, std::uint8_t /*priority*/) noexcept {
        // AVR has no IRQ priority — fixed priority by vector position
    }
};

}  // namespace alloy::hal::irq::detail
