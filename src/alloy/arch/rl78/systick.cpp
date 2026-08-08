// RL78 delays.
//
// With no cycle counter, `sleep_for` is a calibrated busy loop and the comment
// saying so is the important part: it is accurate to roughly the loop's own
// granularity, it drifts with flash wait states and interrupt load, and it is
// NOT a timebase. Anything that needs a real one — uptime_us(), the async
// executor's deadlines — waits for a board to declare which TAU channel feeds
// the tick, because on this architecture that channel is the application's to
// spend.

#include "alloy/arch/rl78/systick.hpp"

#include <chrono>
#include <cstdint>

#include "alloy/time.hpp"

namespace alloy::arch::rl78 {
namespace {

std::uint32_t g_core_hz = 0;

//: Cycles the inner loop below costs per iteration on this core. RL78 is a CISC
//: with multi-cycle memory operands; the decrement-and-branch pair measures 4
//: cycles from internal RAM at 1 wait state, which is the configuration a
//: generated board sets up. Wrong by a wait state means a delay wrong by the
//: same ratio — hence "approximate", stated rather than hidden.
constexpr std::uint32_t kCyclesPerIteration = 4;

}  // namespace

void systick_init(std::uint32_t core_hz) { g_core_hz = core_hz; }

}  // namespace alloy::arch::rl78

namespace alloy {

void sleep_for(std::chrono::microseconds d) {
    if (arch::rl78::g_core_hz == 0 || d.count() <= 0) {
        return;
    }
    // (us * Hz) / 1e6 cycles, then divide by the loop cost. Done in 64-bit
    // because us * Hz overflows 32 bits at ~4 s on a 1 MHz part, and this is a
    // 16-bit machine where that mistake is easy to make and invisible.
    const std::uint64_t cycles =
        (static_cast<std::uint64_t>(d.count()) *
         static_cast<std::uint64_t>(arch::rl78::g_core_hz)) / 1'000'000u;
    std::uint32_t iterations =
        static_cast<std::uint32_t>(cycles / arch::rl78::kCyclesPerIteration);
    while (iterations-- != 0u) {
        // Keeps the loop from being optimised away without pretending to be a
        // timed instruction.
        __asm volatile("nop");
    }
}

}  // namespace alloy
