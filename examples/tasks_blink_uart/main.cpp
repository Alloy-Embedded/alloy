// Cooperative coroutine scheduler example: blink + event-driven counter.
//
// Three tasks run side by side on a single stack:
//
//   - blink_task     (priority::normal): toggles board::led every 500 ms.
//   - producer_task  (priority::low):    waits 1 s, signals `tick_event`, repeats.
//   - consumer_task  (priority::high):   awaits `tick_event` and bumps a counter.
//
// What the example demonstrates:
//
//   1. Linear-looking concurrency. Each task reads top-to-bottom; the only
//      "concurrency" syntax is `co_await`.
//   2. Priority. The high-priority consumer wakes immediately when the event
//      fires, even though the low-priority producer is still ready.
//   3. Event signalling. The same `tasks::Event` API works for ISR -> task
//      hand-offs (the producer here is a stand-in for an interrupt handler).
//   4. Zero heap. Coroutine frames live in `Scheduler<3, 256>` -- 3 slots of
//      256 bytes, total 768 bytes of pool RAM allocated statically.
//
// Pool sizing: a coroutine that only does delay/on/yield typically lands at
// ~150-200 bytes of frame on Cortex-M0+ at -Os. 256 bytes is comfortable.
// `alloy toolchain install arm-none-eabi-gcc` builds at -Os here; the binary
// is ~5 KB total, fitting in 4% of an STM32G071RB's flash.

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
#include "runtime/tasks/event.hpp"
#include "runtime/tasks/scheduler.hpp"
#include "runtime/time.hpp"

using alloy::runtime::time::Duration;
using alloy::tasks::Priority;
using alloy::tasks::Task;

namespace {

// Cooperative scheduler => no preemption between tasks, so this counter does
// not need volatile/atomic. A debugger reading it sees a stable value because
// no one else touches it while a task is suspended at a `co_await` point.
std::uint32_t tick_count = 0;

// Shared event between producer and consumer. In a real application this is
// what an ISR would `signal()` after enqueuing data (e.g. a UART RX byte).
alloy::tasks::Event tick_event;

// Time source for the scheduler. Wraps the board's SysTick into the runtime
// time::Instant the scheduler expects.
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

auto producer_task() -> Task {
    while (true) {
        static_cast<void>(co_await alloy::tasks::delay(Duration::from_millis(1000)));
        tick_event.signal();
    }
}

auto consumer_task() -> Task {
    while (true) {
        static_cast<void>(co_await alloy::tasks::on(tick_event));
        ++tick_count;
    }
}

}  // namespace

int main() {
    board::init();

    alloy::tasks::Scheduler<3, 256> sched;
    sched.set_time_source(time_now);

    // The lambda factories install the scheduler's pool around the coroutine
    // creation; without that the promise's allocator hook returns nullptr.
    auto blink = sched.spawn([] { return blink_task(); }, Priority::Normal);
    auto producer = sched.spawn([] { return producer_task(); }, Priority::Low);
    auto consumer = sched.spawn([] { return consumer_task(); }, Priority::High);

    if (blink.is_err() || producer.is_err() || consumer.is_err()) {
        // Pool too small or task table full. The board has no console here, so
        // we just spin -- a debugger inspecting the scheduler will see the
        // error code in the Result.
        while (true) {
        }
    }

    sched.run();
    return 0;
}
