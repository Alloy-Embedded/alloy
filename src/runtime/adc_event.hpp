#pragma once

// ADC completion token. Single-conversion mode signals from the EOC
// (end-of-conversion) interrupt; DMA-scan mode signals from the DMA
// transfer-complete interrupt of the channel bound to the ADC.
//
// Vendor ISR hooks call `adc_event::token<P>::signal()` from the
// peripheral interrupt; `async_adc.hpp` returns
// `operation<adc_event::token<P>>` from `async::adc::read` and
// `async::adc::scan_dma`.

#include "device/runtime.hpp"
#include "runtime/event.hpp"

namespace alloy::runtime::adc_event {

template <device::PeripheralId Peripheral>
struct tag {
    static constexpr auto value = Peripheral;
};

template <device::PeripheralId Peripheral>
using token = event::completion<tag<Peripheral>>;

}  // namespace alloy::runtime::adc_event
