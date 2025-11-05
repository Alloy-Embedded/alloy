#pragma once

#include "hal/interface/clock.hpp"
#include "core/error.hpp"
#include "core/types.hpp"
#include <cstdint>

// Forward declare generated peripheral access
namespace alloy::generated::stm32f407 {
    namespace rcc { struct Registers; extern Registers* const RCC; }
    namespace flash { struct Registers; extern Registers* const FLASH; }
    namespace pwr { struct Registers; extern Registers* const PWR; }
}

namespace alloy::hal::st::stm32f4 {

using namespace alloy::core;

/**
 * STM32F4 Clock Configuration
 *
 * Type-safe clock configuration for STM32F4 family.
 * Supports:
 * - Internal RC oscillator (HSI) @ 16 MHz
 * - External crystal oscillator (HSE) @ 4-26 MHz
 * - Advanced PLL with VCO, M, N, P, Q dividers
 * - Up to 168 MHz (STM32F407) or 180 MHz (STM32F429)
 * - Voltage scaling for optimal power/performance
 *
 * Usage:
 *   using Clock = Stm32F4Clock<8000000, 168000000>;
 *   auto result = Clock::configure_max_performance();
 *   if (result.is_ok()) {
 *       // Clock configured to 168 MHz
 *   }
 */
template<u32 ExternalCrystalHz = 8000000, u32 MaxFreqHz = 168000000>
class Stm32F4Clock {
public:
    // Clock source frequencies
    static constexpr u32 HSI_FREQ_HZ = 16000000;          // Internal RC @ 16 MHz
    static constexpr u32 HSE_FREQ_HZ = ExternalCrystalHz;  // External crystal
    static constexpr u32 MAX_FREQ_HZ = MaxFreqHz;          // Max CPU frequency
    static constexpr u32 VCO_MIN_HZ = 100000000;          // VCO minimum
    static constexpr u32 VCO_MAX_HZ = 432000000;          // VCO maximum

    /**
     * Quick configuration: Maximum performance
     *
     * Standard configuration for STM32F407 @ 168 MHz:
     * - HSE @ 8 MHz
     * - VCO = HSE / M * N = 8MHz / 4 * 168 = 336 MHz
     * - System = VCO / P = 336 MHz / 2 = 168 MHz
     * - USB/SDIO = VCO / Q = 336 MHz / 7 = 48 MHz (USB compatible)
     * - AHB = 168 MHz
     * - APB1 = 42 MHz (max)
     * - APB2 = 84 MHz
     */
    static Result<void> configure_max_performance() {
        PllSettings pll{};

        if constexpr (HSE_FREQ_HZ == 8000000 && MAX_FREQ_HZ == 168000000) {
            pll.M = 4;    // VCO input = 8MHz / 4 = 2 MHz
            pll.N = 168;  // VCO output = 2MHz * 168 = 336 MHz
            pll.P = 2;    // System = 336MHz / 2 = 168 MHz
            pll.Q = 7;    // USB = 336MHz / 7 = 48 MHz
        } else if constexpr (HSE_FREQ_HZ == 25000000 && MAX_FREQ_HZ == 168000000) {
            pll.M = 25;   // VCO input = 25MHz / 25 = 1 MHz
            pll.N = 336;  // VCO output = 1MHz * 336 = 336 MHz
            pll.P = 2;    // System = 336MHz / 2 = 168 MHz
            pll.Q = 7;    // USB = 336MHz / 7 = 48 MHz
        } else {
            // Auto-calculate for other frequencies
            pll = calculate_pll_settings(HSE_FREQ_HZ, MAX_FREQ_HZ);
        }

        hal::ClockConfig config{
            .source = hal::ClockSource::ExternalCrystal,
            .crystal_frequency_hz = HSE_FREQ_HZ,
            .pll_multiplier = pll.N,
            .pll_divider = pll.P,
            .ahb_divider = 1,    // AHB = 168 MHz
            .apb1_divider = 4,   // APB1 = 42 MHz (max)
            .apb2_divider = 2,   // APB2 = 84 MHz
            .target_frequency_hz = MAX_FREQ_HZ
        };

        return configure(config, pll);
    }

    /**
     * Quick configuration: USB-compatible 48 MHz
     *
     * Configuration for USB device applications
     */
    static Result<void> configure_48mhz_usb() {
        PllSettings pll{
            .M = 8,   // VCO input = 8MHz / 8 = 1 MHz
            .N = 192, // VCO output = 1MHz * 192 = 192 MHz
            .P = 4,   // System = 192MHz / 4 = 48 MHz
            .Q = 4    // USB = 192MHz / 4 = 48 MHz
        };

        hal::ClockConfig config{
            .source = hal::ClockSource::ExternalCrystal,
            .crystal_frequency_hz = HSE_FREQ_HZ,
            .pll_multiplier = pll.N,
            .pll_divider = pll.P,
            .ahb_divider = 1,
            .apb1_divider = 2,
            .apb2_divider = 1,
            .target_frequency_hz = 48000000
        };

        return configure(config, pll);
    }

    /**
     * Quick configuration: Low power mode (HSI, no PLL)
     */
    static Result<void> configure_low_power() {
        hal::ClockConfig config{
            .source = hal::ClockSource::InternalRC,
            .crystal_frequency_hz = 0,
            .pll_multiplier = 0,
            .pll_divider = 1,
            .ahb_divider = 1,
            .apb1_divider = 1,
            .apb2_divider = 1,
            .target_frequency_hz = HSI_FREQ_HZ
        };

        PllSettings pll{};  // Not used
        return configure(config, pll);
    }

    /**
     * Configure system clock with custom settings
     */
    static Result<void> configure(const hal::ClockConfig& config, const PllSettings& pll) {
        using namespace alloy::generated::stm32f407;

        // Validate configuration
        if (config.target_frequency_hz > MAX_FREQ_HZ) {
            return Error(ErrorCode::ClockInvalidFrequency);
        }

        // Enable HSI (always needed during clock switching)
        rcc::RCC->CR |= (1 << 0);  // HSION
        while (!(rcc::RCC->CR & (1 << 1))) {}  // Wait HSIRDY

        // Configure voltage scaling for performance
        // Scale 1 for frequencies > 144 MHz
        rcc::RCC->APB1ENR |= (1 << 28);  // PWREN
        if (config.target_frequency_hz > 144000000) {
            pwr::PWR->CR |= (3 << 14);  // VOS = Scale 1
        } else {
            pwr::PWR->CR |= (2 << 14);  // VOS = Scale 2
        }

        // Configure flash latency BEFORE increasing frequency
        // STM32F4 @ 3.3V: 0WS (0-30MHz), 1WS (30-60MHz), 2WS (60-90MHz),
        //                 3WS (90-120MHz), 4WS (120-150MHz), 5WS (150-180MHz)
        u32 flash_latency = calculate_flash_latency(config.target_frequency_hz);
        flash::FLASH->ACR = (flash::FLASH->ACR & ~0x7) |
                           flash_latency |
                           (1 << 8) |   // PRFTEN (prefetch enable)
                           (1 << 9) |   // ICEN (instruction cache enable)
                           (1 << 10);   // DCEN (data cache enable)

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

                // Configure main PLL
                // PLLCFGR = PLLSRC | PLLM | PLLN | PLLP | PLLQ
                u32 pllcfgr = (1 << 22) |                       // PLLSRC = HSE
                             (pll.M & 0x3F) |                   // PLLM (0-5 bits)
                             ((pll.N & 0x1FF) << 6) |           // PLLN (6-14 bits)
                             (((pll.P / 2 - 1) & 0x3) << 16) |  // PLLP (16-17 bits)
                             ((pll.Q & 0xF) << 24);             // PLLQ (24-27 bits)

                rcc::RCC->PLLCFGR = pllcfgr;

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

        // APB1 divider (PPRE1) - max 42 MHz
        u32 apb1_bits = calculate_apb_bits(config.apb1_divider);
        cfgr = (cfgr & ~(0x7 << 10)) | (apb1_bits << 10);

        // APB2 divider (PPRE2) - max 84 MHz
        u32 apb2_bits = calculate_apb_bits(config.apb2_divider);
        cfgr = (cfgr & ~(0x7 << 13)) | (apb2_bits << 13);

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

    // Frequency getters
    static u32 get_frequency() { return current_frequencies_.system; }
    static u32 get_ahb_frequency() { return current_frequencies_.ahb; }
    static u32 get_apb1_frequency() { return current_frequencies_.apb1; }
    static u32 get_apb2_frequency() { return current_frequencies_.apb2; }

    /**
     * Enable peripheral clock
     */
    static Result<void> enable_peripheral(hal::Peripheral peripheral) {
        using namespace alloy::generated::stm32f407;

        u16 periph_id = static_cast<u16>(peripheral);
        u8 bit = periph_id & 0xFF;

        // Determine which bus
        if (periph_id < 0x0100) {
            // AHB1 (GPIO)
            rcc::RCC->AHB1ENR |= (1 << bit);
        } else if (periph_id >= 0x0100 && periph_id < 0x0200) {
            // APB2
            rcc::RCC->APB2ENR |= (1 << (bit % 32));
        } else if (periph_id >= 0x0200 && periph_id < 0x0300) {
            // APB1
            rcc::RCC->APB1ENR |= (1 << (bit % 32));
        }

        return Ok();
    }

    /**
     * Disable peripheral clock
     */
    static Result<void> disable_peripheral(hal::Peripheral peripheral) {
        using namespace alloy::generated::stm32f407;

        u16 periph_id = static_cast<u16>(peripheral);
        u8 bit = periph_id & 0xFF;

        if (periph_id < 0x0100) {
            rcc::RCC->AHB1ENR &= ~(1 << bit);
        } else if (periph_id >= 0x0100 && periph_id < 0x0200) {
            rcc::RCC->APB2ENR &= ~(1 << (bit % 32));
        } else if (periph_id >= 0x0200 && periph_id < 0x0300) {
            rcc::RCC->APB1ENR &= ~(1 << (bit % 32));
        }

        return Ok();
    }

private:
    struct PllSettings {
        u8 M = 8;   // Input divider (2-63), VCO input = HSE / M
        u16 N = 336; // Multiplier (50-432), VCO output = (HSE / M) * N
        u8 P = 2;   // System divider (2,4,6,8), SYSCLK = VCO / P
        u8 Q = 7;   // USB/SDIO divider (2-15), USB = VCO / Q
    };

    struct Frequencies {
        u32 system = HSI_FREQ_HZ;
        u32 ahb = HSI_FREQ_HZ;
        u32 apb1 = HSI_FREQ_HZ;
        u32 apb2 = HSI_FREQ_HZ;
    };

    static inline Frequencies current_frequencies_;

    static constexpr u32 calculate_flash_latency(u32 freq_hz) {
        if (freq_hz <= 30000000) return 0;
        if (freq_hz <= 60000000) return 1;
        if (freq_hz <= 90000000) return 2;
        if (freq_hz <= 120000000) return 3;
        if (freq_hz <= 150000000) return 4;
        return 5;
    }

    static constexpr PllSettings calculate_pll_settings(u32 input_hz, u32 target_hz) {
        PllSettings pll{};
        // Simple calculation for common cases
        pll.M = (input_hz / 1000000);  // Target ~1-2 MHz VCO input
        pll.N = (target_hz * 2) / (input_hz / pll.M);
        pll.P = 2;
        pll.Q = (pll.N * (input_hz / pll.M)) / 48000000;  // Target 48 MHz for USB
        return pll;
    }

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
using Stm32F407Clock = Stm32F4Clock<8000000, 168000000>;   // STM32F407 @ 168 MHz
using Stm32F429Clock = Stm32F4Clock<8000000, 180000000>;   // STM32F429 @ 180 MHz

} // namespace alloy::hal::st::stm32f4
