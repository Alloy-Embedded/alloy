#include <span>
#include <type_traits>

#include "hal/rtc.hpp"

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
#include "runtime/async_rtc.hpp"
#endif

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE && !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
using PeripheralId = alloy::hal::rtc::PeripheralId;
using Rtc = alloy::hal::rtc::handle<PeripheralId::RTC>;
static_assert(Rtc::valid);

namespace {

template <typename Handle>
void exercise_rtc_extended(const Handle& rtc) {
    // ---- original API (backwards compat) ----
    [[maybe_unused]] const auto configure_result    = rtc.configure();
    [[maybe_unused]] const auto enable_write_result = rtc.enable_write_access();
    [[maybe_unused]] const auto enter_init_result   = rtc.enter_init_mode();
    [[maybe_unused]] const bool init_ready          = rtc.init_ready();
    [[maybe_unused]] const auto alarm_irq_result    = rtc.enable_alarm_interrupt();
    [[maybe_unused]] const bool alarm_pending       = rtc.alarm_pending();
    [[maybe_unused]] const auto clear_alarm_result  = rtc.clear_alarm();
    [[maybe_unused]] const auto time_result         = rtc.read_time();
    [[maybe_unused]] const auto date_result         = rtc.read_date();
    [[maybe_unused]] const auto leave_init_result   = rtc.leave_init_mode();
    [[maybe_unused]] const auto disable_write_result = rtc.disable_write_access();

    // ---- task 1.1: set_hour_mode ----
    using HM = alloy::hal::rtc::HourMode;
    [[maybe_unused]] const auto hm24 = rtc.set_hour_mode(HM::Hour24);
    [[maybe_unused]] const auto hm12 = rtc.set_hour_mode(HM::Hour12AmPm);

    // ---- task 1.2: prescalers ----
    [[maybe_unused]] const auto sync_r  = rtc.set_sync_prescaler(0x00FFu);
    [[maybe_unused]] const auto async_r = rtc.set_async_prescaler(0x7Fu);

    // ---- task 1.3: structured time + date ----
    using Time = alloy::hal::rtc::Time;
    using Date = alloy::hal::rtc::Date;

    [[maybe_unused]] const auto set_t = rtc.set_time(Time{.hours=12u, .minutes=30u, .seconds=0u});
    [[maybe_unused]] const Time cur_t = rtc.current_time();
    static_assert(std::is_same_v<decltype(cur_t), const Time>);

    [[maybe_unused]] const auto set_d = rtc.set_date(Date{.year=24u, .month=4u, .day=27u, .weekday=7u});
    [[maybe_unused]] const Date cur_d = rtc.current_date();
    static_assert(std::is_same_v<decltype(cur_d), const Date>);

    // ---- task 2.1: set_alarm ----
    using AlarmConfig = alloy::hal::rtc::AlarmConfig;
    [[maybe_unused]] const auto alarm_r = rtc.set_alarm(AlarmConfig{
        .hours = 12u, .minutes = 31u, .seconds = 0u,
        .match_seconds = true, .match_minutes = true, .match_hours = true,
    });

    // ---- task 2.2: calendar event ----
    [[maybe_unused]] const auto cal_en   = rtc.enable_calendar_event(true);
    [[maybe_unused]] const auto cal_off  = rtc.enable_calendar_event(false);
    [[maybe_unused]] const bool cal_pend = rtc.calendar_event_pending();
    [[maybe_unused]] const auto cal_clr  = rtc.clear_calendar_event();

    // ---- task 2.3: tamper / second / time-event clear ----
    [[maybe_unused]] const auto tamp_clr = rtc.clear_tamper_error();
    [[maybe_unused]] const auto sec_clr  = rtc.clear_second();
    [[maybe_unused]] const auto tev_clr  = rtc.clear_time_event();

    // ---- task 3.1: typed interrupts ----
    using K = alloy::hal::rtc::InterruptKind;
    [[maybe_unused]] const auto en_alarm  = rtc.enable_interrupt(K::Alarm);
    [[maybe_unused]] const auto dis_alarm = rtc.disable_interrupt(K::Alarm);
    [[maybe_unused]] const auto en_sec    = rtc.enable_interrupt(K::Second);
    [[maybe_unused]] const auto en_tev    = rtc.enable_interrupt(K::TimeEvent);
    [[maybe_unused]] const auto en_cal    = rtc.enable_interrupt(K::CalendarEvent);
    [[maybe_unused]] const auto en_ab     = rtc.enable_interrupt(K::AlarmB);
    [[maybe_unused]] const auto en_wup    = rtc.enable_interrupt(K::Wakeup);
    [[maybe_unused]] const auto en_tamp   = rtc.enable_interrupt(K::Tamper);

    // ---- task 3.2: backup registers ----
    [[maybe_unused]] const auto bk_r = rtc.read_backup(0u);
    [[maybe_unused]] const auto bk_w = rtc.write_backup(0u, 0xDEAD'BEEFu);

    // ---- task 3.3: irq_numbers ----
    [[maybe_unused]] const auto irqs = Handle::irq_numbers();
    static_assert(std::is_same_v<decltype(irqs), const std::span<const std::uint32_t>>);

    // ---- task 4.2: async wait_for ----
    [[maybe_unused]] const auto wait_op =
        alloy::runtime::async::rtc::wait_for<K::Alarm>(rtc);
}

}  // namespace

[[maybe_unused]] void compile_rtc_backend() {
    auto rtc = alloy::hal::rtc::open<PeripheralId::RTC>(
        alloy::hal::rtc::Config{
            .enable_write_access = true,
            .enter_init_mode     = true,
            .enable_alarm_interrupt = true,
        });
    exercise_rtc_extended(rtc);
}
#endif

int main() {
    return 0;
}
