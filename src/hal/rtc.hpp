#pragma once

#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::rtc {

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------

enum class HourMode : std::uint8_t {
    Hour24,
    Hour12AmPm,
};

/// Typed interrupt kinds. Presence of each kind varies by backend; unsupported
/// kinds return NotSupported from enable/disable_interrupt.
enum class InterruptKind : std::uint8_t {
    Alarm,          ///< Alarm A (STM32) / ALARM (SAME70)
    AlarmB,         ///< Alarm B (STM32 only — no kAlarmBInterruptEnableField yet)
    Wakeup,         ///< Periodic wakeup timer (STM32 RTC WUT — not yet in DB)
    CalendarEvent,  ///< Calendar event / minute event (SAME70 CALEN)
    Tamper,         ///< Tamper detection (not yet in DB)
    Second,         ///< Per-second event (SAME70 SECEN)
    TimeEvent,      ///< Time event (STM32 TSIE / SAME70 TIMEN)
};

// ---------------------------------------------------------------------------
// Value types
// ---------------------------------------------------------------------------

/// BCD-decoded time value. Hours are 0–23 (24h) or 1–12 with pm flag (12h).
struct Time {
    std::uint8_t hours   = 0u;
    std::uint8_t minutes = 0u;
    std::uint8_t seconds = 0u;
    bool         pm      = false;  ///< AM/PM flag — only meaningful in 12h mode.
};

/// Date value. year is 0–99 (years since 2000). weekday: 1–7, where 1 = Monday
/// following the STM32 RTC convention; SAME70 stores the same range.
struct Date {
    std::uint8_t year    = 0u;
    std::uint8_t month   = 1u;
    std::uint8_t day     = 1u;
    std::uint8_t weekday = 1u;
};

/// Alarm configuration. match_* flags select which fields to compare.
/// On STM32: MSKn=0 means match; on SAME70: ENn=1 means match — the HAL
/// abstracts this difference.
struct AlarmConfig {
    std::uint8_t hours   = 0u;
    std::uint8_t minutes = 0u;
    std::uint8_t seconds = 0u;
    std::uint8_t day     = 0u;      ///< Day of month or weekday number.
    bool match_seconds   = true;
    bool match_minutes   = true;
    bool match_hours     = true;
    bool match_day       = false;
    bool pm              = false;   ///< AM/PM flag for 12h mode.
    bool use_weekday     = false;   ///< day field is weekday (1–7), not date.
};

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

struct Config {
    bool enable_write_access = false;
    bool enter_init_mode = false;
    bool enable_alarm_interrupt = false;
};

// ---------------------------------------------------------------------------
// handle<Peripheral>
// ---------------------------------------------------------------------------

template <PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::RtcSemanticTraits<Peripheral>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // -----------------------------------------------------------------------
    // configure
    // -----------------------------------------------------------------------

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        core::Result<void, core::ErrorCode> result =
            detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
        if (result.is_err() && result.err() != core::ErrorCode::NotSupported) {
            return result;
        }
        if (config_.enable_write_access) {
            result = enable_write_access();
            if (!result.is_ok()) { return result; }
        }
        if (config_.enter_init_mode) {
            result = enter_init_mode();
            if (!result.is_ok()) { return result; }
        }
        if (config_.enable_alarm_interrupt) {
            result = enable_alarm_interrupt();
        }
        return result;
    }

    // -----------------------------------------------------------------------
    // write-protect gate
    // -----------------------------------------------------------------------

    [[nodiscard]] auto enable_write_access() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (semantic_traits::kHasWriteProtection &&
                      semantic_traits::kWriteProtectKeyField.valid) {
            auto result = detail::runtime::modify_field(semantic_traits::kWriteProtectKeyField,
                                                        semantic_traits::kWriteProtectDisableKey0);
            if (!result.is_ok()) { return result; }
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

    // -----------------------------------------------------------------------
    // init-mode
    // -----------------------------------------------------------------------

    [[nodiscard]] auto enter_init_mode() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (semantic_traits::kInitField.valid) {
            return detail::runtime::modify_field(semantic_traits::kInitField, 1u);
        } else if constexpr (semantic_traits::kUpdateTimeField.valid &&
                             semantic_traits::kUpdateCalendarField.valid) {
            auto result = detail::runtime::modify_field(semantic_traits::kUpdateTimeField, 1u);
            if (!result.is_ok()) { return result; }
            result = detail::runtime::modify_field(semantic_traits::kUpdateCalendarField, 1u);
            if (!result.is_ok()) { return result; }
            if constexpr (semantic_traits::kUpdateAckField.valid) {
                constexpr auto kPollLimit = 1'000'000u;
                for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
                    const auto value =
                        detail::runtime::read_field(semantic_traits::kUpdateAckField);
                    if (value.is_err()) {
                        return core::Err(core::ErrorCode{value.unwrap_err()});
                    }
                    if (value.unwrap() != 0u) { return core::Ok(); }
                }
                return core::Err(core::ErrorCode::Timeout);
            }
            return core::Ok();
        } else {
            return core::Ok();
        }
    }

    [[nodiscard]] auto leave_init_mode() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (semantic_traits::kInitField.valid) {
            return detail::runtime::modify_field(semantic_traits::kInitField, 0u);
        } else if constexpr (semantic_traits::kUpdateAckField.valid &&
                             semantic_traits::kClearTimeEventField.valid &&
                             semantic_traits::kClearCalendarEventField.valid) {
            auto result =
                detail::runtime::modify_field(semantic_traits::kClearTimeEventField, 1u);
            if (!result.is_ok()) { return result; }
            return detail::runtime::modify_field(semantic_traits::kClearCalendarEventField, 1u);
        } else {
            return core::Ok();
        }
    }

    [[nodiscard]] auto init_ready() const -> bool {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (semantic_traits::kInitReadyField.valid) {
            const auto state =
                detail::runtime::read_field(semantic_traits::kInitReadyField);
            return state.is_ok() && state.unwrap() != 0u;
        } else {
            return false;
        }
    }

    // -----------------------------------------------------------------------
    // extend-rtc-coverage: hour mode + prescalers
    // -----------------------------------------------------------------------

    /// Set hour mode (24h or 12h AM/PM). Gated on kHourModeField.valid.
    [[nodiscard]] auto set_hour_mode(HourMode mode) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kHourModeField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(semantic_traits::kHourModeField,
                                                 mode == HourMode::Hour12AmPm ? 1u : 0u);
        }
    }

    /// Set the synchronous prescaler (PREDIV_S, bits 0–14 of RTC_PRER on STM32).
    /// Returns NotSupported when kPrescalerRegister is not valid.
    [[nodiscard]] auto set_sync_prescaler(std::uint16_t prediv_s) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kPrescalerRegister.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            const auto cur = detail::runtime::read_register(semantic_traits::kPrescalerRegister);
            if (cur.is_err()) { return core::Err(core::ErrorCode{cur.unwrap_err()}); }
            const std::uint32_t newval =
                (cur.unwrap() & ~std::uint32_t{0x7FFFu}) | (prediv_s & 0x7FFFu);
            return detail::runtime::write_register(semantic_traits::kPrescalerRegister, newval);
        }
    }

    /// Set the asynchronous prescaler (PREDIV_A, bits 16–22 of RTC_PRER on STM32).
    /// Returns NotSupported when kPrescalerRegister is not valid.
    [[nodiscard]] auto set_async_prescaler(std::uint8_t prediv_a) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kPrescalerRegister.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            const auto cur = detail::runtime::read_register(semantic_traits::kPrescalerRegister);
            if (cur.is_err()) { return core::Err(core::ErrorCode{cur.unwrap_err()}); }
            const std::uint32_t newval =
                (cur.unwrap() & ~std::uint32_t{0x007F'0000u}) |
                ((static_cast<std::uint32_t>(prediv_a) & 0x7Fu) << 16u);
            return detail::runtime::write_register(semantic_traits::kPrescalerRegister, newval);
        }
    }

    // -----------------------------------------------------------------------
    // extend-rtc-coverage: time + date setters / getters
    // -----------------------------------------------------------------------

    /// Write time. Calls enter_init_mode / leave_init_mode internally.
    /// Returns NotSupported when kTimeRegister is not valid.
    [[nodiscard]] auto set_time(Time t) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kTimeRegister.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            // Both STM32 and SAME70 share the same BCD time bit layout:
            //   bits  0-3: seconds units    bits  4-6: seconds tens
            //   bits  8-11: minutes units   bits 12-14: minutes tens
            //   bits 16-19: hours units     bits 20-21: hours tens
            //   bit  22: AM/PM
            const std::uint32_t packed =
                to_bcd_(t.seconds) |
                (to_bcd_(t.minutes) << 8u) |
                (to_bcd_(t.hours)   << 16u) |
                (t.pm ? (1u << 22u) : 0u);
            auto r = enter_init_mode();
            if (!r.is_ok()) { return r; }
            r = detail::runtime::write_register(semantic_traits::kTimeRegister, packed);
            if (!r.is_ok()) { return r; }
            return leave_init_mode();
        }
    }

    /// Read and decode the current time. Returns a zero-initialised Time on error.
    [[nodiscard]] auto current_time() const -> Time {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kTimeRegister.valid) {
            return Time{};
        } else {
            const auto raw = detail::runtime::read_register(semantic_traits::kTimeRegister);
            if (raw.is_err()) { return Time{}; }
            const auto v = raw.unwrap();
            return Time{
                .hours   = from_bcd_((v >> 16u) & 0x3Fu),
                .minutes = from_bcd_((v >>  8u) & 0x7Fu),
                .seconds = from_bcd_(v & 0x7Fu),
                .pm      = ((v >> 22u) & 0x1u) != 0u,
            };
        }
    }

    /// Write date. Calls enter_init_mode / leave_init_mode internally.
    /// Returns NotSupported when kDateRegister is not valid.
    [[nodiscard]] auto set_date(Date d) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kDateRegister.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            const std::uint32_t packed = pack_date_(d);
            auto r = enter_init_mode();
            if (!r.is_ok()) { return r; }
            r = detail::runtime::write_register(semantic_traits::kDateRegister, packed);
            if (!r.is_ok()) { return r; }
            return leave_init_mode();
        }
    }

    /// Read and decode the current date. Returns a default-initialised Date on error.
    [[nodiscard]] auto current_date() const -> Date {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kDateRegister.valid) {
            return Date{};
        } else {
            const auto raw = detail::runtime::read_register(semantic_traits::kDateRegister);
            if (raw.is_err()) { return Date{}; }
            return unpack_date_(raw.unwrap());
        }
    }

    // -----------------------------------------------------------------------
    // extend-rtc-coverage: alarm
    // -----------------------------------------------------------------------

    /// Configure and arm alarm A (SAME70) / alarm A (STM32).
    /// Also enables the alarm via kAlarmEnableField when valid.
    [[nodiscard]] auto set_alarm(AlarmConfig cfg) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kHasAlarm ||
                      !semantic_traits::kAlarmTimeRegister.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            auto r = write_alarm_(cfg);
            if (!r.is_ok()) { return r; }
            if constexpr (semantic_traits::kAlarmEnableField.valid) {
                r = detail::runtime::modify_field(semantic_traits::kAlarmEnableField, 1u);
            }
            return r;
        }
    }

    // -----------------------------------------------------------------------
    // Backwards-compat convenience wrapper (original API)
    // -----------------------------------------------------------------------

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

    // -----------------------------------------------------------------------
    // extend-rtc-coverage: calendar event
    // -----------------------------------------------------------------------

    /// Enable/disable the calendar event interrupt (SAME70 CALEN).
    [[nodiscard]] auto enable_calendar_event(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kCalendarEventInterruptEnableField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return interrupt_arm_field_(
                semantic_traits::kCalendarEventInterruptEnableField, enable);
        }
    }

    /// True when a calendar event is pending (SAME70 SR.CALEV).
    [[nodiscard]] auto calendar_event_pending() const -> bool {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kStatusCalendarEventField.valid) {
            return false;
        } else {
            const auto v =
                detail::runtime::read_field(semantic_traits::kStatusCalendarEventField);
            return v.is_ok() && v.unwrap() != 0u;
        }
    }

    /// Clear the calendar-event status flag (SAME70 SCCR.CALCLR).
    [[nodiscard]] auto clear_calendar_event() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kClearCalendarEventField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(
                semantic_traits::kClearCalendarEventField, 1u);
        }
    }

    // -----------------------------------------------------------------------
    // extend-rtc-coverage: tamper / second / time-event clear
    // -----------------------------------------------------------------------

    [[nodiscard]] auto clear_tamper_error() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kClearTamperErrorField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(semantic_traits::kClearTamperErrorField, 1u);
        }
    }

    [[nodiscard]] auto clear_second() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kClearSecondField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(semantic_traits::kClearSecondField, 1u);
        }
    }

    [[nodiscard]] auto clear_time_event() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        if constexpr (!semantic_traits::kClearTimeEventField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(semantic_traits::kClearTimeEventField, 1u);
        }
    }

    // -----------------------------------------------------------------------
    // extend-rtc-coverage: raw register read/write (backwards compat)
    // -----------------------------------------------------------------------

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

    // -----------------------------------------------------------------------
    // extend-rtc-coverage: typed interrupts
    // -----------------------------------------------------------------------

    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        return interrupt_arm_(kind, true);
    }

    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        return interrupt_arm_(kind, false);
    }

    // -----------------------------------------------------------------------
    // extend-rtc-coverage: backup registers
    // -----------------------------------------------------------------------

    /// Read a backup register by index. Returns NotSupported — no
    /// kBackupRegisterBase is published in any current device database.
    [[nodiscard]] auto read_backup(std::uint8_t /*index*/) const
        -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Write a backup register by index. Returns NotSupported — no
    /// kBackupRegisterBase is published in any current device database.
    [[nodiscard]] auto write_backup(std::uint8_t /*index*/, std::uint32_t /*value*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested RTC is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    // -----------------------------------------------------------------------
    // extend-rtc-coverage: IRQ vector
    // -----------------------------------------------------------------------

    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
    }

   private:
    Config config_{};

    // ------------------------------------------------------------------
    // BCD helpers
    // ------------------------------------------------------------------

    [[nodiscard]] static constexpr auto to_bcd_(std::uint8_t v) -> std::uint32_t {
        return ((static_cast<std::uint32_t>(v) / 10u) << 4u) |
               (static_cast<std::uint32_t>(v) % 10u);
    }

    [[nodiscard]] static constexpr auto from_bcd_(std::uint32_t v) -> std::uint8_t {
        return static_cast<std::uint8_t>(((v >> 4u) & 0xFu) * 10u + (v & 0xFu));
    }

    // ------------------------------------------------------------------
    // Date pack / unpack — schema-specific
    //
    // Detect SAME70 by kUpdateAckField.valid (only SAME70 RTC has it).
    //
    // STM32 RTC_DR layout:
    //   bits  0-3: DU  (day units BCD)
    //   bits  4-5: DT  (day tens BCD)
    //   bits  8-11: MU (month units BCD)
    //   bit  12: MT    (month tens BCD)
    //   bits 13-15: WDU (weekday, 1=Mon...7=Sun)
    //   bits 16-19: YU (year units BCD)
    //   bits 20-23: YT (year tens BCD)
    //
    // SAME70 CALR layout:
    //   bits  0-3: CENT_U  bits  4-6: CENT_T  (century, fixed 20)
    //   bits  8-11: YEAR_U bits 12-15: YEAR_T
    //   bits 16-19: MONTH_U bit 20: MONTH_T
    //   bits 21-23: DAY (weekday)
    //   bits 24-27: DATE_U bit 28: DATE_T
    // ------------------------------------------------------------------

    [[nodiscard]] auto pack_date_(Date d) const -> std::uint32_t {
        if constexpr (semantic_traits::kUpdateAckField.valid) {
            // SAME70
            return 0x00000020u                                            // century = 20
                   | (to_bcd_(d.year) << 8u)
                   | (to_bcd_(d.month) << 16u)
                   | ((static_cast<std::uint32_t>(d.weekday) & 0x7u) << 21u)
                   | (to_bcd_(d.day) << 24u);
        } else {
            // STM32
            return to_bcd_(d.day)
                   | (to_bcd_(d.month) << 8u)
                   | ((static_cast<std::uint32_t>(d.weekday) & 0x7u) << 13u)
                   | (to_bcd_(d.year) << 16u);
        }
    }

    [[nodiscard]] static constexpr auto unpack_date_(std::uint32_t v) -> Date {
        if constexpr (semantic_traits::kUpdateAckField.valid) {
            // SAME70
            return Date{
                .year    = from_bcd_((v >>  8u) & 0xFFu),
                .month   = from_bcd_((v >> 16u) & 0x1Fu),
                .day     = from_bcd_((v >> 24u) & 0x3Fu),
                .weekday = static_cast<std::uint8_t>((v >> 21u) & 0x7u),
            };
        } else {
            // STM32
            return Date{
                .year    = from_bcd_((v >> 16u) & 0xFFu),
                .month   = from_bcd_((v >>  8u) & 0x1Fu),
                .day     = from_bcd_(v & 0x3Fu),
                .weekday = static_cast<std::uint8_t>((v >> 13u) & 0x7u),
            };
        }
    }

    // ------------------------------------------------------------------
    // Alarm pack — schema-specific
    //
    // SAME70 uses separate TIMALR (time) + CALALR (date) registers and
    // EN bits = 1 means "match".
    // STM32 packs everything into ALRMAR and MSK bits = 1 means "don't match".
    // ------------------------------------------------------------------

    [[nodiscard]] auto write_alarm_(AlarmConfig cfg) const
        -> core::Result<void, core::ErrorCode> {
        if constexpr (semantic_traits::kUpdateAckField.valid) {
            // SAME70: TIMALR then CALALR
            const std::uint32_t time_pack =
                (to_bcd_(cfg.seconds) & 0x7Fu) | (cfg.match_seconds ? (1u << 7u) : 0u) |
                ((to_bcd_(cfg.minutes) & 0x7Fu) << 8u) | (cfg.match_minutes ? (1u << 15u) : 0u) |
                ((to_bcd_(cfg.hours) & 0x3Fu) << 16u) | (cfg.pm ? (1u << 22u) : 0u) |
                (cfg.match_hours ? (1u << 23u) : 0u);
            auto r = detail::runtime::write_register(semantic_traits::kAlarmTimeRegister,
                                                     time_pack);
            if (!r.is_ok()) { return r; }
            if constexpr (semantic_traits::kAlarmDateRegister.valid) {
                const std::uint32_t date_pack =
                    cfg.match_day
                        ? ((to_bcd_(cfg.day) << 24u) | (1u << 29u))
                        : 0u;
                r = detail::runtime::write_register(semantic_traits::kAlarmDateRegister,
                                                    date_pack);
            }
            return r;
        } else {
            // STM32: full alarm in ALRMAR (MSKn=1 → don't match)
            const std::uint32_t pack =
                (to_bcd_(cfg.seconds) & 0x7Fu) | (cfg.match_seconds ? 0u : (1u << 7u)) |
                ((to_bcd_(cfg.minutes) & 0x7Fu) << 8u) | (cfg.match_minutes ? 0u : (1u << 15u)) |
                ((to_bcd_(cfg.hours) & 0x3Fu) << 16u) | (cfg.pm ? (1u << 22u) : 0u) |
                (cfg.match_hours ? 0u : (1u << 23u)) |
                ((to_bcd_(cfg.day) & 0x3Fu) << 24u) |
                (cfg.use_weekday ? (1u << 30u) : 0u) |
                (cfg.match_day ? 0u : (1u << 31u));
            return detail::runtime::write_register(semantic_traits::kAlarmTimeRegister, pack);
        }
    }

    // ------------------------------------------------------------------
    // interrupt arm/disarm
    //
    // SAME70 uses separate IER / IDR registers: enable writes 1 to IER,
    // disable writes 1 to IDR at the same bit position.
    // STM32 uses a single IE bit in the control register.
    // ------------------------------------------------------------------

    [[nodiscard]] auto interrupt_arm_field_(
        const detail::runtime::FieldRef& field, bool enable) const
        -> core::Result<void, core::ErrorCode> {
        if (enable) {
            return detail::runtime::modify_field(field, 1u);
        }
        if constexpr (semantic_traits::kInterruptDisableRegister.valid) {
            // SAME70: write 1 to IDR at the same bit position as IER
            return detail::runtime::write_register(
                semantic_traits::kInterruptDisableRegister,
                1u << field.bit_offset);
        } else {
            return detail::runtime::modify_field(field, 0u);
        }
    }

    [[nodiscard]] auto interrupt_arm_(InterruptKind kind, bool enable) const
        -> core::Result<void, core::ErrorCode> {
        switch (kind) {
            case InterruptKind::Alarm:
                if constexpr (semantic_traits::kAlarmInterruptEnableField.valid) {
                    return interrupt_arm_field_(
                        semantic_traits::kAlarmInterruptEnableField, enable);
                } else {
                    return core::Err(core::ErrorCode::NotSupported);
                }
            case InterruptKind::Second:
                if constexpr (semantic_traits::kSecondInterruptEnableField.valid) {
                    return interrupt_arm_field_(
                        semantic_traits::kSecondInterruptEnableField, enable);
                } else {
                    return core::Err(core::ErrorCode::NotSupported);
                }
            case InterruptKind::TimeEvent:
                if constexpr (semantic_traits::kTimeEventInterruptEnableField.valid) {
                    return interrupt_arm_field_(
                        semantic_traits::kTimeEventInterruptEnableField, enable);
                } else {
                    return core::Err(core::ErrorCode::NotSupported);
                }
            case InterruptKind::CalendarEvent:
                if constexpr (semantic_traits::kCalendarEventInterruptEnableField.valid) {
                    return interrupt_arm_field_(
                        semantic_traits::kCalendarEventInterruptEnableField, enable);
                } else {
                    return core::Err(core::ErrorCode::NotSupported);
                }
            case InterruptKind::AlarmB:
            case InterruptKind::Wakeup:
            case InterruptKind::Tamper:
                break;
        }
        return core::Err(core::ErrorCode::NotSupported);
    }
};

// ---------------------------------------------------------------------------
// open<Peripheral>
// ---------------------------------------------------------------------------

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested RTC is not published for the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::rtc

// ---------------------------------------------------------------------------
// Lite driver (no descriptor-runtime dependency)
// ---------------------------------------------------------------------------
#include "hal/rtc/lite.hpp"
