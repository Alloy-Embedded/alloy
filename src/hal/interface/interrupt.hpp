/**
 * @file interrupt.hpp
 * @brief Platform-agnostic Interrupt Management Interface
 * 
 * Provides unified interrupt control for all platforms including:
 * - Global interrupt enable/disable (critical sections)
 * - Specific interrupt enable/disable (NVIC on ARM, etc)
 * - Interrupt priority configuration
 * - Interrupt pending/active status
 * 
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

#include <concepts>
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Interrupt Request Number
 * 
 * Platform-specific IRQ numbers. Each platform defines its own IRQ map.
 * Common examples:
 * - ARM Cortex-M: IRQ0-255 (NVIC)
 * - RISC-V: Platform-specific PLIC
 * - x86: Legacy PIC or APIC
 */
enum class IrqNumber : u16 {
    // Timer/SysTick
    SysTick = 0,
    
    // UART
    UART0 = 10,
    UART1 = 11,
    UART2 = 12,
    UART3 = 13,
    
    // SPI
    SPI0 = 20,
    SPI1 = 21,
    SPI2 = 22,
    
    // I2C
    I2C0 = 30,
    I2C1 = 31,
    I2C2 = 32,
    
    // DMA
    DMA_Stream0 = 40,
    DMA_Stream1 = 41,
    DMA_Stream2 = 42,
    DMA_Stream3 = 43,
    DMA_Stream4 = 44,
    DMA_Stream5 = 45,
    DMA_Stream6 = 46,
    DMA_Stream7 = 47,
    
    // ADC
    ADC0 = 50,
    ADC1 = 51,
    
    // Timers
    TIMER0 = 60,
    TIMER1 = 61,
    TIMER2 = 62,
    TIMER3 = 63,
    
    // GPIO/External Interrupts
    EXTI0 = 70,
    EXTI1 = 71,
    EXTI2 = 72,
    EXTI3 = 73,
    EXTI4 = 74,
    EXTI5_9 = 75,
    EXTI10_15 = 76,
    
    // Platform-specific can extend this
};

/**
 * @brief Interrupt priority level
 * 
 * Lower number = higher priority
 * Valid range: 0-255 (platform may support fewer levels)
 */
enum class IrqPriority : u8 {
    Highest = 0,
    VeryHigh = 32,
    High = 64,
    AboveNormal = 96,
    Normal = 128,
    BelowNormal = 160,
    Low = 192,
    VeryLow = 224,
    Lowest = 255
};

/**
 * @brief Interrupt configuration
 */
struct InterruptConfig {
    IrqNumber irq_number;
    IrqPriority priority;
    bool enable;

    constexpr bool is_valid() const {
        return true;  // Basic validation
    }
};

/**
 * @brief Critical Section RAII wrapper
 * 
 * Automatically disables interrupts on construction and
 * restores them on destruction. Use for thread-safe critical sections.
 * 
 * Example:
 * @code
 * {
 *     CriticalSection cs;  // Interrupts disabled
 *     // ... critical code ...
 * }  // Interrupts restored
 * @endcode
 */
class CriticalSection {
public:
    CriticalSection() noexcept;
    ~CriticalSection() noexcept;
    
    // Non-copyable, non-movable
    CriticalSection(const CriticalSection&) = delete;
    CriticalSection& operator=(const CriticalSection&) = delete;
    CriticalSection(CriticalSection&&) = delete;
    CriticalSection& operator=(CriticalSection&&) = delete;

private:
    u32 saved_state_;  // Platform-specific interrupt state
};

/**
 * @brief Interrupt Controller concept
 * 
 * Defines the interface for interrupt management.
 * Implemented by platform-specific code (NVIC, PLIC, etc).
 */
template <typename T>
concept InterruptController = requires(T controller, IrqNumber irq, IrqPriority priority) {
    /// Enable global interrupts
    { T::enable_global() } -> std::same_as<void>;
    
    /// Disable global interrupts
    { T::disable_global() } -> std::same_as<void>;
    
    /// Save and disable global interrupts (returns saved state)
    { T::save_and_disable() } -> std::same_as<u32>;
    
    /// Restore global interrupts from saved state
    { T::restore(u32{}) } -> std::same_as<void>;
    
    /// Enable specific interrupt
    { T::enable(irq) } -> std::same_as<Result<void, ErrorCode>>;
    
    /// Disable specific interrupt
    { T::disable(irq) } -> std::same_as<Result<void, ErrorCode>>;
    
    /// Set interrupt priority
    { T::set_priority(irq, priority) } -> std::same_as<Result<void, ErrorCode>>;
    
    /// Get interrupt priority
    { T::get_priority(irq) } -> std::same_as<Result<IrqPriority, ErrorCode>>;
    
    /// Check if interrupt is enabled
    { T::is_enabled(irq) } -> std::same_as<bool>;
    
    /// Check if interrupt is pending
    { T::is_pending(irq) } -> std::same_as<bool>;
    
    /// Set interrupt pending
    { T::set_pending(irq) } -> std::same_as<Result<void, ErrorCode>>;
    
    /// Clear interrupt pending
    { T::clear_pending(irq) } -> std::same_as<Result<void, ErrorCode>>;
};

}  // namespace alloy::hal
