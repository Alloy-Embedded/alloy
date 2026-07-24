// alloy::async::delay — `co_await delay(100ms)` suspends the current task until
// `ms` of uptime elapse, yielding the CPU to other tasks (and to arch::idle())
// meanwhile. Built on the executor's timer list over alloy::uptime_ms(); the
// deadline node lives on the coroutine frame (no heap), and is wrap-safe.
//
// Named `delay`, not `sleep`, on purpose: POSIX libc declares ::sleep(unsigned)
// (via <unistd.h>, pulled in by the Xtensa toolchain), which would make an
// unqualified `co_await sleep(...)` ambiguous under `using namespace`.
//
//   alloy::task blink(alloy::async::task_storage<256>&) {
//       for (;;) { board::led.toggle(); co_await alloy::async::delay(500ms); }
//   }

#pragma once

#include <chrono>
#include <coroutine>
#include <cstdint>

#include "alloy/async/executor.hpp"
#include "alloy/time.hpp"

namespace alloy::async {

class delay {
    std::uint32_t ms_;
    timer_node node_{};

public:
    explicit delay(std::chrono::milliseconds d)
        : ms_(d.count() > 0 ? static_cast<std::uint32_t>(d.count()) : 0u) {}

    // If the awaiter is destroyed while still armed (its task was destroyed
    // mid-delay), unlink the node so the executor never dereferences freed frame.
    ~delay() {
        if (node_.armed) {
            executor_core::the().cancel_timer(node_);
        }
    }
    delay(const delay&) = delete;
    delay& operator=(const delay&) = delete;

    bool await_ready() const noexcept { return ms_ == 0u; }  // 0 ms never suspends

    void await_suspend(std::coroutine_handle<> h) noexcept {
        node_.waiter = h;
        node_.deadline_ms = alloy::uptime_ms() + ms_;
        executor_core::the().arm_timer(node_);
    }

    void await_resume() const noexcept {}
};

}  // namespace alloy::async
