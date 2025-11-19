/**
 * @file pwm.hpp
 * @brief Template-based PWM implementation for STM32F4 (Platform Layer)
 *
 * This file implements PWM peripheral using templates with ZERO virtual
 * functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Peripheral address and IRQ resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Error handling: Uses Result<T, ErrorCode> for robust error handling
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: stm32f4
 * Generator: generate_platform_pwm.py
 * Generated: 2025-11-07 17:51:30
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

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief PWM clock prescaler (maps to PSC register)
 */
enum class PwmPrescaler : uint8_t {
    DIV_1 = 0,      ///< Prescaler = 1 (no division)
    DIV_2 = 1,      ///< Prescaler = 2
    DIV_4 = 3,      ///< Prescaler = 4
    DIV_8 = 7,      ///< Prescaler = 8
    DIV_16 = 15,    ///< Prescaler = 16
    DIV_32 = 31,    ///< Prescaler = 32
    DIV_64 = 63,    ///< Prescaler = 64
    DIV_128 = 127,  ///< Prescaler = 128
};


/**
 * @brief PWM configuration structure
 */
struct PwmConfig {
    PwmAlignment alignment = PwmAlignment::Edge;  ///< Alignment mode (from hal/types.hpp)
    PwmPolarity polarity = PwmPolarity::Normal;   ///< Polarity (from hal/types.hpp)
    uint16_t prescaler = 0;                       ///< Prescaler value (0-65535)
    uint32_t period = 1000;                       ///< Auto-reload period
    uint32_t duty_cycle = 500;                    ///< Duty cycle value (0 to period)
};


/**
 * @brief Template-based PWM peripheral for STM32F4
 *
 * This class provides a template-based PWM implementation with ZERO runtime
 * overhead. All peripheral configuration is resolved at compile-time.
 *
 * Template Parameters:
 * - BASE_ADDR: Timer peripheral base address
 * - CHANNEL: PWM channel number (1-4)
 * - IRQ_ID: Timer IRQ ID
 *
 * Example usage:
 * @code
 * // Basic PWM usage
 * using MyPwm = Pwm<TIM2_BASE, 1, TIM2_IRQ>;
 * auto pwm = MyPwm{};
 * PwmConfig config;
 * config.prescaler = 83;  // 84MHz / (83+1) = 1MHz
 * config.period = 1000;   // 1kHz
 * config.duty_cycle = 500;  // 50%
 * pwm.open();
 * pwm.configure(config);
 * pwm.start();
 * @endcode
 *
 * @tparam BASE_ADDR Timer peripheral base address
 * @tparam CHANNEL PWM channel number (1-4)
 * @tparam IRQ_ID Timer IRQ ID
 */
template <uint32_t BASE_ADDR, uint8_t CHANNEL, uint32_t IRQ_ID>
class Pwm {
    static_assert(CHANNEL >= 1 && CHANNEL <= 4, "TIM has 4 PWM channels (1-4)");

   public:
    // Compile-time constants
    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint8_t channel = CHANNEL;
    static constexpr uint32_t irq_id = IRQ_ID;

    // Configuration constants
    static constexpr uint32_t PWM_MODE1 = 6;  ///< PWM mode 1 (active high)
    static constexpr uint32_t PWM_MODE2 = 7;  ///< PWM mode 2 (active low)

    /**
     * @brief Get PWM (TIM-based) peripheral registers
     *
     * Returns pointer to PWM (TIM-based) registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile alloy::hal::st::stm32f4::tim2::TIM2_Registers* get_hw() {
#ifdef ALLOY_PWM_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_PWM_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::st::stm32f4::tim2::TIM2_Registers*>(BASE_ADDR);
#endif
    }


    constexpr Pwm() = default;

    /**
     * @brief Initialize PWM
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> open() {
        auto* hw = get_hw();

        if (m_opened) {
            return Err(ErrorCode::AlreadyInitialized);
        }

        // Enable timer clock (RCC)
        // TODO: Enable peripheral clock via RCC

        // Disable counter initially
        hw->CR1 &= ~tim::cr1::CEN::mask;

        m_opened = true;

        return Ok();
    }

    /**
     * @brief Close PWM
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> close() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Disable counter
        hw->CR1 &= ~tim::cr1::CEN::mask;

        // Disable channel output
        uint32_t ccer = hw->CCER;
        ccer &= ~(tim::ccer::CC1E::mask << ((CHANNEL - 1) * 4));
        hw->CCER = ccer;

        m_opened = false;

        return Ok();
    }

    /**
     * @brief Configure PWM
     *
     * @param config PWM configuration
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> configure(const PwmConfig& config) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Disable counter during configuration
        hw->CR1 &= ~tim::cr1::CEN::mask;

        // Configure counting mode based on alignment
        uint32_t cr1 = hw->CR1 & ~(tim::cr1::DIR::mask | tim::cr1::CMS::mask);

        if (config.alignment == PwmAlignment::Center) {
            // Center-aligned mode 1
            cr1 = tim::cr1::CMS::write(cr1, 1);
        }
        // Edge-aligned is default (CMS = 0, DIR = 0 for up-counting)

        hw->CR1 = cr1;

        // Set prescaler and period
        hw->PSC = config.prescaler;
        hw->ARR = config.period;

        // Configure PWM mode for the channel
        uint32_t pwm_mode = (config.polarity == PwmPolarity::Inverted) ? PWM_MODE2 : PWM_MODE1;

        if (CHANNEL == 1 || CHANNEL == 2) {
            uint32_t ccmr1 = hw->CCMR1_Output;

            if (CHANNEL == 1) {
                ccmr1 = tim::ccmr1_output::CC1S::write(ccmr1, 0);  // Output
                ccmr1 = tim::ccmr1_output::OC1M::write(ccmr1, pwm_mode);
                ccmr1 = tim::ccmr1_output::OC1PE::set(ccmr1);  // Preload enable
            } else {
                ccmr1 = tim::ccmr1_output::CC2S::write(ccmr1, 0);
                ccmr1 = tim::ccmr1_output::OC2M::write(ccmr1, pwm_mode);
                ccmr1 = tim::ccmr1_output::OC2PE::set(ccmr1);
            }

            hw->CCMR1_Output = ccmr1;
        } else {
            uint32_t ccmr2 = hw->CCMR2_Output;

            if (CHANNEL == 3) {
                ccmr2 = tim::ccmr2_output::CC3S::write(ccmr2, 0);
                ccmr2 = tim::ccmr2_output::OC3M::write(ccmr2, pwm_mode);
                ccmr2 = tim::ccmr2_output::OC3PE::set(ccmr2);
            } else {
                ccmr2 = tim::ccmr2_output::CC4S::write(ccmr2, 0);
                ccmr2 = tim::ccmr2_output::OC4M::write(ccmr2, pwm_mode);
                ccmr2 = tim::ccmr2_output::OC4PE::set(ccmr2);
            }

            hw->CCMR2_Output = ccmr2;
        }

        // Set duty cycle
        switch (CHANNEL) {
            case 1:
                hw->CCR1 = config.duty_cycle;
                break;
            case 2:
                hw->CCR2 = config.duty_cycle;
                break;
            case 3:
                hw->CCR3 = config.duty_cycle;
                break;
            case 4:
                hw->CCR4 = config.duty_cycle;
                break;
        }

        // Generate update event to load prescaler and ARR
        hw->EGR = tim::egr::UG::mask;

        //
        m_config = config;

        return Ok();
    }

    /**
     * @brief Start PWM output
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> start() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Enable channel output
        uint32_t ccer = hw->CCER;
        ccer |= (tim::ccer::CC1E::mask << ((CHANNEL - 1) * 4));
        hw->CCER = ccer;

        // Enable counter
        hw->CR1 |= tim::cr1::CEN::mask;

        return Ok();
    }

    /**
     * @brief Stop PWM output
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> stop() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Disable channel output
        uint32_t ccer = hw->CCER;
        ccer &= ~(tim::ccer::CC1E::mask << ((CHANNEL - 1) * 4));
        hw->CCER = ccer;

        return Ok();
    }

    /**
     * @brief Set PWM duty cycle
     *
     * @param duty_cycle Duty cycle value (0 to period)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setDutyCycle(uint32_t duty_cycle) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        //
        switch (CHANNEL) {
            case 1:
                hw->CCR1 = duty_cycle;
                break;
            case 2:
                hw->CCR2 = duty_cycle;
                break;
            case 3:
                hw->CCR3 = duty_cycle;
                break;
            case 4:
                hw->CCR4 = duty_cycle;
                break;
        }
        m_config.duty_cycle = duty_cycle;

        return Ok();
    }

    /**
     * @brief Set PWM period (frequency)
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
     * @brief Check if PWM is running
     *
     * @return bool Check if PWM is running     */
    bool isRunning() const {
        auto* hw = get_hw();

        //
        if (!m_opened)
            return false;
        return (hw->CR1 & tim::cr1::CEN::mask) != 0;

        return false;
    }

    /**
     * @brief Check if PWM is open
     *
     * @return bool Check if PWM is open     */
    bool isOpen() const { return m_opened; }

   private:
    bool m_opened = false;    ///< Tracks if peripheral is initialized
    PwmConfig m_config = {};  ///< Current configuration
};

// ==============================================================================
// Predefined PWM Instances
// ==============================================================================

constexpr uint32_t PWM2CH1_BASE = 0x40000000;
constexpr uint32_t PWM2CH1_IRQ = 28;

constexpr uint32_t PWM2CH2_BASE = 0x40000000;
constexpr uint32_t PWM2CH2_IRQ = 28;

constexpr uint32_t PWM2CH3_BASE = 0x40000000;
constexpr uint32_t PWM2CH3_IRQ = 28;

constexpr uint32_t PWM2CH4_BASE = 0x40000000;
constexpr uint32_t PWM2CH4_IRQ = 28;

constexpr uint32_t PWM3CH1_BASE = 0x40000400;
constexpr uint32_t PWM3CH1_IRQ = 29;

constexpr uint32_t PWM3CH2_BASE = 0x40000400;
constexpr uint32_t PWM3CH2_IRQ = 29;

constexpr uint32_t PWM3CH3_BASE = 0x40000400;
constexpr uint32_t PWM3CH3_IRQ = 29;

constexpr uint32_t PWM3CH4_BASE = 0x40000400;
constexpr uint32_t PWM3CH4_IRQ = 29;

constexpr uint32_t PWM4CH1_BASE = 0x40000800;
constexpr uint32_t PWM4CH1_IRQ = 30;

constexpr uint32_t PWM4CH2_BASE = 0x40000800;
constexpr uint32_t PWM4CH2_IRQ = 30;

constexpr uint32_t PWM4CH3_BASE = 0x40000800;
constexpr uint32_t PWM4CH3_IRQ = 30;

constexpr uint32_t PWM4CH4_BASE = 0x40000800;
constexpr uint32_t PWM4CH4_IRQ = 30;

using Pwm2Ch1 = Pwm<PWM2CH1_BASE, 1, PWM2CH1_IRQ>;  ///< TIM2 Channel 1 (PWM)
using Pwm2Ch2 = Pwm<PWM2CH2_BASE, 2, PWM2CH2_IRQ>;  ///< TIM2 Channel 2 (PWM)
using Pwm2Ch3 = Pwm<PWM2CH3_BASE, 3, PWM2CH3_IRQ>;  ///< TIM2 Channel 3 (PWM)
using Pwm2Ch4 = Pwm<PWM2CH4_BASE, 4, PWM2CH4_IRQ>;  ///< TIM2 Channel 4 (PWM)
using Pwm3Ch1 = Pwm<PWM3CH1_BASE, 1, PWM3CH1_IRQ>;  ///< TIM3 Channel 1 (PWM)
using Pwm3Ch2 = Pwm<PWM3CH2_BASE, 2, PWM3CH2_IRQ>;  ///< TIM3 Channel 2 (PWM)
using Pwm3Ch3 = Pwm<PWM3CH3_BASE, 3, PWM3CH3_IRQ>;  ///< TIM3 Channel 3 (PWM)
using Pwm3Ch4 = Pwm<PWM3CH4_BASE, 4, PWM3CH4_IRQ>;  ///< TIM3 Channel 4 (PWM)
using Pwm4Ch1 = Pwm<PWM4CH1_BASE, 1, PWM4CH1_IRQ>;  ///< TIM4 Channel 1 (PWM)
using Pwm4Ch2 = Pwm<PWM4CH2_BASE, 2, PWM4CH2_IRQ>;  ///< TIM4 Channel 2 (PWM)
using Pwm4Ch3 = Pwm<PWM4CH3_BASE, 3, PWM4CH3_IRQ>;  ///< TIM4 Channel 3 (PWM)
using Pwm4Ch4 = Pwm<PWM4CH4_BASE, 4, PWM4CH4_IRQ>;  ///< TIM4 Channel 4 (PWM)

}  // namespace alloy::hal::stm32f4
