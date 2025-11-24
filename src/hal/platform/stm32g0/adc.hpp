/**
 * @file adc.hpp
 * @brief STM32G0 ADC Platform Integration Layer
 *
 * This file provides type aliases for the ADC tier APIs (Simple/Fluent/Expert)
 * tailored to the STM32G0 family. The STM32G0 has a single modern ADC with
 * advanced features like oversampling, hardware averaging, and low-power modes.
 *
 * STM32G0 ADC Features:
 * - 1 ADC (ADC1) with modern architecture
 * - 12-bit resolution (default) with hardware oversampling up to 16-bit
 * - Up to 19 channels (16 external + 3 internal)
 * - Conversion speed: 2.5 MSPS @ 12-bit
 * - Hardware oversampling (up to 256x for 16-bit effective resolution)
 * - Low-power auto-off mode (stops ADC between conversions)
 * - DMA support with circular mode
 * - Wakeup from STOP mode
 * - Temperature sensor, Vrefint, Vbat internal channels
 *
 * Example Usage (Simple API):
 * @code
 * using namespace ucore::hal::stm32g0;
 * auto adc = Adc1::quick_setup<AdcPin>(AdcChannel::CH0);
 * auto value = adc.read();  // Read channel 0
 * @endcode
 *
 * Example Usage (Fluent API):
 * @code
 * auto adc = Adc1Builder()
 *     .channel(AdcChannel::CH1)
 *     .resolution(AdcResolution::Bits12)
 *     .enable_oversampling(16)  // 16x oversampling for 14-bit
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

namespace ucore::hal::stm32g0 {

using namespace ucore::hal;

// ============================================================================
// Level 1: Simple API - One-liner setup with defaults
// ============================================================================

/**
 * @brief ADC1 Simple API (Default: 12-bit, Vdd reference, 12.5 cycles)
 *
 * STM32G071RB: 16 external channels (PA0-PA7, PB0-PB2, PB10-PB12, PC4-PC5) + 3 internal
 * STM32G0B1RE: 19 external channels (full GPIO coverage) + 3 internal
 *
 * Example:
 * @code
 * auto adc = Adc1::quick_setup<AdcPin>(AdcChannel::CH0);
 *
 * // Optional: Enable oversampling for better accuracy
 * adc.enable_oversampling(16);  // 16x → 14-bit effective resolution
 *
 * auto result = adc.read();
 * if (result.is_ok()) {
 *     uint16_t value = result.unwrap();
 *     uint32_t voltage_mv = (value * 3300) / 4095;
 * }
 * @endcode
 */
using Adc1 = Adc<PeripheralId::ADC1>;

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
 *     .enable_oversampling(64)    // 64x → 15-bit effective
 *     .enable_low_power()          // Auto-off between conversions
 *     .initialize();
 * @endcode
 */
using Adc1Builder = AdcBuilder<PeripheralId::ADC1>;

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
 * // Or with oversampling for 16-bit effective resolution
 * constexpr auto oversample_config = AdcExpertConfig{
 *     .peripheral = PeripheralId::ADC1,
 *     .channel = AdcChannel::CH0,
 *     .resolution = AdcResolution::Bits12,
 *     .reference = AdcReference::Vdd,
 *     .sample_time = AdcSampleTime::Cycles12_5,
 *     .enable_dma = false,
 *     .enable_continuous = false,
 *     .enable_timer_trigger = false,
 *     .enable_oversampling = true,
 *     .oversampling_ratio = 256  // 256x → 16-bit
 * };
 *
 * expert::configure(config);
 * @endcode
 *
 * @note The Expert API is accessed through the expert namespace
 */
// Expert API is accessed through: ucore::hal::expert::configure(config)

}  // namespace ucore::hal::stm32g0

// ============================================================================
// Platform-Specific Notes
// ============================================================================

/**
 * STM32G0 ADC Architecture:
 *
 * Clock Source:
 * - ADC clock from system clock, HSI16, or PLLP
 * - Asynchronous clock for low-power operation
 * - Maximum ADC clock: 35 MHz (G071), 70 MHz (G0B1)
 * - Recommended: 16 MHz from HSI16 for accuracy and low power
 *
 * Resolution Options:
 * - 12-bit: 4096 levels (default) - 12.5 ADC cycles @ 16 MHz = 1.28 MSPS
 * - 10-bit: 1024 levels - 10.5 ADC cycles = 1.52 MSPS
 * - 8-bit:  256 levels - 8.5 ADC cycles = 1.88 MSPS
 * - 6-bit:  64 levels - 6.5 ADC cycles = 2.46 MSPS
 *
 * Hardware Oversampling (Unique to G0):
 * - 2x, 4x, 8x, 16x, 32x, 64x, 128x, 256x ratios
 * - Automatic right-shift for scaling
 * - Effective resolution increase:
 *   - 2x/4x → +1 bit (13-bit from 12-bit)
 *   - 16x → +2 bits (14-bit)
 *   - 64x → +3 bits (15-bit)
 *   - 256x → +4 bits (16-bit)
 * - Hardware averaging reduces noise without CPU overhead
 * - Example: 12-bit @ 256x oversample = 16-bit @ 5 kSPS
 *
 * Sample Time Configuration:
 * - 1.5 cycles (fastest, for low-impedance sources)
 * - 3.5, 7.5, 12.5, 19.5, 39.5, 79.5, 160.5 cycles
 * - Longer sample time = better accuracy for high-impedance sources
 * - Recommended: 12.5 cycles for general-purpose (default)
 *
 * Low-Power Features:
 * - Auto-off mode: ADC turns off between conversions (saves power)
 * - Auto-wait mode: Automatic delay between conversions
 * - Wakeup from STOP mode: ADC can trigger wakeup
 * - Low-frequency mode: < 2.8V operation
 *
 * DMA Integration:
 * - Single DMA channel for ADC1
 * - Circular mode for continuous sampling
 * - ADC1: DMA1 Channel 1
 * - Overrun detection and handling
 *
 * Internal Channels:
 * - Channel 12: Temperature sensor (typical: 30°C = 2500 LSB @ 12-bit)
 * - Channel 13: Vrefint (internal reference, 1.212V typical)
 * - Channel 14: Vbat (battery voltage / 3)
 *
 * Calibration (Optional):
 * - Unlike STM32F1, calibration is optional on G0
 * - Self-calibration can improve accuracy by ~1 LSB
 * - Procedure:
 *   1. Ensure ADC is disabled
 *   2. Set ADCAL bit in ADC_CR
 *   3. Wait for ADCAL to clear (takes ~116 ADC cycles)
 * - Recommended after power-on for maximum accuracy
 *
 * Voltage Reference:
 * - Vdda: Analog power supply (1.65V to 3.6V)
 * - Vref+: External reference (optional, 2.0V to Vdda)
 * - Input range: 0V to Vref+ (max Vdda)
 *
 * Accuracy:
 * - Integral non-linearity: ±1.5 LSB (12-bit)
 * - Differential non-linearity: ±0.8 LSB (12-bit)
 * - Offset error: ±2 LSB (±0.5 LSB after calibration)
 * - Gain error: ±1%
 * - With 256x oversampling: ±0.5 LSB effective @ 16-bit
 *
 * Performance Tips:
 * 1. Use oversampling for noise-free high-resolution measurements
 * 2. Enable auto-off mode for battery-powered applications
 * 3. Use DMA circular mode for continuous streaming
 * 4. Calibrate after power-on for best accuracy
 * 5. Use wakeup feature for ultra-low-power sensor monitoring
 *
 * Common Use Cases:
 * - Simple Tier: Battery monitoring, button analog input, quick readings
 * - Fluent Tier: High-resolution sensor (16-bit with oversample), multi-channel
 * - Expert Tier: Ultra-low-power data logger, wakeup-on-threshold detection
 *
 * Oversampling Examples:
 * - 12-bit + 16x oversample = 14-bit @ 80 kSPS (great for sensors)
 * - 12-bit + 256x oversample = 16-bit @ 5 kSPS (noise-free precision)
 * - 8-bit + 64x oversample = 14-bit @ 29 kSPS (fast high-res)
 *
 * Power Consumption:
 * - Active: 240 µA @ 12-bit, 16 MHz clock
 * - Auto-off: 90 µA average @ 1 kSPS
 * - With DMA + auto-off: < 100 µA total system overhead
 *
 * STM32G071RB Specifics:
 * - 1 ADC (ADC1)
 * - 16 external channels
 * - 64 MHz max CPU, 35 MHz max ADC clock
 * - Arduino-compatible Nucleo board
 *
 * STM32G0B1RE Specifics:
 * - 1 ADC (ADC1)
 * - 19 external channels
 * - 64 MHz max CPU, 70 MHz max ADC clock (2x faster ADC!)
 * - USB support for data streaming
 */
