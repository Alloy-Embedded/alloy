#pragma once

// Async SPI runtime adapter — DMA-driven write / read / transfer.
//
// Three operations:
//
//   async::spi::write_dma<Connector>(handle, tx_channel, buffer)
//      -> Result<operation<dma_event::token<P, signal_TX>>, ErrorCode>
//
//   async::spi::read_dma<Connector>(handle, rx_channel, buffer)
//      -> Result<operation<dma_event::token<P, signal_RX>>, ErrorCode>
//
//   async::spi::transfer_dma<Connector>(handle, tx_channel, rx_channel, tx, rx)
//      -> Result<operation<dma_event::token<P, signal_RX>>, ErrorCode>
//
// The TX-complete and RX-complete DMA interrupts signal the corresponding
// `dma_event::token` from the vendor ISR. The blocking SPI API in
// `hal/spi/spi.hpp` is unchanged; this header is opt-in.
//
// The wrappers are HAL-handle-duck-typed: any `port` whose `write_dma`,
// `read_dma`, or `transfer_dma` method takes the same arguments and
// returns `Result<void, ErrorCode>` plugs in. Vendor backends (STM32,
// SAME70) wire those methods to start the DMA transfer and arm the
// completion interrupt.

#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "hal/dma.hpp"
#include "hal/spi.hpp"
#include "runtime/async.hpp"
#include "runtime/dma_event.hpp"
#include "runtime/spi_event.hpp"

namespace alloy::runtime::async::spi {

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE

template <typename PortHandle, typename DmaChannel>
[[nodiscard]] auto write_dma(const PortHandle& port, const DmaChannel& channel,
                             std::span<const std::byte> buffer)
    -> core::Result<
        operation<alloy::runtime::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>>,
        core::ErrorCode> {
    static_assert(DmaChannel::valid);
    static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_TX,
                  "SPI write_dma requires a TX-direction DMA channel");

    using completion_type =
        alloy::runtime::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>;
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
        operation<alloy::runtime::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>>,
        core::ErrorCode> {
    static_assert(DmaChannel::valid);
    static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_RX,
                  "SPI read_dma requires an RX-direction DMA channel");

    using completion_type =
        alloy::runtime::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>;
    using operation_type = operation<completion_type>;

    const auto start_result = port.read_dma(channel, buffer);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

template <typename PortHandle, typename TxDmaChannel, typename RxDmaChannel>
[[nodiscard]] auto transfer_dma(const PortHandle& port,
                                const TxDmaChannel& tx_channel,
                                const RxDmaChannel& rx_channel,
                                std::span<const std::byte> tx,
                                std::span<std::byte> rx)
    -> core::Result<
        operation<alloy::runtime::dma_event::token<RxDmaChannel::peripheral_id, RxDmaChannel::signal_id>>,
        core::ErrorCode> {
    static_assert(TxDmaChannel::valid && RxDmaChannel::valid);
    static_assert(TxDmaChannel::signal_id == alloy::hal::dma::SignalId::signal_TX);
    static_assert(RxDmaChannel::signal_id == alloy::hal::dma::SignalId::signal_RX);
    static_assert(TxDmaChannel::peripheral_id == RxDmaChannel::peripheral_id,
                  "SPI transfer_dma requires both DMA channels on the same peripheral");

    using completion_type =
        alloy::runtime::dma_event::token<RxDmaChannel::peripheral_id, RxDmaChannel::signal_id>;
    using operation_type = operation<completion_type>;

    const auto start_result = port.transfer_dma(tx_channel, rx_channel, tx, rx);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

#endif  // ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE

/// Interrupt-driven single-event wait.
///
/// Resets the spi_event::token<P, Kind> static flag, arms the interrupt
/// via port.enable_interrupt(Kind), and returns an awaitable operation.
///
/// Usage:
///   auto op = async::spi::wait_for<InterruptKind::CrcError>(spi_port);
///   op.wait_for<SysTickSource>(time::Duration::from_millis(100));
///   spi_port.disable_interrupt(InterruptKind::CrcError);
///
/// The vendor SPI ISR must call:
///   spi_event::token<P, InterruptKind::CrcError>::signal();
template <hal::spi::InterruptKind Kind, typename PortHandle>
[[nodiscard]] auto wait_for(const PortHandle& port)
    -> core::Result<
        operation<alloy::runtime::spi_event::token<PortHandle::peripheral_id, Kind>>,
        core::ErrorCode> {
    using token_type     = alloy::runtime::spi_event::token<PortHandle::peripheral_id, Kind>;
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

}  // namespace alloy::runtime::async::spi
