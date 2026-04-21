#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::rtc {

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
using PeripheralId = device::runtime::PeripheralId;

struct Config {
    bool enable_write_access = false;
    bool enter_init_mode = false;
    bool enable_alarm_interrupt = false;
};

template <PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::runtime::RtcSemanticTraits<Peripheral>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.enable_write_access) {
            result = enable_write_access();
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.enter_init_mode) {
            result = enter_init_mode();
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.enable_alarm_interrupt) {
            result = enable_alarm_interrupt();
        }
        return result;
    }

    [[nodiscard]] auto enable_write_access() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kHasWriteProtection &&
                      semantic_traits::kWriteProtectKeyField.valid) {
            auto result = detail::runtime::modify_field(semantic_traits::kWriteProtectKeyField,
                                                        semantic_traits::kWriteProtectDisableKey0);
            if (!result.is_ok()) {
                return result;
            }
            return detail::runtime::modify_field(semantic_traits::kWriteProtectKeyField,
                                                 semantic_traits::kWriteProtectDisableKey1);
        } else {
            return core::Ok();
        }
    }

    [[nodiscard]] auto disable_write_access() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kHasWriteProtection &&
                      semantic_traits::kWriteProtectKeyField.valid) {
            return detail::runtime::modify_field(semantic_traits::kWriteProtectKeyField,
                                                 semantic_traits::kWriteProtectEnableKey);
        } else {
            return core::Ok();
        }
    }

    [[nodiscard]] auto enter_init_mode() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kInitField.valid) {
            return detail::runtime::modify_field(semantic_traits::kInitField, 1u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto leave_init_mode() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kInitField.valid) {
            return detail::runtime::modify_field(semantic_traits::kInitField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto init_ready() const -> bool {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kInitReadyField.valid) {
            const auto state = detail::runtime::read_field(semantic_traits::kInitReadyField);
            return state.is_ok() && state.unwrap() != 0u;
        } else {
            return false;
        }
    }

    [[nodiscard]] auto enable_alarm_interrupt() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kAlarmInterruptEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kAlarmInterruptEnableField, 1u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto alarm_pending() const -> bool {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kStatusAlarmField.valid) {
            const auto state = detail::runtime::read_field(semantic_traits::kStatusAlarmField);
            return state.is_ok() && state.unwrap() != 0u;
        } else {
            return false;
        }
    }

    [[nodiscard]] auto clear_alarm() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kClearAlarmField.valid) {
            return detail::runtime::modify_field(semantic_traits::kClearAlarmField, 1u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto read_time() const -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kTimeRegister.valid) {
            return detail::runtime::read_register(semantic_traits::kTimeRegister);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto read_date() const -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");

        if constexpr (semantic_traits::kDateRegister.valid) {
            return detail::runtime::read_register(semantic_traits::kDateRegister);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested RTC is not published for the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::rtc
