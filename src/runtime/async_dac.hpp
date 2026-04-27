#pragma once

// Async DAC runtime adapter — interrupt-driven single-event wait.
//
//   async::dac::wait_for<Kind>(handle)
//      -> Result<operation<dac_event::token<P, Kind>>, ErrorCode>
//
// Arms the DAC interrupt for `Kind`, returns an awaitable that resolves
// when the vendor ISR calls `dac_event::token<P, Kind>::signal()`.
//
// Typical usage: arm TransferComplete before a DMA-driven sine write,
// or Underrun to detect missed conversions.

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/dac.hpp"
#include "runtime/async.hpp"

#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
#include "runtime/dac_event.hpp"
#endif

namespace alloy::runtime::async::dac {

#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE

/// Interrupt-driven single-event wait for a DAC peripheral.
///
/// Resets the dac_event::token<P, Kind> static flag, arms the interrupt
/// via handle.enable_interrupt(Kind), and returns an awaitable operation.
///
/// The vendor DAC ISR must call:
///   dac_event::token<P, Kind>::signal();
///
/// Example:
///   auto op = async::dac::wait_for<dac::InterruptKind::TransferComplete>(dac_handle);
///   op.wait_until<SysTickSource>(deadline);
template <hal::dac::InterruptKind Kind, typename Handle>
[[nodiscard]] auto wait_for(const Handle& handle)
    -> core::Result<
        operation<alloy::runtime::dac_event::token<Handle::peripheral_id, Kind>>,
        core::ErrorCode> {
    using token_type     = alloy::runtime::dac_event::token<Handle::peripheral_id, Kind>;
    using operation_type = operation<token_type>;

    token_type::reset();

    const auto arm_result = handle.enable_interrupt(Kind);
    if (arm_result.is_err()) {
        return core::Err(core::ErrorCode{arm_result.err()});
    }
    return core::Ok(operation_type{});
}

#endif  // ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE

}  // namespace alloy::runtime::async::dac
