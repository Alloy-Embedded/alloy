/**
 * @file timer.hpp
 * @brief Template-based Timer implementation for SAME70 (Platform Layer)
 *
 * This file implements Timer peripheral using templates with ZERO virtual
 * functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Peripheral address and IRQ resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Error handling: Uses Result<T, ErrorCode> for robust error handling
 * - Master mode only (for simplicity)
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: same70
 * Generator: generate_platform_timer.py
 * Generated: 2025-11-07 17:39:59
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
#include "hal/vendors/atmel/same70/registers/tc0_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/atmel/same70/bitfields/tc0_bitfields.hpp"


namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfield access
namespace tc = alloy::hal::atmel::same70::tc0;

// Note: Timer uses common I2cMode from hal/types.hpp:
// - Mode0: CPOL=0, CPHA=0
// - Mode1: CPOL=0, CPHA=1
// - Mode2: CPOL=1, CPHA=0
// - Mode3: CPOL=1, CPHA=1

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief Timer operating mode
 */
enum class TimerMode : uint8_t {
    Capture = 0,   ///< Capture mode (frequency/pulse width measurement)
    Waveform = 1,  ///< Waveform generation mode (PWM, pulses)
};

/**
 * @brief Timer clock source selection
 */
enum class TimerClock : uint8_t {
    MCK_DIV_2 = 0,    ///< MCK/2 (75 MHz)
    MCK_DIV_8 = 1,    ///< MCK/8 (18.75 MHz)
    MCK_DIV_32 = 2,   ///< MCK/32 (4.6875 MHz)
    MCK_DIV_128 = 3,  ///< MCK/128 (1.171875 MHz)
    TCLK1 = 4,        ///< External clock 1
    TCLK2 = 5,        ///< External clock 2
    XC0 = 6,          ///< External clock 0
    XC1 = 7,          ///< External clock 1
    XC2 = 8,          ///< External clock 2
};

/**
 * @brief Waveform generation type
 */
enum class WaveformType : uint8_t {
    UpAuto = 0,     ///< Up counting with automatic trigger on RC compare
    UpDown = 1,     ///< Up/down counting with automatic trigger on RC compare
    UpReset = 2,    ///< Up counting with trigger on RC compare and reset
    UpDownAlt = 3,  ///< Up/down counting with trigger on RC compare
};


/**
 * @brief Timer configuration structure
 */
struct TimerConfig {
    TimerMode mode = TimerMode::Waveform;           ///< Operating mode
    TimerClock clock = TimerClock::MCK_DIV_8;       ///< Clock source
    WaveformType waveform = WaveformType::UpReset;  ///< Waveform type
    uint32_t period = 1000;                         ///< Period (RC register value)
    uint32_t duty_a = 500;                          ///< Duty cycle for output A (RA register)
    uint32_t duty_b = 500;                          ///< Duty cycle for output B (RB register)
    bool invert_output = false;                     ///< Invert output polarity
};

/**
 * @brief Template-based Timer peripheral for SAME70
 *
 * This class provides a template-based Timer implementation with ZERO runtime
 * overhead. All peripheral configuration is resolved at compile-time.
 *
 * Template Parameters:
 * - BASE_ADDR: TC block base address
 * - CHANNEL: Channel number (0-2)
 * - IRQ_ID: Peripheral IRQ ID (for channel 0)
 *
 * Example usage:
 * @code
 * // Basic timer PWM usage
 * using MyTimer = Timer<TC0_BASE, 0, TC0_CH0_IRQ>;
 * auto timer = MyTimer{};
 * TimerConfig config;
 * config.mode = TimerMode::Waveform;
 * config.clock = TimerClock::MCK_DIV_8;
 * config.period = 1000;
 * config.duty_a = 500;
 * timer.open();
 * timer.configure(config);
 * timer.start();
 * @endcode
 *
 * @tparam BASE_ADDR TC block base address
 * @tparam CHANNEL Channel number (0-2)
 * @tparam IRQ_ID Peripheral IRQ ID (for channel 0)
 */
template <uint32_t BASE_ADDR, uint8_t CHANNEL, uint32_t IRQ_ID>
class Timer {
    static_assert(CHANNEL < 3, "TC has 3 channels (0-2)");

   public:
    // Compile-time constants
    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint8_t channel = CHANNEL;
    static constexpr uint32_t irq_id = IRQ_ID;

    // Configuration constants
    static constexpr uint32_t channel_offset = 0x40 * CHANNEL;  ///< Channel register offset

    /**
     * @brief Get TC peripheral registers
     *
     * Returns pointer to TC registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile alloy::hal::atmel::same70::tc0::TC0_Registers* get_hw() {
#ifdef ALLOY_TIMER_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_TIMER_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::atmel::same70::tc0::TC0_Registers*>(
            BASE_ADDR + channel_offset);
#endif
    }

    constexpr Timer() = default;

    /**
     * @brief Initialize timer
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> open() {
        auto* hw = get_hw();

        if (m_opened) {
            return Err(ErrorCode::AlreadyInitialized);
        }

        // Enable TC clock (PMC)
        // TODO: Enable peripheral clock via PMC
        // uint32_t periph_id = IRQ_ID + CHANNEL;

        m_opened = true;

        return Ok();
    }

    /**
     * @brief Close timer
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> close() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Disable clock
        hw->CCR = tc::ccr::CLKDIS::mask;

        m_opened = false;

        return Ok();
    }

    /**
     * @brief Configure timer
     *
     * @param config Timer configuration
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> configure(const TimerConfig& config) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Build CMR based on mode
        uint32_t cmr = 0;

        // Clock selection
        cmr = tc::cmr_waveform_mode::TCCLKS::write(cmr, static_cast<uint32_t>(config.clock) & 0x07);

        if (config.mode == TimerMode::Waveform) {
            // Waveform mode
            cmr = tc::cmr_waveform_mode::WAVE::set(cmr);

            // Waveform selection
            cmr = tc::cmr_waveform_mode::WAVSEL::write(cmr, static_cast<uint32_t>(config.waveform));

            // Output configuration for TIOA (RA compare)
            if (config.invert_output) {
                cmr = tc::cmr_waveform_mode::ACPA::write(cmr, tc::cmr_waveform_mode::acpa::SET);
                cmr = tc::cmr_waveform_mode::ACPC::write(cmr, tc::cmr_waveform_mode::acpc::CLEAR);
            } else {
                cmr = tc::cmr_waveform_mode::ACPA::write(cmr, tc::cmr_waveform_mode::acpa::CLEAR);
                cmr = tc::cmr_waveform_mode::ACPC::write(cmr, tc::cmr_waveform_mode::acpc::SET);
            }

            // Output configuration for TIOB (RB compare)
            if (config.invert_output) {
                cmr = tc::cmr_waveform_mode::BCPB::write(cmr, tc::cmr_waveform_mode::bcpb::SET);
                cmr = tc::cmr_waveform_mode::BCPC::write(cmr, tc::cmr_waveform_mode::bcpc::CLEAR);
            } else {
                cmr = tc::cmr_waveform_mode::BCPB::write(cmr, tc::cmr_waveform_mode::bcpb::CLEAR);
                cmr = tc::cmr_waveform_mode::BCPC::write(cmr, tc::cmr_waveform_mode::bcpc::SET);
            }
        }

        hw->CMR_WAVEFORM_MODE = cmr;

        // Set period (RC)
        hw->RC = config.period;

        // Set duty cycles
        hw->RA = config.duty_a;
        hw->RB = config.duty_b;

        m_config = config;

        return Ok();
    }

    /**
     * @brief Start timer
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> start() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Enable clock and trigger
        hw->CCR = tc::ccr::CLKEN::mask | tc::ccr::SWTRG::mask;

        return Ok();
    }

    /**
     * @brief Stop timer
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> stop() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Disable clock
        hw->CCR = tc::ccr::CLKDIS::mask;

        return Ok();
    }

    /**
     * @brief Set PWM duty cycle for output A
     *
     * @param duty Duty cycle value (0 to period)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setDutyA(uint32_t duty) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        //
        hw->RA = duty;

        return Ok();
    }

    /**
     * @brief Set PWM duty cycle for output B
     *
     * @param duty Duty cycle value (0 to period)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setDutyB(uint32_t duty) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        //
        hw->RB = duty;

        return Ok();
    }

    /**
     * @brief Set period (frequency)
     *
     * @param period Period value (RC register)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setPeriod(uint32_t period) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        //
        hw->RC = period;

        return Ok();
    }

    /**
     * @brief Get current counter value
     *
     * @return Result<uint32_t, ErrorCode> Get current counter value     */
    Result<uint32_t, ErrorCode> getCounter() const {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        return Ok(uint32_t(hw->CV));
    }

    /**
     * @brief Enable interrupts
     *
     * @param interrupt_mask Interrupt mask
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> enableInterrupts(uint32_t interrupt_mask) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        //
        hw->IER = interrupt_mask;

        return Ok();
    }

    /**
     * @brief Check if timer is open
     *
     * @return bool Check if timer is open     */
    bool isOpen() const { return m_opened; }

   private:
    bool m_opened = false;      ///< Tracks if peripheral is initialized
    TimerConfig m_config = {};  ///< Current configuration
};

// ==============================================================================
// Predefined Timer Instances
// ==============================================================================

constexpr uint32_t TIMER0CH0_BASE = 0x4000C000;
constexpr uint32_t TIMER0CH0_IRQ = 23;

constexpr uint32_t TIMER0CH1_BASE = 0x4000C000;
constexpr uint32_t TIMER0CH1_IRQ = 23;

constexpr uint32_t TIMER0CH2_BASE = 0x4000C000;
constexpr uint32_t TIMER0CH2_IRQ = 23;

constexpr uint32_t TIMER1CH0_BASE = 0x40010000;
constexpr uint32_t TIMER1CH0_IRQ = 26;

constexpr uint32_t TIMER1CH1_BASE = 0x40010000;
constexpr uint32_t TIMER1CH1_IRQ = 26;

constexpr uint32_t TIMER1CH2_BASE = 0x40010000;
constexpr uint32_t TIMER1CH2_IRQ = 26;

constexpr uint32_t TIMER2CH0_BASE = 0x40014000;
constexpr uint32_t TIMER2CH0_IRQ = 29;

constexpr uint32_t TIMER2CH1_BASE = 0x40014000;
constexpr uint32_t TIMER2CH1_IRQ = 29;

constexpr uint32_t TIMER2CH2_BASE = 0x40014000;
constexpr uint32_t TIMER2CH2_IRQ = 29;

using Timer0Ch0 = Timer<TIMER0CH0_BASE, 0, TIMER0CH0_IRQ>;  ///< TC0 Channel 0
using Timer0Ch1 = Timer<TIMER0CH1_BASE, 1, TIMER0CH1_IRQ>;  ///< TC0 Channel 1
using Timer0Ch2 = Timer<TIMER0CH2_BASE, 2, TIMER0CH2_IRQ>;  ///< TC0 Channel 2
using Timer1Ch0 = Timer<TIMER1CH0_BASE, 0, TIMER1CH0_IRQ>;  ///< TC1 Channel 0
using Timer1Ch1 = Timer<TIMER1CH1_BASE, 1, TIMER1CH1_IRQ>;  ///< TC1 Channel 1
using Timer1Ch2 = Timer<TIMER1CH2_BASE, 2, TIMER1CH2_IRQ>;  ///< TC1 Channel 2
using Timer2Ch0 = Timer<TIMER2CH0_BASE, 0, TIMER2CH0_IRQ>;  ///< TC2 Channel 0
using Timer2Ch1 = Timer<TIMER2CH1_BASE, 1, TIMER2CH1_IRQ>;  ///< TC2 Channel 1
using Timer2Ch2 = Timer<TIMER2CH2_BASE, 2, TIMER2CH2_IRQ>;  ///< TC2 Channel 2

}  // namespace alloy::hal::same70
