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

// Set a line's interrupt priority. level 0 == most urgent; higher == less
// urgent. Effective resolution depends on the part's implemented priority bits
// (2 on Cortex-M0+, 3-4 on M3/M4/M7). No-op on Xtensa v1 (level-1 sources only).
void irq_line_priority(unsigned n, std::uint8_t level);

// Priority-masking critical section: mask interrupts at `level` OR LESS urgency,
// leaving MORE-urgent interrupts (e.g. a high-priority control loop) live. Uses
// BASEPRI on ARMv7-M; on ARMv6-M and Xtensa (no priority mask) it degrades to a
// full mask (same as irq_save). Restore with irq_restore_below().
[[nodiscard]] irq_state irq_save_below(std::uint8_t level);
void irq_restore_below(irq_state state);

}  // namespace alloy::arch
