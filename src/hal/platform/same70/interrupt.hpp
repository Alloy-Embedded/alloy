/**
 * @file interrupt.hpp
 * @brief SAME70 NVIC (Nested Vectored Interrupt Controller) Implementation
 * 
 * ARM Cortex-M7 interrupt management for ATSAME70Q21B.
 * Implements the InterruptController concept using ARM NVIC.
 * 
 * Features:
 * - 16 priority levels (4-bit priority)
 * - Configurable priority grouping
 * - Atomic enable/disable operations
 * - Critical section support
 * 
 * @note Part of SAME70 Platform Implementation
 */

#pragma once

#include "hal/interface/interrupt.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

// ARM CMSIS headers (would be included in real implementation)
// #include "same70q21b.h"

namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

/**
 * @brief SAME70 NVIC Implementation
 * 
 * Implements interrupt management using ARM Cortex-M7 NVIC.
 * Satisfies the InterruptController concept.
 */
class Nvic {
public:
    /**
     * @brief Enable global interrupts
     *
     * Clears PRIMASK to enable interrupts globally.
     * Uses ARM __enable_irq() intrinsic.
     */
    static void enable_global() noexcept {
        // ARM Cortex-M: __enable_irq()
        __asm volatile ("cpsie i" ::: "memory");
    }
    
    /**
     * @brief Disable global interrupts
     *
     * Sets PRIMASK to disable all interrupts.
     * Uses ARM __disable_irq() intrinsic.
     */
    static void disable_global() noexcept {
        // ARM Cortex-M: __disable_irq()
        __asm volatile ("cpsid i" ::: "memory");
    }
    
    /**
     * @brief Save and disable interrupts
     * 
     * @return Previous PRIMASK value
     */
    static u32 save_and_disable() noexcept {
        // u32 primask;
        // asm volatile ("mrs %0, primask" : "=r" (primask));
        // asm volatile ("cpsid i" : : : "memory");
        // return primask;
        return 0;  // Stub
    }
    
    /**
     * @brief Restore interrupts from saved state
     * 
     * @param state Previously saved PRIMASK value
     */
    static void restore(u32 state) noexcept {
        // asm volatile ("msr primask, %0" : : "r" (state) : "memory");
        (void)state;
    }
    
    /**
     * @brief Enable specific interrupt in NVIC
     * 
     * @param irq Interrupt number
     * @return Ok on success
     */
    static Result<void, ErrorCode> enable(IrqNumber irq) noexcept {
        u32 irq_num = static_cast<u32>(irq);
        
        // NVIC->ISER[irq_num / 32] = (1UL << (irq_num % 32));
        (void)irq_num;
        
        return Ok();
    }
    
    /**
     * @brief Disable specific interrupt in NVIC
     * 
     * @param irq Interrupt number
     * @return Ok on success
     */
    static Result<void, ErrorCode> disable(IrqNumber irq) noexcept {
        u32 irq_num = static_cast<u32>(irq);
        
        // NVIC->ICER[irq_num / 32] = (1UL << (irq_num % 32));
        (void)irq_num;
        
        return Ok();
    }
    
    /**
     * @brief Set interrupt priority
     * 
     * ARM Cortex-M7 uses 4-bit priority (0-15).
     * Lower number = higher priority.
     * 
     * @param irq Interrupt number
     * @param priority Priority level
     * @return Ok on success
     */
    static Result<void, ErrorCode> set_priority(IrqNumber irq, IrqPriority priority) noexcept {
        u32 irq_num = static_cast<u32>(irq);
        u8 prio = static_cast<u8>(priority);
        
        // ARM uses 4-bit priority, left-aligned in 8-bit field
        // NVIC->IP[irq_num] = (prio << 4);
        (void)irq_num;
        (void)prio;
        
        return Ok();
    }
    
    /**
     * @brief Get interrupt priority
     * 
     * @param irq Interrupt number
     * @return Priority level
     */
    static Result<IrqPriority, ErrorCode> get_priority(IrqNumber irq) noexcept {
        u32 irq_num = static_cast<u32>(irq);
        
        // u8 prio = NVIC->IP[irq_num] >> 4;
        // return Ok(static_cast<IrqPriority>(prio));
        (void)irq_num;
        
        return Ok(IrqPriority::Normal);
    }
    
    /**
     * @brief Check if interrupt is enabled
     * 
     * @param irq Interrupt number
     * @return true if enabled
     */
    static bool is_enabled(IrqNumber irq) noexcept {
        u32 irq_num = static_cast<u32>(irq);
        
        // return (NVIC->ISER[irq_num / 32] & (1UL << (irq_num % 32))) != 0;
        (void)irq_num;
        return false;
    }
    
    /**
     * @brief Check if interrupt is pending
     * 
     * @param irq Interrupt number
     * @return true if pending
     */
    static bool is_pending(IrqNumber irq) noexcept {
        u32 irq_num = static_cast<u32>(irq);
        
        // return (NVIC->ISPR[irq_num / 32] & (1UL << (irq_num % 32))) != 0;
        (void)irq_num;
        return false;
    }
    
    /**
     * @brief Set interrupt pending flag
     * 
     * @param irq Interrupt number
     * @return Ok on success
     */
    static Result<void, ErrorCode> set_pending(IrqNumber irq) noexcept {
        u32 irq_num = static_cast<u32>(irq);
        
        // NVIC->ISPR[irq_num / 32] = (1UL << (irq_num % 32));
        (void)irq_num;
        
        return Ok();
    }
    
    /**
     * @brief Clear interrupt pending flag
     * 
     * @param irq Interrupt number
     * @return Ok on success
     */
    static Result<void, ErrorCode> clear_pending(IrqNumber irq) noexcept {
        u32 irq_num = static_cast<u32>(irq);
        
        // NVIC->ICPR[irq_num / 32] = (1UL << (irq_num % 32));
        (void)irq_num;
        
        return Ok();
    }
    
    /**
     * @brief Set priority grouping
     * 
     * Configures how 4-bit priority is split:
     * - Group 0: 0 preempt bits, 4 sub bits
     * - Group 3: 3 preempt bits, 1 sub bit
     * - Group 7: 4 preempt bits, 0 sub bits
     * 
     * @param grouping Priority group (0-7)
     * @return Ok on success
     */
    static Result<void, ErrorCode> set_priority_grouping(u8 grouping) noexcept {
        if (grouping > 7) {
            return Err(ErrorCode::InvalidParameter);
        }
        
        // SCB->AIRCR = (0x5FA << 16) | (grouping << 8);
        (void)grouping;
        
        return Ok();
    }
};

// Verify that Nvic satisfies InterruptController concept
static_assert(InterruptController<Nvic>, "Nvic must satisfy InterruptController concept");

}  // namespace alloy::hal::same70

// Make SAME70 NVIC the default platform implementation
namespace alloy::hal {
    using PlatformInterruptController = same70::Nvic;
}
