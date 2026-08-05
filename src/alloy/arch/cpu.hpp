// Architecture-neutral CPU-power seam, implemented per arch (Cortex-M: WFI, in
// arch/cortex_m/systick.cpp; Xtensa: WAITI, in arch/xtensa/irq_ctrl.cpp; host:
// no-op, in tests/host_support.cpp). Declared here so the cooperative async
// executor can idle between wakes without any arch #ifdef.

#pragma once

#include <cstdint>

namespace alloy::arch {

// Put the core to sleep until the next interrupt. An interrupt that is already
// pending returns immediately, so this is safe to call in a superloop even when
// a wake is racing in. No effect on the host.
void idle();

// Hand control to a firmware image whose vector table lives at `vector_base`
// (Cortex-M: retarget VTOR, reload MSP from vt[0], branch to vt[1]). The
// bootloader's tick is stopped first so the app starts from a quiet core.
// Cortex-M only for now — the bootloader path; Xtensa boots via the ROM loader.
[[noreturn]] void boot_image(std::uintptr_t vector_base);

// Reset the whole system (Cortex-M: AIRCR.SYSRESETREQ). The bootloader's exit
// after a successful update — the fresh boot re-runs slot selection.
[[noreturn]] void system_reset();

}  // namespace alloy::arch
