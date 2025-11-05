#pragma once

#include "hal/interface/clock.hpp"
#include "core/error.hpp"
#include "core/types.hpp"
#include <cstdint>

// Forward declare generated peripheral access
// Include the specific MCU's peripherals.hpp in implementation
namespace alloy::generated::stm32f103c8 {
    namespace rcc { struct Registers; extern Registers* const RCC; }
    namespace flash { struct Registers; extern Registers* const FLASH; }
}

namespace alloy::hal::st::stm32f1 {

using namespace alloy::core;

/**
 * STM32F1 Clock Configuration
 *
 * Type-safe clock configuration for STM32F1 family.
 * Supports:
 * - Internal RC oscillator (HSI) @ 8 MHz
 * - External crystal oscillator (HSE) @ 4-16 MHz
 * - PLL multiplication up to 72 MHz
 * - Configurable bus dividers (AHB, APB1, APB2)
 *
 * Usage:
 *   using Clock = Stm32F1Clock<8000000>;  // 8 MHz external crystal
 *   auto result = Clock::configure_72mhz();
 *   if (result.is_ok()) {
 *       // Clock configured to 72 MHz
 *   }
 */
template<u32 ExternalCrystalHz = 8000000>
class Stm32F1Clock {
public:
    // Clock source frequencies
    static constexpr u32 HSI_FREQ_HZ = 8000000;           // Internal RC @ 8 MHz
    static constexpr u32 HSE_FREQ_HZ = ExternalCrystalHz;  // External crystal
    static constexpr u32 MAX_FREQ_HZ = 72000000;          // Max CPU frequency

    /**
     * Quick configuration: 72 MHz from external crystal
     *
     * Standard configuration:
     * - HSE @ 8 MHz
     * - PLL x9 = 72 MHz
     * - AHB = 72 MHz
     * - APB1 = 36 MHz (max for STM32F1)
     * - APB2 = 72 MHz
     */
    static Result<void> configure_72mhz() {
        static_assert(HSE_FREQ_HZ == 8000000 || HSE_FREQ_HZ == 12000000 || HSE_FREQ_HZ == 16000000,
                     "For 72MHz, HSE must be 8, 12, or 16 MHz");

        hal::ClockConfig config{
            .source = hal::ClockSource::ExternalCrystal,
            .crystal_frequency_hz = HSE_FREQ_HZ,
            .pll_multiplier = calculate_pll_multiplier(HSE_FREQ_HZ, MAX_FREQ_HZ),
            .pll_divider = 1,
            .ahb_divider = 1,    // AHB = 72 MHz
            .apb1_divider = 2,   // APB1 = 36 MHz (max)
            .apb2_divider = 1,   // APB2 = 72 MHz
            .target_frequency_hz = MAX_FREQ_HZ
        };

        return configure(config);
    }

    /**
     * Quick configuration: 48 MHz from external crystal
     *
     * USB-compatible configuration (48 MHz required for USB)
     */
    static Result<void> configure_48mhz_usb() {
        hal::ClockConfig config{
            .source = hal::ClockSource::ExternalCrystal,
            .crystal_frequency_hz = HSE_FREQ_HZ,
            .pll_multiplier = calculate_pll_multiplier(HSE_FREQ_HZ, 48000000),
            .pll_divider = 1,
            .ahb_divider = 1,    // AHB = 48 MHz
            .apb1_divider = 2,   // APB1 = 24 MHz
            .apb2_divider = 1,   // APB2 = 48 MHz
            .target_frequency_hz = 48000000
        };

        return configure(config);
    }

    /**
     * Quick configuration: Internal RC oscillator (no external crystal)
     *
     * Uses HSI @ 8 MHz, PLL disabled
     * Useful for testing without external components
     */
    static Result<void> configure_hsi() {
        hal::ClockConfig config{
            .source = hal::ClockSource::InternalRC,
            .crystal_frequency_hz = 0,
            .pll_multiplier = 0,  // No PLL
            .pll_divider = 1,
            .ahb_divider = 1,
            .apb1_divider = 1,
            .apb2_divider = 1,
            .target_frequency_hz = HSI_FREQ_HZ
        };

        return configure(config);
    }

    /**
     * Configure system clock with custom settings
     */
    static Result<void> configure(const hal::ClockConfig& config) {
        using namespace alloy::generated::stm32f103c8;

        // Validate configuration
        if (config.target_frequency_hz > MAX_FREQ_HZ) {
            return Error(ErrorCode::ClockInvalidFrequency);
        }

        // Enable HSI (always needed during clock switching)
        rcc::RCC->CR |= (1 << 0);  // HSION
        while (!(rcc::RCC->CR & (1 << 1)));  // Wait HSIRDY

        // Configure flash latency BEFORE increasing frequency
        // STM32F1: 0 WS (0-24MHz), 1 WS (24-48MHz), 2 WS (48-72MHz)
        u32 flash_latency = 0;
        if (config.target_frequency_hz > 48000000) {
            flash_latency = 2;
        } else if (config.target_frequency_hz > 24000000) {
            flash_latency = 1;
        }
        flash::FLASH->ACR = (flash::FLASH->ACR & ~0x7) | flash_latency;

        // Configure clock source
        if (config.source == hal::ClockSource::ExternalCrystal) {
            // Enable HSE
            rcc::RCC->CR |= (1 << 16);  // HSEON
            u32 timeout = 100000;
            while (!(rcc::RCC->CR & (1 << 17)) && timeout--) {}  // Wait HSERDY
            if (timeout == 0) {
                return Error(ErrorCode::ClockSourceNotReady);
            }

            // Configure PLL if enabled
            if (config.pll_multiplier > 0) {
                // Disable PLL before configuration
                rcc::RCC->CR &= ~(1 << 24);  // PLLON = 0
                while (rcc::RCC->CR & (1 << 25)) {}  // Wait !PLLRDY

                // Configure PLL: HSE as source, set multiplier
                u32 pll_mul = (config.pll_multiplier - 2) & 0xF;  // 2-16 -> 0-14
                rcc::RCC->CFGR = (rcc::RCC->CFGR & ~(0xF << 18)) | (pll_mul << 18);  // PLLMUL
                rcc::RCC->CFGR |= (1 << 16);   // PLLSRC = HSE
                rcc::RCC->CFGR &= ~(1 << 17);  // PLLXTPRE = HSE not divided

                // Enable PLL
                rcc::RCC->CR |= (1 << 24);  // PLLON
                timeout = 100000;
                while (!(rcc::RCC->CR & (1 << 25)) && timeout--) {}  // Wait PLLRDY
                if (timeout == 0) {
                    return Error(ErrorCode::PllLockFailed);
                }
            }
        }

        // Configure bus dividers
        u32 cfgr = rcc::RCC->CFGR;

        // AHB divider (HPRE)
        u32 ahb_bits = calculate_ahb_bits(config.ahb_divider);
        cfgr = (cfgr & ~(0xF << 4)) | (ahb_bits << 4);

        // APB1 divider (PPRE1) - max 36 MHz
        u32 apb1_bits = calculate_apb_bits(config.apb1_divider);
        cfgr = (cfgr & ~(0x7 << 8)) | (apb1_bits << 8);

        // APB2 divider (PPRE2)
        u32 apb2_bits = calculate_apb_bits(config.apb2_divider);
        cfgr = (cfgr & ~(0x7 << 11)) | (apb2_bits << 11);

        // Select system clock source
        u8 sw_bits = 0;  // HSI
        if (config.source == hal::ClockSource::ExternalCrystal) {
            sw_bits = (config.pll_multiplier > 0) ? 2 : 1;  // PLL or HSE
        }
        cfgr = (cfgr & ~0x3) | sw_bits;

        rcc::RCC->CFGR = cfgr;

        // Wait for switch to complete
        u32 timeout = 100000;
        u32 expected_sws = sw_bits << 2;
        while ((rcc::RCC->CFGR & (0x3 << 2)) != expected_sws && timeout--) {}
        if (timeout == 0) {
            return Error(ErrorCode::ClockSourceNotReady);
        }

        // Update current frequencies
        current_frequencies_.system = config.target_frequency_hz;
        current_frequencies_.ahb = config.target_frequency_hz / config.ahb_divider;
        current_frequencies_.apb1 = current_frequencies_.ahb / config.apb1_divider;
        current_frequencies_.apb2 = current_frequencies_.ahb / config.apb2_divider;

        return Ok();
    }

    /**
     * Get current system frequency
     */
    static u32 get_frequency() {
        return current_frequencies_.system;
    }

    /**
     * Get AHB bus frequency
     */
    static u32 get_ahb_frequency() {
        return current_frequencies_.ahb;
    }

    /**
     * Get APB1 bus frequency
     */
    static u32 get_apb1_frequency() {
        return current_frequencies_.apb1;
    }

    /**
     * Get APB2 bus frequency
     */
    static u32 get_apb2_frequency() {
        return current_frequencies_.apb2;
    }

    /**
     * Enable peripheral clock
     */
    static Result<void> enable_peripheral(hal::Peripheral peripheral) {
        using namespace alloy::generated::stm32f103c8;

        u16 periph_id = static_cast<u16>(peripheral);
        u8 bit = periph_id & 0xFF;

        // Determine which bus the peripheral is on
        if (periph_id >= 0x0100 && periph_id < 0x0200) {
            // APB2 peripherals (UART1, TIM1, ADC, etc.)
            rcc::RCC->APB2ENR |= (1 << (bit % 32));
        } else if (periph_id >= 0x0200 && periph_id < 0x0300) {
            // APB1 peripherals (UART2/3, TIM2-4, I2C, SPI2/3)
            rcc::RCC->APB1ENR |= (1 << (bit % 32));
        } else {
            // GPIO and others on APB2
            rcc::RCC->APB2ENR |= (1 << (bit % 32));
        }

        return Ok();
    }

    /**
     * Disable peripheral clock
     */
    static Result<void> disable_peripheral(hal::Peripheral peripheral) {
        using namespace alloy::generated::stm32f103c8;

        u16 periph_id = static_cast<u16>(peripheral);
        u8 bit = periph_id & 0xFF;

        if (periph_id >= 0x0100 && periph_id < 0x0200) {
            rcc::RCC->APB2ENR &= ~(1 << (bit % 32));
        } else if (periph_id >= 0x0200 && periph_id < 0x0300) {
            rcc::RCC->APB1ENR &= ~(1 << (bit % 32));
        } else {
            rcc::RCC->APB2ENR &= ~(1 << (bit % 32));
        }

        return Ok();
    }

private:
    struct Frequencies {
        u32 system = HSI_FREQ_HZ;
        u32 ahb = HSI_FREQ_HZ;
        u32 apb1 = HSI_FREQ_HZ;
        u32 apb2 = HSI_FREQ_HZ;
    };

    static inline Frequencies current_frequencies_;

    /**
     * Calculate PLL multiplier for target frequency
     */
    static constexpr u8 calculate_pll_multiplier(u32 input_hz, u32 target_hz) {
        u8 mul = static_cast<u8>((target_hz + input_hz / 2) / input_hz);
        return (mul < 2) ? 2 : (mul > 16) ? 16 : mul;
    }

    /**
     * Calculate AHB divider bits
     */
    static constexpr u32 calculate_ahb_bits(u8 divider) {
        if (divider == 1) return 0b0000;
        if (divider == 2) return 0b1000;
        if (divider == 4) return 0b1001;
        if (divider == 8) return 0b1010;
        if (divider == 16) return 0b1011;
        if (divider == 64) return 0b1100;
        if (divider == 128) return 0b1101;
        if (divider == 256) return 0b1110;
        if (divider == 512) return 0b1111;
        return 0b0000;
    }

    /**
     * Calculate APB divider bits
     */
    static constexpr u32 calculate_apb_bits(u8 divider) {
        if (divider == 1) return 0b000;
        if (divider == 2) return 0b100;
        if (divider == 4) return 0b101;
        if (divider == 8) return 0b110;
        if (divider == 16) return 0b111;
        return 0b000;
    }
};

// Type aliases for common configurations
using Stm32F1Clock8MHz = Stm32F1Clock<8000000>;
using Stm32F1Clock12MHz = Stm32F1Clock<12000000>;
using Stm32F1Clock16MHz = Stm32F1Clock<16000000>;

} // namespace alloy::hal::st::stm32f1
