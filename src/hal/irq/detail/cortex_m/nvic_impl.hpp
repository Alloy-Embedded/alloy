#pragma once

// nvic_impl.hpp — Task 2.2 (add-irq-vector-hal)
//
// Cortex-M NVIC backend for irq::enable/disable/set_priority.
// Wraps CMSIS NVIC_EnableIRQ, NVIC_DisableIRQ, NVIC_SetPriority.

#include <cstdint>

#include "hal/irq/irq_id.hpp"

namespace alloy::hal::irq::detail {

struct CortexMNvicImpl {
    static void enable(IrqId irq, std::uint8_t /*priority*/) noexcept {
#if defined(__ARM_ARCH) && defined(__NVIC_EnableIRQ)
        NVIC_EnableIRQ(static_cast<IRQn_Type>(irq.value));
#else
        // Inline NVIC ISER write for bare-metal without CMSIS header
        if (!irq.is_valid()) { return; }
        const std::uint32_t n = irq.value;
        volatile std::uint32_t* const iser =
            reinterpret_cast<volatile std::uint32_t*>(0xE000E100u);
        iser[n >> 5u] = (1u << (n & 31u));
#endif
    }

    static void disable(IrqId irq) noexcept {
#if defined(__ARM_ARCH) && defined(__NVIC_DisableIRQ)
        NVIC_DisableIRQ(static_cast<IRQn_Type>(irq.value));
#else
        if (!irq.is_valid()) { return; }
        const std::uint32_t n = irq.value;
        volatile std::uint32_t* const icer =
            reinterpret_cast<volatile std::uint32_t*>(0xE000E180u);
        icer[n >> 5u] = (1u << (n & 31u));
#endif
    }

    static void set_priority(IrqId irq, std::uint8_t priority) noexcept {
#if defined(__ARM_ARCH) && defined(__NVIC_SetPriority)
        NVIC_SetPriority(static_cast<IRQn_Type>(irq.value), priority);
#else
        if (!irq.is_valid()) { return; }
        // NVIC_IPR: 1 byte per IRQ, base 0xE000E400
        volatile std::uint8_t* const ipr =
            reinterpret_cast<volatile std::uint8_t*>(0xE000E400u);
        ipr[irq.value] = priority;
#endif
    }
};

}  // namespace alloy::hal::irq::detail
