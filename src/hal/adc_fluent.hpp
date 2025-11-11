/**
 * @file adc_fluent.hpp
 * @brief Level 2 Fluent API for ADC
 * @note Part of Phase 6.4: ADC Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/interface/adc.hpp"
#include "hal/adc_simple.hpp"

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;


struct AdcBuilderState {
    bool has_channel = false;
    constexpr bool is_valid() const { return has_channel; }
};

struct FluentAdcConfig {
    PeripheralId peripheral;
    AdcChannel channel;
    AdcConfig config;
    
    Result<void, ErrorCode> apply() const { return Ok(); }
};

template <PeripheralId PeriphId>
class AdcBuilder {
public:
    constexpr AdcBuilder()
        : channel_(AdcChannel::Channel0),
          resolution_(AdcDefaults::resolution),
          reference_(AdcDefaults::reference),
          sample_time_(AdcDefaults::sample_time),
          state_() {}

    constexpr AdcBuilder& channel(AdcChannel ch) {
        channel_ = ch;
        state_.has_channel = true;
        return *this;
    }

    constexpr AdcBuilder& resolution(AdcResolution res) {
        resolution_ = res;
        return *this;
    }

    constexpr AdcBuilder& bits_12() {
        resolution_ = AdcResolution::Bits12;
        return *this;
    }

    Result<FluentAdcConfig, ErrorCode> initialize() const {
        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        FluentAdcConfig config{
            PeriphId,
            channel_,
            AdcConfig{resolution_, reference_, sample_time_}
        };

        return Ok(std::move(config));
    }

private:
    AdcChannel channel_;
    AdcResolution resolution_;
    AdcReference reference_;
    AdcSampleTime sample_time_;
    AdcBuilderState state_;
};

}  // namespace alloy::hal
