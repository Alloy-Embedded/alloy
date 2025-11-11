/**
 * @file peripheral_interrupts_demo.cpp
 * @brief Demonstration of Interrupt Management with Peripherals
 * 
 * Shows how to:
 * - Configure interrupts for UART, SPI, I2C, Timer
 * - Use critical sections for thread-safe operations
 * - Handle interrupt priorities
 * - Use RAII guards for safety
 */

#include "hal/uart_expert.hpp"
#include "hal/spi_expert.hpp"
#include "hal/i2c_expert.hpp"
#include "hal/timer_expert.hpp"
#include "hal/interrupt_simple.hpp"
#include "hal/interrupt_expert.hpp"
#include "hal/peripheral_interrupt.hpp"

using namespace alloy::hal;
using namespace alloy::core;

// ============================================================================
// Example 1: Simple Interrupt Configuration
// ============================================================================

void example_simple_interrupt_config() {
    // Enable global interrupts
    Interrupt::enable_all();
    
    // Configure UART interrupt with normal priority
    auto result = configure_peripheral_interrupt(
        PeripheralId::USART0,
        IrqPriority::Normal
    );
    
    if (result.is_ok()) {
        // UART0 interrupt is now enabled and configured
    }
    
    // Configure SPI interrupt with high priority
    configure_peripheral_interrupt(
        PeripheralId::SPI0,
        IrqPriority::High
    );
}

// ============================================================================
// Example 2: Expert Interrupt Configuration
// ============================================================================

void example_expert_interrupt_config() {
    // Configure UART with precise priority control
    // Preempt priority = 2 (high), Sub-priority = 0
    auto result = configure_peripheral_interrupt_expert(
        PeripheralId::USART0,
        2,  // preempt_priority
        0   // sub_priority
    );
    
    // Configure Timer with lower priority
    // Preempt priority = 8 (normal), Sub-priority = 1
    configure_peripheral_interrupt_expert(
        PeripheralId::TIMER0,
        8,  // preempt_priority
        1   // sub_priority
    );
}

// ============================================================================
// Example 3: Critical Sections for Thread-Safe Operations
// ============================================================================

volatile uint32_t shared_counter = 0;

void example_critical_section() {
    // Method 1: RAII critical section (preferred)
    {
        auto cs = Interrupt::critical_section();
        // Interrupts disabled - safe to access shared data
        shared_counter++;
        shared_counter += 100;
    }  // Interrupts automatically restored
    
    // Method 2: Scoped disable (explicit naming)
    {
        ScopedInterruptDisable guard;
        // Interrupts disabled
        shared_counter--;
    }  // Interrupts restored
    
    // Method 3: Manual control (use with caution)
    Interrupt::disable_all();
    shared_counter *= 2;
    Interrupt::enable_all();
}

// ============================================================================
// Example 4: UART with Interrupt Configuration
// ============================================================================

void example_uart_with_interrupts() {
    // Configure UART with interrupts enabled
    UartExpertConfig uart_config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = true,  // Enable UART interrupts
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };
    
    // Apply UART configuration
    // (Platform-specific implementation would configure hardware)
    
    // Configure UART interrupt priority
    configure_peripheral_interrupt(
        PeripheralId::USART0,
        IrqPriority::High
    );
}

// ============================================================================
// Example 5: Multiple Peripherals with Different Priorities
// ============================================================================

void example_multiple_peripherals() {
    // Configure interrupts for multiple peripherals
    // with appropriate priorities
    
    // Critical peripherals - highest priority
    configure_peripheral_interrupt(PeripheralId::USART0, IrqPriority::Highest);
    
    // Important peripherals - high priority
    configure_peripheral_interrupt(PeripheralId::SPI0, IrqPriority::High);
    configure_peripheral_interrupt(PeripheralId::I2C0, IrqPriority::High);
    
    // Normal peripherals - normal priority
    configure_peripheral_interrupt(PeripheralId::TIMER0, IrqPriority::Normal);
    configure_peripheral_interrupt(PeripheralId::ADC0, IrqPriority::Normal);
    
    // Background tasks - low priority
    configure_peripheral_interrupt(PeripheralId::TIMER1, IrqPriority::Low);
}

// ============================================================================
// Example 6: Dynamic Interrupt Control
// ============================================================================

void example_dynamic_interrupt_control() {
    // Temporarily disable peripheral interrupt
    disable_peripheral_interrupt(PeripheralId::USART0);
    
    // Do work without interruptions
    // ... critical code ...
    
    // Re-enable peripheral interrupt
    enable_peripheral_interrupt(PeripheralId::USART0);
    
    // Check if interrupt is enabled
    if (is_peripheral_interrupt_enabled(PeripheralId::USART0)) {
        // Interrupt is active
    }
}

// ============================================================================
// Example 7: Combining UART Expert Config with Interrupts
// ============================================================================

void example_complete_uart_setup() {
    // Step 1: Configure UART hardware
    UartExpertConfig uart_config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = true,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };
    
    // Step 2: Configure interrupt priority (expert mode)
    InterruptExpertConfig irq_config = 
        InterruptExpertConfig::high_priority(IrqNumber::UART0);
    expert::configure(irq_config);
    
    // Step 3: Enable global interrupts
    Interrupt::enable_all();
    
    // Now UART is fully configured with interrupts enabled
}

// ============================================================================
// Main
// ============================================================================

int main() {
    // Run examples
    example_simple_interrupt_config();
    example_expert_interrupt_config();
    example_critical_section();
    example_uart_with_interrupts();
    example_multiple_peripherals();
    example_dynamic_interrupt_control();
    example_complete_uart_setup();
    
    return 0;
}
