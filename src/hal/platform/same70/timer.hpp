/**
 * @file timer.hpp
 * @brief Template-based Timer Counter (TC) implementation for SAME70
 *
 * This file implements the Timer Counter for SAME70 using templates
 * with ZERO virtual functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Timer instance and channel resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Versatile: Supports capture, compare, PWM generation, frequency measurement
 *
 * SAME70 TC Features:
 * - 3 TC blocks (TC0, TC1, TC2), each with 3 channels
 * - 32-bit counter
 * - Multiple clock sources (MCK/2, MCK/8, MCK/32, MCK/128, external)
 * - Capture mode: Measure frequency, pulse width, duty cycle
 * - Waveform mode: Generate PWM, pulses, frequencies
 * - Quadrature decoder mode (QDEC)
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// Include SAME70 register definitions
#include "hal/vendors/atmel/same70/registers/tc0_registers.hpp"
#include "hal/vendors/atmel/same70/bitfields/tc0_bitfields.hpp"
#include "hal/platform/same70/clock.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;
namespace tc = atmel::same70::tc0;  // Alias for easier bitfield access

/**
 * @brief Timer Mode
 */
enum class TimerMode : uint8_t {
    Capture = 0,     // Capture mode (frequency/pulse width measurement)
    Waveform = 1,    // Waveform generation mode (PWM, pulses)
};

/**
 * @brief Timer Clock Source
 */
enum class TimerClock : uint8_t {
    MCK_DIV_2 = 0,    // MCK/2   (75 MHz)
    MCK_DIV_8 = 1,    // MCK/8   (18.75 MHz)
    MCK_DIV_32 = 2,   // MCK/32  (4.6875 MHz)
    MCK_DIV_128 = 3,  // MCK/128 (1.171875 MHz)
    TCLK1 = 4,        // External clock 1
    TCLK2 = 5,        // External clock 2
    XC0 = 6,          // External clock 0
    XC1 = 7,          // External clock 1
    XC2 = 8,          // External clock 2
};

/**
 * @brief Waveform Selection
 */
enum class WaveformType : uint8_t {
    UpAuto = 0,       // Up counting with automatic trigger on RC compare
    UpDown = 1,       // Up/down counting with automatic trigger on RC compare
    UpReset = 2,      // Up counting with trigger on RC compare and reset
    UpDownAlt = 3,    // Up/down counting with trigger on RC compare
};

/**
 * @brief Timer Configuration
 */
struct TimerConfig {
    TimerMode mode = TimerMode::Waveform;
    TimerClock clock = TimerClock::MCK_DIV_8;
    WaveformType waveform = WaveformType::UpReset;
    uint32_t period = 1000;           ///< Period (RC register value)
    uint32_t duty_a = 500;            ///< Duty cycle for output A (RA register)
    uint32_t duty_b = 500;            ///< Duty cycle for output B (RB register)
    bool invert_output = false;       ///< Invert output polarity
};

/**
 * @brief Template-based Timer Counter for SAME70
 *
 * @tparam BASE_ADDR TC block base address
 * @tparam CHANNEL Channel number (0-2)
 * @tparam IRQ_ID Peripheral IRQ ID
 */
template <uint32_t BASE_ADDR, uint8_t CHANNEL, uint32_t IRQ_ID>
class Timer {
    static_assert(CHANNEL < 3, "TC has 3 channels (0-2)");

public:
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint8_t channel = CHANNEL;
    static constexpr uint32_t irq_id = IRQ_ID;

    // Channel register offset: 0x40 * channel
    static constexpr uint32_t channel_offset = 0x40 * CHANNEL;

    static inline volatile atmel::same70::tc0::TC0_Registers* get_hw() {
#ifdef ALLOY_TIMER_MOCK_HW
        return ALLOY_TIMER_MOCK_HW();
#else
        return reinterpret_cast<volatile atmel::same70::tc0::TC0_Registers*>(
            BASE_ADDR + channel_offset
        );
#endif
    }

    constexpr Timer() = default;

    /**
     * @brief Initialize timer
     */
    Result<void> open() {
        if (m_opened) {
            return Result<void>::error(ErrorCode::AlreadyInitialized);
        }

        // Enable TC clock using Clock class (IRQ_ID is for TC0_CH0, add channel offset)
        uint32_t periph_id = IRQ_ID + CHANNEL;
        auto clock_result = Clock::enablePeripheralClock(periph_id);
        if (!clock_result.is_ok()) {
            return Result<void>::error(clock_result.error());
        }

        m_opened = true;
        return Result<void>::ok();
    }

    /**
     * @brief Close timer
     */
    Result<void> close() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();

        // Disable clock using type-safe bitfield
        hw->CCR = tc::ccr::CLKDIS::mask;

        m_opened = false;
        return Result<void>::ok();
    }

    /**
     * @brief Configure timer
     */
    Result<void> configure(const TimerConfig& config) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();

        // Build CMR (Channel Mode Register) using type-safe bitfields
        uint32_t cmr = 0;

        // Clock selection (same for both modes)
        cmr = tc::cmr_waveform_mode::TCCLKS::write(cmr, static_cast<uint32_t>(config.clock) & 0x07);

        if (config.mode == TimerMode::Waveform) {
            // Waveform mode
            cmr = tc::cmr_waveform_mode::WAVE::set(cmr);

            // Waveform selection
            cmr = tc::cmr_waveform_mode::WAVSEL::write(cmr, static_cast<uint32_t>(config.waveform));

            // Output configuration for TIOA (RA compare)
            if (config.invert_output) {
                // Inverted: SET on RA, CLEAR on RC
                cmr = tc::cmr_waveform_mode::ACPA::write(cmr, tc::cmr_waveform_mode::acpa::SET);
                cmr = tc::cmr_waveform_mode::ACPC::write(cmr, tc::cmr_waveform_mode::acpc::CLEAR);
            } else {
                // Normal: CLEAR on RA, SET on RC
                cmr = tc::cmr_waveform_mode::ACPA::write(cmr, tc::cmr_waveform_mode::acpa::CLEAR);
                cmr = tc::cmr_waveform_mode::ACPC::write(cmr, tc::cmr_waveform_mode::acpc::SET);
            }

            // Output configuration for TIOB (RB compare)
            if (config.invert_output) {
                // Inverted: SET on RB, CLEAR on RC
                cmr = tc::cmr_waveform_mode::BCPB::write(cmr, tc::cmr_waveform_mode::bcpb::SET);
                cmr = tc::cmr_waveform_mode::BCPC::write(cmr, tc::cmr_waveform_mode::bcpc::CLEAR);
            } else {
                // Normal: CLEAR on RB, SET on RC
                cmr = tc::cmr_waveform_mode::BCPB::write(cmr, tc::cmr_waveform_mode::bcpb::CLEAR);
                cmr = tc::cmr_waveform_mode::BCPC::write(cmr, tc::cmr_waveform_mode::bcpc::SET);
            }
        } else {
            // Capture mode - WAVE bit is 0 by default
            // Additional capture config would go here
        }

        hw->CMR_WAVEFORM_MODE = cmr;

        // Set period (RC)
        hw->RC = config.period;

        // Set duty cycles
        hw->RA = config.duty_a;
        hw->RB = config.duty_b;

        m_config = config;
        return Result<void>::ok();
    }

    /**
     * @brief Start timer
     */
    Result<void> start() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();

        // Enable clock and trigger using type-safe bitfields
        hw->CCR = tc::ccr::CLKEN::mask | tc::ccr::SWTRG::mask;

        return Result<void>::ok();
    }

    /**
     * @brief Stop timer
     */
    Result<void> stop() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();

        // Disable clock using type-safe bitfield
        hw->CCR = tc::ccr::CLKDIS::mask;

        return Result<void>::ok();
    }

    /**
     * @brief Set PWM duty cycle for output A
     *
     * @param duty Duty cycle value (0 to period)
     */
    Result<void> setDutyA(uint32_t duty) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        hw->RA = duty;

        return Result<void>::ok();
    }

    /**
     * @brief Set PWM duty cycle for output B
     *
     * @param duty Duty cycle value (0 to period)
     */
    Result<void> setDutyB(uint32_t duty) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        hw->RB = duty;

        return Result<void>::ok();
    }

    /**
     * @brief Set period (frequency)
     *
     * @param period Period value (RC register)
     */
    Result<void> setPeriod(uint32_t period) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        hw->RC = period;

        return Result<void>::ok();
    }

    /**
     * @brief Get current counter value
     */
    Result<uint32_t> getCounter() const {
        if (!m_opened) {
            return Result<uint32_t>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        return Result<uint32_t>::ok(hw->CV);
    }

    /**
     * @brief Calculate frequency from clock and period
     *
     * @param clock Timer clock source
     * @param period Period value (RC)
     * @return Frequency in Hz
     */
    static uint32_t calculateFrequency(TimerClock clock, uint32_t period) {
        uint32_t mck = Clock::getMasterClockFrequency();
        uint32_t clock_freq = 0;

        switch (clock) {
            case TimerClock::MCK_DIV_2:   clock_freq = mck / 2; break;
            case TimerClock::MCK_DIV_8:   clock_freq = mck / 8; break;
            case TimerClock::MCK_DIV_32:  clock_freq = mck / 32; break;
            case TimerClock::MCK_DIV_128: clock_freq = mck / 128; break;
            default: clock_freq = mck / 8; break;
        }

        return clock_freq / (period + 1);
    }

    /**
     * @brief Calculate period from desired frequency
     *
     * @param clock Timer clock source
     * @param frequency Desired frequency in Hz
     * @return Period value (RC)
     */
    static uint32_t calculatePeriod(TimerClock clock, uint32_t frequency) {
        uint32_t mck = Clock::getMasterClockFrequency();
        uint32_t clock_freq = 0;

        switch (clock) {
            case TimerClock::MCK_DIV_2:   clock_freq = mck / 2; break;
            case TimerClock::MCK_DIV_8:   clock_freq = mck / 8; break;
            case TimerClock::MCK_DIV_32:  clock_freq = mck / 32; break;
            case TimerClock::MCK_DIV_128: clock_freq = mck / 128; break;
            default: clock_freq = mck / 8; break;
        }

        return (clock_freq / frequency) - 1;
    }

    /**
     * @brief Enable interrupts
     */
    Result<void> enableInterrupts(uint32_t interrupt_mask) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        hw->IER = interrupt_mask;

        return Result<void>::ok();
    }

    bool isOpen() const {
        return m_opened;
    }

private:
    bool m_opened = false;
    TimerConfig m_config;
};

// ============================================================================
// Predefined Timer instances for SAME70
// ============================================================================

// TC0 (Timer Counter 0)
constexpr uint32_t TC0_BASE = 0x4000C000;
constexpr uint32_t TC0_CH0_IRQ = 23;

using Timer0Ch0 = Timer<TC0_BASE, 0, TC0_CH0_IRQ>;
using Timer0Ch1 = Timer<TC0_BASE, 1, TC0_CH0_IRQ>;
using Timer0Ch2 = Timer<TC0_BASE, 2, TC0_CH0_IRQ>;

// TC1 (Timer Counter 1)
constexpr uint32_t TC1_BASE = 0x40010000;
constexpr uint32_t TC1_CH0_IRQ = 26;

using Timer1Ch0 = Timer<TC1_BASE, 0, TC1_CH0_IRQ>;
using Timer1Ch1 = Timer<TC1_BASE, 1, TC1_CH0_IRQ>;
using Timer1Ch2 = Timer<TC1_BASE, 2, TC1_CH0_IRQ>;

// TC2 (Timer Counter 2)
constexpr uint32_t TC2_BASE = 0x40014000;
constexpr uint32_t TC2_CH0_IRQ = 29;

using Timer2Ch0 = Timer<TC2_BASE, 0, TC2_CH0_IRQ>;
using Timer2Ch1 = Timer<TC2_BASE, 1, TC2_CH0_IRQ>;
using Timer2Ch2 = Timer<TC2_BASE, 2, TC2_CH0_IRQ>;

} // namespace alloy::hal::same70
