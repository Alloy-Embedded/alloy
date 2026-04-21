#pragma once

#include <cstddef>
#include <cstdint>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::timer {

#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
using PeripheralId = device::runtime::PeripheralId;

template <PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::runtime::TimerSemanticTraits<Peripheral>;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

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
};

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open() -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested timer is not published for the selected device.");
    return {};
}
#endif

}  // namespace alloy::hal::timer
