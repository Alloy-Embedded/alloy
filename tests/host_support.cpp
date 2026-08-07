// Host definitions of the arch/timebase seams that are implemented per-arch in
// arch/<ns>/*.cpp on silicon. Linking this TU lets the SAME portable code
// (scheduler critical sections, anything touching the timebase) build and run
// natively for the unit tests. This is the link-time platform seam — no #ifdef.

#include <chrono>
#include <cstdint>

#include "alloy/arch/irq.hpp"
#include "alloy/time.hpp"

namespace alloy::arch {

// Host has no interrupts to mask; the critical section is a no-op.
irq_state irq_save() { return 0; }
void irq_restore(irq_state) {}
void irq_line_enable(unsigned) {}
void irq_line_disable(unsigned) {}
void irq_line_priority(unsigned, std::uint8_t) {}
irq_state irq_save_below(std::uint8_t) { return 0; }
void irq_restore_below(irq_state) {}
void idle() {}  // host has nothing to sleep on

}  // namespace alloy::arch

namespace alloy {

// A trivial host timebase so scheduler::run()/sleep_for link if exercised;
// unit tests inject their own clock into run_once() instead. Kept in
// MICROSECONDS internally so uptime_us() and uptime_ms() can never disagree —
// the derivation ms = us/1000 is the invariant test_uptime_us.cpp pins.
namespace {
std::uint32_t g_host_us = 0;
}
std::uint32_t uptime_ms() { return g_host_us / 1'000u; }
std::uint32_t uptime_us() { return g_host_us; }
void sleep_for(std::chrono::microseconds) {}

// Test-only clock control: the async sleep/executor read uptime_ms() internally,
// so a test advances virtual time through this hook to drive timer wakes. The
// ms hooks predate uptime_us() and stay — they scale onto the µs counter.
namespace test {
void set_uptime_ms(std::uint32_t ms) { g_host_us = ms * 1'000u; }
void advance_uptime_ms(std::uint32_t d) { g_host_us += d * 1'000u; }
void set_uptime_us(std::uint32_t us) { g_host_us = us; }
void advance_uptime_us(std::uint32_t d) { g_host_us += d; }
}  // namespace test

}  // namespace alloy
