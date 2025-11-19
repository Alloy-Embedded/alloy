/**
 * @file timer.hpp
 * @brief Template-based Timer implementation for STM32F4 (Platform Layer)
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
 * Auto-generated from: stm32f4
 * Generator: generate_platform_timer.py
 * Generated: 2025-11-07 17:44:05
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
#include "hal/vendors/st/stm32f4/registers/tim2_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/st/stm32f4/bitfields/tim2_bitfields.hpp"


namespace alloy::hal::stm32f4 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::st::stm32f4;

// Namespace alias for bitfield access
namespace tim = alloy::hal::st::stm32f4::tim2;

// Note: Timer uses common I2cMode from hal/types.hpp:
// - Mode0: CPOL=0, CPHA=0
// - Mode1: CPOL=0, CPHA=1
// - Mode2: CPOL=1, CPHA=0
// - Mode3: CPOL=1, CPHA=1

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief Timer counting mode
 */
enum class TimerMode : uint8_t {
    Up = 0,              ///< Up counting mode
    Down = 1,            ///< Down counting mode
    CenterAligned1 = 2,  ///< Center-aligned mode 1
    CenterAligned2 = 3,  ///< Center-aligned mode 2
    CenterAligned3 = 4,  ///< Center-aligned mode 3
};

/**
 * @brief PWM mode selection
 */
enum class PwmMode : uint8_t {
    Mode1 = 6,  ///< PWM mode 1 (active high)
    Mode2 = 7,  ///< PWM mode 2 (active low)
};

/**
 * @brief Timer channel selection
 */
enum class Channel : uint8_t {
    CH1 = 1,  ///< Channel 1
    CH2 = 2,  ///< Channel 2
    CH3 = 3,  ///< Channel 3
    CH4 = 4,  ///< Channel 4
};


/**
 * @brief Timer configuration structure
 */
struct TimerConfig {
    TimerMode mode = TimerMode::Up;  ///< Counting mode
    uint16_t prescaler = 0;          ///< Prescaler value (0-65535)
    uint32_t period = 1000;          ///< Auto-reload period
    uint8_t clock_division = 0;      ///< Clock division (0=1x, 1=2x, 2=4x)
};

/**
 * @brief Template-based Timer peripheral for STM32F4
 *
 * This class provides a template-based Timer implementation with ZERO runtime
 * overhead. All peripheral configuration is resolved at compile-time.
 *
 * Template Parameters:
 * - BASE_ADDR: Timer peripheral base address
 * - IRQ_ID: Timer IRQ ID
 *
 * Example usage:
 * @code
 * // Basic timer PWM usage
 * using MyTimer = Timer<TIM2_BASE, TIM2_IRQ>;
 * auto timer = MyTimer{};
 * TimerConfig config;
 * config.mode = TimerMode::Up;
 * config.prescaler = 83;  // 84MHz / (83+1) = 1MHz
 * config.period = 1000;   // 1kHz
 * timer.open();
 * timer.configure(config);
 * timer.configurePwmChannel(Channel::CH1, PwmMode::Mode1, 500);  // 50% duty
 * timer.start();
 * @endcode
 *
 * @tparam BASE_ADDR Timer peripheral base address
 * @tparam IRQ_ID Timer IRQ ID
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Timer {
   public:
    // Compile-time constants
    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;


    /**
     * @brief Get TIM peripheral registers
     *
     * Returns pointer to TIM registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile alloy::hal::st::stm32f4::tim2::TIM2_Registers* get_hw() {
#ifdef ALLOY_TIMER_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_TIMER_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::st::stm32f4::tim2::TIM2_Registers*>(BASE_ADDR);
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

        // Enable timer clock (RCC)
        // TODO: Enable peripheral clock via RCC

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

        // Disable counter
        hw->CR1 &= ~tim::cr1::CEN::mask;

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

        // Disable counter during configuration
        hw->CR1 &= ~tim::cr1::CEN::mask;

        // Configure CR1: counting mode and clock division
        uint32_t cr1 = hw->CR1 & ~(tim::cr1::DIR::mask | tim::cr1::CMS::mask | tim::cr1::CKD::mask);

        // Set counting direction/mode
        if (config.mode == TimerMode::Down) {
            cr1 = tim::cr1::DIR::set(cr1);
        } else if (config.mode >= TimerMode::CenterAligned1 &&
                   config.mode <= TimerMode::CenterAligned3) {
            uint32_t cms_val = static_cast<uint32_t>(config.mode) - 2;
            cr1 = tim::cr1::CMS::write(cr1, cms_val);
        }

        // Set clock division
        cr1 = tim::cr1::CKD::write(cr1, config.clock_division);
        hw->CR1 = cr1;

        // Set prescaler and period
        hw->PSC = config.prescaler;
        hw->ARR = config.period;

        // Generate update event to load prescaler
        hw->EGR = tim::egr::UG::mask;

        //
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

        // Enable counter
        hw->CR1 |= tim::cr1::CEN::mask;

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

        // Disable counter
        hw->CR1 &= ~tim::cr1::CEN::mask;

        return Ok();
    }

    /**
     * @brief Configure PWM on a specific channel
     *
     * @param channel Channel to configure
     * @param mode PWM mode
     * @param duty Duty cycle value (0 to period)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> configurePwmChannel(Channel channel, PwmMode mode, uint32_t duty) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Configure channel mode and duty
        uint8_t ch = static_cast<uint8_t>(channel);
        uint32_t mode_val = static_cast<uint32_t>(mode);

        if (ch == 1 || ch == 2) {
            // Channel 1 or 2 - use CCMR1
            uint32_t ccmr1 = hw->CCMR1_Output;

            if (ch == 1) {
                // Configure CC1 as output
                ccmr1 = tim::ccmr1_output::CC1S::write(ccmr1, 0);
                ccmr1 = tim::ccmr1_output::OC1M::write(ccmr1, mode_val);
                ccmr1 = tim::ccmr1_output::OC1PE::set(ccmr1);  // Preload enable
                hw->CCR1 = duty;
            } else {
                // Configure CC2 as output
                ccmr1 = tim::ccmr1_output::CC2S::write(ccmr1, 0);
                ccmr1 = tim::ccmr1_output::OC2M::write(ccmr1, mode_val);
                ccmr1 = tim::ccmr1_output::OC2PE::set(ccmr1);
                hw->CCR2 = duty;
            }

            hw->CCMR1_Output = ccmr1;
        } else {
            // Channel 3 or 4 - use CCMR2
            uint32_t ccmr2 = hw->CCMR2_Output;

            if (ch == 3) {
                ccmr2 = tim::ccmr2_output::CC3S::write(ccmr2, 0);
                ccmr2 = tim::ccmr2_output::OC3M::write(ccmr2, mode_val);
                ccmr2 = tim::ccmr2_output::OC3PE::set(ccmr2);
                hw->CCR3 = duty;
            } else {
                ccmr2 = tim::ccmr2_output::CC4S::write(ccmr2, 0);
                ccmr2 = tim::ccmr2_output::OC4M::write(ccmr2, mode_val);
                ccmr2 = tim::ccmr2_output::OC4PE::set(ccmr2);
                hw->CCR4 = duty;
            }

            hw->CCMR2_Output = ccmr2;
        }

        // Enable the channel output
        uint32_t ccer = hw->CCER;
        ccer |= (tim::ccer::CC1E::mask << ((ch - 1) * 4));
        hw->CCER = ccer;

        return Ok();
    }

    /**
     * @brief Set PWM duty cycle for a channel
     *
     * @param channel Channel to update
     * @param duty Duty cycle value (0 to period)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setDuty(Channel channel, uint32_t duty) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        //
        uint8_t ch = static_cast<uint8_t>(channel);
        switch (ch) {
            case 1:
                hw->CCR1 = duty;
                break;
            case 2:
                hw->CCR2 = duty;
                break;
            case 3:
                hw->CCR3 = duty;
                break;
            case 4:
                hw->CCR4 = duty;
                break;
            default:
                return Err(ErrorCode::InvalidParameter);
        }

        return Ok();
    }

    /**
     * @brief Set period (frequency)
     *
     * @param period Period value (ARR register)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setPeriod(uint32_t period) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        //
        hw->ARR = period;
        m_config.period = period;

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

        return Ok(uint32_t(hw->CNT));
    }

    /**
     * @brief Enable timer interrupts
     *
     * @param interrupt_mask Interrupt mask (DIER register bits)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> enableInterrupts(uint32_t interrupt_mask) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        //
        hw->DIER |= interrupt_mask;

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

constexpr uint32_t TIMER1_BASE = 0x40010000;
constexpr uint32_t TIMER1_IRQ = 25;

constexpr uint32_t TIMER2_BASE = 0x40000000;
constexpr uint32_t TIMER2_IRQ = 28;

constexpr uint32_t TIMER3_BASE = 0x40000400;
constexpr uint32_t TIMER3_IRQ = 29;

constexpr uint32_t TIMER4_BASE = 0x40000800;
constexpr uint32_t TIMER4_IRQ = 30;

constexpr uint32_t TIMER5_BASE = 0x40000C00;
constexpr uint32_t TIMER5_IRQ = 50;

constexpr uint32_t TIMER9_BASE = 0x40014000;
constexpr uint32_t TIMER9_IRQ = 24;

constexpr uint32_t TIMER10_BASE = 0x40014400;
constexpr uint32_t TIMER10_IRQ = 25;

constexpr uint32_t TIMER11_BASE = 0x40014800;
constexpr uint32_t TIMER11_IRQ = 26;

using Timer1 = Timer<TIMER1_BASE, TIMER1_IRQ>;     ///< Advanced-control timer TIM1
using Timer2 = Timer<TIMER2_BASE, TIMER2_IRQ>;     ///< General-purpose timer TIM2
using Timer3 = Timer<TIMER3_BASE, TIMER3_IRQ>;     ///< General-purpose timer TIM3
using Timer4 = Timer<TIMER4_BASE, TIMER4_IRQ>;     ///< General-purpose timer TIM4
using Timer5 = Timer<TIMER5_BASE, TIMER5_IRQ>;     ///< General-purpose timer TIM5
using Timer9 = Timer<TIMER9_BASE, TIMER9_IRQ>;     ///< General-purpose timer TIM9
using Timer10 = Timer<TIMER10_BASE, TIMER10_IRQ>;  ///< General-purpose timer TIM10
using Timer11 = Timer<TIMER11_BASE, TIMER11_IRQ>;  ///< General-purpose timer TIM11

}  // namespace alloy::hal::stm32f4
