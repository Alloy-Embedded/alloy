/**
 * @file adc.hpp
 * @brief STM32F7 ADC Platform Integration Layer
 *
 * This file provides type aliases for the ADC tier APIs (Simple/Fluent/Expert)
 * tailored to the STM32F7 family. The STM32F7 has 3 ADCs (ADC1, ADC2, ADC3)
 * with 12-bit resolution and variable resolution support (12/10/8/6-bit).
 *
 * STM32F7 ADC Features:
 * - 3 ADCs (ADC1, ADC2, ADC3)
 * - 12-bit resolution (default) with 10/8/6-bit options
 * - Up to 24 channels per ADC
 * - Conversion speed: 2.4 MSPS @ 12-bit (84 MHz APB2)
 * - Dual/Triple mode for synchronized sampling
 * - DMA support for continuous conversion
 * - Hardware oversampling for noise reduction
 * - Temperature sensor, Vrefint, Vbat internal channels
 *
 * Example Usage (Simple API):
 * @code
 * using namespace ucore::hal::stm32f7;
 * auto adc = Adc1::quick_setup<AdcPin>(AdcChannel::CH0);
 * auto value = adc.read();  // Read channel 0
 * @endcode
 *
 * Example Usage (Fluent API):
 * @code
 * auto adc = Adc1Builder()
 *     .channel(AdcChannel::CH1)
 *     .resolution(AdcResolution::Bits12)
 *     .initialize();
 * @endcode
 *
 * Example Usage (Expert API):
 * @code
 * constexpr auto config = AdcExpertConfig::standard(PeripheralId::ADC1, AdcChannel::CH0);
 * expert::configure(config);
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

namespace ucore::hal::stm32f7 {

using namespace ucore::hal;

// ============================================================================
// Level 1: Simple API - One-liner setup with defaults
// ============================================================================

/**
 * @brief ADC1 Simple API (Default: 12-bit, Vdd reference, 84 cycles)
 *
 * STM32F722: 16 external channels (PA0-PA7, PB0-PB1, PC0-PC5) + internal
 * STM32F767: 19 external channels (PA0-PA7, PB0-PB1, PC0-PC5, PF3-PF10) + internal
 *
 * Example:
 * @code
 * auto adc = Adc1::quick_setup<AdcPin>(AdcChannel::CH0);
 * auto result = adc.read();  // Returns Result<uint16_t, ErrorCode>
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
 */
using Adc2 = Adc<PeripheralId::ADC2>;

/**
 * @brief ADC3 Simple API
 *
 * Same channels as ADC1/ADC2, can be used in triple mode
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
 *     .resolution(AdcResolution::Bits12)
 *     .bits_12()                         // Or use convenience method
 *     .initialize();
 * @endcode
 */
using Adc1Builder = AdcBuilder<PeripheralId::ADC1>;

/**
 * @brief ADC2 Fluent Builder API
 */
using Adc2Builder = AdcBuilder<PeripheralId::ADC2>;

/**
 * @brief ADC3 Fluent Builder API
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
 * // Or DMA-enabled continuous mode
 * constexpr auto dma_config = AdcExpertConfig::with_dma(
 *     PeripheralId::ADC1,
 *     AdcChannel::CH0
 * );
 *
 * expert::configure(config);
 * @endcode
 *
 * @note The Expert API is accessed through the expert namespace
 */
// Expert API is accessed through: ucore::hal::expert::configure(config)

}  // namespace ucore::hal::stm32f7

// ============================================================================
// Platform-Specific Notes
// ============================================================================

/**
 * STM32F7 ADC Architecture:
 *
 * Clock Source:
 * - ADC clock from APB2 (max 108 MHz on STM32F722, 216 MHz on STM32F767)
 * - ADC prescaler: /2, /4, /6, /8
 * - Recommended ADC clock: 30-36 MHz for optimal accuracy
 *
 * Resolution Options:
 * - 12-bit: 4096 levels (default) - 15 ADC cycles @ 36 MHz = 2.4 MSPS
 * - 10-bit: 1024 levels - 13 ADC cycles = 2.8 MSPS
 * - 8-bit:  256 levels - 11 ADC cycles = 3.3 MSPS
 * - 6-bit:  64 levels - 9 ADC cycles = 4.0 MSPS
 *
 * Sample Time Configuration:
 * - 3 cycles (fastest, noisier)
 * - 15, 28, 56, 84, 112, 144, 480 cycles
 * - Longer sample time = better accuracy, slower conversion
 *
 * DMA Integration:
 * - Each ADC has dedicated DMA channel
 * - Circular mode for continuous sampling
 * - ADC1: DMA2 Stream 0/4, Channel 0
 * - ADC2: DMA2 Stream 2/3, Channel 1
 * - ADC3: DMA2 Stream 0/1, Channel 2
 *
 * Internal Channels:
 * - Channel 16: Temperature sensor (typical: 25°C = 760 mV, 2.5 mV/°C)
 * - Channel 17: Vrefint (internal reference, ~1.21V)
 * - Channel 18: Vbat (battery voltage / 4)
 *
 * Multi-Mode Operation:
 * - Dual mode: ADC1 + ADC2 synchronized
 * - Triple mode: ADC1 + ADC2 + ADC3 synchronized
 * - Simultaneous, interleaved, or alternate trigger modes
 * - Increases sampling rate up to 7.2 MSPS (triple interleaved)
 *
 * Calibration:
 * - STM32F7 ADC does NOT require calibration (unlike F1)
 * - Factory-calibrated at 3.3V
 *
 * Voltage Reference:
 * - Vdd: Power supply (typically 3.3V)
 * - Vref+: External reference (if available)
 * - Input range: 0V to Vref+ (max 3.6V)
 *
 * Accuracy:
 * - Integral non-linearity: ±2 LSB (12-bit)
 * - Differential non-linearity: ±1 LSB (12-bit)
 * - Offset error: ±5 mV
 * - Gain error: ±1.5%
 *
 * Performance Tips:
 * 1. Use DMA for continuous sampling (zero CPU overhead)
 * 2. Enable oversampling for 13-16 bit effective resolution
 * 3. Increase sample time for high-impedance sources
 * 4. Use dual/triple mode for multi-channel high-speed acquisition
 * 5. Monitor temperature sensor for thermal drift compensation
 *
 * Common Use Cases:
 * - Simple Tier: Battery voltage monitoring, button analog input
 * - Fluent Tier: Multi-channel data acquisition, sensor arrays
 * - Expert Tier: High-speed DMA logging, synchronized multi-ADC sampling
 */
