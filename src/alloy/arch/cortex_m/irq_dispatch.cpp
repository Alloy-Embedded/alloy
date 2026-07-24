// Slot-table dispatch target of the generated weak per-IRQ wrappers.
// A fired-but-unattached line is ignored: the NVIC line is only ever
// enabled through alloy::irq::enable, so this can only happen in the
// window of a detach that raced a pend — dropping it is correct.

#include "alloy/irq.hpp"

extern "C" void alloy_irq_dispatch(unsigned n) {
    alloy::irq::dispatch(n);  // walks the line's handler chain (shared vectors)
}

#include "alloy/arch/cortex_m/nvic.hpp"

namespace alloy::arch {

void irq_line_enable(unsigned n) {
    cortex_m::nvic::set_priority(n, 0x80);  // mid-scale default (top bits)
    cortex_m::nvic::clear_pending(n);
    cortex_m::nvic::enable(n);
}

void irq_line_disable(unsigned n) { cortex_m::nvic::disable(n); }

void irq_line_priority(unsigned n, std::uint8_t level) {
    cortex_m::nvic::set_priority(n, cortex_m::nvic::prio_value(level));
}

}  // namespace alloy::arch
