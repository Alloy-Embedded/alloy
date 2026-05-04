#pragma once

#include <cstdint>
#include <span>

#include "core/types.hpp"
#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal {

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
namespace watchdog {

/// Typed interrupt kinds. Currently only EarlyWarning is published — the
/// canonical use is to capture crash state in a backup register before the
/// inevitable reset, NOT to "recover" (recovery defeats the watchdog's
/// safety guarantee).
enum class InterruptKind : std::uint8_t {
    EarlyWarning,
};

/// Kernel clock source selector. The available values are device-specific;
/// pass the raw integer from your device's RCC field encoding. Returns
/// NotSupported on backends that do not expose a kernel clock source field
/// (e.g. STM32 IWDG, SAME70 WDT — these always run from the internal LSI/RC).
enum class KernelClockSource : std::uint8_t {
    Default = 0u,
};

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

template <device::PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::WatchdogSemanticTraits<Peripheral>;

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

    // ------------------------------------------------------------------
    // extend-watchdog-coverage: window mode
    // ------------------------------------------------------------------

    /// Set the window value (raw cycles). Refreshes earlier than this value
    /// trigger a reset on STM32 WWDG; on SAME70 WDT the window is the lower
    /// bound of the legal refresh interval. Returns NotSupported when the
    /// peripheral has no window field.
    [[nodiscard]] auto set_window(std::uint16_t cycles) const
        -> core::Result<void, core::ErrorCode> {
        if constexpr (!semantic_traits::kHasWindow ||
                      !semantic_traits::kWindowField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(semantic_traits::kWindowField,
                                                 static_cast<std::uint32_t>(cycles));
        }
    }

    /// Toggle window-mode enforcement. `true` keeps the previously written
    /// window value active; `false` writes the maximum allowed window
    /// (effectively bypasses the early-refresh check). Returns NotSupported
    /// when the peripheral has no window field.
    [[nodiscard]] auto enable_window_mode(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        if constexpr (!semantic_traits::kHasWindow ||
                      !semantic_traits::kWindowField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            if (enable) {
                return core::Ok();
            }
            return detail::runtime::modify_field(semantic_traits::kWindowField,
                                                 max_field_value(semantic_traits::kWindowField));
        }
    }

    // ------------------------------------------------------------------
    // extend-watchdog-coverage: early-warning interrupt
    // ------------------------------------------------------------------

    /// Enable the early-warning interrupt. The `cycles_before_timeout`
    /// argument is informational on backends without a separate threshold
    /// register (STM32 WWDG / SAME70 WDT fire EWI at a hardware-fixed
    /// position) — set the timeout/window via `set_window` to control the
    /// effective trigger point. Returns NotSupported when the peripheral
    /// has no early-warning enable field.
    [[nodiscard]] auto enable_early_warning(std::uint16_t cycles_before_timeout) const
        -> core::Result<void, core::ErrorCode> {
        static_cast<void>(cycles_before_timeout);
        if constexpr (!semantic_traits::kEarlyWarningInterruptEnableField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(
                semantic_traits::kEarlyWarningInterruptEnableField, 1u);
        }
    }

    /// True when the early-warning flag is asserted. On STM32 WWDG this is
    /// SR.EWIF; on SAME70 WDT it is SR.WDUNF.
    [[nodiscard]] auto early_warning_pending() const -> bool {
        if constexpr (!semantic_traits::kStatusTimeoutField.valid) {
            return false;
        } else {
            const auto v = detail::runtime::read_field(semantic_traits::kStatusTimeoutField);
            return v.is_ok() && v.unwrap() != 0u;
        }
    }

    /// Clear the early-warning flag (write 0 — STM32 WWDG and SAME70 WDT
    /// both treat the bit as `rc_w0`).
    [[nodiscard]] auto clear_early_warning() const -> core::Result<void, core::ErrorCode> {
        if constexpr (!semantic_traits::kStatusTimeoutField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(semantic_traits::kStatusTimeoutField, 0u);
        }
    }

    // ------------------------------------------------------------------
    // extend-watchdog-coverage: status flags
    // ------------------------------------------------------------------

    /// True when the watchdog has timed out.
    [[nodiscard]] auto timeout_occurred() const -> bool {
        if constexpr (!semantic_traits::kStatusTimeoutField.valid) {
            return false;
        } else {
            const auto v = detail::runtime::read_field(semantic_traits::kStatusTimeoutField);
            return v.is_ok() && v.unwrap() != 0u;
        }
    }

    /// True while a prescaler update is propagating to the prescaled clock
    /// domain (STM32 IWDG SR.PVU).
    [[nodiscard]] auto prescaler_update_in_progress() const -> bool {
        if constexpr (!semantic_traits::kStatusPrescalerUpdateField.valid) {
            return false;
        } else {
            const auto v = detail::runtime::read_field(
                semantic_traits::kStatusPrescalerUpdateField);
            return v.is_ok() && v.unwrap() != 0u;
        }
    }

    /// True while a reload-register update is propagating (STM32 IWDG SR.RVU).
    [[nodiscard]] auto reload_update_in_progress() const -> bool {
        if constexpr (!semantic_traits::kStatusReloadUpdateField.valid) {
            return false;
        } else {
            const auto v =
                detail::runtime::read_field(semantic_traits::kStatusReloadUpdateField);
            return v.is_ok() && v.unwrap() != 0u;
        }
    }

    /// True while a window-register update is propagating.
    [[nodiscard]] auto window_update_in_progress() const -> bool {
        if constexpr (!semantic_traits::kStatusWindowUpdateField.valid) {
            return false;
        } else {
            const auto v =
                detail::runtime::read_field(semantic_traits::kStatusWindowUpdateField);
            return v.is_ok() && v.unwrap() != 0u;
        }
    }

    /// True when the peripheral reports an error condition (SAME70 WDT
    /// SR.WDERR — early refresh from the wrong key, etc.).
    [[nodiscard]] auto error() const -> bool {
        if constexpr (!semantic_traits::kStatusErrorField.valid) {
            return false;
        } else {
            const auto v = detail::runtime::read_field(semantic_traits::kStatusErrorField);
            return v.is_ok() && v.unwrap() != 0u;
        }
    }

    // ------------------------------------------------------------------
    // extend-watchdog-coverage: kernel clock source
    // ------------------------------------------------------------------

    /// Select the kernel clock source feeding the watchdog prescaler.
    /// Returns NotSupported on all current backends (IWDG/WWDG/WDT) because
    /// no `kKernelClockSourceField` is published in the device database.
    /// The API exists so callers can be written portably; a future device
    /// descriptor can enable it by adding the field reference.
    [[nodiscard]] auto set_kernel_clock_source(KernelClockSource) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ------------------------------------------------------------------
    // extend-watchdog-coverage: reset-on-timeout
    // ------------------------------------------------------------------

    /// Toggle whether a watchdog timeout drives the system reset. When
    /// `false`, the watchdog acts as a free-running timer that only fires
    /// the early-warning interrupt. Returns NotSupported on backends
    /// without a separate reset-enable bit (e.g. STM32 IWDG, where reset
    /// is implicit once the device is started).
    [[nodiscard]] auto set_reset_on_timeout(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        if constexpr (!semantic_traits::kResetEnableField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(semantic_traits::kResetEnableField,
                                                 enable ? 1u : 0u);
        }
    }

    // ------------------------------------------------------------------
    // extend-watchdog-coverage: typed interrupts + IRQ vector lookup
    // ------------------------------------------------------------------

    /// Enable a typed interrupt. Currently only `InterruptKind::EarlyWarning`
    /// is published — equivalent to `enable_early_warning(0)`.
    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        switch (kind) {
            case InterruptKind::EarlyWarning:
                if constexpr (!semantic_traits::kEarlyWarningInterruptEnableField.valid) {
                    return core::Err(core::ErrorCode::NotSupported);
                } else {
                    return detail::runtime::modify_field(
                        semantic_traits::kEarlyWarningInterruptEnableField, 1u);
                }
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Disable a typed interrupt. Note: STM32 WWDG's EWI bit is `rs` (set-
    /// only via software) — disabling requires a peripheral reset. The HAL
    /// writes 0 best-effort and returns whatever `modify_field` returns.
    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        switch (kind) {
            case InterruptKind::EarlyWarning:
                if constexpr (!semantic_traits::kEarlyWarningInterruptEnableField.valid) {
                    return core::Err(core::ErrorCode::NotSupported);
                } else {
                    return detail::runtime::modify_field(
                        semantic_traits::kEarlyWarningInterruptEnableField, 0u);
                }
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns the NVIC IRQ line numbers for this peripheral. The span is
    /// empty when the descriptor publishes no IRQ for this watchdog.
    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
    }
};

/// Open a watchdog handle for \p Peripheral and apply \p config.  The handle
/// is lightweight (zero stored state); configure() can be called again later
/// to reapply any of the Config actions.
///
/// Errors from config application are silently discarded here.  Callers that
/// need the Result should call handle::configure() directly after open().
template <device::PeripheralId Peripheral>
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

// alloy.device.v2.1 concept-based watchdog — no descriptor-runtime required.
// StIwdg (kTemplate="iwdg"): all STM32 IWDG families (v1_1 and v2_0).
// StWwdg (kTemplate="wwdg"): all STM32 WWDG families.
#include "hal/watchdog/lite.hpp"
