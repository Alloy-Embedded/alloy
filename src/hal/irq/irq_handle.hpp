#pragma once

// irq_handle.hpp — Tasks 2.1–2.5 (add-irq-vector-hal)
//
// irq::enable<P>(priority), irq::disable<P>(), irq::set_priority<P>(priority)
//
// Architecture dispatch:
//   Cortex-M  → nvic_impl.hpp (NVIC_EnableIRQ, NVIC_SetPriority, NVIC_DisableIRQ)
//   RISC-V    → plic_impl.hpp (PLIC enable/priority registers)
//   Xtensa    → xthal_impl.hpp (placeholder)
//   AVR       → avr_impl.hpp (sei/cli + mask register)

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/irq/irq_id.hpp"
#include "hal/irq/irq_vector_traits.hpp"
#include "device/runtime.hpp"

// Architecture detection — include the right backend
#if defined(__ARM_ARCH) || defined(__arm__) || defined(__aarch64__)
#  include "hal/irq/detail/cortex_m/nvic_impl.hpp"
   namespace alloy::hal::irq::detail { using Impl = CortexMNvicImpl; }
#elif defined(__riscv)
#  include "hal/irq/detail/riscv/plic_impl.hpp"
   namespace alloy::hal::irq::detail { using Impl = RiscVPlicImpl; }
#elif defined(__XTENSA__)
#  include "hal/irq/detail/xtensa/xthal_impl.hpp"
   namespace alloy::hal::irq::detail { using Impl = XtensaImpl; }
#elif defined(__AVR__)
#  include "hal/irq/detail/avr/avr_impl.hpp"
   namespace alloy::hal::irq::detail { using Impl = AvrImpl; }
#else
   // Host build / unknown arch — use a no-op stub
   namespace alloy::hal::irq::detail {
       struct NoopImpl {
           static void enable(IrqId, std::uint8_t) {}
           static void disable(IrqId) {}
           static void set_priority(IrqId, std::uint8_t) {}
       };
       using Impl = NoopImpl;
   }
#endif

namespace alloy::hal::irq {

// ---------------------------------------------------------------------------
// enable<P>(priority)
// ---------------------------------------------------------------------------

/// Enable the IRQ for peripheral P at the given NVIC priority level (0 = highest).
///
/// Returns:
///   Ok()                — enabled
///   Err(NotSupported)   — peripheral has no IRQ or device not compiled with IRQ data
template <device::runtime::PeripheralId P>
[[nodiscard]] auto enable(std::uint8_t priority = 4u) -> core::Result<void, core::ErrorCode> {
    if constexpr (!kHasIrq<P>) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        constexpr auto irq = IrqVectorTraits<P>::kIrqId;
        detail::Impl::set_priority(irq, priority);
        detail::Impl::enable(irq, priority);
        return core::Ok();
    }
}

// ---------------------------------------------------------------------------
// disable<P>()
// ---------------------------------------------------------------------------

/// Disable the IRQ for peripheral P.
template <device::runtime::PeripheralId P>
[[nodiscard]] auto disable() -> core::Result<void, core::ErrorCode> {
    if constexpr (!kHasIrq<P>) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        detail::Impl::disable(IrqVectorTraits<P>::kIrqId);
        return core::Ok();
    }
}

// ---------------------------------------------------------------------------
// set_priority<P>(priority)
// ---------------------------------------------------------------------------

/// Set the priority of the IRQ for peripheral P without enabling it.
template <device::runtime::PeripheralId P>
[[nodiscard]] auto set_priority(std::uint8_t priority) -> core::Result<void, core::ErrorCode> {
    if constexpr (!kHasIrq<P>) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        detail::Impl::set_priority(IrqVectorTraits<P>::kIrqId, priority);
        return core::Ok();
    }
}

}  // namespace alloy::hal::irq
