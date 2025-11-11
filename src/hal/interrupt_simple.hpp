/**
 * @file interrupt_simple.hpp
 * @brief Level 1 Simple API for Interrupt Management
 * @note Unified interrupt control for applications
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/interface/interrupt.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Simple interrupt management API
 * 
 * Provides easy-to-use functions for common interrupt operations:
 * - Global enable/disable
 * - Critical sections (RAII)
 * - Specific interrupt enable/disable
 * 
 * Example:
 * @code
 * // Global interrupt control
 * Interrupt::disable_all();
 * // ... critical code ...
 * Interrupt::enable_all();
 * 
 * // RAII critical section (preferred)
 * {
 *     auto cs = Interrupt::critical_section();
 *     // ... critical code ...
 * }  // Interrupts restored automatically
 * 
 * // Specific interrupt
 * Interrupt::enable(IrqNumber::UART0);
 * Interrupt::disable(IrqNumber::UART0);
 * @endcode
 */
class Interrupt {
public:
    /**
     * @brief Enable all interrupts globally
     * 
     * Enables the global interrupt flag (I-bit on ARM, etc).
     */
    static void enable_all() noexcept;
    
    /**
     * @brief Disable all interrupts globally
     * 
     * Disables the global interrupt flag.
     * Use for very short critical sections only.
     * Prefer critical_section() for RAII safety.
     */
    static void disable_all() noexcept;
    
    /**
     * @brief Create RAII critical section
     * 
     * Returns object that disables interrupts on construction
     * and restores them on destruction.
     * 
     * @return CriticalSection object
     */
    static CriticalSection critical_section() noexcept {
        return CriticalSection{};
    }
    
    /**
     * @brief Enable specific interrupt
     * 
     * @param irq Interrupt number to enable
     * @return Result with error code
     */
    static Result<void, ErrorCode> enable(IrqNumber irq) noexcept;
    
    /**
     * @brief Disable specific interrupt
     * 
     * @param irq Interrupt number to disable
     * @return Result with error code
     */
    static Result<void, ErrorCode> disable(IrqNumber irq) noexcept;
    
    /**
     * @brief Check if interrupt is enabled
     * 
     * @param irq Interrupt number to check
     * @return true if enabled
     */
    static bool is_enabled(IrqNumber irq) noexcept;
    
    /**
     * @brief Set interrupt priority to common levels
     * 
     * @param irq Interrupt number
     * @param priority Priority level (use IrqPriority enum)
     * @return Result with error code
     */
    static Result<void, ErrorCode> set_priority(
        IrqNumber irq,
        IrqPriority priority) noexcept;
};

/**
 * @brief Scoped interrupt disable (RAII)
 * 
 * Alternative to critical_section() with explicit naming.
 * Disables interrupts on construction, enables on destruction.
 * 
 * Example:
 * @code
 * {
 *     ScopedInterruptDisable guard;
 *     // ... interrupts disabled ...
 * }  // interrupts restored
 * @endcode
 */
class ScopedInterruptDisable {
public:
    ScopedInterruptDisable() noexcept : cs_() {}
    
    ~ScopedInterruptDisable() noexcept = default;
    
    // Non-copyable, non-movable
    ScopedInterruptDisable(const ScopedInterruptDisable&) = delete;
    ScopedInterruptDisable& operator=(const ScopedInterruptDisable&) = delete;
    ScopedInterruptDisable(ScopedInterruptDisable&&) = delete;
    ScopedInterruptDisable& operator=(ScopedInterruptDisable&&) = delete;

private:
    CriticalSection cs_;
};

}  // namespace alloy::hal
