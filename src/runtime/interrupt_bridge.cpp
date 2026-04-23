#include "event.hpp"

#include <cstdint>

#include "device/interrupt_stubs.hpp"
#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::runtime::detail {

namespace rt = alloy::hal::detail::runtime;

template <device::interrupt_stubs::InterruptId Id>
inline auto signal_interrupt_token() -> void {
    alloy::interrupt_event::token<Id>::signal();
}

template <typename InterruptEnum>
inline constexpr bool has_dma_channel1_interrupt_v = requires { InterruptEnum::DMA_Channel1; };

template <typename InterruptEnum>
inline constexpr bool has_dma_channel23_interrupt_v = requires { InterruptEnum::DMA_Channel2_3; };

template <typename InterruptEnum>
inline constexpr bool has_usart1_interrupt_v = requires { InterruptEnum::USART1; };

template <typename InterruptEnum>
inline constexpr bool has_xdmac_interrupt_v = requires { InterruptEnum::XDMAC; };

template <typename InterruptEnum>
inline auto signal_dma_channel1_interrupt_if_present() -> void {
    if constexpr (has_dma_channel1_interrupt_v<InterruptEnum>) {
        signal_interrupt_token<InterruptEnum::DMA_Channel1>();
    }
}

template <typename InterruptEnum>
inline auto signal_dma_channel23_interrupt_if_present() -> void {
    if constexpr (has_dma_channel23_interrupt_v<InterruptEnum>) {
        signal_interrupt_token<InterruptEnum::DMA_Channel2_3>();
    }
}

template <typename InterruptEnum>
inline auto signal_usart1_interrupt_if_present() -> void {
    if constexpr (has_usart1_interrupt_v<InterruptEnum>) {
        signal_interrupt_token<InterruptEnum::USART1>();
    }
}

template <typename InterruptEnum>
inline auto signal_xdmac_interrupt_if_present() -> void {
    if constexpr (has_xdmac_interrupt_v<InterruptEnum>) {
        signal_interrupt_token<InterruptEnum::XDMAC>();
    }
}

template <hal::dma::PeripheralId Peripheral, hal::dma::SignalId Signal>
inline auto signal_dma_token_if_present() -> void {
    if constexpr (hal::dma::BindingTraits<Peripheral, Signal>::kPresent) {
        alloy::dma_event::token<Peripheral, Signal>::signal();
    }
}

template <typename FieldEnum>
inline constexpr bool has_dma1_isr_tcif1_v = requires { FieldEnum::field_dma1_isr_tcif1; };

template <typename FieldEnum>
inline constexpr bool has_dma1_ifcr_ctcif1_v = requires { FieldEnum::field_dma1_ifcr_ctcif1; };

template <typename FieldEnum>
inline constexpr bool has_dma1_isr_tcif2_v = requires { FieldEnum::field_dma1_isr_tcif2; };

template <typename FieldEnum>
inline constexpr bool has_dma1_ifcr_ctcif5_v = requires { FieldEnum::field_dma1_ifcr_ctcif5; };

template <device::FieldId StatusFieldId, device::FieldId ClearFieldId, hal::dma::PeripheralId Peripheral,
          hal::dma::SignalId Signal>
inline auto bridge_dma_transfer_complete_if_set() -> void {
    constexpr auto status_field = rt::field_ref<StatusFieldId>();
    constexpr auto clear_field = rt::field_ref<ClearFieldId>();

    static_assert(status_field.valid);
    static_assert(clear_field.valid);

    const auto status_value = rt::read_register(status_field.reg);
    if (status_value.is_err()) {
        return;
    }

    const auto status_mask = rt::field_mask(status_field);
    if ((status_value.unwrap() & status_mask) == 0u) {
        return;
    }

    signal_dma_token_if_present<Peripheral, Signal>();
    static_cast<void>(rt::write_register(clear_field.reg, rt::field_mask(clear_field)));
}

template <typename FieldEnum>
inline auto bridge_stm32g0_dma_channel1() -> void {
    if constexpr (has_dma1_isr_tcif1_v<FieldEnum> && has_dma1_ifcr_ctcif1_v<FieldEnum>) {
        bridge_dma_transfer_complete_if_set<FieldEnum::field_dma1_isr_tcif1,
                                            FieldEnum::field_dma1_ifcr_ctcif1,
                                            hal::dma::PeripheralId::USART1,
                                            hal::dma::SignalId::signal_RX>();
    }
}

template <typename FieldEnum>
inline auto bridge_stm32g0_dma_channel2() -> void {
    if constexpr (has_dma1_isr_tcif2_v<FieldEnum> && has_dma1_ifcr_ctcif5_v<FieldEnum>) {
        bridge_dma_transfer_complete_if_set<FieldEnum::field_dma1_isr_tcif2,
                                            FieldEnum::field_dma1_ifcr_ctcif5,
                                            hal::dma::PeripheralId::USART1,
                                            hal::dma::SignalId::signal_TX>();
    }
}

}  // namespace alloy::runtime::detail

extern "C" void DMA_Channel1_IRQHandler() {
#if ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE
    if constexpr (alloy::runtime::detail::has_dma_channel1_interrupt_v<
                      alloy::device::interrupt_stubs::InterruptId>) {
        alloy::runtime::detail::signal_dma_channel1_interrupt_if_present<
            alloy::device::interrupt_stubs::InterruptId>();
        alloy::runtime::detail::bridge_stm32g0_dma_channel1<alloy::device::FieldId>();
    }
#endif
}

extern "C" void DMA_Channel2_3_IRQHandler() {
#if ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE
    if constexpr (alloy::runtime::detail::has_dma_channel23_interrupt_v<
                      alloy::device::interrupt_stubs::InterruptId>) {
        alloy::runtime::detail::signal_dma_channel23_interrupt_if_present<
            alloy::device::interrupt_stubs::InterruptId>();
        alloy::runtime::detail::bridge_stm32g0_dma_channel2<alloy::device::FieldId>();
    }
#endif
}

extern "C" void USART1_IRQHandler() {
#if ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE
    if constexpr (alloy::runtime::detail::has_usart1_interrupt_v<
                      alloy::device::interrupt_stubs::InterruptId>) {
        alloy::runtime::detail::signal_usart1_interrupt_if_present<
            alloy::device::interrupt_stubs::InterruptId>();
    }
#endif
}

extern "C" void XDMAC_IRQHandler() {
#if ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE
    if constexpr (alloy::runtime::detail::has_xdmac_interrupt_v<
                      alloy::device::interrupt_stubs::InterruptId>) {
        alloy::runtime::detail::signal_xdmac_interrupt_if_present<
            alloy::device::interrupt_stubs::InterruptId>();
    }
#endif
}
