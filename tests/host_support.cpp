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

}  // namespace alloy::arch

namespace alloy {

// A trivial host timebase so scheduler::run()/sleep_for link if exercised;
// unit tests inject their own clock into run_once() instead.
namespace {
std::uint32_t g_host_ms = 0;
}
std::uint32_t uptime_ms() { return g_host_ms; }
void sleep_for(std::chrono::microseconds) {}

}  // namespace alloy
