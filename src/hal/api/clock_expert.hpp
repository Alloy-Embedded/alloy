/**
 * @file clock_expert.hpp
 * @brief Level 3 Expert API for System Clock
 *
 * Provides full control over clock configuration.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Expert clock configuration
 *
 * Allows precise control over all clock parameters.
 * Use this when predefined configurations don't meet your needs.
 */
struct ClockExpertConfig {
    u32 target_frequency_hz;  ///< Target CPU frequency
    bool use_external_crystal;  ///< true = external crystal, false = internal RC
    bool enable_pll;  ///< Enable PLL for frequency multiplication
    u8 pll_multiplier;  ///< PLL multiplier (1-62)
    u8 pll_divider;  ///< PLL divider (1-255)
    u8 mck_prescaler;  ///< Master clock prescaler divider

    constexpr bool is_valid() const {
        if (target_frequency_hz == 0 || target_frequency_hz > 300000000) {
            return false;  // 0 Hz or > 300 MHz invalid
        }
        if (enable_pll && (pll_multiplier == 0 || pll_multiplier > 62)) {
            return false;
        }
        if (enable_pll && pll_divider == 0) {
            return false;
        }
        return true;
    }

    constexpr const char* error_message() const {
        if (target_frequency_hz == 0) return "Frequency cannot be zero";
        if (target_frequency_hz > 300000000) return "Frequency too high (max 300 MHz)";
        if (enable_pll && pll_multiplier == 0) return "PLL multiplier cannot be zero";
        if (enable_pll && pll_multiplier > 62) return "PLL multiplier max is 62";
        if (enable_pll && pll_divider == 0) return "PLL divider cannot be zero";
        return "Valid";
    }

    // Factory methods for common configurations

    /**
     * @brief 12 MHz using internal RC (no PLL) - safest option
     */
    static constexpr ClockExpertConfig internal_rc_12mhz() {
        return ClockExpertConfig{
            .target_frequency_hz = 12000000,
            .use_external_crystal = false,
            .enable_pll = false,
            .pll_multiplier = 0,
            .pll_divider = 1,
            .mck_prescaler = 1
        };
    }

    /**
     * @brief 150 MHz using external crystal + PLL
     *
     * Formula: (12 MHz crystal * 25) / 1 / 2 = 150 MHz
     */
    static constexpr ClockExpertConfig crystal_pll_150mhz() {
        return ClockExpertConfig{
            .target_frequency_hz = 150000000,
            .use_external_crystal = true,
            .enable_pll = true,
            .pll_multiplier = 25,
            .pll_divider = 1,
            .mck_prescaler = 2
        };
    }

    /**
     * @brief 144 MHz using internal RC + PLL
     *
     * Formula: (12 MHz RC * 24) / 1 / 2 = 144 MHz
     */
    static constexpr ClockExpertConfig rc_pll_144mhz() {
        return ClockExpertConfig{
            .target_frequency_hz = 144000000,
            .use_external_crystal = false,
            .enable_pll = true,
            .pll_multiplier = 24,
            .pll_divider = 1,
            .mck_prescaler = 2
        };
    }

    /**
     * @brief Custom configuration calculator
     *
     * Calculates PLL parameters for target frequency.
     *
     * @param target_freq Target frequency in Hz
     * @param use_crystal true for external crystal, false for RC
     * @return ClockExpertConfig with calculated parameters
     */
    static constexpr ClockExpertConfig calculate(u32 target_freq, bool use_crystal = false) {
        // Simple calculation (can be improved)
        u32 source_freq = 12000000;  // Both crystal and RC are 12 MHz
        u8 multiplier = static_cast<u8>(target_freq / source_freq);
        u8 prescaler = 1;

        if (multiplier > 62) {
            multiplier = 62;
            prescaler = static_cast<u8>((multiplier * source_freq) / target_freq);
        }

        return ClockExpertConfig{
            .target_frequency_hz = target_freq,
            .use_external_crystal = use_crystal,
            .enable_pll = (target_freq > source_freq),
            .pll_multiplier = multiplier,
            .pll_divider = 1,
            .mck_prescaler = prescaler
        };
    }
};

namespace expert {

/**
 * @brief Configure clock with expert settings
 *
 * @tparam ClockImpl Platform-specific clock implementation
 * @param config Expert configuration
 * @return Result with error code
 *
 * NOTE: This function would need to construct a platform-specific
 * ClockConfig from the expert settings. For now, it's a placeholder.
 */
template <typename ClockImpl>
Result<void, ErrorCode> configure(const ClockExpertConfig& config) {
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }

    // TODO: Convert ClockExpertConfig to platform-specific ClockConfig
    // This requires knowledge of the platform's ClockConfig structure
    // For now, return error indicating not implemented
    return Err(ErrorCode::NotImplemented);
}

}  // namespace expert

}  // namespace alloy::hal
