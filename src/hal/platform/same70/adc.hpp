/**
 * @file adc.hpp
 * @brief SAME70 ADC Platform Integration
 *
 * This file combines the generic ADC API with SAME70-specific hardware policies.
 * It provides convenient type aliases for all ADC instances on SAME70.
 */

#pragma once

#include "hal/vendors/atmel/same70/adc_hardware_policy.hpp"

namespace alloy::hal::same70 {

// ============================================================================
// Hardware Policy Type Aliases
// ============================================================================

using Adc0 = Adc0Hardware;
using Adc1 = Adc1Hardware;

}  // namespace alloy::hal::same70

/**
 * @example ADC usage on SAME70
 *
 * @code
 * #include "hal/platform/same70/adc.hpp"
 * using namespace alloy::hal::same70;
 *
 * int main() {
 *     // Initialize ADC0
 *     Adc0::reset();
 *     Adc0::configure_resolution();
 *     Adc0::set_prescaler(9);  // 150MHz / (9+1) / 2 = 7.5MHz
 *     Adc0::enable_channel(0);
 *     
 *     // Start conversion and read
 *     Adc0::select_channel(0);
 *     Adc0::start_conversion();
 *     if (Adc0::wait_conversion(0, 100000)) {
 *         uint16_t value = Adc0::read_value();  // 12-bit value
 *         // Convert to voltage: voltage = (value / 4095.0f) * 3.3f
 *     }
 * }
 * @endcode
 */
