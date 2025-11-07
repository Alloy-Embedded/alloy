/**
 * @file pwm.hpp
 * @brief Template-based PWM implementation for SAME70
 *
 * This file implements the Pulse Width Modulation Controller for SAME70
 * using templates with ZERO virtual functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: PWM instance and channel resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Feature-rich: Supports center-aligned/left-aligned PWM, dead-time, fault protection
 *
 * SAME70 PWM Features:
 * - 2 PWM blocks (PWM0, PWM1), each with 4 channels
 * - 16-bit counter resolution
 * - Configurable prescaler (MCK/1, MCK/2, MCK/4... MCK/1024)
 * - Center-aligned and left-aligned modes
 * - Dead-time generation (for motor control)
 * - Fault protection inputs
 * - Synchronous update of multiple channels
 * - DMA support
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// Include SAME70 register definitions
#include "hal/vendors/atmel/same70/registers/pwm0_registers.hpp"
#include "hal/platform/same70/clock.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

/**
 * @brief PWM Clock Prescaler
 */
enum class PwmPrescaler : uint8_t {
    DIV_1 = 0,      // MCK/1
    DIV_2 = 1,      // MCK/2
    DIV_4 = 2,      // MCK/4
    DIV_8 = 3,      // MCK/8
    DIV_16 = 4,     // MCK/16
    DIV_32 = 5,     // MCK/32
    DIV_64 = 6,     // MCK/64
    DIV_128 = 7,    // MCK/128
    DIV_256 = 8,    // MCK/256
    DIV_512 = 9,    // MCK/512
    DIV_1024 = 10,  // MCK/1024
};

/**
 * @brief SAME70 PWM Configuration
 */
struct Same70PwmConfig {
    PwmAlignment alignment = PwmAlignment::Edge;
    PwmPolarity polarity = PwmPolarity::Normal;
    PwmPrescaler prescaler = PwmPrescaler::DIV_1;
    uint16_t period = 1000;           ///< Period (CPRD register value)
    uint16_t duty_cycle = 500;        ///< Duty cycle (CDTY register value)
    uint16_t dead_time_h = 0;         ///< Dead-time for high side output (motor control)
    uint16_t dead_time_l = 0;         ///< Dead-time for low side output (motor control)
};

/**
 * @brief PWM Channel Registers (per-channel)
 *
 * Each of the 4 PWM channels has its own register block at offset 0x200 + (channel * 0x20)
 */
struct PwmChannelRegisters {
    volatile uint32_t CMR;       ///< Channel Mode Register
    volatile uint32_t CDTY;      ///< Channel Duty Cycle Register
    volatile uint32_t CDTYUPD;   ///< Channel Duty Cycle Update Register
    volatile uint32_t CPRD;      ///< Channel Period Register
    volatile uint32_t CPRDUPD;   ///< Channel Period Update Register
    volatile uint32_t CCNT;      ///< Channel Counter Register (read-only)
    volatile uint32_t DT;        ///< Channel Dead Time Register
    volatile uint32_t DTUPD;     ///< Channel Dead Time Update Register
};

/**
 * @brief Template-based PWM for SAME70
 *
 * @tparam BASE_ADDR PWM block base address
 * @tparam CHANNEL Channel number (0-3)
 * @tparam IRQ_ID Peripheral IRQ ID
 */
template <uint32_t BASE_ADDR, uint8_t CHANNEL, uint32_t IRQ_ID>
class Pwm {
    static_assert(CHANNEL < 4, "PWM has 4 channels (0-3)");

public:
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint8_t channel = CHANNEL;
    static constexpr uint32_t irq_id = IRQ_ID;

    // Channel register offset: 0x200 + (channel * 0x20)
    static constexpr uint32_t channel_offset = 0x200 + (CHANNEL * 0x20);

    static inline volatile atmel::same70::pwm0::PWM0_Registers* get_pwm() {
#ifdef ALLOY_PWM_MOCK_HW
        return ALLOY_PWM_MOCK_HW();
#else
        return reinterpret_cast<volatile atmel::same70::pwm0::PWM0_Registers*>(BASE_ADDR);
#endif
    }

    static inline volatile PwmChannelRegisters* get_channel() {
#ifdef ALLOY_PWM_MOCK_CHANNEL
        return ALLOY_PWM_MOCK_CHANNEL();
#else
        return reinterpret_cast<volatile PwmChannelRegisters*>(BASE_ADDR + channel_offset);
#endif
    }

    constexpr Pwm() = default;

    /**
     * @brief Initialize PWM
     */
    Result<void> open() {
        if (m_opened) {
            return Result<void>::error(ErrorCode::AlreadyInitialized);
        }

        auto* pwm = get_pwm();

        // Enable peripheral clock using Clock class
        auto clock_result = Clock::enablePeripheralClock(IRQ_ID);
        if (!clock_result.is_ok()) {
            return Result<void>::error(clock_result.error());
        }

        // Disable channel initially
        pwm->DIS = (1u << CHANNEL);

        m_opened = true;
        return Result<void>::ok();
    }

    /**
     * @brief Close PWM
     */
    Result<void> close() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* pwm = get_pwm();

        // Disable channel
        pwm->DIS = (1u << CHANNEL);

        m_opened = false;
        return Result<void>::ok();
    }

    /**
     * @brief Configure PWM channel
     */
    Result<void> configure(const Same70PwmConfig& config) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* channel = get_channel();

        // Build CMR (Channel Mode Register)
        uint32_t cmr = 0;

        // Clock prescaler
        cmr |= (static_cast<uint32_t>(config.prescaler) & 0x0F);

        // Alignment mode
        if (config.alignment == PwmAlignment::Center) {
            cmr |= (1 << 8);  // CALG = 1 (center-aligned)
        }

        // Polarity
        if (config.polarity == PwmPolarity::Inverted) {
            cmr |= (1 << 9);  // CPOL = 1 (inverted)
        }

        // Dead-time enable (if non-zero)
        if (config.dead_time_h > 0 || config.dead_time_l > 0) {
            cmr |= (1 << 16);  // DTE = 1 (dead-time enabled)
        }

        channel->CMR = cmr;

        // Set period
        channel->CPRD = config.period;

        // Set duty cycle
        channel->CDTY = config.duty_cycle;

        // Set dead-time (if enabled)
        if (config.dead_time_h > 0 || config.dead_time_l > 0) {
            uint32_t dt = (config.dead_time_h & 0xFFFF) | ((config.dead_time_l & 0xFFFF) << 16);
            channel->DT = dt;
        }

        m_config = config;
        return Result<void>::ok();
    }

    /**
     * @brief Start PWM output
     */
    Result<void> start() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* pwm = get_pwm();

        // Enable channel
        pwm->ENA = (1u << CHANNEL);

        return Result<void>::ok();
    }

    /**
     * @brief Stop PWM output
     */
    Result<void> stop() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* pwm = get_pwm();

        // Disable channel
        pwm->DIS = (1u << CHANNEL);

        return Result<void>::ok();
    }

    /**
     * @brief Set PWM duty cycle
     *
     * @param duty_cycle Duty cycle value (0 to period)
     *
     * @note For left-aligned: 0 = always low, period = always high
     *       For center-aligned: 0 = always low, period/2 = 50% duty
     */
    Result<void> setDutyCycle(uint16_t duty_cycle) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* channel = get_channel();

        // Write to update register (takes effect at next period)
        channel->CDTYUPD = duty_cycle;

        return Result<void>::ok();
    }

    /**
     * @brief Set PWM period (frequency)
     *
     * @param period Period value (CPRD register)
     *
     * @note Takes effect at next period boundary
     */
    Result<void> setPeriod(uint16_t period) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* channel = get_channel();

        // Write to update register
        channel->CPRDUPD = period;

        return Result<void>::ok();
    }

    /**
     * @brief Get current counter value
     */
    Result<uint32_t> getCounter() const {
        if (!m_opened) {
            return Result<uint32_t>::error(ErrorCode::NotInitialized);
        }

        auto* channel = get_channel();
        return Result<uint32_t>::ok(channel->CCNT);
    }

    /**
     * @brief Check if PWM is running
     */
    bool isRunning() const {
        if (!m_opened) {
            return false;
        }

        auto* pwm = get_pwm();
        return (pwm->SR & (1u << CHANNEL)) != 0;
    }

    /**
     * @brief Calculate frequency from clock and period
     *
     * @param prescaler Clock prescaler
     * @param period Period value
     * @param alignment Alignment mode
     * @return Frequency in Hz
     */
    static uint32_t calculateFrequency(PwmPrescaler prescaler, uint16_t period,
                                        PwmAlignment alignment) {
        uint32_t mck = Clock::getMasterClockFrequency();
        uint32_t divisor = 1u << static_cast<uint32_t>(prescaler);
        uint32_t clock_freq = mck / divisor;

        // Center-aligned mode counts up AND down, so effective period is 2x
        if (alignment == PwmAlignment::Center) {
            return clock_freq / (period * 2);
        } else {
            return clock_freq / period;
        }
    }

    /**
     * @brief Calculate period from desired frequency
     *
     * @param prescaler Clock prescaler
     * @param frequency Desired frequency in Hz
     * @param alignment Alignment mode
     * @return Period value
     */
    static uint16_t calculatePeriod(PwmPrescaler prescaler, uint32_t frequency,
                                     PwmAlignment alignment) {
        uint32_t mck = Clock::getMasterClockFrequency();
        uint32_t divisor = 1u << static_cast<uint32_t>(prescaler);
        uint32_t clock_freq = mck / divisor;

        if (alignment == PwmAlignment::Center) {
            return static_cast<uint16_t>(clock_freq / (frequency * 2));
        } else {
            return static_cast<uint16_t>(clock_freq / frequency);
        }
    }

    /**
     * @brief Calculate duty cycle value from percentage
     *
     * @param period Current period value
     * @param duty_percent Duty cycle percentage (0-100)
     * @return Duty cycle value
     */
    static constexpr uint16_t calculateDuty(uint16_t period, uint8_t duty_percent) {
        return static_cast<uint16_t>((period * duty_percent) / 100);
    }

    /**
     * @brief Enable channel interrupts
     */
    Result<void> enableInterrupts() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* pwm = get_pwm();

        // Enable period interrupt for this channel
        pwm->IER1 = (1u << CHANNEL);

        return Result<void>::ok();
    }

    bool isOpen() const {
        return m_opened;
    }

private:
    bool m_opened = false;
    Same70PwmConfig m_config;
};

// ============================================================================
// Predefined PWM instances for SAME70
// ============================================================================

// PWM0 (4 channels)
constexpr uint32_t PWM0_BASE = 0x40020000;
constexpr uint32_t PWM0_IRQ = 31;

using Pwm0Ch0 = Pwm<PWM0_BASE, 0, PWM0_IRQ>;
using Pwm0Ch1 = Pwm<PWM0_BASE, 1, PWM0_IRQ>;
using Pwm0Ch2 = Pwm<PWM0_BASE, 2, PWM0_IRQ>;
using Pwm0Ch3 = Pwm<PWM0_BASE, 3, PWM0_IRQ>;

// PWM1 (4 channels)
constexpr uint32_t PWM1_BASE = 0x4005C000;
constexpr uint32_t PWM1_IRQ = 60;

using Pwm1Ch0 = Pwm<PWM1_BASE, 0, PWM1_IRQ>;
using Pwm1Ch1 = Pwm<PWM1_BASE, 1, PWM1_IRQ>;
using Pwm1Ch2 = Pwm<PWM1_BASE, 2, PWM1_IRQ>;
using Pwm1Ch3 = Pwm<PWM1_BASE, 3, PWM1_IRQ>;

} // namespace alloy::hal::same70
