// Tier-1 services demo: the cooperative scheduler, the compile-time logger, a
// moving-average filter, and generated board-role accessors — the "peripherals
// to product" layer. Two periodic tasks share one stack with no RTOS: a
// heartbeat blinks the status LED and smooths a counter, a poller watches the
// user button. Zero #ifdefs — status_led()/user_button() exist on every board
// (real or no-op), and the console is guarded by `if constexpr (caps)`.
#include <alloy/board.hpp>
#include <alloy/log.hpp>
#include <alloy/sched.hpp>
#include <alloy/util/moving_average.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {

alloy::scheduler<4> sched;
alloy::moving_average<std::uint32_t, 8> load_avg;
std::uint32_t ticks = 0;

void heartbeat(void*, std::uint32_t) {
    board::status_led().toggle();
    ticks += 1;
    (void)load_avg.add(ticks);
}

void poll_button(void*, std::uint32_t) {
    // A no-op null_input on boards without a button — still compiles.
    if (board::user_button().is_active()) {
        board::status_led().on();
    }
}

}  // namespace

int main() {
    board::init();

    if constexpr (board::caps::debug_uart) {
        auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
        auto log = alloy::log::make<alloy::log::level::info>(uart);
        log.info("alloy services demo up");
        log.debug("this line is compiled out below info level");
        log.info("moving-average window value", load_avg.value());
    }

    sched.add(&heartbeat, nullptr, 500ms);
    sched.add(&poll_button, nullptr, 50ms);
    sched.run();  // never returns
}
