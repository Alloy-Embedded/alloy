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

// Per-arch interrupt-line control consumed by alloy::irq (implemented in
// arch/cortex_m/irq_dispatch.cpp via the NVIC, arch/xtensa/irq_ctrl.cpp via
// the interrupt matrix + INTENABLE). `n` is the chip-data irq number: an
// NVIC line on Cortex-M, a matrix SOURCE on Xtensa.
void irq_line_enable(unsigned n);
void irq_line_disable(unsigned n);

}  // namespace alloy::arch
