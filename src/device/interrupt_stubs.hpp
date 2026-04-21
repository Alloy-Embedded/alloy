#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

#if ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE
namespace interrupt_stubs {

namespace device_contract = selected::runtime_interrupt_stub_device_contract;

using InterruptId = device_contract::InterruptId;
using StartupSymbolId = device_contract::StartupSymbolId;
using Descriptor = device_contract::InterruptStubDescriptor;

inline constexpr auto all = std::span{device_contract::kInterruptStubs};

template <InterruptId Id>
using Traits = device_contract::InterruptStubTraits<Id>;

}  // namespace interrupt_stubs
#endif

struct SelectedInterruptStubs {
    static constexpr bool available = selected::interrupt_stubs_available;
};

}  // namespace alloy::device
