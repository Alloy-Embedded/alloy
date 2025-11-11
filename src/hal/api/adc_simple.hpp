/**
 * @file adc_simple.hpp
 * @brief Level 1 Simple API for ADC
 * @note Part of Phase 6.4: ADC Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/adc.hpp"
#include "hal/signals.hpp"

namespace alloy::hal {

using namespace alloy::hal::signals;

using namespace alloy::core;

struct AdcDefaults {
    static constexpr AdcResolution resolution = AdcResolution::Bits12;
    static constexpr AdcReference reference = AdcReference::Vdd;
    static constexpr AdcSampleTime sample_time = AdcSampleTime::Cycles84;
};

template <PeripheralId PeriphId>
class Adc {
public:
    template <typename ChannelPin>
    static constexpr auto quick_setup(
        AdcChannel channel = AdcChannel::Channel0,
        AdcResolution res = AdcDefaults::resolution) {
        
        return AdcConfig{res, AdcDefaults::reference, AdcDefaults::sample_time};
    }
};

}  // namespace alloy::hal
