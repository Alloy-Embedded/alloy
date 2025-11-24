/**
 * @file adc.hpp
 * @brief STM32F1 ADC Platform Integration Layer
 *
 * This file provides type aliases for the ADC tier APIs (Simple/Fluent/Expert)
 * tailored to the STM32F1 family. The STM32F1 has 2-3 ADCs (depending on variant)
 * with fixed 12-bit resolution and requires calibration after power-on.
 *
 * STM32F1 ADC Features:
 * - 2 ADCs (STM32F103C8/RB) or 3 ADCs (STM32F103VE/ZE)
 * - Fixed 12-bit resolution (4096 levels)
 * - Up to 18 channels per ADC (16 external + 2 internal)
 * - Conversion speed: 1 MSPS @ 12-bit (56 MHz ADC clock max)
 * - Dual mode for synchronized sampling
 * - DMA support for continuous conversion
 * - IMPORTANT: Requires calibration after power-on!
 * - Temperature sensor, Vrefint internal channels
 *
 * Example Usage (Simple API):
 * @code
 * using namespace ucore::hal::stm32f1;
 * auto adc = Adc1::quick_setup<AdcPin>(AdcChannel::CH0);
 * auto value = adc.read();  // Read channel 0
 * @endcode
 *
 * Example Usage (Fluent API):
 * @code
 * auto adc = Adc1Builder()
 *     .channel(AdcChannel::CH1)
 *     .bits_12()  // Fixed resolution on F1
 *     .initialize();
 * @endcode
 *
 * Example Usage (Expert API):
 * @code
 * constexpr auto config = AdcExpertConfig::standard(PeripheralId::ADC1, AdcChannel::CH0);
 * expert::configure(config);
 * // IMPORTANT: Run calibration after configuration!
 * @endcode
 *
 * @note Part of Phase 3.5: ADC Implementation
 * @see docs/API_TIERS.md for tier philosophy
 */

#pragma once

#include "hal/api/adc_simple.hpp"
#include "hal/api/adc_fluent.hpp"
#include "hal/api/adc_expert.hpp"
#include "hal/core/peripheral_id.hpp"

namespace ucore::hal::stm32f1 {

using namespace ucore::hal;

// ============================================================================
// Level 1: Simple API - One-liner setup with defaults
// ============================================================================

/**
 * @brief ADC1 Simple API (Default: 12-bit, Vdd reference, 28.5 cycles)
 *
 * STM32F103C8/RB: 10 external channels (PA0-PA7, PB0-PB1) + 2 internal
 * STM32F103VE/ZE: 16 external channels (PA0-PA7, PB0-PB1, PC0-PC5, etc.) + 2 internal
 *
 * Example:
 * @code
 * auto adc = Adc1::quick_setup<AdcPin>(AdcChannel::CH0);
 *
 * // IMPORTANT: Calibrate after power-on!
 * adc.calibrate();  // Typically takes 5.9 µs
 *
 * auto result = adc.read();
 * if (result.is_ok()) {
 *     uint16_t value = result.unwrap();
 *     uint32_t voltage_mv = (value * 3300) / 4095;
 * }
 * @endcode
 */
using Adc1 = Adc<PeripheralId::ADC1>;

/**
 * @brief ADC2 Simple API
 *
 * Same channels as ADC1, can be used in dual mode with ADC1
 * Available on all STM32F103 variants
 */
using Adc2 = Adc<PeripheralId::ADC2>;

/**
 * @brief ADC3 Simple API
 *
 * Only available on high-density devices (STM32F103VE/ZE)
 * Not present on STM32F103C8/RB
 */
using Adc3 = Adc<PeripheralId::ADC3>;

// ============================================================================
// Level 2: Fluent API - Builder pattern with method chaining
// ============================================================================

/**
 * @brief ADC1 Fluent Builder API
 *
 * Provides method chaining for readable configuration:
 *
 * Example:
 * @code
 * auto adc = Adc1Builder()
 *     .channel(AdcChannel::CH1)
 *     .bits_12()  // Fixed on F1
 *     .initialize();
 *
 * // Calibrate after initialization
 * adc.calibrate();
 * @endcode
 */
using Adc1Builder = AdcBuilder<PeripheralId::ADC1>;

/**
 * @brief ADC2 Fluent Builder API
 */
using Adc2Builder = AdcBuilder<PeripheralId::ADC2>;

/**
 * @brief ADC3 Fluent Builder API (high-density devices only)
 */
using Adc3Builder = AdcBuilder<PeripheralId::ADC3>;

// ============================================================================
// Level 3: Expert API - Full control with compile-time configuration
// ============================================================================

/**
 * @brief ADC Expert API - Compile-time configuration
 *
 * The Expert API uses AdcExpertConfig struct for compile-time validation
 * and maximum performance.
 *
 * Example:
 * @code
 * // Standard configuration (12-bit, no DMA)
 * constexpr auto config = AdcExpertConfig::standard(
 *     PeripheralId::ADC1,
 *     AdcChannel::CH0
 * );
 *
 * expert::configure(config);
 *
 * // CRITICAL: STM32F1 requires calibration after every power-on!
 * // Calibration removes offset errors and improves accuracy
 * expert::calibrate(PeripheralId::ADC1);
 * @endcode
 *
 * @note The Expert API is accessed through the expert namespace
 */
// Expert API is accessed through: ucore::hal::expert::configure(config)

}  // namespace ucore::hal::stm32f1

// ============================================================================
// Platform-Specific Notes
// ============================================================================

/**
 * STM32F1 ADC Architecture:
 *
 * Clock Source:
 * - ADC clock from APB2 via prescaler (max 72 MHz APB2)
 * - ADC prescaler: /2, /4, /6, /8
 * - Maximum ADC clock: 14 MHz (56 MHz / 4)
 * - Recommended: 12 MHz for best accuracy (72 MHz / 6)
 *
 * Resolution:
 * - Fixed 12-bit resolution (4096 levels)
 * - No variable resolution like F4/F7
 * - Conversion time: 12.5 + sample time cycles
 *
 * Sample Time Configuration:
 * - 1.5 cycles (fastest, noisier, high-impedance sources not recommended)
 * - 7.5, 13.5, 28.5, 41.5, 55.5, 71.5, 239.5 cycles
 * - Longer sample time = better accuracy for high-impedance sources
 * - Recommended: 28.5 cycles for general-purpose (default)
 *
 * Calibration (CRITICAL):
 * - STM32F1 ADC MUST be calibrated after every power-on!
 * - Calibration takes 83 ADC clock cycles (~7 µs @ 12 MHz)
 * - Procedure:
 *   1. Enable ADC (ADON bit)
 *   2. Set CAL bit in ADC_CR2
 *   3. Wait for CAL bit to clear (hardware clears when done)
 * - Calibration removes offset errors (typically 5-10 LSB improvement)
 * - Re-calibrate if ADC clock changes or after long idle periods
 *
 * DMA Integration:
 * - Each ADC has dedicated DMA channel
 * - Circular mode for continuous sampling
 * - ADC1: DMA1 Channel 1
 * - ADC2: DMA2 Channel 5 (dual mode uses DMA1 Channel 1)
 * - ADC3: DMA2 Channel 5 (if present)
 *
 * Internal Channels:
 * - Channel 16: Temperature sensor (typical: 25°C = 1.43V, 4.3 mV/°C)
 * - Channel 17: Vrefint (internal reference, 1.20V typical)
 *
 * Dual Mode Operation:
 * - ADC1 + ADC2 synchronized
 * - Regular simultaneous, injected simultaneous, or alternate trigger
 * - Increases sampling rate up to 2 MSPS (interleaved mode)
 * - Data stored in ADC1_DR (ADC2 data in upper 16 bits)
 *
 * Voltage Reference:
 * - Vdda: Analog power supply (2.4V to 3.6V)
 * - Vref+: Positive reference (typically connected to Vdda)
 * - Input range: 0V to Vref+ (max 3.6V)
 *
 * Accuracy (after calibration):
 * - Integral non-linearity: ±2 LSB (12-bit)
 * - Differential non-linearity: ±1 LSB (12-bit)
 * - Offset error: ±1 LSB (after calibration, ±10 LSB before)
 * - Gain error: ±1.5%
 *
 * Performance Tips:
 * 1. ALWAYS calibrate after power-on (mandatory on F1!)
 * 2. Use DMA for continuous sampling (zero CPU overhead)
 * 3. Use 28.5+ cycles sample time for accuracy
 * 4. Use dual mode for simultaneous multi-channel sampling
 * 5. Re-calibrate if accuracy degrades over time
 *
 * Common Pitfalls:
 * 1. Forgetting to calibrate → ±10 LSB offset error!
 * 2. ADC clock > 14 MHz → conversion errors
 * 3. Sample time too short for high-impedance sources → inaccurate readings
 * 4. Not enabling temperature sensor before reading → reads 0
 *
 * Blue Pill (STM32F103C8T6) Specifics:
 * - 2 ADCs (ADC1, ADC2)
 * - 10 external channels: PA0-PA7, PB0-PB1
 * - Arduino pins: A0-A7 map to PA0-PA7
 * - 12 MHz ADC clock (72 MHz / 6)
 * - Conversion time: ~3.5 µs @ 28.5 cycles
 *
 * Common Use Cases:
 * - Simple Tier: Battery voltage monitoring, potentiometer reading
 * - Fluent Tier: Multi-channel sensor arrays, analog joystick
 * - Expert Tier: High-speed DMA logging, dual-mode synchronized sampling
 */
