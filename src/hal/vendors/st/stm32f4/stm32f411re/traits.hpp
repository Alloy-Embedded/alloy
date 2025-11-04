#pragma once

#include <cstdint>

namespace alloy::hal::stm32f411::stm32f411re {

// ============================================================================
// MCU Characteristics for STM32F411RE
// Auto-generated from SVD: STM32F411
// ============================================================================

struct Traits {
    // Device identification
    static constexpr const char* DEVICE_NAME = "STM32F411RE";
    static constexpr const char* VENDOR = "STMicroelectronics";
    static constexpr const char* FAMILY = "STM32F411";
    static constexpr const char* SERIES = "STM32F411";

    // Package information
    static constexpr const char* PACKAGE = "LQFP64";
    static constexpr uint8_t PIN_COUNT = 51;

    // Memory configuration
    static constexpr uint32_t FLASH_SIZE = 0 * 1024;      // 0 KB
    static constexpr uint32_t SRAM_SIZE = 0 * 1024;       // 0 KB
    static constexpr uint32_t FLASH_BASE = 0x08000000;
    static constexpr uint32_t SRAM_BASE = 0x20000000;

    // Clock configuration
    static constexpr uint32_t MAX_FREQ_HZ = 72'000'000;    // 72 MHz max
    static constexpr uint32_t HSI_FREQ_HZ = 8'000'000;     // 8 MHz HSI
    static constexpr uint32_t LSI_FREQ_HZ = 40'000;        // 40 kHz LSI

    // Peripheral availability
    struct Peripherals {
        static constexpr uint8_t UART_COUNT = 3;
        static constexpr uint8_t I2C_COUNT = 3;
        static constexpr uint8_t SPI_COUNT = 5;
        static constexpr uint8_t ADC_COUNT = 2;
        static constexpr uint8_t ADC_CHANNELS = 17;
        static constexpr uint8_t TIMER_COUNT = 9;
        static constexpr uint8_t DMA_CHANNELS = 0;

        // Feature flags
        static constexpr bool HAS_USB = false;
        static constexpr bool HAS_CAN = false;
        static constexpr bool HAS_DAC = false;
        static constexpr bool HAS_RTC = true;
        static constexpr bool HAS_FSMC = false;
    };
};

}  // namespace alloy::hal::stm32f411::stm32f411re
