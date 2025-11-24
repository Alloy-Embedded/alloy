/**
 * @file expert_adc_dma_logging.cpp
 * @brief Level 3 Expert API - ADC DMA Continuous Logging Example
 *
 * Demonstrates the Expert tier ADC API with compile-time configuration, DMA,
 * and advanced features. Perfect for high-speed data acquisition and logging.
 *
 * This example shows:
 * - Compile-time ADC configuration (zero runtime overhead)
 * - DMA setup for continuous circular buffering
 * - Hardware oversampling for increased resolution (STM32G0)
 * - High-speed multi-channel acquisition (up to 100 kSPS)
 * - Ring buffer management for data streaming
 *
 * Hardware Setup:
 * - CH0 (PA0): Analog signal source (sensor, function generator, etc.)
 * - Optional: Connect multiple channels for multi-channel logging
 *
 * Use Cases:
 * - Data logger: SD card recording @ 10 kSPS
 * - Oscilloscope: Real-time waveform capture @ 100 kSPS
 * - Vibration analysis: Accelerometer FFT @ 10 kSPS
 * - Audio recorder: Microphone sampling @ 44.1 kSPS
 *
 * Expected Behavior:
 * - Configures ADC with DMA at compile-time
 * - Continuously acquires samples in background (zero CPU)
 * - Processes data in main loop when buffer half-full
 * - Blinks LED on each buffer half completion
 *
 * @note Part of Phase 3.5: ADC Implementation
 * @see docs/API_TIERS.md
 */

// ============================================================================
// Board-Specific Configuration
// ============================================================================

#if defined(PLATFORM_STM32F401RE)
#include "boards/nucleo_f401re/board.hpp"
using namespace board::nucleo_f401re;

using AdcPin = Pin<PeripheralId::ADC1, PinId::PA0>;  // A0
constexpr auto AdcChannelNum = AdcChannel::CH0;

#elif defined(PLATFORM_STM32F722ZE)
#include "boards/nucleo_f722ze/board.hpp"
using namespace board::nucleo_f722ze;

using AdcPin = Pin<PeripheralId::ADC1, PinId::PA0>;
constexpr auto AdcChannelNum = AdcChannel::CH0;

#elif defined(PLATFORM_STM32G071RB)
#include "boards/nucleo_g071rb/board.hpp"
using namespace board::nucleo_g071rb;

using AdcPin = Pin<PeripheralId::ADC1, PinId::PA0>;
constexpr auto AdcChannelNum = AdcChannel::CH0;

#elif defined(PLATFORM_STM32G0B1RE)
#include "boards/nucleo_g0b1re/board.hpp"
using namespace board::nucleo_g0b1re;

using AdcPin = Pin<PeripheralId::ADC1, PinId::PA0>;
constexpr auto AdcChannelNum = AdcChannel::CH0;

#elif defined(PLATFORM_SAME70)
#include "boards/same70_xplained/board.hpp"
using namespace board::same70_xplained;

using AdcPin = Pin<PeripheralId::AFEC0, PinId::PD30>;
constexpr auto AdcChannelNum = AdcChannel::CH0;

#else
#error "Unsupported platform. Please define one of the supported platforms."
#endif

#include "hal/gpio.hpp"

using namespace ucore;
using namespace ucore::hal;

// ============================================================================
// DMA Buffer Configuration
// ============================================================================

// DMA buffer size (must be power of 2 for efficiency)
constexpr size_t DMA_BUFFER_SIZE = 512;

// Dual-buffer for continuous acquisition (ping-pong buffering)
uint16_t dma_buffer[DMA_BUFFER_SIZE] __attribute__((aligned(4)));

// Processing buffer (for copying DMA data)
uint16_t process_buffer[DMA_BUFFER_SIZE / 2];

// ============================================================================
// Statistics and Analysis
// ============================================================================

struct AnalysisResult {
    uint16_t min_value;
    uint16_t max_value;
    uint32_t avg_value;
    uint16_t peak_to_peak;
    uint32_t rms_value;
};

/**
 * @brief Analyze ADC buffer and calculate statistics
 *
 * @param buffer Pointer to ADC samples
 * @param length Number of samples
 * @return AnalysisResult Statistics (min, max, avg, pk-pk, RMS)
 */
AnalysisResult analyze_buffer(const uint16_t* buffer, size_t length) {
    AnalysisResult result = {};

    result.min_value = 4095;
    result.max_value = 0;
    uint64_t sum = 0;
    uint64_t sum_squares = 0;

    for (size_t i = 0; i < length; ++i) {
        uint16_t value = buffer[i];

        // Min/Max
        if (value < result.min_value) result.min_value = value;
        if (value > result.max_value) result.max_value = value;

        // Sum for average
        sum += value;

        // Sum of squares for RMS
        sum_squares += static_cast<uint64_t>(value) * value;
    }

    // Average
    result.avg_value = static_cast<uint32_t>(sum / length);

    // Peak-to-peak
    result.peak_to_peak = result.max_value - result.min_value;

    // RMS (Root Mean Square)
    // RMS = sqrt(sum(x^2) / N)
    uint64_t mean_square = sum_squares / length;
    // Simplified sqrt (Newton's method, 10 iterations)
    uint32_t rms = static_cast<uint32_t>(mean_square);
    for (int i = 0; i < 10; ++i) {
        rms = (rms + mean_square / rms) / 2;
    }
    result.rms_value = rms;

    return result;
}

// ============================================================================
// Main Application
// ============================================================================

/**
 * @brief Expert ADC DMA logging example
 *
 * Demonstrates compile-time configuration and DMA for high-speed acquisition.
 */
int main() {
    // ========================================================================
    // Compile-Time Configuration (Expert API)
    // ========================================================================

#if defined(PLATFORM_STM32G071RB) || defined(PLATFORM_STM32G0B1RE)
    // STM32G0: Use hardware oversampling for 16-bit effective resolution
    constexpr auto adc_config = AdcExpertConfig{
        .peripheral = PeripheralId::ADC1,
        .channel = AdcChannelNum,
        .resolution = AdcResolution::Bits12,
        .reference = AdcReference::Vdd,
        .sample_time = AdcSampleTime::Cycles12_5,  // Fast sampling
        .enable_dma = true,
        .enable_continuous = true,
        .enable_timer_trigger = true,               // Timer-triggered for precise rate
        .enable_oversampling = true,                // Hardware oversampling
        .oversampling_ratio = 16                    // 16x → 14-bit effective
    };
#else
    // STM32F4/F7/SAME70: Standard 12-bit DMA configuration
    constexpr auto adc_config = AdcExpertConfig{
        .peripheral = PeripheralId::ADC1,
        .channel = AdcChannelNum,
        .resolution = AdcResolution::Bits12,
        .reference = AdcReference::Vdd,
        .sample_time = AdcSampleTime::Cycles84,
        .enable_dma = true,
        .enable_continuous = true,
        .enable_timer_trigger = false
    };
#endif

    // Validate configuration at compile-time
    static_assert(adc_config.is_valid(), "Invalid ADC configuration");

    // Initialize with compile-time config
    auto init_result = expert::adc::configure(adc_config);
    if (!init_result.is_ok()) {
        // Configuration failed
        while (true);
    }

    // ========================================================================
    // DMA Setup
    // ========================================================================

    // Configure DMA for circular mode
    // - Source: ADC data register
    // - Destination: dma_buffer
    // - Mode: Circular (wraps around)
    // - Interrupt: Half-transfer and transfer-complete

    // DMA configuration (pseudo-code, platform-specific)
    // dma_configure(DMA1_Channel1, &ADC1->DR, dma_buffer,
    //               DMA_BUFFER_SIZE, DMA_CIRCULAR | DMA_HALF_INTERRUPT);

    // Setup LED for visual feedback
    auto led = LedPin::output();

    // ========================================================================
    // Start Continuous ADC Acquisition
    // ========================================================================

    // Start ADC with DMA
    expert::adc::start_dma(PeripheralId::ADC1);

    // DMA is now running in background!
    // CPU is free to do other work while ADC samples continuously

    size_t half_transfer_count = 0;
    size_t full_transfer_count = 0;

    // ========================================================================
    // Main Processing Loop
    // ========================================================================

    while (true) {
        // ====================================================================
        // Check DMA Status (Half-Transfer or Transfer-Complete)
        // ====================================================================

        // In a real implementation, this would be interrupt-driven
        // For this example, we poll the DMA status flags

        bool half_transfer = check_dma_half_transfer();
        bool full_transfer = check_dma_full_transfer();

        if (half_transfer) {
            // First half of buffer is complete (samples 0 to 255)
            half_transfer_count++;

            // Copy data to processing buffer (avoid DMA overwrite)
            for (size_t i = 0; i < DMA_BUFFER_SIZE / 2; ++i) {
                process_buffer[i] = dma_buffer[i];
            }

            // Clear flag
            clear_dma_half_transfer_flag();

            // Blink LED
            led.toggle();
        }

        if (full_transfer) {
            // Second half of buffer is complete (samples 256 to 511)
            full_transfer_count++;

            // Copy data to processing buffer
            for (size_t i = 0; i < DMA_BUFFER_SIZE / 2; ++i) {
                process_buffer[i] = dma_buffer[DMA_BUFFER_SIZE / 2 + i];
            }

            // Clear flag
            clear_dma_full_transfer_flag();

            // Blink LED
            led.toggle();
        }

        // ====================================================================
        // Process ADC Data
        // ====================================================================

        if (half_transfer || full_transfer) {
            // Analyze buffer
            auto stats = analyze_buffer(process_buffer, DMA_BUFFER_SIZE / 2);

            // ================================================================
            // Data Processing Examples
            // ================================================================

            // 1. Threshold detection
            if (stats.max_value > 3500) {
                // High signal detected (> 2.8V)
                // Action: Trigger alarm, save event to flash
            }

            // 2. Signal quality check
            uint16_t noise = stats.peak_to_peak;
            if (noise < 50) {
                // Very stable signal (< 40 mV pk-pk)
                // Action: High-confidence measurement
            }

            // 3. RMS calculation for AC signals
            uint32_t rms_mv = (stats.rms_value * 3300) / 4095;
            // For AC signal: Vrms = (RMS - DC_offset) / sqrt(2)

            // 4. Frequency analysis (FFT)
            // - Apply windowing function (Hanning, Hamming)
            // - Compute FFT
            // - Find peak frequency

            // ================================================================
            // Data Logging
            // ================================================================

            // In a real application:
            // 1. Store to SD card in binary format
            // 2. Transmit via UART/SPI/USB
            // 3. Compress data (delta encoding, LZ77)
            // 4. Add timestamp from RTC

            // Example: UART transmission (pseudo-code)
            // uart_write_buffer(process_buffer, DMA_BUFFER_SIZE / 2);

            // ================================================================
            // Visual Feedback
            // ================================================================

            // Blink rate indicates signal strength
            uint32_t blink_delay = 500000;
            if (stats.avg_value > 2048) {
                // High average → fast blink
                blink_delay = 100000;
            } else if (stats.avg_value < 1000) {
                // Low average → slow blink
                blink_delay = 1000000;
            }

            for (volatile uint32_t i = 0; i < blink_delay; ++i);
        }

        // CPU can do other work here while DMA acquires samples!
        // - Update display
        // - Process user input
        // - Network communication
        // - etc.
    }

    return 0;
}

// ============================================================================
// DMA Helper Functions (Platform-Specific)
// ============================================================================

/**
 * @brief Check if DMA half-transfer interrupt flag is set
 */
bool check_dma_half_transfer() {
    // Platform-specific implementation
    // Example for STM32:
    // return (DMA1->ISR & DMA_ISR_HTIF1) != 0;
    return false;  // Placeholder
}

/**
 * @brief Check if DMA full-transfer interrupt flag is set
 */
bool check_dma_full_transfer() {
    // Platform-specific implementation
    // Example for STM32:
    // return (DMA1->ISR & DMA_ISR_TCIF1) != 0;
    return false;  // Placeholder
}

/**
 * @brief Clear DMA half-transfer interrupt flag
 */
void clear_dma_half_transfer_flag() {
    // Platform-specific implementation
    // Example for STM32:
    // DMA1->IFCR = DMA_IFCR_CHTIF1;
}

/**
 * @brief Clear DMA full-transfer interrupt flag
 */
void clear_dma_full_transfer_flag() {
    // Platform-specific implementation
    // Example for STM32:
    // DMA1->IFCR = DMA_IFCR_CTCIF1;
}

/**
 * Key Points:
 *
 * 1. Compile-time config: Zero runtime overhead
 * 2. DMA circular mode: Continuous background acquisition
 * 3. Ping-pong buffering: Process one half while DMA fills other half
 * 4. Hardware oversampling: 16-bit effective resolution on STM32G0
 * 5. Zero CPU overhead: DMA transfers data, CPU processes in background
 *
 * Expert API Benefits:
 * - Compile-time validation
 * - Maximum control
 * - Zero abstraction cost
 * - Platform-specific optimizations
 *
 * DMA Circular Buffer Pattern:
 *
 * Buffer: [0............255|256...........511]
 *          ^- First half  ^  ^- Second half ^
 *
 * 1. DMA fills first half (0-255) → Half-transfer interrupt
 * 2. CPU processes first half while DMA fills second half
 * 3. DMA fills second half (256-511) → Transfer-complete interrupt
 * 4. CPU processes second half while DMA wraps to first half
 * 5. Repeat forever (continuous streaming)
 *
 * Performance:
 *
 * STM32F4 (84 MHz APB2):
 * - ADC clock: 21 MHz (÷4)
 * - Conversion time: 12.5 + 84 = 96.5 ADC cycles
 * - Sample rate: 21 MHz / 96.5 = 218 kSPS
 * - With DMA: Zero CPU overhead!
 *
 * STM32G0 (64 MHz, 16x oversampling):
 * - ADC clock: 16 MHz (HSI16)
 * - Conversion time: (12.5 + 12.5) × 16 = 400 cycles
 * - Sample rate: 16 MHz / 400 = 40 kSPS @ 14-bit effective
 * - Memory: 40 kSPS × 2 bytes = 80 KB/s
 *
 * DMA vs Polling:
 * - Polling: 100% CPU, blocks other tasks
 * - Interrupt: ~5% CPU (context switching overhead)
 * - DMA: ~0% CPU (hardware transfers data)
 *
 * Buffer Size Selection:
 * - Too small: High interrupt rate, CPU overhead
 * - Too large: High latency, memory usage
 * - Recommended: 256-1024 samples (0.5-2 KB)
 * - Rule of thumb: 10-100 ms of data
 *
 * Oversampling on STM32G0:
 * - 2x: 13-bit effective (1 bit gain)
 * - 4x: 13-bit effective (1 bit gain)
 * - 16x: 14-bit effective (2 bits gain)
 * - 64x: 15-bit effective (3 bits gain)
 * - 256x: 16-bit effective (4 bits gain)
 * - Trade-off: Higher resolution vs lower sample rate
 *
 * Use Cases:
 *
 * 1. Data Logger (10 kSPS):
 *    - Temperature, pressure, humidity
 *    - Store to SD card with timestamp
 *    - Run for days/weeks on battery
 *
 * 2. Oscilloscope (100 kSPS):
 *    - Capture waveforms
 *    - Trigger on threshold
 *    - Display on LCD/OLED
 *
 * 3. Vibration Analysis (10 kSPS):
 *    - Accelerometer data
 *    - FFT for frequency analysis
 *    - Predictive maintenance
 *
 * 4. Audio Recorder (44.1 kSPS):
 *    - Microphone sampling
 *    - Store to SD card (WAV format)
 *    - Real-time audio processing
 *
 * Advanced Features:
 * - Timer-triggered ADC: Precise sampling rate
 * - Multi-channel scan: Sequential channel conversion
 * - Injected channels: High-priority emergency samples
 * - Analog watchdog: Hardware threshold detection
 * - Dual/Triple ADC: Simultaneous multi-channel
 */
