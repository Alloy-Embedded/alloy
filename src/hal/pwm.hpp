#pragma once

#include <cstddef>
#include <cstdint>

#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::pwm {

#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

struct Config {
    std::uint32_t period = 0u;
    bool apply_period = false;
    std::uint32_t duty_cycle = 0u;
    bool apply_duty_cycle = false;
    bool start_immediately = false;
};

template <PeripheralId Peripheral, std::size_t Channel>
class handle {
   public:
    using peripheral_traits = device::PwmSemanticTraits<Peripheral>;
    using channel_traits = device::PwmChannelSemanticTraits<Peripheral, Channel>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr auto channel_index = Channel;
    static constexpr bool valid = peripheral_traits::kPresent && channel_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.apply_period) {
            result = set_period(config_.period);
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.apply_duty_cycle) {
            result = set_duty_cycle(config_.duty_cycle);
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.start_immediately) {
            result = start();
        }
        return result;
    }

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

    template <typename DmaChannel>
    [[nodiscard]] auto configure_dma(const DmaChannel& channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_TX);
        return channel.configure();
    }

    [[nodiscard]] static constexpr auto data_register_address() -> std::uintptr_t {
        if constexpr (channel_traits::kDutyField.valid) {
            return channel_traits::kDutyField.reg.base_address +
                   channel_traits::kDutyField.reg.offset_bytes;
        } else if constexpr (channel_traits::kPeriodField.valid) {
            return channel_traits::kPeriodField.reg.base_address +
                   channel_traits::kPeriodField.reg.offset_bytes;
        } else {
            return 0u;
        }
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral, std::size_t Channel>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral, Channel> {
    static_assert(handle<Peripheral, Channel>::valid,
                  "Requested PWM channel is not published for the selected device.");
    return handle<Peripheral, Channel>{config};
}
#endif

}  // namespace alloy::hal::pwm
