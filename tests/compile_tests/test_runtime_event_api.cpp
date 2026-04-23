#include BOARD_HEADER

#include "async.hpp"
#include "event.hpp"
#include "time.hpp"

using BoardTime = alloy::time::source<board::BoardSysTick>;

namespace {

using LocalCompletion = alloy::event::completion<alloy::event::tag_constant<0u>>;
using LocalOperation = alloy::async::operation<LocalCompletion>;

}  // namespace

int main() {
    LocalCompletion::reset();
    [[maybe_unused]] auto operation = LocalOperation{};
    [[maybe_unused]] const auto pending = operation.poll();
    [[maybe_unused]] const auto timeout =
        LocalCompletion::wait_for<BoardTime>(alloy::time::Duration::from_micros(0));
    LocalCompletion::signal();
    [[maybe_unused]] const auto callback_ran = operation.notify_if_ready([] {});
    [[maybe_unused]] const auto ready_now =
        operation.wait_for<BoardTime>(alloy::time::Duration::from_millis(1));

#if ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE
    constexpr auto interrupt_id = alloy::device::interrupt_stubs::all.front().interrupt_id;
    using InterruptCompletion = alloy::interrupt_event::token<interrupt_id>;
    InterruptCompletion::reset();
    InterruptCompletion::signal();
    [[maybe_unused]] const auto interrupt_ready =
        InterruptCompletion::wait_for<BoardTime>(alloy::time::Duration::from_millis(1));
#endif

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
