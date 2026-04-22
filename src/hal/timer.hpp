#pragma once

#include <cstddef>
#include <cstdint>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::timer {

#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

struct Config {
    std::uint32_t period = 0u;
    bool apply_period = false;
    bool start_immediately = false;
};

template <PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::TimerSemanticTraits<Peripheral>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

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
        if (config_.start_immediately) {
            result = start();
        }
        return result;
    }

    [[nodiscard]] auto start() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");

        if constexpr (semantic_traits::kStartField.valid) {
            return detail::runtime::modify_field(semantic_traits::kStartField, 1u);
        } else if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 1u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto stop() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");

        if constexpr (semantic_traits::kStopField.valid) {
            return detail::runtime::modify_field(semantic_traits::kStopField, 1u);
        } else if constexpr (semantic_traits::kDisableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kDisableField, 1u);
        } else if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_period(std::uint32_t period) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");

        if constexpr (semantic_traits::kPeriodField.valid) {
            return detail::runtime::modify_field(semantic_traits::kPeriodField, period);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto get_count() const -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");

        if constexpr (semantic_traits::kCounterRegister.valid) {
            return detail::runtime::read_register(semantic_traits::kCounterRegister);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto is_running() const -> bool {
        static_assert(valid, "Requested timer is not published for the selected device.");

        if constexpr (semantic_traits::kEnableField.valid) {
            const auto state = detail::runtime::read_field(semantic_traits::kEnableField);
            return state.is_ok() && state.unwrap() != 0u;
        } else {
            return false;
        }
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested timer is not published for the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::timer
