#pragma once

// plic_impl.hpp — Task 2.3 (add-irq-vector-hal)
//
// RISC-V PLIC backend (placeholder — pending nRF9160 / ESP32-C3 HW validation).
// PLIC registers vary by implementation (SiFive, PolarFire, etc.).
// Default base from SiFive PLIC specification.

#include <cstdint>

#include "hal/irq/irq_id.hpp"

namespace alloy::hal::irq::detail {

#ifndef ALLOY_PLIC_BASE
#  define ALLOY_PLIC_BASE 0x0C000000u
#endif

struct RiscVPlicImpl {
    static void enable(IrqId irq, std::uint8_t /*priority*/) noexcept {
        if (!irq.is_valid()) { return; }
        // PLIC enable: context 0, word = irq / 32, bit = irq % 32
        // Enable register base: PLIC_BASE + 0x2000 + context * 0x80
        volatile std::uint32_t* const en =
            reinterpret_cast<volatile std::uint32_t*>(
                ALLOY_PLIC_BASE + 0x2000u + 0u);  // context 0
        en[irq.value >> 5u] |= (1u << (irq.value & 31u));
    }

    static void disable(IrqId irq) noexcept {
        if (!irq.is_valid()) { return; }
        volatile std::uint32_t* const en =
            reinterpret_cast<volatile std::uint32_t*>(
                ALLOY_PLIC_BASE + 0x2000u + 0u);
        en[irq.value >> 5u] &= ~(1u << (irq.value & 31u));
    }

    static void set_priority(IrqId irq, std::uint8_t priority) noexcept {
        if (!irq.is_valid()) { return; }
        // Priority register: PLIC_BASE + source * 4
        volatile std::uint32_t* const prio =
            reinterpret_cast<volatile std::uint32_t*>(ALLOY_PLIC_BASE);
        prio[irq.value] = priority & 0x7u;  // 3-bit priority, vendor-specific max
    }
};

}  // namespace alloy::hal::irq::detail
