#pragma once

#include <span>

#include "event.hpp"
#include "hal/uart.hpp"
#include "runtime/async.hpp"

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

}  // namespace alloy::runtime::async::uart
