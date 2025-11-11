/**
 * @file systick_fluent.hpp
 * @brief Level 2 Fluent API for SysTick
 * @note Part of Phase 6.6: SysTick Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/interface/systick.hpp"
#include "hal/systick_simple.hpp"

namespace alloy::hal {

using namespace alloy::core;

struct SysTickBuilderState {
    bool has_frequency = false;
    
    constexpr bool is_valid() const { 
        return has_frequency; 
    }
};

struct FluentSysTickConfig {
    SysTickConfig config;
    
    Result<void, ErrorCode> apply() const { 
        return Ok(); 
    }
};

/**
 * @brief Fluent SysTick configuration builder
 * 
 * Provides chainable methods for readable SysTick configuration.
 * 
 * Example:
 * @code
 * auto config = SysTickBuilder()
 *     .frequency_hz(1000000)
 *     .initialize();
 * @endcode
 */
class SysTickBuilder {
public:
    constexpr SysTickBuilder()
        : frequency_hz_(SysTickDefaults::frequency_hz),
          state_() {}

    /**
     * @brief Set tick frequency in Hz
     * 
     * @param freq_hz Frequency in Hz
     * @return Reference to builder for chaining
     */
    constexpr SysTickBuilder& frequency_hz(u32 freq_hz) {
        frequency_hz_ = freq_hz;
        state_.has_frequency = true;
        return *this;
    }

    /**
     * @brief Configure for 1us resolution (1MHz)
     * 
     * @return Reference to builder for chaining
     */
    constexpr SysTickBuilder& micros() {
        frequency_hz_ = 1000000;
        state_.has_frequency = true;
        return *this;
    }

    /**
     * @brief Configure for 1ms resolution (1kHz)
     * 
     * @return Reference to builder for chaining
     */
    constexpr SysTickBuilder& millis() {
        frequency_hz_ = 1000;
        state_.has_frequency = true;
        return *this;
    }

    /**
     * @brief Configure for RTOS use (1kHz = 1ms ticks)
     * 
     * @return Reference to builder for chaining
     */
    constexpr SysTickBuilder& rtos() {
        frequency_hz_ = 1000;
        state_.has_frequency = true;
        return *this;
    }

    /**
     * @brief Initialize with configured settings
     * 
     * @return Result with FluentSysTickConfig or error
     */
    Result<FluentSysTickConfig, ErrorCode> initialize() const {
        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        FluentSysTickConfig config{
            SysTickConfig{frequency_hz_}
        };

        return Ok(std::move(config));
    }

private:
    u32 frequency_hz_;
    SysTickBuilderState state_;
};

}  // namespace alloy::hal
