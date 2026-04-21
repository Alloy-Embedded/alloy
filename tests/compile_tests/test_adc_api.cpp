#include "hal/adc.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
using PeripheralId = alloy::hal::adc::PeripheralId;

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
using Adc = alloy::hal::adc::handle<PeripheralId::ADC1>;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using Adc = alloy::hal::adc::handle<PeripheralId::ADC1>;
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
using Adc = alloy::hal::adc::handle<PeripheralId::AFEC0>;
#endif

static_assert(Adc::valid);
#endif

int main() {
#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
    auto adc = alloy::hal::adc::open<Adc::peripheral_id>(
        alloy::hal::adc::Config{.enable_on_configure = true});
    [[maybe_unused]] const auto configure_result = adc.configure();
    [[maybe_unused]] const auto enable_result = adc.enable();
    [[maybe_unused]] const auto start_result = adc.start();
    [[maybe_unused]] const auto ready = adc.ready();
    [[maybe_unused]] const auto read_result = adc.read();
#endif
    return 0;
}
