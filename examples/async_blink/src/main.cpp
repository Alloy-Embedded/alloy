// Portable async blink — cooperative coroutine multitasking, zero heap, no RTOS.
//
// Two independent tasks share one core through alloy's coroutine executor: the
// LED blinks on its own timeline while a second task keeps an uptime counter on
// another. Both suspend at `co_await delay(...)`, so the core idles between
// events instead of busy-waiting a single hard-coded period. This is the thing a
// plain superloop with one sleep_for() can't express cleanly — two schedules,
// straight-line code, no state machine.
//
// No #ifdef, no arch code: retarget to any board by changing the id in
// alloy.toml. The coroutine frames live in the static task_storage objects
// below (checked to fit at compile time); nothing touches the heap.

#include <cstdint>

#include <alloy/async/delay.hpp>
#include <alloy/async/executor.hpp>
#include <alloy/async/task.hpp>
#include <alloy/board.hpp>

using namespace alloy::async;
using namespace alloy::literals;

namespace {

executor<4> sched;
task_storage<256> blink_frame;
task_storage<256> uptime_frame;

// A second, observable timeline (readable over SWD / in an emulator).
volatile std::uint32_t g_uptime_s = 0;

// Blink the board LED forever on its own 200 ms cadence.
task blink(task_storage<256>&) {
    for (;;) {
        board::led.toggle();
        co_await delay(200ms);
    }
}

// Count seconds forever on a separate 1 s cadence — proves two schedules run
// concurrently on one stack with no preemption and no heap.
task uptime(task_storage<256>&) {
    for (;;) {
        co_await delay(1'000ms);
        g_uptime_s = g_uptime_s + 1;
    }
}

}  // namespace

int main() {
    board::init();
    sched.spawn(blink(blink_frame));
    sched.spawn(uptime(uptime_frame));
    sched.run();  // cooperative superloop; never returns
}
