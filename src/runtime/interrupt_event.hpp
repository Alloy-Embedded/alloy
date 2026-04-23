#pragma once

#include "device/interrupt_stubs.hpp"
#include "runtime/event.hpp"

namespace alloy::runtime::interrupt_event {

#if ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE
template <device::interrupt_stubs::InterruptId Id>
using token = event::completion<event::tag_constant<Id>>;
#endif

}  // namespace alloy::runtime::interrupt_event
