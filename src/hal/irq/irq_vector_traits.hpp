#pragma once

// irq_vector_traits.hpp — Task 1.1 (add-irq-vector-hal)
//
// IrqVectorTraits<PeripheralId> — maps a PeripheralId to its CMSIS IRQ number.
//
// When ALLOY_DEVICE_IRQ_SEMANTICS_AVAILABLE=1 (device provides driver_semantics/irq.hpp),
// the primary template delegates to device::runtime::IrqSemanticTraits<P>.
// Otherwise kPresent=false / kIrqId=invalid (safe no-op for all irq:: functions).

#include <cstdint>

#include "hal/irq/irq_id.hpp"
#include "device/runtime.hpp"  // defines ALLOY_DEVICE_IRQ_SEMANTICS_AVAILABLE

namespace alloy::hal::irq {

/// Primary template — delegates to device IrqSemanticTraits when available.
template <device::runtime::PeripheralId P>
struct IrqVectorTraits {
#if defined(ALLOY_DEVICE_IRQ_SEMANTICS_AVAILABLE) && ALLOY_DEVICE_IRQ_SEMANTICS_AVAILABLE
    static constexpr bool kPresent =
        device::runtime::IrqSemanticTraits<P>::kPresent;
    static constexpr IrqId kIrqId =
        kPresent
            ? IrqId{static_cast<std::uint16_t>(
                  device::runtime::IrqSemanticTraits<P>::kPrimaryIrqLine)}
            : kInvalidIrqId;
#else
    static constexpr bool kPresent = false;
    static constexpr IrqId kIrqId  = kInvalidIrqId;
#endif
};

/// Helper: does a peripheral have a valid IRQ?
template <device::runtime::PeripheralId P>
inline constexpr bool kHasIrq =
    IrqVectorTraits<P>::kPresent && IrqVectorTraits<P>::kIrqId.is_valid();

}  // namespace alloy::hal::irq
