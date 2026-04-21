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
#include "hal/pwm.hpp"
#include "hal/systick.hpp"
#include "hal/timer.hpp"

using namespace alloy::hal;

namespace {

using PeripheralId = alloy::device::runtime::PeripheralId;

#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
constexpr auto kTimerPeripheral = PeripheralId::TC0;
constexpr auto kPwmPeripheral = PeripheralId::PWM0;
#else
constexpr auto kTimerPeripheral = PeripheralId::TIM1;
constexpr auto kPwmPeripheral = PeripheralId::TIM1;
#endif

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

}  // namespace

int main() {
    board::init();

    auto timer = alloy::hal::timer::open<kTimerPeripheral>();
    auto pwm = alloy::hal::pwm::open<kPwmPeripheral, 0u>();

    if (!timer.start().is_ok()) {
        blink_error(100);
    }
    if (!timer.set_period(1000u).is_ok()) {
        blink_error(150);
    }
    if (!pwm.set_period(1000u).is_ok()) {
        blink_error(200);
    }
    if (!pwm.set_duty_cycle(500u).is_ok()) {
        blink_error(250);
    }
    if (!pwm.start().is_ok()) {
        blink_error(300);
    }
    if (!timer.stop().is_ok()) {
        blink_error(350);
    }
    if (!pwm.stop().is_ok()) {
        blink_error(400);
    }

    while (true) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}
