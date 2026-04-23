#pragma once

#include "hal/dma.hpp"
#include "runtime/event.hpp"

namespace alloy::runtime::dma_event {

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
template <hal::dma::PeripheralId Peripheral, hal::dma::SignalId Signal>
using token = event::completion<event::tag_constant<hal::dma::BindingTraits<Peripheral, Signal>::kBindingId>>;
#endif

}  // namespace alloy::runtime::dma_event
