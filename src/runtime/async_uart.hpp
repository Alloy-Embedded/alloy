#pragma once

// Async UART runtime adapters.
//
// DMA-driven transfers (pre-existing):
//
//   async::uart::write_dma(port, tx_channel, buffer)
//     -> Result<operation<dma_event::token<P, signal_TX>>, ErrorCode>
//
//   async::uart::read_dma(port, rx_channel, buffer)
//     -> Result<operation<dma_event::token<P, signal_RX>>, ErrorCode>
//
// Interrupt-driven single-event wait (new):
//
//   async::uart::wait_for<Kind>(port)
//     -> Result<operation<uart_event::token<P, Kind>>, ErrorCode>
//
//   Resets the per-(peripheral, Kind) completion token, arms the interrupt
//   via port.enable_interrupt(Kind), and returns an awaitable operation.
//   The vendor ISR calls uart_event::token<P, Kind>::signal() when the
//   interrupt fires.
//
//   The caller is responsible for disarming the interrupt after completion:
//     port.disable_interrupt(Kind);
//
//   Useful patterns:
//     - IdleLine: IDLE-line-driven end-of-frame detection for DMA RX
//     - LinBreak: LIN bus break detection
//     - Rxne / Tc: single-byte IRQ-driven transfers without DMA

#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "event.hpp"
#include "hal/uart.hpp"
#include "runtime/async.hpp"
#include "runtime/uart_event.hpp"

namespace alloy::runtime::async::uart {

template <typename PortHandle, typename DmaChannel>
[[nodiscard]] auto write_dma(const PortHandle& port, const DmaChannel& channel,
                             std::span<const std::byte> buffer)
    -> core::Result<
        operation<alloy::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>>,
        core::ErrorCode> {
    static_assert(DmaChannel::valid);
    static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_TX);

    using completion_type = alloy::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>;
    using operation_type = operation<completion_type>;

    const auto start_result = port.write_dma(channel, buffer);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }

    return core::Ok(operation_type{});
}

template <typename PortHandle, typename DmaChannel>
[[nodiscard]] auto read_dma(const PortHandle& port, const DmaChannel& channel,
                            std::span<std::byte> buffer)
    -> core::Result<
        operation<alloy::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>>,
        core::ErrorCode> {
    static_assert(DmaChannel::valid);
    static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_RX);

    using completion_type = alloy::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>;
    using operation_type = operation<completion_type>;

    const auto start_result = port.read_dma(channel, buffer);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }

    return core::Ok(operation_type{});
}

/// Interrupt-driven single-event wait.
///
/// Resets the uart_event::token<P, Kind> static flag, arms the interrupt
/// via port.enable_interrupt(Kind), and returns an awaitable operation.
///
/// Usage:
///   auto op = async::uart::wait_for<InterruptKind::IdleLine>(uart_port);
///   op.wait_for<SysTickSource>(time::Duration::from_millis(100));
///   uart_port.disable_interrupt(InterruptKind::IdleLine);
///
/// The vendor UART ISR must call:
///   uart_event::token<P, InterruptKind::IdleLine>::signal();
///
/// @tparam Kind   The interrupt event to wait for (compile-time constant).
/// @param  port   Any port_handle whose peripheral_id is constexpr and that
///                exposes enable_interrupt(InterruptKind) -> Result<void, ErrorCode>.
template <hal::uart::InterruptKind Kind, typename PortHandle>
[[nodiscard]] auto wait_for(const PortHandle& port)
    -> core::Result<
        operation<alloy::runtime::uart_event::token<PortHandle::peripheral_id, Kind>>,
        core::ErrorCode> {
    using token_type     = alloy::runtime::uart_event::token<PortHandle::peripheral_id, Kind>;
    using operation_type = operation<token_type>;

    // Reset the completion flag before arming so a stale signal is not
    // seen as an immediate completion.
    token_type::reset();

    const auto arm_result = port.enable_interrupt(Kind);
    if (arm_result.is_err()) {
        return core::Err(core::ErrorCode{arm_result.err()});
    }
    return core::Ok(operation_type{});
}

}  // namespace alloy::runtime::async::uart
