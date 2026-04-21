#include "hal/rtc.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
using PeripheralId = alloy::hal::rtc::PeripheralId;
using Rtc = alloy::hal::rtc::handle<PeripheralId::RTC>;
static_assert(Rtc::valid);
#endif

int main() {
#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
    auto rtc = alloy::hal::rtc::open<Rtc::peripheral_id>(
        alloy::hal::rtc::Config{
            .enable_write_access = true, .enter_init_mode = true, .enable_alarm_interrupt = true});
    [[maybe_unused]] const auto configure_result = rtc.configure();
    [[maybe_unused]] const auto enable_write_result = rtc.enable_write_access();
    [[maybe_unused]] const auto enter_init_result = rtc.enter_init_mode();
    [[maybe_unused]] const auto init_ready = rtc.init_ready();
    [[maybe_unused]] const auto alarm_irq_result = rtc.enable_alarm_interrupt();
    [[maybe_unused]] const auto alarm_pending = rtc.alarm_pending();
    [[maybe_unused]] const auto clear_alarm_result = rtc.clear_alarm();
    [[maybe_unused]] const auto time_result = rtc.read_time();
    [[maybe_unused]] const auto date_result = rtc.read_date();
    [[maybe_unused]] const auto leave_init_result = rtc.leave_init_mode();
    [[maybe_unused]] const auto disable_write_result = rtc.disable_write_access();
#endif
    return 0;
}
