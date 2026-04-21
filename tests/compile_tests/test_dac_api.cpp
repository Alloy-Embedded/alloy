#include "hal/dac.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
using PeripheralId = alloy::hal::dac::PeripheralId;

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
using Dac = alloy::hal::dac::handle<PeripheralId::DAC, 0u>;
#define ALLOY_TEST_HAS_RUNTIME_DAC 1
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
using Dac = alloy::hal::dac::handle<PeripheralId::DACC, 0u>;
#define ALLOY_TEST_HAS_RUNTIME_DAC 1
#endif

#if defined(ALLOY_TEST_HAS_RUNTIME_DAC)
static_assert(Dac::valid);
#endif
#endif

int main() {
#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE && defined(ALLOY_TEST_HAS_RUNTIME_DAC)
    auto dac = alloy::hal::dac::open<Dac::peripheral_id, Dac::channel_index>(
        alloy::hal::dac::Config{
            .enable_on_configure = true, .write_initial_value = true, .initial_value = 0u});
    [[maybe_unused]] const auto configure_result = dac.configure();
    [[maybe_unused]] const auto enable_result = dac.enable();
    [[maybe_unused]] const auto ready = dac.ready();
    [[maybe_unused]] const auto write_result = dac.write(0x123u);
    [[maybe_unused]] const auto disable_result = dac.disable();
#endif
    return 0;
}
