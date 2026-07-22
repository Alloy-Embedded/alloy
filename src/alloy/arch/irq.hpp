// Architecture-neutral IRQ mask save/restore, implemented per arch
// (Cortex-M: PRIMASK, in arch/cortex_m/systick.cpp). Declared here so
// timing-critical drivers stay arch-agnostic at the source level.

#pragma once

#include <cstdint>

namespace alloy::arch {

using irq_state = std::uint32_t;

// Disable interrupts, returning the previous state.
[[nodiscard]] irq_state irq_save();

// Restore the state returned by irq_save().
void irq_restore(irq_state state);

}  // namespace alloy::arch
