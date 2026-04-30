#pragma once

// Async ADC runtime adapter.
//
// Two operations:
//
//   async::adc::read<Peripheral>(port)
//      -> Result<operation<adc_event::token<Peripheral>>, ErrorCode>
//
//      Single-conversion mode. The EOC (end-of-conversion) interrupt
//      signals the token. The conversion result is read from the ADC
//      data register synchronously after the await resolves.
//
//   async::adc::scan_dma<Peripheral, DmaChannel>(port, dma_channel, samples)
//      -> Result<operation<adc_event::token<Peripheral>>, ErrorCode>
//
//      Multi-channel DMA scan. By default the DMA transfer-complete interrupt
//      signals the token (CompletionTrigger::DmaTransferComplete). Pass
//      CompletionTrigger::EndOfSequence to signal on ADC end-of-sequence
//      instead, which gives per-channel precision when the ADC fires EOS
//      before the DMA TC fires.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "runtime/adc_event.hpp"
#include "runtime/async.hpp"

namespace alloy::runtime::async::adc {

enum class CompletionTrigger : std::uint8_t {
    DmaTransferComplete = 0,
    EndOfSequence       = 1,
};

template <device::PeripheralId Peripheral, typename PortHandle>
[[nodiscard]] auto read(const PortHandle& port)
    -> core::Result<operation<alloy::runtime::adc_event::token<Peripheral>>,
                    core::ErrorCode> {
    using operation_type = operation<alloy::runtime::adc_event::token<Peripheral>>;
    const auto start_result = port.read_async();
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

template <device::PeripheralId Peripheral, typename PortHandle, typename DmaChannel,
          CompletionTrigger Trigger = CompletionTrigger::DmaTransferComplete>
[[nodiscard]] auto scan_dma(const PortHandle& port, const DmaChannel& dma_channel,
                             std::span<std::uint16_t> samples)
    -> core::Result<operation<alloy::runtime::adc_event::token<Peripheral>>,
                    core::ErrorCode> {
    using operation_type = operation<alloy::runtime::adc_event::token<Peripheral>>;
    const auto start_result = port.scan_dma_async(dma_channel, samples);
    if (start_result.is_err()) {
        return core::Err(core::ErrorCode{start_result.err()});
    }
    return core::Ok(operation_type{});
}

}  // namespace alloy::runtime::async::adc
