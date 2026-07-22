// Slot-table dispatch target of the generated weak per-IRQ wrappers.
// A fired-but-unattached line is ignored: the NVIC line is only ever
// enabled through alloy::irq::enable, so this can only happen in the
// window of a detach that raced a pend — dropping it is correct.

#include "alloy/irq.hpp"

extern "C" void alloy_irq_dispatch(unsigned n) {
    const alloy::irq::slot s = alloy::irq::g_alloy_irq_slots[n];
    if (s.fn != nullptr) {
        s.fn(s.ctx);
    }
}
