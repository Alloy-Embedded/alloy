// Cooperative coroutine scheduler example: blink + heartbeat counter.
//
// Two tasks run side by side on a single stack:
//
//   - blink_task       (priority::normal): toggles board::led every 500 ms.
//   - heartbeat_task   (priority::low):    counts up once per second; the
//                                          counter could be sent over UART
//                                          or just inspected with a debugger.
//
// The point of the example is to show how concurrent flows look in alloy's
// task model: each `co_await alloy::tasks::delay(...)` is a yield point;
// between yields no other task runs, so there are no races on shared state
// (here, just `heartbeat_count`) without any mutex.
//
// Pool sizing: for two tasks doing only `delay` plus some local state, ~256
// bytes per slot is comfortable. The Scheduler<2, 256> declaration makes the
// RAM cost explicit -- 2 * 256 = 512 bytes for coroutine frames plus the
// scheduler's task table and ready-queue overhead.

#include <cstdint>

// The board catalog is conditional just like in examples/blink: the build
// system defines exactly one ALLOY_BOARD_* macro and we pick the matching
// header. Adds a board => one new line here.
#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board.hpp"
#elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    #include "raspberry_pi_pico/board.hpp"
#elif defined(ALLOY_BOARD_ESP32C3_DEVKITM)
    #include "esp32c3_devkitm/board.hpp"
#elif defined(ALLOY_BOARD_ESP32S3_DEVKITC)
    #include "esp32s3_devkitc/board.hpp"
#else
    #error "Unsupported board! Define ALLOY_BOARD_* in your build system."
#endif

#include "hal/systick.hpp"
#include "runtime/tasks/scheduler.hpp"
#include "runtime/time.hpp"

using alloy::runtime::time::Duration;
using alloy::tasks::Priority;
using alloy::tasks::Task;

namespace {

// Cooperative scheduler => no preemption between tasks, so this counter does
// not need volatile/atomic. A debugger reading it sees a stable value because
// no one else touches it while a task is suspended at a `co_await` point.
std::uint32_t heartbeat_count = 0;

// Time source for the scheduler. Wraps the board's SysTick into the runtime
// time::Instant the scheduler expects. Real apps that already have a
// `runtime::time::source<BoardSysTick>` typedef can use that directly.
auto time_now() -> alloy::runtime::time::Instant {
    return alloy::runtime::time::Instant::from_micros(
        alloy::hal::SysTickTimer::micros<board::BoardSysTick>());
}

auto blink_task() -> Task {
    while (true) {
        board::led::toggle();
        // The cast to void discards the Result<void, Cancelled>. This task
        // never opts into cancellation, so the awaiter always succeeds.
        static_cast<void>(co_await alloy::tasks::delay(Duration::from_millis(500)));
    }
}

auto heartbeat_task() -> Task {
    while (true) {
        ++heartbeat_count;
        static_cast<void>(co_await alloy::tasks::delay(Duration::from_millis(1000)));
    }
}

}  // namespace

int main() {
    board::init();

    alloy::tasks::Scheduler<2, 256> sched;
    sched.set_time_source(time_now);

    // The lambda factories install the scheduler's pool around the coroutine
    // creation; without that the promise's allocator hook returns nullptr.
    auto blink = sched.spawn([] { return blink_task(); }, Priority::Normal);
    auto heartbeat = sched.spawn([] { return heartbeat_task(); }, Priority::Low);

    if (blink.is_err() || heartbeat.is_err()) {
        // Pool too small or task table full. The board has no console here, so
        // we just spin -- a debugger inspecting the scheduler will see the
        // error code in the Result.
        while (true) {
        }
    }

    sched.run();
    return 0;
}
