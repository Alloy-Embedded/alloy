// Compile-time proof that the canonical completion+timeout path stays usable
// without linking the async adapter layer.
//
// This TU deliberately includes only `event.hpp` and `time.hpp` and NEVER
// `async.hpp`. It exercises the same `runtime::event::completion` primitive
// used by the canonical `async_uart_timeout` example and the
// `test_async_completion_timeout` unit test.
//
// If this file compiles, the blocking+completion code path is valid on its
// own: users who stick to it pay for zero async-adapter surface.
//
// The `alloy::async` namespace is intentionally NOT referenced here. A
// repo-level grep guard in tests/compile/blocking_only_guard.cmake backs this
// invariant — it fails if anyone adds `#include "async.hpp"` or names
// `alloy::async::` in this file.

#include BOARD_HEADER

#include "event.hpp"
#include "time.hpp"

using BoardTime = alloy::time::source<board::BoardSysTick>;

namespace {

using BlockingCompletion = alloy::event::completion<alloy::event::tag_constant<0xB10CCu>>;

}  // namespace

int main() {
    BlockingCompletion::reset();

    // Signal-first path: completion already ready, wait_for returns Ok
    // without ever sleeping.
    BlockingCompletion::signal();
    [[maybe_unused]] const auto ready_now =
        BlockingCompletion::wait_for<BoardTime>(alloy::time::Duration::from_millis(1));

    // Timeout path: reset, don't signal, let wait_for exit via the deadline.
    BlockingCompletion::reset();
    [[maybe_unused]] const auto timed_out =
        BlockingCompletion::wait_for<BoardTime>(alloy::time::Duration::from_micros(0));

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
    constexpr auto binding = alloy::device::dma::bindings.front();
    using DmaCompletion = alloy::dma_event::token<binding.peripheral_id, binding.signal_id>;
    DmaCompletion::reset();
    DmaCompletion::signal();
    [[maybe_unused]] const auto dma_ready =
        DmaCompletion::wait_for<BoardTime>(alloy::time::Duration::from_millis(1));
#endif

    return 0;
}
