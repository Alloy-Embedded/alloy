#pragma once

#include <cstdint>

#include "device/dma.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/dma/types.hpp"

namespace alloy::hal::dma {

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
template <PeripheralId Peripheral, SignalId Signal>
class channel_handle {
   public:
    using binding_traits = BindingTraits<Peripheral, Signal>;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr auto signal_id = Signal;
    static constexpr bool valid = binding_traits::kPresent;
    static constexpr auto controller_id = binding_traits::kControllerId;
    static constexpr auto request_line_id = binding_traits::kRequestLineId;
    static constexpr auto route_id = binding_traits::kRouteId;
    static constexpr auto conflict_group_id = binding_traits::kConflictGroupId;

    constexpr explicit channel_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static consteval auto descriptor() {
        static_assert(valid, "Requested DMA binding is not published for the selected device.");
        return binding_traits::descriptor();
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral, SignalId Signal>
[[nodiscard]] constexpr auto open(Config config = {}) -> channel_handle<Peripheral, Signal> {
    static_assert(BindingTraits<Peripheral, Signal>::kPresent,
                  "Requested DMA binding is not published for the selected device.");
    return channel_handle<Peripheral, Signal>{config};
}
#endif

}  // namespace alloy::hal::dma
