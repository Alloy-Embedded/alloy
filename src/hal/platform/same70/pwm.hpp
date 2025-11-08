/**
 * @file pwm.hpp
 * @brief Template-based PWM implementation for SAME70 (Platform Layer)
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
 * Auto-generated from: same70
 * Generator: generate_platform_pwm.py
 * Generated: 2025-11-07 17:50:07
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
#include "hal/vendors/atmel/same70/registers/pwm0_registers.hpp"



namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfield access
namespace pwm = alloy::hal::atmel::same70::pwm0;

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief PWM Clock Prescaler
 */
enum class PwmPrescaler : uint8_t {
    DIV_1 = 0,  ///< MCK/1
    DIV_2 = 1,  ///< MCK/2
    DIV_4 = 2,  ///< MCK/4
    DIV_8 = 3,  ///< MCK/8
    DIV_16 = 4,  ///< MCK/16
    DIV_32 = 5,  ///< MCK/32
    DIV_64 = 6,  ///< MCK/64
    DIV_128 = 7,  ///< MCK/128
    DIV_256 = 8,  ///< MCK/256
    DIV_512 = 9,  ///< MCK/512
    DIV_1024 = 10,  ///< MCK/1024
};


/**
 * @brief PWM configuration structure
 */
struct PwmConfig {
    PwmAlignment alignment = PwmAlignment::Edge;  ///< Alignment mode (from hal/types.hpp)
    PwmPolarity polarity = PwmPolarity::Normal;  ///< Polarity (from hal/types.hpp)
    PwmPrescaler prescaler = PwmPrescaler::DIV_1;  ///< Clock prescaler
    uint16_t period = 1000;  ///< Period (CPRD register value)
    uint16_t duty_cycle = 500;  ///< Duty cycle (CDTY register value)
    uint16_t dead_time_h = 0;  ///< Dead-time for high side output
    uint16_t dead_time_l = 0;  ///< Dead-time for low side output
};

// ============================================================================
// Additional Structures
// ============================================================================

/**
 * @brief PWM Channel Registers (per-channel). Each of the 4 PWM channels has its own register block at offset 0x200 + (channel * 0x20)
 */
struct PwmChannelRegisters {
    volatile uint32_t CMR;  ///< Channel Mode Register
    volatile uint32_t CDTY;  ///< Channel Duty Cycle Register
    volatile uint32_t CDTYUPD;  ///< Channel Duty Cycle Update Register
    volatile uint32_t CPRD;  ///< Channel Period Register
    volatile uint32_t CPRDUPD;  ///< Channel Period Update Register
    volatile uint32_t CCNT;  ///< Channel Counter Register (read-only)
    volatile uint32_t DT;  ///< Channel Dead Time Register
    volatile uint32_t DTUPD;  ///< Channel Dead Time Update Register
};


/**
 * @brief Template-based PWM peripheral for SAME70
 *
 * This class provides a template-based PWM implementation with ZERO runtime
 * overhead. All peripheral configuration is resolved at compile-time.
 *
 * Template Parameters:
 * - BASE_ADDR: PWM block base address
 * - CHANNEL: Channel number (0-3)
 * - IRQ_ID: PWM IRQ ID
 *
 * Example usage:
 * @code
 * // Basic PWM usage
 * using MyPwm = Pwm<PWM0_BASE, 0, PWM0_IRQ>;
 * auto pwm = MyPwm{};
 * PwmConfig config;
 * config.prescaler = PwmPrescaler::DIV_1;
 * config.period = 1000;
 * config.duty_cycle = 500;  // 50%
 * pwm.open();
 * pwm.configure(config);
 * pwm.start();
 * @endcode
 *
 * @tparam BASE_ADDR PWM block base address
 * @tparam CHANNEL Channel number (0-3)
 * @tparam IRQ_ID PWM IRQ ID
 */
template <uint32_t BASE_ADDR, uint8_t CHANNEL, uint32_t IRQ_ID>
class Pwm {
    static_assert(CHANNEL < 4, "PWM has 4 channels (0-3)");
public:
    // Compile-time constants
    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint8_t channel = CHANNEL;
    static constexpr uint32_t irq_id = IRQ_ID;

    // Configuration constants
    static constexpr uint32_t channel_offset = 0x200 + (CHANNEL * 0x20);  ///< Channel register offset

    /**
     * @brief Get PWM peripheral registers
     *
     * Returns pointer to PWM registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile alloy::hal::atmel::same70::pwm0::PWM0_Registers* get_hw() {
#ifdef ALLOY_PWM_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_PWM_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::atmel::same70::pwm0::PWM0_Registers*>(BASE_ADDR);
#endif
    }

    /**
     * @brief Get PWM channel registers
     */
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
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> open() {
        auto* hw = get_hw();

        if (m_opened) {
            return Err(ErrorCode::AlreadyInitialized);
        }

        // Enable peripheral clock
        // TODO: Enable peripheral clock via PMC

        // Disable channel initially
        hw->DIS = (1u << CHANNEL);

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

        // Disable channel
        hw->DIS = (1u << CHANNEL);

        m_opened = false;

        return Ok();
    }

    /**
     * @brief Configure PWM channel
     *
     * @param config PWM configuration
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> configure(const PwmConfig& config) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Build CMR (Channel Mode Register)
        auto* channel = get_channel();
        
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

        // Enable channel
        hw->ENA = (1u << CHANNEL);

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

        // Disable channel
        hw->DIS = (1u << CHANNEL);

        return Ok();
    }

    /**
     * @brief Set PWM duty cycle
     *
     * @param duty_cycle Duty cycle value (0 to period)
     * @return Result<void, ErrorCode>     *
     * @note For left-aligned: 0 = always low, period = always high. For center-aligned: 0 = always low, period/2 = 50% duty
     */
    Result<void, ErrorCode> setDutyCycle(uint16_t duty_cycle) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Write to update register (takes effect at next period)
        auto* channel = get_channel();
        channel->CDTYUPD = duty_cycle;

        return Ok();
    }

    /**
     * @brief Set PWM period (frequency)
     *
     * @param period Period value (CPRD register)
     * @return Result<void, ErrorCode>     *
     * @note Takes effect at next period boundary
     */
    Result<void, ErrorCode> setPeriod(uint16_t period) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Write to update register
        auto* channel = get_channel();
        channel->CPRDUPD = period;

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

        // 
        auto* channel = get_channel();

        return Ok(uint32_t(channel->CCNT));
    }

    /**
     * @brief Check if PWM is running
     *
     * @return bool Check if PWM is running     */
    bool isRunning() const {
        auto* hw = get_hw();

        // 
        if (!m_opened) return false;
        return (hw->SR & (1u << CHANNEL)) != 0;

        return false;
    }

    /**
     * @brief Enable channel interrupts
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> enableInterrupts() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Enable period interrupt for this channel
        hw->IER1 = (1u << CHANNEL);

        return Ok();
    }

    /**
     * @brief Check if PWM is open
     *
     * @return bool Check if PWM is open     */
    bool isOpen() const {
        return m_opened;
    }

private:
    bool m_opened = false;  ///< Tracks if peripheral is initialized
    PwmConfig m_config = {};  ///< Current configuration
};

// ==============================================================================
// Predefined PWM Instances
// ==============================================================================

constexpr uint32_t PWM0CH0_BASE = 0x40020000;
constexpr uint32_t PWM0CH0_IRQ = 31;

constexpr uint32_t PWM0CH1_BASE = 0x40020000;
constexpr uint32_t PWM0CH1_IRQ = 31;

constexpr uint32_t PWM0CH2_BASE = 0x40020000;
constexpr uint32_t PWM0CH2_IRQ = 31;

constexpr uint32_t PWM0CH3_BASE = 0x40020000;
constexpr uint32_t PWM0CH3_IRQ = 31;

constexpr uint32_t PWM1CH0_BASE = 0x4005C000;
constexpr uint32_t PWM1CH0_IRQ = 60;

constexpr uint32_t PWM1CH1_BASE = 0x4005C000;
constexpr uint32_t PWM1CH1_IRQ = 60;

constexpr uint32_t PWM1CH2_BASE = 0x4005C000;
constexpr uint32_t PWM1CH2_IRQ = 60;

constexpr uint32_t PWM1CH3_BASE = 0x4005C000;
constexpr uint32_t PWM1CH3_IRQ = 60;

using Pwm0Ch0 = Pwm<PWM0CH0_BASE, 0, PWM0CH0_IRQ>;  ///< PWM0 Channel 0
using Pwm0Ch1 = Pwm<PWM0CH1_BASE, 1, PWM0CH1_IRQ>;  ///< PWM0 Channel 1
using Pwm0Ch2 = Pwm<PWM0CH2_BASE, 2, PWM0CH2_IRQ>;  ///< PWM0 Channel 2
using Pwm0Ch3 = Pwm<PWM0CH3_BASE, 3, PWM0CH3_IRQ>;  ///< PWM0 Channel 3
using Pwm1Ch0 = Pwm<PWM1CH0_BASE, 0, PWM1CH0_IRQ>;  ///< PWM1 Channel 0
using Pwm1Ch1 = Pwm<PWM1CH1_BASE, 1, PWM1CH1_IRQ>;  ///< PWM1 Channel 1
using Pwm1Ch2 = Pwm<PWM1CH2_BASE, 2, PWM1CH2_IRQ>;  ///< PWM1 Channel 2
using Pwm1Ch3 = Pwm<PWM1CH3_BASE, 3, PWM1CH3_IRQ>;  ///< PWM1 Channel 3

} // namespace alloy::hal::same70
