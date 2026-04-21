#include "hal/timer.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
using PeripheralId = alloy::hal::timer::PeripheralId;

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
using Timer = alloy::hal::timer::handle<PeripheralId::TIM1>;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using Timer = alloy::hal::timer::handle<PeripheralId::TIM1>;
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
using Timer = alloy::hal::timer::handle<PeripheralId::TC0>;
#endif

static_assert(Timer::valid);
#endif

int main() {
#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
    auto timer = alloy::hal::timer::open<Timer::peripheral_id>(
        alloy::hal::timer::Config{.period = 1000u, .apply_period = true});
    [[maybe_unused]] const auto configure_result = timer.configure();
    [[maybe_unused]] const auto start_result = timer.start();
    [[maybe_unused]] const auto stop_result = timer.stop();
    [[maybe_unused]] const auto period_result = timer.set_period(1000u);
    [[maybe_unused]] const auto count_result = timer.get_count();
    [[maybe_unused]] const auto running = timer.is_running();
#endif
    return 0;
}
