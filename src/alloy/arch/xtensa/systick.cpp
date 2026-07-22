#include "alloy/arch/xtensa/systick.hpp"

#include <chrono>

#include "alloy/arch/irq.hpp"
#include "alloy/time.hpp"

namespace alloy::arch::xtensa {
namespace {

std::uint32_t g_core_hz = 0;

}  // namespace

void systick_init(std::uint32_t core_hz) { g_core_hz = core_hz; }

}  // namespace alloy::arch::xtensa

namespace alloy::arch {

irq_state irq_save() {
    std::uint32_t ps;
    __asm volatile("rsil %0, 15" : "=a"(ps));
    return ps;
}

void irq_restore(irq_state state) {
    __asm volatile("wsr %0, ps\n\trsync" ::"a"(state));
}

}  // namespace alloy::arch

namespace alloy {

void sleep_for(std::chrono::microseconds d) {
    // Wrap-safe CCOUNT delta wait; valid for durations < 2^32 CPU cycles.
    const std::uint64_t cycles64 =
        (static_cast<std::uint64_t>(d.count()) * arch::xtensa::g_core_hz) / 1'000'000u;
    const auto target = static_cast<std::uint32_t>(cycles64);
    std::uint32_t start;
    __asm volatile("rsr.ccount %0" : "=a"(start));
    std::uint32_t now = start;
    while (now - start < target) {
        __asm volatile("rsr.ccount %0" : "=a"(now));
    }
}

std::uint32_t uptime_ms() {
    // v1 limitation: derived directly from CCOUNT, wraps every ~53 s at
    // 80 MHz (no tick ISR on Xtensa yet).
    std::uint32_t now;
    __asm volatile("rsr.ccount %0" : "=a"(now));
    return now / (arch::xtensa::g_core_hz / 1'000u);
}

}  // namespace alloy
