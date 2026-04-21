#pragma once

#include <cstddef>
#include <cstdint>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::pwm {

#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
using PeripheralId = device::runtime::PeripheralId;

template <PeripheralId Peripheral, std::size_t Channel>
class handle {
   public:
    using peripheral_traits = device::runtime::PwmSemanticTraits<Peripheral>;
    using channel_traits = device::runtime::PwmChannelSemanticTraits<Peripheral, Channel>;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr auto channel_index = Channel;
    static constexpr bool valid = peripheral_traits::kPresent && channel_traits::kPresent;

    [[nodiscard]] auto start() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");

        if constexpr (channel_traits::kEnableField.valid) {
            auto result = detail::runtime::modify_field(channel_traits::kEnableField, 1u);
            if (!result.is_ok()) {
                return result;
            }
            if constexpr (peripheral_traits::kMasterOutputEnableField.valid) {
                result =
                    detail::runtime::modify_field(peripheral_traits::kMasterOutputEnableField, 1u);
                if (!result.is_ok()) {
                    return result;
                }
            }
            if constexpr (peripheral_traits::kLoadField.valid) {
                result = detail::runtime::modify_field(peripheral_traits::kLoadField, 1u);
            }
            return result;
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto stop() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");

        if constexpr (channel_traits::kEnableField.valid) {
            return detail::runtime::modify_field(channel_traits::kEnableField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_frequency(std::uint32_t) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto set_period(std::uint32_t period) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");

        if constexpr (channel_traits::kPeriodField.valid) {
            return detail::runtime::modify_field(channel_traits::kPeriodField, period);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_duty_cycle(std::uint32_t duty) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");

        if constexpr (channel_traits::kDutyField.valid) {
            return detail::runtime::modify_field(channel_traits::kDutyField, duty);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }
};

template <PeripheralId Peripheral, std::size_t Channel>
[[nodiscard]] constexpr auto open() -> handle<Peripheral, Channel> {
    static_assert(handle<Peripheral, Channel>::valid,
                  "Requested PWM channel is not published for the selected device.");
    return {};
}
#endif

}  // namespace alloy::hal::pwm
