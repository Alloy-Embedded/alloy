// Cooperative coroutine scheduler example: blink + ISR-stand-in feeding a
// SPSC channel that a consumer task drains and tallies.
//
// Three tasks run side by side on a single stack:
//
//   - blink_task     (priority::normal): toggles board::led every 500 ms.
//   - producer_task  (priority::low):    every 250 ms, pushes a fake "byte"
//                                        into rx_channel. Stand-in for what a
//                                        UART RX ISR would do in real code.
//   - consumer_task  (priority::high):   drains rx_channel; tallies the byte
//                                        count and a running checksum.
//
// What the example demonstrates:
//
//   1. Linear-looking concurrency. Each task reads top-to-bottom; the only
//      "concurrency" syntax is `co_await`.
//   2. Priority. The high-priority consumer wakes immediately when the channel
//      receives a byte, even though the low-priority producer is still ready.
//   3. SPSC channel. `try_push` is the call an ISR makes; `wait()` is the
//      consumer's await. Drain-then-wait is the canonical pattern -- the
//      consumer pops everything ready before suspending again, so multiple
//      bytes that arrive between two consumer wakes are never lost.
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
#include "runtime/tasks/scheduler.hpp"
#include "runtime/tasks/uart_channel.hpp"
#include "runtime/time.hpp"

using alloy::runtime::time::Duration;
using alloy::tasks::Priority;
using alloy::tasks::Task;

namespace {

// Cooperative scheduler => no preemption between tasks, so these counters do
// not need volatile/atomic. A debugger reading them sees a stable value because
// no one else touches them while a task is suspended at a `co_await` point.
std::uint32_t bytes_received = 0;
std::uint32_t running_checksum = 0;

// 16-byte ring big enough that a UART RX burst at ~115200 bps would not drop
// even if the consumer task takes a few ms to wake. `feed_from_isr` is what
// a real `USARTx_IRQHandler` would call after reading the data register.
alloy::tasks::UartRxChannel<16> rx_channel;

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
    std::uint8_t next_byte = 0;
    while (true) {
        static_cast<void>(co_await alloy::tasks::delay(Duration::from_millis(250)));
        // feed_from_isr is what a real `USARTx_IRQHandler` would call after
        // reading the data register. We discard the return because dropping
        // a fake byte here is harmless; real code would log
        // `rx_channel.drops()` to detect undersized rings.
        static_cast<void>(rx_channel.feed_from_isr(next_byte++));
    }
}

auto consumer_task() -> Task {
    while (true) {
        // Drain everything ready; only suspend when the ring is empty. This
        // is the canonical SPSC consumer loop -- multiple bytes arriving
        // between two wakeups are all consumed in the inner while.
        while (auto byte = rx_channel.try_pop()) {
            ++bytes_received;
            running_checksum =
                (running_checksum + static_cast<std::uint32_t>(*byte)) & 0xFFu;
        }
        static_cast<void>(co_await rx_channel.wait());
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
