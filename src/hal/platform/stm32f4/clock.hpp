/**
 * @file clock.hpp
 * @brief Reset and Clock Control (RCC) for STM32F4
 *
 * This file implements the clock management for STM32F4 using static methods with
 * ZERO virtual functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Compile-time clock configuration
 * - Type-safe clock source selection
 * - Zero overhead abstractions
 * - Peripheral clock management
 *
 * STM32F4 Clock Architecture:
 * HSI (High Speed Internal): 16 MHz RC oscillator
 * HSE (High Speed External): 4-26 MHz crystal/oscillator
 * PLL: Up to 168 MHz (STM32F4) or 180 MHz (STM32F429)
 * AHB Bus: Up to 168/180 MHz
 * APB1 Bus: Up to 42/45 MHz (max)
 * APB2 Bus: Up to 84/90 MHz (max)
 *
 * Typical Clock Tree:
 * HSE (8 MHz) -> PLL (168 MHz) -> AHB (168 MHz) -> APB1 (42 MHz), APB2 (84 MHz)
 *
 * Auto-generated from: stm32f4
 * Generator: generate_platform_clock.py
 * Generated: 2025-11-07 18:20:19
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "hal/types.hpp"

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/st/stm32f4/registers/rcc_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/st/stm32f4/bitfields/rcc_bitfields.hpp"


namespace alloy::hal::stm32f4 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::st::stm32f4;

// Namespace alias for bitfield access
namespace rcc = alloy::hal::st::stm32f4::rcc;

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief System Clock Source
 */
enum class SystemClockSource : uint8_t {
    HSI = 0,  ///< High Speed Internal (16 MHz)
    HSE = 1,  ///< High Speed External (crystal/oscillator)
    PLL = 2,  ///< PLL output
};

/**
 * @brief AHB Prescaler
 */
enum class AhbPrescaler : uint8_t {
    DIV_1 = 0,    ///< SYSCLK not divided
    DIV_2 = 8,    ///< SYSCLK divided by 2
    DIV_4 = 9,    ///< SYSCLK divided by 4
    DIV_8 = 10,   ///< SYSCLK divided by 8
    DIV_16 = 11,  ///< SYSCLK divided by 16
};

/**
 * @brief APB Prescaler (APB1 and APB2)
 */
enum class ApbPrescaler : uint8_t {
    DIV_1 = 0,   ///< AHB not divided
    DIV_2 = 4,   ///< AHB divided by 2
    DIV_4 = 5,   ///< AHB divided by 4
    DIV_8 = 6,   ///< AHB divided by 8
    DIV_16 = 7,  ///< AHB divided by 16
};


// ============================================================================
// Additional Structures
// ============================================================================

/**
 * @brief PLL Configuration\n\nPLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN\nSYSCLK = PLL_VCO /
 * PLLP\nUSB OTG FS, SDIO = PLL_VCO / PLLQ\n\nFor HSE=8MHz, SYSCLK=168MHz:\n- PLLM = 8\n- PLLN =
 * 336\n- PLLP = 2\nResult: (8MHz / 8) * 336 / 2 = 168 MHz
 */
struct PllConfig {
    uint8_t pllm = 8;     ///< PLL input divider (2-63)
    uint16_t plln = 336;  ///< PLL multiplier (50-432)
    uint8_t pllp = 2;     ///< PLL output divider (2, 4, 6, 8)
    uint8_t pllq = 7;     ///< PLL USB/SDIO divider (2-15)
};


/**
 * @brief Complete Clock Configuration
 */
struct ClockConfig {
    SystemClockSource sysclk_source = SystemClockSource::PLL;  ///< System clock source
    bool use_hse = true;                                       ///< Use external crystal
    uint32_t hse_freq_hz = 8000000;                            ///< HSE frequency (if used)
    PllConfig pll = {8, 336, 2, 7};                            ///< PLL config for 168 MHz
    AhbPrescaler ahb_prescaler = AhbPrescaler::DIV_1;          ///< AHB = 168 MHz
    ApbPrescaler apb1_prescaler = ApbPrescaler::DIV_4;         ///< APB1 = 42 MHz
    ApbPrescaler apb2_prescaler = ApbPrescaler::DIV_2;         ///< APB2 = 84 MHz
};


/**
 * @brief Reset and Clock Control (RCC) for STM32F4
 *
 * Singleton class for managing system and peripheral clocks.
 * All methods are static for zero-overhead access.
 */
class Clock {
   public:
    // Compile-time constants
    static constexpr uint32_t RCC_BASE = 0x40023800;  ///< RCC base address
    static constexpr uint32_t HSI_FREQ = 16000000;    ///< 16 MHz internal oscillator

    /**
     * @brief Initialize system clocks\n\nConfigures HSE/HSI, PLL, and bus prescalers.\nThis should
     * be called early in system initialization.
     *
     * @param config Clock configuration
     * @return Result<void, ErrorCode>     */
    static Result<void, ErrorCode> initialize(const ClockConfig& config) {
        auto* rcc_regs = get_rcc();

        // Save configuration
        s_config = config;

        // Step 1: Enable HSE if used
        if (config.use_hse) {
            rcc_regs->CR |= rcc::cr::HSEON::mask;

            // Wait for HSE ready
            uint32_t timeout = 100000;
            while ((rcc_regs->CR & rcc::cr::HSERDY::mask) == 0 && timeout > 0) {
                --timeout;
            }
            if (timeout == 0) {
                return Err(ErrorCode::Timeout);
            }
        }

        // Step 2: Configure PLL if needed
        if (config.sysclk_source == SystemClockSource::PLL) {
            // Disable PLL before configuration
            rcc_regs->CR &= ~rcc::cr::PLLON::mask;

            // Configure PLL
            uint32_t pllcfgr = 0;

            // PLL source (HSE or HSI)
            if (config.use_hse) {
                pllcfgr = rcc::pllcfgr::PLLSRC::set(pllcfgr);  // HSE
            }

            // PLL parameters - manually set bit fields
            // PLLM (bits 0-5): 6 bits
            pllcfgr |= (config.pll.pllm & 0x3F);

            // PLLN (bits 6-14): 9 bits
            pllcfgr |= ((config.pll.plln & 0x1FF) << 6);

            // PLLP (bits 16-17): 2 bits, encoding: 0=/2, 1=/4, 2=/6, 3=/8
            uint32_t pllp_value = (config.pll.pllp / 2) - 1;
            pllcfgr |= ((pllp_value & 0x3) << 16);

            // PLLQ (bits 24-27): 4 bits
            pllcfgr |= ((config.pll.pllq & 0xF) << 24);

            rcc_regs->PLLCFGR = pllcfgr;

            // Enable PLL
            rcc_regs->CR |= rcc::cr::PLLON::mask;

            // Wait for PLL ready
            uint32_t timeout = 100000;
            while ((rcc_regs->CR & rcc::cr::PLLRDY::mask) == 0 && timeout > 0) {
                --timeout;
            }
            if (timeout == 0) {
                return Err(ErrorCode::Timeout);
            }
        }

        // Step 3: Configure Flash latency (required before increasing frequency)
        // This should be done based on voltage and frequency
        // For 168 MHz @ 3.3V: 5 wait states
        // Note: Flash registers are separate, this is simplified

        // Step 4: Configure bus prescalers
        uint32_t cfgr = rcc_regs->CFGR;

        // AHB prescaler
        cfgr = rcc::cfgr::HPRE::write(cfgr, static_cast<uint32_t>(config.ahb_prescaler));

        // APB1 prescaler (low speed, max 42 MHz)
        cfgr = rcc::cfgr::PPRE1::write(cfgr, static_cast<uint32_t>(config.apb1_prescaler));

        // APB2 prescaler (high speed, max 84 MHz)
        cfgr = rcc::cfgr::PPRE2::write(cfgr, static_cast<uint32_t>(config.apb2_prescaler));

        rcc_regs->CFGR = cfgr;

        // Step 5: Switch system clock source
        cfgr = rcc_regs->CFGR;
        // Clear SW bits (0-1) and set new value
        cfgr &= ~0x3;
        cfgr |= (static_cast<uint32_t>(config.sysclk_source) & 0x3);
        rcc_regs->CFGR = cfgr;

        // Wait for system clock switch
        uint32_t timeout = 100000;
        uint32_t expected_sws = static_cast<uint32_t>(config.sysclk_source);
        // SWS bits are 2-3
        while ((((rcc_regs->CFGR >> 2) & 0x3) != expected_sws) && timeout > 0) {
            --timeout;
        }
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }

        s_initialized = true;
        return Ok();
    }

    /**
     * @brief Enable peripheral clock on AHB1 bus
     *
     * @param peripheral_bit Peripheral bit position
     * @return Result<void, ErrorCode>     */
    static Result<void, ErrorCode> enableAhb1Clock(uint32_t peripheral_bit) {
        if (!s_initialized) {
            return Err(ErrorCode::NotInitialized);
        }
        auto* rcc_regs = get_rcc();
        rcc_regs->AHB1ENR |= (1u << peripheral_bit);

        return Ok();
    }

    /**
     * @brief Enable peripheral clock on APB1 bus
     *
     * @param peripheral_bit Peripheral bit position
     * @return Result<void, ErrorCode>     */
    static Result<void, ErrorCode> enableApb1Clock(uint32_t peripheral_bit) {
        if (!s_initialized) {
            return Err(ErrorCode::NotInitialized);
        }
        auto* rcc_regs = get_rcc();
        rcc_regs->APB1ENR |= (1u << peripheral_bit);

        return Ok();
    }

    /**
     * @brief Enable peripheral clock on APB2 bus
     *
     * @param peripheral_bit Peripheral bit position
     * @return Result<void, ErrorCode>     */
    static Result<void, ErrorCode> enableApb2Clock(uint32_t peripheral_bit) {
        if (!s_initialized) {
            return Err(ErrorCode::NotInitialized);
        }
        auto* rcc_regs = get_rcc();
        rcc_regs->APB2ENR |= (1u << peripheral_bit);

        return Ok();
    }

    /**
     * @brief Get system clock frequency in Hz
     *
     * @return uint32_t     */
    static uint32_t getSystemClockFrequency() {
        if (!s_initialized) {
            return 0;
        }

        uint32_t sysclk = 0;

        switch (s_config.sysclk_source) {
            case SystemClockSource::HSI:
                sysclk = HSI_FREQ;
                break;

            case SystemClockSource::HSE:
                sysclk = s_config.hse_freq_hz;
                break;

            case SystemClockSource::PLL: {
                // PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
                // SYSCLK = PLL_VCO / PLLP
                uint32_t pll_source = s_config.use_hse ? s_config.hse_freq_hz : HSI_FREQ;
                uint32_t pll_vco = (pll_source / s_config.pll.pllm) * s_config.pll.plln;
                sysclk = pll_vco / s_config.pll.pllp;
                break;
            }
        }

        return sysclk;
    }

    /**
     * @brief Get AHB bus clock frequency in Hz
     *
     * @return uint32_t     */
    static uint32_t getAhbClockFrequency() {
        uint32_t sysclk = getSystemClockFrequency();

        // Apply AHB prescaler
        uint32_t ahb_div = 1;
        if (static_cast<uint8_t>(s_config.ahb_prescaler) >= 8) {
            ahb_div = 1u << (static_cast<uint8_t>(s_config.ahb_prescaler) - 7);
        }

        return sysclk / ahb_div;
    }

    /**
     * @brief Get APB1 bus clock frequency in Hz
     *
     * @return uint32_t     */
    static uint32_t getApb1ClockFrequency() {
        uint32_t ahb_freq = getAhbClockFrequency();

        // Apply APB1 prescaler
        uint32_t apb1_div = 1;
        if (static_cast<uint8_t>(s_config.apb1_prescaler) >= 4) {
            apb1_div = 1u << (static_cast<uint8_t>(s_config.apb1_prescaler) - 3);
        }

        return ahb_freq / apb1_div;
    }

    /**
     * @brief Get APB2 bus clock frequency in Hz
     *
     * @return uint32_t     */
    static uint32_t getApb2ClockFrequency() {
        uint32_t ahb_freq = getAhbClockFrequency();

        // Apply APB2 prescaler
        uint32_t apb2_div = 1;
        if (static_cast<uint8_t>(s_config.apb2_prescaler) >= 4) {
            apb2_div = 1u << (static_cast<uint8_t>(s_config.apb2_prescaler) - 3);
        }

        return ahb_freq / apb2_div;
    }

    /**
     * @brief Check if clocks are initialized
     *
     * @return bool     */
    static bool isInitialized() { return s_initialized; }

    /**
     * @brief Get current clock configuration
     *
     * @return const ClockConfig&     */
    static const ClockConfig& getConfig() { return s_config; }


   private:
    /**
     * @brief Get register pointer
     */
    static inline volatile st::stm32f4::rcc::RCC_Registers* get_rcc() {
#ifdef ALLOY_CLOCK_MOCK_HW
        return ALLOY_CLOCK_MOCK_HW();
#else
        return reinterpret_cast<volatile st::stm32f4::rcc::RCC_Registers*>(RCC_BASE);
#endif
    }

    static inline bool s_initialized = false;  ///< Tracks if clocks are initialized
    static inline ClockConfig s_config = {};   ///< Current clock configuration
};

// ============================================================================
// Common Clock Configurations
// ============================================================================

/**
 * @brief Default configuration: 8 MHz HSE -> 168 MHz SYSCLK (STM32F4)
 */
constexpr ClockConfig CLOCK_CONFIG_168MHZ = {
    .sysclk_source = SystemClockSource::PLL,
    .use_hse = true,
    .hse_freq_hz = 8000000,
    .pll = {8, 336, 2, 7},
    .ahb_prescaler = AhbPrescaler::DIV_1,
    .apb1_prescaler = ApbPrescaler::DIV_4,
    .apb2_prescaler = ApbPrescaler::DIV_2  // (8MHz / 8) * 336 / 2 = 168 MHz
};

/**
 * @brief Conservative configuration: 8 MHz HSE -> 100 MHz SYSCLK
 */
constexpr ClockConfig CLOCK_CONFIG_100MHZ = {
    .sysclk_source = SystemClockSource::PLL,
    .use_hse = true,
    .hse_freq_hz = 8000000,
    .pll = {8, 200, 2, 5},
    .ahb_prescaler = AhbPrescaler::DIV_1,
    .apb1_prescaler = ApbPrescaler::DIV_2,
    .apb2_prescaler = ApbPrescaler::DIV_1  // (8MHz / 8) * 200 / 2 = 100 MHz
};

/**
 * @brief Low power configuration: 16 MHz HSI (internal RC)
 */
constexpr ClockConfig CLOCK_CONFIG_16MHZ_HSI = {
    .sysclk_source = SystemClockSource::HSI,
    .use_hse = false,
    .hse_freq_hz = 0,
    .pll = {8, 336, 2, 7},
    .ahb_prescaler = AhbPrescaler::DIV_1,
    .apb1_prescaler = ApbPrescaler::DIV_1,
    .apb2_prescaler = ApbPrescaler::DIV_1  // PLL not used
};

}  // namespace alloy::hal::stm32f4
