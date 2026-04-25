#include <iterator>

#include "device/capabilities.hpp"
#include "device/clock_config.hpp"
#include "device/connectors.hpp"
#include "device/dma.hpp"
#include "device/import.hpp"
#include "device/interrupt_stubs.hpp"
#include "device/low_power.hpp"
#include "device/runtime.hpp"
#include "device/startup.hpp"
#include "device/system_sequences.hpp"
#include "device/system_clock.hpp"
#include "device/traits.hpp"

static_assert(alloy::device::SelectedDeviceTraits::available,
              "Selected device contract must be available for the smoke target.");
static_assert(!alloy::device::SelectedDeviceTraits::vendor.empty());
static_assert(!alloy::device::SelectedDeviceTraits::family.empty());
static_assert(!alloy::device::SelectedDeviceTraits::name.empty());

int main() {
    using namespace alloy::device;

    static_assert(imported::available);
    static_assert(SelectedRuntimeDescriptors::available);
    static_assert(SelectedStartupDescriptors::available);
    static_assert(SelectedSystemClockProfiles::available);
    static_assert(SelectedDmaBindings::available);
    static_assert(!runtime::peripherals.empty());
    static_assert(!runtime::pins.empty());
    static_assert(!runtime::registers.empty());

#if ALLOY_DEVICE_CAPABILITIES_AVAILABLE
    static_assert(SelectedCapabilities::available);
    static_assert(!capabilities::all.empty());
#else
    static_assert(!SelectedCapabilities::available);
#endif

#if ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE
    static_assert(SelectedClockConfig::available);
    static_assert(!clock_config::profile_ids.empty());
#else
    static_assert(!SelectedClockConfig::available);
#endif

#if ALLOY_DEVICE_CONNECTORS_AVAILABLE
    static_assert(SelectedConnectors::available);
    static_assert(!connectors::all.empty());
#else
    static_assert(!SelectedConnectors::available);
#endif

#if ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE
    static_assert(SelectedInterruptStubs::available);
    static_assert(!interrupt_stubs::all.empty());
#else
    static_assert(!SelectedInterruptStubs::available);
#endif

#if ALLOY_DEVICE_SYSTEM_SEQUENCES_AVAILABLE
    static_assert(SelectedSystemSequences::available);
    static_assert(!system_sequences::steps.empty());
#else
    static_assert(!SelectedSystemSequences::available);
#endif

#if ALLOY_DEVICE_LOW_POWER_AVAILABLE
    static_assert(SelectedLowPower::available);
    static_assert(!low_power::modes.empty());
    constexpr auto default_mode = low_power::modes.front().mode_id;
    static_assert(low_power::ModeTraits<default_mode>::kPresent);
#else
    static_assert(!SelectedLowPower::available);
#endif

#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
    static_cast<void>(runtime::adc_semantic_peripherals.size());
#endif
#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
    static_cast<void>(runtime::dac_semantic_peripherals.size());
#endif
#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE
    static_cast<void>(runtime::can_semantic_peripherals.size());
#endif
#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
    static_cast<void>(runtime::rtc_semantic_peripherals.size());
#endif
#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
    [](auto) {
        if constexpr (!runtime::watchdog_semantic_peripherals.empty()) {
            constexpr auto watchdog_peripheral = runtime::watchdog_semantic_peripherals.front();
            static_assert(runtime::WatchdogSemanticTraits<watchdog_peripheral>::kPresent);
        }
    }(0);
#endif
#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
    [](auto) {
        if constexpr (!runtime::timer_semantic_peripherals.empty()) {
            constexpr auto timer_peripheral = runtime::timer_semantic_peripherals.front();
            static_assert(runtime::TimerSemanticTraits<timer_peripheral>::kPresent);
        }
    }(0);
#endif
#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
    [](auto) {
        if constexpr (!runtime::pwm_semantic_peripherals.empty()) {
            constexpr auto pwm_peripheral = runtime::pwm_semantic_peripherals.front();
            static_assert(runtime::PwmSemanticTraits<pwm_peripheral>::kPresent);
        }
    }(0);
#endif

    return 0;
}
