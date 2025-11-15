/**
 * @file clock.hpp
 * @brief Clock and Power Management Controller (PMC) for SAME70
 *
 * This file implements the clock management for SAME70 using static methods with
 * ZERO virtual functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Compile-time clock configuration
 * - Type-safe clock source selection
 * - Zero overhead abstractions
 * - Peripheral clock management
 *
 * SAME70 Clock Architecture:
 * Main Crystal Oscillator: 3-20 MHz (typically 12 MHz)
 * Internal RC: 4/8/12 MHz
 * PLLA: Up to 300 MHz PLL
 * Master Clock (MCK): Up to 150 MHz
 * Peripheral clocks: Individual enable/disable
 * Programmable clocks: 8 configurable outputs
 * 
 * Typical Clock Tree:
 * Crystal (12 MHz) -> PLLA (300 MHz) -> MCK Divider (/2) -> MCK (150 MHz)
 *
 * Auto-generated from: same70
 * Generator: generate_platform_clock.py
 * Generated: 
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/atmel/same70/generated/registers/pmc_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/atmel/same70/generated/bitfields/pmc_bitfields.hpp"

// Peripheral addresses (device-specific)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"


namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfield access
namespace pmc = alloy::hal::atmel::same70::pmc;

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief Main Clock Source
 */
enum class MainClockSource : uint8_t {
    InternalRC_4MHz = 0,  ///< Fast RC 4 MHz
    InternalRC_8MHz = 1,  ///< Fast RC 8 MHz
    InternalRC_12MHz = 2,  ///< Fast RC 12 MHz
    ExternalCrystal = 3,  ///< External crystal/oscillator
};

/**
 * @brief Master Clock Prescaler
 */
enum class MasterClockPrescaler : uint8_t {
    DIV_1 = 0,  ///< No division
    DIV_2 = 1,  ///< Divide by 2
    DIV_3 = 2,  ///< Divide by 3
    DIV_4 = 3,  ///< Divide by 4
};

/**
 * @brief Master Clock Source
 */
enum class MasterClockSource : uint8_t {
    SlowClock = 0,  ///< 32 kHz slow clock
    MainClock = 1,  ///< Main clock (crystal or RC)
    PLLAClock = 2,  ///< PLLA output
    UPLLClock = 3,  ///< UPLL output (480 MHz for USB)
};


// ============================================================================
// Additional Structures
// ============================================================================

/**
 * @brief PLLA Multiplier\n\nPLLA_FREQ = (MAIN_CLK * (MUL + 1)) / DIV\nFor 12 MHz crystal -> 300 MHz PLLA: MUL=24, DIV=1\nResult: (12 * 25) / 1 = 300 MHz
 */
struct PllaConfig {
    uint16_t multiplier = 24;  ///< PLLA multiplier (0-62), actual = multiplier+1
    uint8_t divider = 1;  ///< PLLA divider (1-255)
};


/**
 * @brief Complete Clock Configuration
 */
struct ClockConfig {
    MainClockSource main_source = MainClockSource::ExternalCrystal;  ///< Main clock source
    uint32_t crystal_freq_hz = 12000000;  ///< External crystal frequency
    PllaConfig plla = {24, 1};  ///< PLLA config (12MHz * 25 / 1 = 300MHz)
    MasterClockSource mck_source = MasterClockSource::PLLAClock;  ///< Master clock source
    MasterClockPrescaler mck_prescaler = MasterClockPrescaler::DIV_2;  ///< 300/2 = 150MHz
};


/**
 * @brief Clock and Power Management Controller (PMC) for SAME70
 *
 * Singleton class for managing system and peripheral clocks.
 * All methods are static for zero-overhead access.
 */
class Clock {
public:
    // Compile-time constants
    static constexpr uintptr_t PMC_BASE = alloy::generated::atsame70q21b::peripherals::PMC;  ///< PMC base address (using generated peripheral addresses)
    static constexpr uint32_t SLOW_CLOCK_FREQ = 32768;  ///< 32.768 kHz

    // API Layer constants - defined after preset configurations
    static const ClockConfig& CLOCK_CONFIG_SAFE_DEFAULT;
    static const ClockConfig& CLOCK_CONFIG_HIGH_PERFORMANCE;
    static const ClockConfig& CLOCK_CONFIG_MEDIUM_PERFORMANCE;

    /**
     * @brief Initialize system clocks\n\nConfigures main oscillator, PLLA, and master clock.\nThis should be called early in system initialization.
     *
     * @param config Clock configuration
     * @return Result<void, ErrorCode>     */
    static Result<void, ErrorCode> initialize(const ClockConfig& config) {
        auto* pmc = get_pmc();
                s_config = config;

                // Step 1: Configure main oscillator (Crystal or RC)
                if (config.main_source == MainClockSource::ExternalCrystal) {
                    // Enable both RC (for safety) and Crystal
                    uint32_t mor = 0;
                    mor = pmc::ckgr_mor::KEY::write(mor, pmc::ckgr_mor::key::PASSWD);
                    mor = pmc::ckgr_mor::MOSCXTST::write(mor, 0xFF);
                    mor = pmc::ckgr_mor::MOSCRCEN::set(mor);
                    mor = pmc::ckgr_mor::MOSCXTEN::set(mor);
                    pmc->CKGR_MOR = mor;

                    // Wait for crystal stabilization
                    volatile uint32_t timeout = 0;
                    while (!(pmc->SR & pmc::sr::MOSCXTS::mask)) {
                        if (++timeout > 1000000) return Err(ErrorCode::HardwareError);
                    }

                    // Switch to crystal - set MOSCSEL
                    mor = 0;
                    mor = pmc::ckgr_mor::KEY::write(mor, pmc::ckgr_mor::key::PASSWD);
                    mor = pmc::ckgr_mor::MOSCXTST::write(mor, 0xFF);
                    mor = pmc::ckgr_mor::MOSCSEL::set(mor);
                    mor = pmc::ckgr_mor::MOSCRCEN::set(mor);
                    mor = pmc::ckgr_mor::MOSCXTEN::set(mor);
                    pmc->CKGR_MOR = mor;

                    // Wait for oscillator selection
                    timeout = 0;
                    while (!(pmc->SR & pmc::sr::MOSCSELS::mask)) {
                        if (++timeout > 100000) return Err(ErrorCode::HardwareError);
                    }
                } else {
                    // Use RC oscillator
                    uint32_t rc_freq = (config.main_source == MainClockSource::InternalRC_4MHz) ? pmc::ckgr_mor::moscrcf::_4_MHz :
                                       (config.main_source == MainClockSource::InternalRC_8MHz) ? pmc::ckgr_mor::moscrcf::_8_MHz :
                                                                                                   pmc::ckgr_mor::moscrcf::_12_MHz;

                    uint32_t mor = 0;
                    mor = pmc::ckgr_mor::KEY::write(mor, pmc::ckgr_mor::key::PASSWD);
                    mor = pmc::ckgr_mor::MOSCXTST::write(mor, 0xFF);
                    mor = pmc::ckgr_mor::MOSCRCF::write(mor, rc_freq);
                    mor = pmc::ckgr_mor::MOSCRCEN::set(mor);
                    // MOSCSEL=0 (use RC) - default value, no need to clear
                    pmc->CKGR_MOR = mor;

                    for (volatile int i = 0; i < 100; i++);  // Small delay
                }

                // If using MainClock only (no PLL), we're done
                if (config.mck_source == MasterClockSource::MainClock &&
                    config.mck_prescaler == MasterClockPrescaler::DIV_1) {
                    s_initialized = true;
                    return Ok();
                }

                // Step 2: Configure PLLA if needed
                if (config.mck_source == MasterClockSource::PLLAClock) {
                    uint32_t pllar = 0;
                    pllar = pmc::ckgr_pllar::MULA::write(pllar, config.plla.multiplier);
                    pllar = pmc::ckgr_pllar::DIVA::write(pllar, config.plla.divider);
                    pllar = pmc::ckgr_pllar::PLLACOUNT::write(pllar, 0x3F);
                    pllar = pmc::ckgr_pllar::ONE::set(pllar);
                    pmc->CKGR_PLLAR = pllar;

                    // Wait for PLLA lock
                    volatile uint32_t timeout = 0;
                    while (!(pmc->SR & pmc::sr::LOCKA::mask)) {
                        if (++timeout > 1000000) return Err(ErrorCode::HardwareError);
                    }
                }

                // Step 3: Configure MCK - CRITICAL SEQUENCE per datasheet
                // Must follow exact order: CSS -> wait -> PRES -> wait -> CSS -> wait

                // 3.1: Switch to MAIN_CLK first (safe intermediate state)
                uint32_t mckr = pmc->MCKR;
                mckr = pmc::mckr::CSS::write(mckr, pmc::mckr::css::MAIN_CLK);
                pmc->MCKR = mckr;

                volatile uint32_t timeout = 0;
                while (!(pmc->SR & pmc::sr::MCKRDY::mask)) {
                    if (++timeout > 100000) return Err(ErrorCode::HardwareError);
                }

                // 3.2: Set prescaler
                uint32_t pres_value = (config.mck_prescaler == MasterClockPrescaler::DIV_1) ? pmc::mckr::pres::CLK_1 :
                                      (config.mck_prescaler == MasterClockPrescaler::DIV_2) ? pmc::mckr::pres::CLK_2 :
                                      (config.mck_prescaler == MasterClockPrescaler::DIV_3) ? pmc::mckr::pres::CLK_3 :
                                                                                               pmc::mckr::pres::CLK_4;

                mckr = pmc->MCKR;
                mckr = pmc::mckr::PRES::write(mckr, pres_value);
                pmc->MCKR = mckr;

                timeout = 0;
                while (!(pmc->SR & pmc::sr::MCKRDY::mask)) {
                    if (++timeout > 100000) return Err(ErrorCode::HardwareError);
                }

                // 3.3: Switch to final clock source (PLLA or MAIN)
                uint32_t css_value = (config.mck_source == MasterClockSource::PLLAClock) ? pmc::mckr::css::PLLA_CLK :
                                     pmc::mckr::css::MAIN_CLK;

                mckr = pmc->MCKR;
                mckr = pmc::mckr::CSS::write(mckr, css_value);
                pmc->MCKR = mckr;

                timeout = 0;
                while (!(pmc->SR & pmc::sr::MCKRDY::mask)) {
                    if (++timeout > 100000) return Err(ErrorCode::HardwareError);
                }

                s_initialized = true;
                return Ok();
    }

    /**
     * @brief Enable peripheral clock
     *
     * @param peripheral_id Peripheral ID (0-63)
     * @return Result<void, ErrorCode>     */
    static Result<void, ErrorCode> enablePeripheralClock(uint8_t peripheral_id) {
        if (!s_initialized) {
            return Err(ErrorCode::NotInitialized);
        }
        if (peripheral_id >= 64) {
            return Err(ErrorCode::InvalidParameter);
        }
        auto* pmc = get_pmc();
                if (peripheral_id < 32) {
                    pmc->PCER0 = (1u << peripheral_id);
                } else {
                    pmc->PCER1 = (1u << (peripheral_id - 32));
                }

        return Ok();
    }

    /**
     * @brief Disable peripheral clock
     *
     * @param peripheral_id Peripheral ID (0-63)
     * @return Result<void, ErrorCode>     */
    static Result<void, ErrorCode> disablePeripheralClock(uint8_t peripheral_id) {
        if (!s_initialized) {
            return Err(ErrorCode::NotInitialized);
        }
        if (peripheral_id >= 64) {
            return Err(ErrorCode::InvalidParameter);
        }
        auto* pmc = get_pmc();
                if (peripheral_id < 32) {
                    pmc->PCDR0 = (1u << peripheral_id);
                } else {
                    pmc->PCDR1 = (1u << (peripheral_id - 32));
                }

        return Ok();
    }

    /**
     * @brief Get master clock (MCK) frequency in Hz
     *
     * @return uint32_t     */
    static uint32_t getMasterClockFrequency() {
        if (!s_initialized) {
                    return 0;
                }

                uint32_t main_freq = 0;

                // Get main clock frequency
                switch (s_config.main_source) {
                    case MainClockSource::InternalRC_4MHz:
                        main_freq = 4000000;
                        break;
                    case MainClockSource::InternalRC_8MHz:
                        main_freq = 8000000;
                        break;
                    case MainClockSource::InternalRC_12MHz:
                        main_freq = 12000000;
                        break;
                    case MainClockSource::ExternalCrystal:
                        main_freq = s_config.crystal_freq_hz;
                        break;
                }

                uint32_t mck_freq = 0;

                // Calculate MCK based on source
                switch (s_config.mck_source) {
                    case MasterClockSource::SlowClock:
                        mck_freq = SLOW_CLOCK_FREQ;
                        break;

                    case MasterClockSource::MainClock:
                        mck_freq = main_freq;
                        break;

                    case MasterClockSource::PLLAClock: {
                        // PLLA = (MAIN_CLK * (MUL+1)) / DIV
                        uint32_t plla_freq = (main_freq * (s_config.plla.multiplier + 1)) / s_config.plla.divider;
                        mck_freq = plla_freq;
                        break;
                    }

                    case MasterClockSource::UPLLClock:
                        mck_freq = 480000000;  // UPLL is fixed at 480 MHz
                        break;
                }

                // Apply prescaler
                uint32_t prescaler = 1;
                switch (s_config.mck_prescaler) {
                    case MasterClockPrescaler::DIV_1: prescaler = 1; break;
                    case MasterClockPrescaler::DIV_2: prescaler = 2; break;
                    case MasterClockPrescaler::DIV_3: prescaler = 3; break;
                    case MasterClockPrescaler::DIV_4: prescaler = 4; break;
                }

                return mck_freq / prescaler;
    }

    /**
     * @brief Check if clocks are initialized
     *
     * @return bool     */
    static bool isInitialized() {

        return s_initialized;
    }

    /**
     * @brief Get current clock configuration
     *
     * @return const ClockConfig&     */
    static const ClockConfig& getConfig() {

        return s_config;
    }


private:
    /**
     * @brief Get register pointer
     */
static inline volatile alloy::hal::atmel::same70::pmc::PMC_Registers* get_pmc() {
        #ifdef ALLOY_CLOCK_MOCK_HW
                return ALLOY_CLOCK_MOCK_HW();
        #else
                return reinterpret_cast<volatile alloy::hal::atmel::same70::pmc::PMC_Registers*>(PMC_BASE);
        #endif
    }

    static inline bool s_initialized = false;  ///< Tracks if clocks are initialized
    static inline ClockConfig s_config = {};  ///< Current clock configuration
};

// ============================================================================
// Common Clock Configurations
// ============================================================================

/**
 * @brief Default configuration: 12 MHz Crystal -> 300 MHz PLLA -> 150 MHz MCK
 */
constexpr ClockConfig CLOCK_CONFIG_150MHZ = {
    .main_source = MainClockSource::ExternalCrystal,
    .crystal_freq_hz = 12000000,
    .plla = {24, 1},
    .mck_source = MasterClockSource::PLLAClock,
    .mck_prescaler = MasterClockPrescaler::DIV_2
};

/**
 * @brief Conservative configuration: 12 MHz Crystal -> 240 MHz PLLA -> 120 MHz MCK
 */
constexpr ClockConfig CLOCK_CONFIG_120MHZ = {
    .main_source = MainClockSource::ExternalCrystal,
    .crystal_freq_hz = 12000000,
    .plla = {19, 1},
    .mck_source = MasterClockSource::PLLAClock,
    .mck_prescaler = MasterClockPrescaler::DIV_2
};

/**
 * @brief Low power configuration: 12 MHz Crystal direct (no PLL)
 */
constexpr ClockConfig CLOCK_CONFIG_12MHZ_CRYSTAL = {
    .main_source = MainClockSource::ExternalCrystal,
    .crystal_freq_hz = 12000000,
    .plla = {24, 1},
    .mck_source = MasterClockSource::MainClock,
    .mck_prescaler = MasterClockPrescaler::DIV_1
};

/**
 * @brief Workaround configuration: 12 MHz RC direct (no PLL) - Use until PLL issue is resolved
 */
constexpr ClockConfig CLOCK_CONFIG_12MHZ_RC = {
    .main_source = MainClockSource::InternalRC_12MHz,
    .crystal_freq_hz = 0,
    .plla = {24, 1},
    .mck_source = MasterClockSource::MainClock,
    .mck_prescaler = MasterClockPrescaler::DIV_1
};

// ============================================================================
// API Layer Constants (for hal::SystemClock compatibility)
// ============================================================================

// Define static member constants
inline const ClockConfig& Clock::CLOCK_CONFIG_SAFE_DEFAULT = CLOCK_CONFIG_12MHZ_RC;
inline const ClockConfig& Clock::CLOCK_CONFIG_HIGH_PERFORMANCE = CLOCK_CONFIG_150MHZ;
inline const ClockConfig& Clock::CLOCK_CONFIG_MEDIUM_PERFORMANCE = CLOCK_CONFIG_120MHZ;

} // namespace alloy::hal::same70