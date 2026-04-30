#pragma once

// irq_id.hpp — Task 1.4 (add-irq-vector-hal)
//
// IrqId: strong typedef wrapping the raw Cortex-M IRQ number.
// Prevents accidental mixing of IRQ numbers with other uint16_t values.

#include <cstdint>

namespace alloy::hal::irq {

/// Strong typedef for IRQ vector numbers.
/// Value matches the CMSIS IRQn_Type (0-based position in the exception table
/// after the 16 system exceptions, i.e. IRQ0 = vector 16 in the vector table).
struct IrqId {
    std::uint16_t value;

    constexpr bool operator==(const IrqId&) const = default;
    constexpr bool operator!=(const IrqId&) const = default;

    [[nodiscard]] constexpr bool is_valid() const noexcept { return value != kInvalidValue; }
    [[nodiscard]] constexpr std::uint16_t raw() const noexcept { return value; }

    static constexpr std::uint16_t kInvalidValue = 0xFFFFu;
};

inline constexpr IrqId kInvalidIrqId = { IrqId::kInvalidValue };

/// Convenience factory matching CMSIS IRQn integer directly.
[[nodiscard]] constexpr auto make_irq_id(int irqn) -> IrqId {
    if (irqn < 0 || irqn > 0xFFFEu) { return kInvalidIrqId; }
    return IrqId{ static_cast<std::uint16_t>(irqn) };
}

}  // namespace alloy::hal::irq
