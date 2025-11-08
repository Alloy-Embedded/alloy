/**
 * @file adc_dma_example.cpp
 * @brief ADC with DMA example for SAME70
 *
 * This example demonstrates high-speed continuous ADC sampling using DMA.
 * Perfect for audio sampling, oscilloscope, data logging, signal processing.
 *
 * Features:
 * - Continuous ADC sampling at high rates (up to 2 MSPS)
 * - DMA transfers data to memory automatically (zero CPU overhead)
 * - Circular buffer for continuous streaming
 * - Multiple channel scanning
 *
 * Hardware Setup:
 * - Connect analog signals to AD0-AD3 (PD30, PA21, PB0, PE5) on AFEC0
 * - Voltage range: 0V to 3.3V
 * - Connect GND and ADVREF
 *
 * Use Cases:
 * - Audio sampling/recording
 * - Oscilloscope implementation
 * - High-speed data acquisition
 * - Signal processing applications
 * - Continuous sensor monitoring
 */

#include "hal/platform/same70/adc.hpp"
#include "hal/platform/same70/dma.hpp"

using namespace alloy::hal::same70;

/**
 * @brief High-speed ADC with DMA data acquisition
 */
class AdcDmaAcquisition {
   public:
    constexpr AdcDmaAcquisition(size_t buffer_size = 1024) : m_buffer_size(buffer_size) {}

    /**
     * @brief Initialize ADC and DMA
     */
    auto init() -> alloy::core::Result<void> {
        // Initialize ADC
        auto adc_result = m_adc.open();
        if (!adc_result.is_ok()) {
            return adc_result;
        }

        // Configure ADC for continuous conversion with DMA
        AdcConfig adc_config;
        adc_config.resolution = AdcResolution::Bits12;
        adc_config.trigger = AdcTrigger::Continuous;  // Free-running mode
        adc_config.sample_rate = 1000000;             // 1 MSPS
        adc_config.use_dma = true;

        auto config_result = m_adc.configure(adc_config);
        if (!config_result.is_ok()) {
            return config_result;
        }

        // Enable DMA on ADC
        auto dma_enable_result = m_adc.enableDma();
        if (!dma_enable_result.is_ok()) {
            return dma_enable_result;
        }

        // Initialize DMA channel
        auto dma_result = m_dma.open();
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        // Configure DMA for ADC -> Memory transfer
        DmaConfig dma_config;
        dma_config.direction = DmaDirection::PeripheralToMemory;
        dma_config.src_width = DmaWidth::Word;      // 32-bit reads from LCDR
        dma_config.dst_width = DmaWidth::HalfWord;  // 16-bit writes to buffer
        dma_config.peripheral = DmaPeripheralId::AFEC0;
        dma_config.src_increment = false;  // Read from same LCDR register
        dma_config.dst_increment = true;   // Write to sequential buffer

        return m_dma.configure(dma_config);
    }

    /**
     * @brief Start continuous ADC sampling with DMA
     *
     * @param buffer Destination buffer for ADC samples
     * @param size Buffer size in samples (uint16_t elements)
     */
    auto startAcquisition(uint16_t* buffer, size_t size) -> alloy::core::Result<void> {
        // Enable ADC channel(s)
        auto enable_result = m_adc.enableChannel(AdcChannel::CH0);
        if (!enable_result.is_ok()) {
            return enable_result;
        }

        // Start DMA transfer from ADC data register to buffer
        auto dma_result = m_dma.transfer(m_adc.getDmaSourceAddress(),  // Source: ADC LCDR
                                         buffer,                       // Destination: RAM buffer
                                         size                          // Number of samples
        );

        if (!dma_result.is_ok()) {
            return dma_result;
        }

        // Start ADC conversion (free-running)
        return m_adc.startConversion();
    }

    /**
     * @brief Stop acquisition
     */
    auto stopAcquisition() -> alloy::core::Result<void> {
        m_adc.disableChannel(AdcChannel::CH0);
        return m_dma.close();
    }

    /**
     * @brief Check if acquisition is complete
     */
    bool isComplete() const { return m_dma.isComplete(); }

    /**
     * @brief Wait for acquisition to complete
     */
    auto waitComplete(uint32_t timeout_ms = 5000) -> alloy::core::Result<void> {
        return m_dma.waitComplete(timeout_ms);
    }

    auto close() -> alloy::core::Result<void> {
        auto adc_result = m_adc.close();
        auto dma_result = m_dma.close();

        if (!adc_result.is_ok()) {
            return adc_result;
        }
        return dma_result;
    }

   private:
    Adc0 m_adc;
    DmaAdcChannel0 m_dma;
    size_t m_buffer_size;
};

/**
 * @brief Multi-channel ADC scanning with DMA
 *
 * Scans multiple ADC channels continuously and stores them in an interleaved buffer.
 */
class MultiChannelAdcDma {
   public:
    /**
     * @brief Initialize multi-channel ADC with DMA
     */
    auto init(const AdcChannel* channels, size_t num_channels) -> alloy::core::Result<void> {
        m_num_channels = num_channels;

        // Initialize ADC
        auto adc_result = m_adc.open();
        if (!adc_result.is_ok()) {
            return adc_result;
        }

        // Configure ADC
        AdcConfig config;
        config.resolution = AdcResolution::Bits12;
        config.trigger = AdcTrigger::Continuous;
        config.channels = static_cast<uint8_t>(num_channels);
        config.use_dma = true;

        auto config_result = m_adc.configure(config);
        if (!config_result.is_ok()) {
            return config_result;
        }

        // Enable all channels
        for (size_t i = 0; i < num_channels; ++i) {
            auto enable_result = m_adc.enableChannel(channels[i]);
            if (!enable_result.is_ok()) {
                return enable_result;
            }
        }

        // Initialize DMA
        auto dma_result = m_dma.open();
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        DmaConfig dma_config;
        dma_config.direction = DmaDirection::PeripheralToMemory;
        dma_config.src_width = DmaWidth::Word;
        dma_config.dst_width = DmaWidth::HalfWord;
        dma_config.peripheral = DmaPeripheralId::AFEC0;
        dma_config.src_increment = false;
        dma_config.dst_increment = true;

        return m_dma.configure(dma_config);
    }

    /**
     * @brief Start multi-channel acquisition
     *
     * Buffer will contain interleaved samples: [CH0, CH1, CH2, CH3, CH0, CH1, ...]
     */
    auto startAcquisition(uint16_t* buffer, size_t samples_per_channel)
        -> alloy::core::Result<void> {
        size_t total_samples = samples_per_channel * m_num_channels;

        auto dma_result = m_dma.transfer(m_adc.getDmaSourceAddress(), buffer, total_samples);

        if (!dma_result.is_ok()) {
            return dma_result;
        }

        return m_adc.startConversion();
    }

    bool isComplete() const { return m_dma.isComplete(); }

    auto close() -> alloy::core::Result<void> {
        m_adc.close();
        return m_dma.close();
    }

   private:
    Adc0 m_adc;
    DmaAdcChannel0 m_dma;
    size_t m_num_channels = 1;
};

/**
 * @brief Simple signal processing example
 */
namespace SignalProcessing {
/**
 * @brief Calculate average of buffer
 */
uint32_t calculateAverage(const uint16_t* buffer, size_t size) {
    uint32_t sum = 0;
    for (size_t i = 0; i < size; ++i) {
        sum += buffer[i];
    }
    return sum / size;
}

/**
 * @brief Find min and max values
 */
void findMinMax(const uint16_t* buffer, size_t size, uint16_t* min, uint16_t* max) {
    *min = 4095;
    *max = 0;

    for (size_t i = 0; i < size; ++i) {
        if (buffer[i] < *min)
            *min = buffer[i];
        if (buffer[i] > *max)
            *max = buffer[i];
    }
}

/**
 * @brief Calculate RMS (Root Mean Square)
 */
uint32_t calculateRms(const uint16_t* buffer, size_t size) {
    uint64_t sum_squares = 0;
    for (size_t i = 0; i < size; ++i) {
        uint32_t val = buffer[i];
        sum_squares += val * val;
    }
    uint32_t mean_square = sum_squares / size;

    // Simple integer square root
    uint32_t rms = 0;
    uint32_t bit = 1u << 30;
    while (bit > mean_square) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (mean_square >= rms + bit) {
            mean_square -= rms + bit;
            rms = (rms >> 1) + bit;
        } else {
            rms >>= 1;
        }
        bit >>= 2;
    }
    return rms;
}
}  // namespace SignalProcessing

/**
 * @brief Example usage
 */
int main() {
    // ========================================================================
    // Example 1: Single channel high-speed acquisition
    // ========================================================================
    {
        AdcDmaAcquisition acquisition(1024);

        [[maybe_unused]] auto init_result = acquisition.init();

        // Buffer for 1024 samples
        uint16_t sample_buffer[1024];

        // Start acquisition
        [[maybe_unused]] auto start_result = acquisition.startAcquisition(sample_buffer, 1024);

        // Wait for completion
        [[maybe_unused]] auto wait_result = acquisition.waitComplete();

        // Process data
        uint32_t average = SignalProcessing::calculateAverage(sample_buffer, 1024);
        uint16_t min, max;
        SignalProcessing::findMinMax(sample_buffer, 1024, &min, &max);
        uint32_t rms = SignalProcessing::calculateRms(sample_buffer, 1024);

        // Use the values
        (void)average;
        (void)min;
        (void)max;
        (void)rms;

        [[maybe_unused]] auto close_result = acquisition.close();
    }

    // ========================================================================
    // Example 2: Multi-channel scanning
    // ========================================================================
    {
        MultiChannelAdcDma multi_adc;

        // Configure 4 channels
        AdcChannel channels[] = {AdcChannel::CH0, AdcChannel::CH1, AdcChannel::CH2,
                                 AdcChannel::CH3};

        [[maybe_unused]] auto init_result = multi_adc.init(channels, 4);

        // Buffer for 256 samples per channel = 1024 total samples
        // Interleaved: [CH0, CH1, CH2, CH3, CH0, CH1, CH2, CH3, ...]
        uint16_t multi_buffer[1024];

        [[maybe_unused]] auto start_result = multi_adc.startAcquisition(multi_buffer, 256);

        // Wait for completion
        while (!multi_adc.isComplete()) {
            // Do other work while DMA is running
        }

        // Extract per-channel data
        uint16_t channel_0_data[256];
        for (size_t i = 0; i < 256; ++i) {
            channel_0_data[i] = multi_buffer[i * 4 + 0];  // Every 4th sample starting at 0
        }

        [[maybe_unused]] auto close_result = multi_adc.close();
    }

    // ========================================================================
    // Example 3: Continuous streaming with circular buffer
    // ========================================================================
    {
        AdcDmaAcquisition stream(512);

        [[maybe_unused]] auto init_result = stream.init();

        // Double buffer for continuous streaming
        uint16_t buffer_a[512];
        uint16_t buffer_b[512];
        bool use_buffer_a = true;

        // Start first acquisition
        [[maybe_unused]] auto start_result = stream.startAcquisition(buffer_a, 512);

        // Simulate continuous streaming
        for (int iteration = 0; iteration < 10; ++iteration) {
            // Wait for current buffer to fill
            [[maybe_unused]] auto wait_result = stream.waitComplete();

            // Process the filled buffer while next one fills
            if (use_buffer_a) {
                // Process buffer_a
                [[maybe_unused]] auto avg = SignalProcessing::calculateAverage(buffer_a, 512);

                // Start filling buffer_b
                stream.startAcquisition(buffer_b, 512);
            } else {
                // Process buffer_b
                [[maybe_unused]] auto avg = SignalProcessing::calculateAverage(buffer_b, 512);

                // Start filling buffer_a
                stream.startAcquisition(buffer_a, 512);
            }

            use_buffer_a = !use_buffer_a;
        }

        [[maybe_unused]] auto close_result = stream.close();
    }

    return 0;
}
