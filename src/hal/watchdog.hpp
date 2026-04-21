#pragma once

#include <cstdint>

#include "core/types.hpp"
#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal {

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
namespace watchdog {

[[nodiscard]] consteval auto max_field_value(const detail::runtime::FieldRef& field)
    -> std::uint32_t {
    return field.bit_width >= 32u ? 0xFFFF'FFFFu : ((std::uint32_t{1u} << field.bit_width) - 1u);
}

/// Configuration applied when opening or reconfiguring a watchdog handle.
/// Defaults are no-ops — the watchdog state is unchanged unless explicitly
/// requested.
struct Config {
    /// If true, configure() calls disable().  No-op on hardware that does not
    /// support software disable (e.g. STM32 IWDG); the error is surfaced to
    /// the caller only when configure() is called explicitly.
    bool disable_on_configure = false;
    /// If true, configure() calls refresh() after any disable step.
    bool refresh_on_configure = false;
};

template <device::runtime::PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::runtime::WatchdogSemanticTraits<Peripheral>;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    [[nodiscard]] auto disable() const -> core::Result<void, core::ErrorCode> {
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

    [[nodiscard]] auto enable() const -> core::Result<void, core::ErrorCode> {
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

    [[nodiscard]] auto refresh() const -> core::Result<void, core::ErrorCode> {
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

    /// Apply \p cfg to this handle.  Returns the first error encountered;
    /// subsequent steps are skipped on failure.  Calling with a default-
    /// constructed Config is a no-op.
    [[nodiscard]] auto configure(Config cfg = {}) const -> core::Result<void, core::ErrorCode> {
        if (cfg.disable_on_configure) {
            if (auto r = disable(); !r.is_ok()) {
                return r;
            }
        }
        if (cfg.refresh_on_configure) {
            if (auto r = refresh(); !r.is_ok()) {
                return r;
            }
        }
        return core::Ok();
    }
};

/// Open a watchdog handle for \p Peripheral and apply \p config.  The handle
/// is lightweight (zero stored state); configure() can be called again later
/// to reapply any of the Config actions.
///
/// Errors from config application are silently discarded here.  Callers that
/// need the Result should call handle::configure() directly after open().
template <device::runtime::PeripheralId Peripheral>
[[nodiscard]] auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested watchdog is not published for the selected device.");
    handle<Peripheral> h{};
    [[maybe_unused]] const auto r = h.configure(config);
    return h;
}

}  // namespace watchdog
#endif

}  // namespace alloy::hal
