#pragma once

// Async GPIO edge-event runtime adapter.
//
//   async::gpio::wait_edge<Pin>(port, edge)
//      -> Result<operation<gpio_event::token<Pin>>, ErrorCode>
//
// Configures the pin's external-interrupt line for the requested edge
// (Rising / Falling / Both), arms the interrupt, and returns an
// awaitable operation. The line-matched ISR (EXTI on STM32, PIO on
// SAME70) signals `gpio_event::token<Pin>::signal()` from the ISR.
//
// Debounce: the vendor backend MAY apply a re-arm gap (typical 10 ms)
// before accepting the next edge — the gap is documented per backend
// and is configurable via the existing `hal::gpio` configuration; the
// runtime adapter itself does not implement debounce.

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "runtime/async.hpp"
#include "runtime/gpio_event.hpp"

namespace alloy::runtime::async::gpio {

enum class Edge : unsigned char {
    Rising = 0,
    Falling = 1,
    Both = 2,
};

template <device::PinId Pin, typename PortHandle>
[[nodiscard]] auto wait_edge(const PortHandle& port, Edge edge)
    -> core::Result<operation<alloy::runtime::gpio_event::token<Pin>>,
                    core::ErrorCode> {
    using operation_type = operation<alloy::runtime::gpio_event::token<Pin>>;
    const auto start_result = port.template arm_edge_async<Pin>(edge);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

}  // namespace alloy::runtime::async::gpio
