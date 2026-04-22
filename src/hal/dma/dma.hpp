#pragma once

#include <cstdint>

#include "device/dma.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/dma/detail/backend.hpp"
#include "hal/dma/types.hpp"

namespace alloy::hal::dma {

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
template <PeripheralId Peripheral, SignalId Signal>
class channel_handle {
   public:
    using binding_traits = BindingTraits<Peripheral, Signal>;
    using semantic_traits = device::DmaSemanticTraits<Peripheral, Signal>;
    using controller_traits = ControllerTraits<binding_traits::kControllerId>;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr auto signal_id = Signal;
    static constexpr auto binding_id = binding_traits::kBindingId;
    static constexpr auto schema = detail::to_dma_schema(semantic_traits::kControllerSchemaId);
    static constexpr bool valid = binding_traits::kPresent && semantic_traits::kPresent &&
                                  controller_traits::kPresent &&
                                  schema != detail::DmaSchema::unknown;
    static constexpr auto controller_id = binding_traits::kControllerId;
    static constexpr auto request_line_id = binding_traits::kRequestLineId;
    static constexpr auto route_id = binding_traits::kRouteId;
    static constexpr auto conflict_group_id = binding_traits::kConflictGroupId;
    static constexpr auto controller_peripheral_id = semantic_traits::kControllerPeripheralId;
    static constexpr auto router_peripheral_id = semantic_traits::kRouterPeripheralId;
    static constexpr auto controller_base_address =
        controller_traits::kPresent
            ? device::PeripheralInstanceTraits<controller_peripheral_id>::kBaseAddress
            : std::uintptr_t{0u};
    static constexpr int fixed_channel_index = semantic_traits::kChannelIndex;
    static constexpr int request_value = semantic_traits::kRequestValue;
    static constexpr int channel_selector = semantic_traits::kChannelSelector;
    static constexpr auto route_selector_field = semantic_traits::kRouteSelectorField;

    constexpr explicit channel_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }
    [[nodiscard]] constexpr auto selected_channel_index() const -> int {
        return fixed_channel_index >= 0 ? fixed_channel_index : config_.channel_index;
    }

    [[nodiscard]] static consteval auto descriptor() {
        static_assert(valid, "Requested DMA binding is not published for the selected device.");
        return binding_traits::descriptor();
    }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return detail::configure_dma(*this);
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral, SignalId Signal>
[[nodiscard]] constexpr auto open(Config config = {}) -> channel_handle<Peripheral, Signal> {
    static_assert(channel_handle<Peripheral, Signal>::valid,
                  "Requested DMA binding is not published for the selected device.");
    return channel_handle<Peripheral, Signal>{config};
}
#endif

}  // namespace alloy::hal::dma
