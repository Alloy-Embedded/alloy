/**
 * @file peripheral_interrupt.hpp
 * @brief Peripheral Interrupt Integration Helpers
 * 
 * Provides helper functions to integrate interrupt management
 * with peripheral configurations.
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/api/interrupt_simple.hpp"
#include "hal/api/interrupt_expert.hpp"
#include "hal/signals.hpp"

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;

/**
 * @brief Map PeripheralId to IrqNumber
 * 
 * @param peripheral Peripheral ID
 * @return Corresponding IRQ number
 */
constexpr IrqNumber peripheral_to_irq(PeripheralId peripheral) {
    switch (peripheral) {
        case PeripheralId::USART0:
            return IrqNumber::UART0;
        case PeripheralId::USART1:
            return IrqNumber::UART1;
        case PeripheralId::USART2:
            return IrqNumber::UART2;
            
        case PeripheralId::SPI0:
            return IrqNumber::SPI0;
        case PeripheralId::SPI1:
            return IrqNumber::SPI1;
        case PeripheralId::SPI2:
            return IrqNumber::SPI2;
            
        case PeripheralId::I2C0:
            return IrqNumber::I2C0;
        case PeripheralId::I2C1:
            return IrqNumber::I2C1;
        case PeripheralId::I2C2:
            return IrqNumber::I2C2;
            
        case PeripheralId::ADC0:
            return IrqNumber::ADC0;
        case PeripheralId::ADC1:
            return IrqNumber::ADC1;
            
        case PeripheralId::TIMER0:
            return IrqNumber::TIMER0;
        case PeripheralId::TIMER1:
            return IrqNumber::TIMER1;
        case PeripheralId::TIMER2:
            return IrqNumber::TIMER2;
            
        default:
            return IrqNumber::UART0;  // Fallback
    }
}

/**
 * @brief Configure peripheral interrupt with simple settings
 * 
 * @param peripheral Peripheral ID
 * @param priority Interrupt priority
 * @return Result with error code
 */
inline Result<void, ErrorCode> configure_peripheral_interrupt(
    PeripheralId peripheral,
    IrqPriority priority = IrqPriority::Normal) {
    
    auto irq = peripheral_to_irq(peripheral);
    
    // Set priority
    auto result = Interrupt::set_priority(irq, priority);
    if (!result.is_ok()) {
        return result;
    }
    
    // Enable interrupt
    return Interrupt::enable(irq);
}

/**
 * @brief Configure peripheral interrupt with expert settings
 * 
 * @param peripheral Peripheral ID
 * @param preempt_priority Preemption priority (0-15)
 * @param sub_priority Sub-priority (0-15)
 * @return Result with error code
 */
inline Result<void, ErrorCode> configure_peripheral_interrupt_expert(
    PeripheralId peripheral,
    u8 preempt_priority,
    u8 sub_priority = 0) {
    
    auto irq = peripheral_to_irq(peripheral);
    
    InterruptExpertConfig config = {
        .irq_number = irq,
        .preempt_priority = preempt_priority,
        .sub_priority = sub_priority,
        .enable = true,
        .trigger_pending = false
    };
    
    return expert::configure(config);
}

/**
 * @brief Disable peripheral interrupt
 * 
 * @param peripheral Peripheral ID
 * @return Result with error code
 */
inline Result<void, ErrorCode> disable_peripheral_interrupt(PeripheralId peripheral) {
    auto irq = peripheral_to_irq(peripheral);
    return Interrupt::disable(irq);
}

/**
 * @brief Enable peripheral interrupt
 * 
 * @param peripheral Peripheral ID
 * @return Result with error code
 */
inline Result<void, ErrorCode> enable_peripheral_interrupt(PeripheralId peripheral) {
    auto irq = peripheral_to_irq(peripheral);
    return Interrupt::enable(irq);
}

/**
 * @brief Check if peripheral interrupt is enabled
 * 
 * @param peripheral Peripheral ID
 * @return true if enabled
 */
inline bool is_peripheral_interrupt_enabled(PeripheralId peripheral) {
    auto irq = peripheral_to_irq(peripheral);
    return Interrupt::is_enabled(irq);
}

}  // namespace alloy::hal
