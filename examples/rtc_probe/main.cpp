#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board.hpp"
#else
    #error "Unsupported board! Define ALLOY_BOARD_* in your build system."
#endif

#include "device/runtime.hpp"
#include "hal/rtc.hpp"
#include "hal/systick.hpp"

using namespace alloy::hal;

namespace {

using PeripheralId = alloy::device::runtime::PeripheralId;
constexpr auto kRtcPeripheral = PeripheralId::RTC;

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

}  // namespace

int main() {
    board::init();

    auto rtc = alloy::hal::rtc::open<kRtcPeripheral>({
        .enable_write_access = true,
        .enter_init_mode = true,
        .enable_alarm_interrupt = true,
    });

    if (!rtc.configure().is_ok()) {
        blink_error(100);
    }
    [[maybe_unused]] const auto init_ready = rtc.init_ready();
    [[maybe_unused]] const auto time_result = rtc.read_time();
    [[maybe_unused]] const auto date_result = rtc.read_date();
    [[maybe_unused]] const auto alarm_pending = rtc.alarm_pending();
    [[maybe_unused]] const auto clear_result = rtc.clear_alarm();
    [[maybe_unused]] const auto leave_result = rtc.leave_init_mode();
    [[maybe_unused]] const auto disable_result = rtc.disable_write_access();

    while (true) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}
