/**
 * @file interrupt_expert.hpp
 * @brief Level 2 Expert API for Interrupt Management
 * @note Advanced interrupt control with fine-grained configuration
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/interface/interrupt.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Expert interrupt configuration
 * 
 * Provides full control over interrupt configuration including:
 * - Priority grouping
 * - Sub-priority
 * - Pending/Active flags
 * - Platform-specific features
 */
struct InterruptExpertConfig {
    IrqNumber irq_number;
    u8 preempt_priority;   ///< Preemption priority (0-15, lower = higher)
    u8 sub_priority;       ///< Sub-priority within same preemption level
    bool enable;
    bool trigger_pending;  ///< Set pending flag on init

    constexpr bool is_valid() const {
        // ARM Cortex-M typically has 4 bits priority (0-15)
        if (preempt_priority > 15) {
            return false;
        }
        if (sub_priority > 15) {
            return false;
        }
        return true;
    }

    constexpr const char* error_message() const {
        if (preempt_priority > 15) {
            return "Preempt priority out of range (0-15)";
        }
        if (sub_priority > 15) {
            return "Sub-priority out of range (0-15)";
        }
        return "Valid";
    }

    /**
     * @brief Standard configuration with normal priority
     */
    static constexpr InterruptExpertConfig standard(IrqNumber irq) {
        return InterruptExpertConfig{
            .irq_number = irq,
            .preempt_priority = 8,
            .sub_priority = 0,
            .enable = true,
            .trigger_pending = false
        };
    }

    /**
     * @brief High priority configuration
     */
    static constexpr InterruptExpertConfig high_priority(IrqNumber irq) {
        return InterruptExpertConfig{
            .irq_number = irq,
            .preempt_priority = 2,
            .sub_priority = 0,
            .enable = true,
            .trigger_pending = false
        };
    }

    /**
     * @brief Low priority configuration
     */
    static constexpr InterruptExpertConfig low_priority(IrqNumber irq) {
        return InterruptExpertConfig{
            .irq_number = irq,
            .preempt_priority = 14,
            .sub_priority = 0,
            .enable = true,
            .trigger_pending = false
        };
    }
};

namespace expert {

/**
 * @brief Configure interrupt with expert settings
 * 
 * @param config Expert configuration
 * @return Result with error code
 */
inline Result<void, ErrorCode> configure(const InterruptExpertConfig& config) {
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }
    // Platform implementation would configure hardware here
    return Ok();
}

/**
 * @brief Get interrupt pending status
 * 
 * @param irq Interrupt number
 * @return true if pending
 */
inline bool is_pending(IrqNumber irq) {
    // Platform implementation
    return false;
}

/**
 * @brief Set interrupt pending
 * 
 * @param irq Interrupt number
 * @return Result with error code
 */
inline Result<void, ErrorCode> set_pending(IrqNumber irq) {
    // Platform implementation
    return Ok();
}

/**
 * @brief Clear interrupt pending
 * 
 * @param irq Interrupt number
 * @return Result with error code
 */
inline Result<void, ErrorCode> clear_pending(IrqNumber irq) {
    // Platform implementation
    return Ok();
}

/**
 * @brief Check if interrupt is active (currently executing)
 * 
 * @param irq Interrupt number
 * @return true if active
 */
inline bool is_active(IrqNumber irq) {
    // Platform implementation
    return false;
}

/**
 * @brief Get current interrupt priority
 * 
 * @param irq Interrupt number
 * @return Result with priority or error
 */
inline Result<u8, ErrorCode> get_priority(IrqNumber irq) {
    // Platform implementation
    return Ok(static_cast<u8>(8));
}

/**
 * @brief Set priority grouping (ARM Cortex-M specific)
 * 
 * Configures how the 4-bit priority is split between
 * preemption priority and sub-priority.
 * 
 * @param grouping Priority grouping (0-7)
 * @return Result with error code
 */
inline Result<void, ErrorCode> set_priority_grouping(u8 grouping) {
    if (grouping > 7) {
        return Err(ErrorCode::InvalidParameter);
    }
    // Platform implementation
    return Ok();
}

}  // namespace expert

}  // namespace alloy::hal
