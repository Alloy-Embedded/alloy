#include "hal/pwm.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
using PeripheralId = alloy::hal::pwm::PeripheralId;

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
using Pwm = alloy::hal::pwm::handle<PeripheralId::TIM1, 0u>;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using Pwm = alloy::hal::pwm::handle<PeripheralId::TIM1, 0u>;
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
using Pwm = alloy::hal::pwm::handle<PeripheralId::PWM0, 0u>;
#elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
using Pwm = alloy::hal::pwm::handle<PeripheralId::PWM, 0u>;
#endif

#if !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
static_assert(Pwm::valid);
#endif
#endif

int main() {
#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE && !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    auto pwm = alloy::hal::pwm::open<Pwm::peripheral_id, Pwm::channel_index>(
        alloy::hal::pwm::Config{
            .period = 1000u, .apply_period = true, .duty_cycle = 500u, .apply_duty_cycle = true});
    [[maybe_unused]] const auto configure_result = pwm.configure();
    [[maybe_unused]] const auto start_result = pwm.start();
    [[maybe_unused]] const auto stop_result = pwm.stop();
    [[maybe_unused]] const auto period_result = pwm.set_period(1000u);
    [[maybe_unused]] const auto duty_result = pwm.set_duty_cycle(500u);
    [[maybe_unused]] const auto freq_result = pwm.set_frequency(1000u);
#endif
    return 0;
}
