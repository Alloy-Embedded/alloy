// The RL78 half of alloy::arch::cpu — idle, reset, and the one thing this
// architecture cannot do yet.

#include <cstdint>

#include "alloy/arch/cpu.hpp"
#include "alloy/arch/rl78/intc.hpp"
#include "alloy/arch/rl78/systick.hpp"

using alloy::arch::rl78::near;

namespace {

//: Writing anything other than 0xAC to WDTE is an internal reset. That is the
//: architecture's software-reset mechanism — there is no AIRCR here.
//: near() because uintptr_t is 16 bits on this machine; see intc.hpp.
constexpr std::uintptr_t kWdte = near(0xFFFAB);
constexpr std::uint8_t kWdteBadValue = 0x00;

}  // namespace

namespace alloy::arch {

void idle() {
    // HALT stops the CPU clock and resumes on any accepted interrupt. A request
    // that is already pending returns immediately, so this is safe in a
    // superloop with a wake racing in.
    __asm volatile("halt" ::: "memory");
}

[[noreturn]] void system_reset() {
    *reinterpret_cast<volatile std::uint8_t*>(kWdte) = kWdteBadValue;
    for (;;) {
    }
}

[[noreturn]] void boot_image(std::uintptr_t /*vector_base*/) {
    // Deliberately not implemented rather than approximated.
    //
    // The Cortex-M version retargets VTOR, reloads the stack pointer from the
    // new table and branches. RL78 has no VTOR: its vector table is fixed at
    // 0x00000 and switching images is done in hardware, by swapping boot
    // clusters. An A/B jump on this part needs that mechanism, and pretending
    // otherwise would hand a bootloader a jump that silently lands nowhere.
    //
    // Until then the honest outcome is a reset: the fresh boot re-runs slot
    // selection, which is where the decision belongs anyway.
    system_reset();
}

}  // namespace alloy::arch
