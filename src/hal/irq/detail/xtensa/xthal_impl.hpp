#pragma once

// xthal_impl.hpp — Task 2.4 (add-irq-vector-hal)
//
// Xtensa interrupt backend (ESP32, ESP32-S2, ESP32-S3).
// Placeholder — actual implementation needs xthal.h / ESP-IDF interrupt_alloc.

#include <cstdint>

#include "hal/irq/irq_id.hpp"

namespace alloy::hal::irq::detail {

struct XtensaImpl {
    static void enable(IrqId /*irq*/, std::uint8_t /*priority*/) noexcept {
        // TODO: call xt_ints_on(1 << irq.value) or esp_intr_enable
    }

    static void disable(IrqId /*irq*/) noexcept {
        // TODO: call xt_ints_off(1 << irq.value) or esp_intr_disable
    }

    static void set_priority(IrqId /*irq*/, std::uint8_t /*priority*/) noexcept {
        // TODO: use ESP-IDF esp_intr_alloc with priority
    }
};

}  // namespace alloy::hal::irq::detail
