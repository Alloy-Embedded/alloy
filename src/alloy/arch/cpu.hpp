// Architecture-neutral CPU-power seam, implemented per arch (Cortex-M: WFI, in
// arch/cortex_m/systick.cpp; Xtensa: WAITI, in arch/xtensa/irq_ctrl.cpp; host:
// no-op, in tests/host_support.cpp). Declared here so the cooperative async
// executor can idle between wakes without any arch #ifdef.

#pragma once

namespace alloy::arch {

// Put the core to sleep until the next interrupt. An interrupt that is already
// pending returns immediately, so this is safe to call in a superloop even when
// a wake is racing in. No effect on the host.
void idle();

}  // namespace alloy::arch
