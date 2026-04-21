#pragma once

#include <cstdint>

#include "core/types.hpp"
#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal {

using namespace alloy::core;

class Watchdog {
   public:
    template <typename WatchdogPolicy>
    static void disable() {
        WatchdogPolicy::disable();
    }

    template <typename WatchdogPolicy>
    static void enable() {
        WatchdogPolicy::enable();
    }

    template <typename WatchdogPolicy>
    static void enable_with_timeout(u16 timeout_ms) {
        WatchdogPolicy::enable_with_timeout(timeout_ms);
    }

    template <typename WatchdogPolicy>
    static void feed() {
        WatchdogPolicy::feed();
    }

    template <typename WatchdogPolicy>
    [[nodiscard]] static auto is_enabled() -> bool {
        return WatchdogPolicy::is_enabled();
    }

    template <typename WatchdogPolicy>
    [[nodiscard]] static auto get_remaining_time_ms() -> u16 {
        return WatchdogPolicy::get_remaining_time_ms();
    }
};

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
namespace watchdog {

[[nodiscard]] consteval auto max_field_value(const detail::runtime::FieldRef& field)
    -> std::uint32_t {
    return field.bit_width >= 32u ? 0xFFFF'FFFFu : ((std::uint32_t{1u} << field.bit_width) - 1u);
}

template <device::runtime::PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::runtime::WatchdogSemanticTraits<Peripheral>;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    [[nodiscard]] static auto disable() -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested watchdog is not published for the selected device.");

        if constexpr (!semantic_traits::kConfigRegister.valid ||
                      !semantic_traits::kDisableField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            auto value =
                detail::runtime::field_bits(semantic_traits::kDisableField, 1u).unwrap();
            if constexpr (semantic_traits::kRequiredConfigField.valid) {
                value |= detail::runtime::field_bits(semantic_traits::kRequiredConfigField,
                                                     semantic_traits::kRequiredConfigValue)
                             .unwrap();
            } else if constexpr (semantic_traits::kWindowField.valid) {
                value |= detail::runtime::field_bits(semantic_traits::kWindowField,
                                                     max_field_value(semantic_traits::kWindowField))
                             .unwrap();
            }
            return detail::runtime::write_register(semantic_traits::kConfigRegister, value);
        }
    }

    [[nodiscard]] static auto enable() -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested watchdog is not published for the selected device.");

        if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 1u);
        } else if constexpr (semantic_traits::kKeyField.valid &&
                             semantic_traits::kStartKeyValue != 0u) {
            return detail::runtime::modify_field(semantic_traits::kKeyField,
                                                 semantic_traits::kStartKeyValue);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] static auto refresh() -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested watchdog is not published for the selected device.");

        if constexpr (semantic_traits::kRestartField.valid) {
            return detail::runtime::modify_field(semantic_traits::kRestartField, 1u);
        } else if constexpr (semantic_traits::kKeyField.valid &&
                             semantic_traits::kRefreshKeyValue != 0u) {
            return detail::runtime::modify_field(semantic_traits::kKeyField,
                                                 semantic_traits::kRefreshKeyValue);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }
};

template <device::runtime::PeripheralId Peripheral>
[[nodiscard]] constexpr auto open() -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested watchdog is not published for the selected device.");
    return {};
}

}  // namespace watchdog
#endif

}  // namespace alloy::hal
