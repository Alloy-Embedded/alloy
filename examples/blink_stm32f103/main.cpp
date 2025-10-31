/// Blink Example for STM32F103C8 (Blue Pill)
///
/// This example demonstrates the modern C++20 board API inspired by modm.
/// The board pre-defines commonly used peripherals that users can access
/// directly without needing to know pin numbers.
///
/// Architecture:
///   User Code (main.cpp)
///     ↓ uses Board::Led
///   Board API (board.hpp - pre-instantiated pins)
///     ↓ type aliases to GpioPin<N>
///   HAL Layer (gpio.hpp - templates + concepts)
///     ↓ uses MCU namespace
///   Generated Peripherals (peripherals.hpp)
///     ↓ raw register access
///   Hardware
///
/// This approach provides:
/// - Zero-cost abstractions (everything inline/constexpr)
/// - Type safety (compile-time validation)
/// - Simple API (Board::Led::on())
/// - Portable (same code works on different boards)
///
/// Hardware: STM32F103C8 (Blue Pill)
/// LED: PC13 (built-in LED, active LOW)

#include "stm32f103c8/board.hpp"

int main() {
    // Initialize LED
    Board::Led::init();

    // Blink LED forever - ultra simple!
    while (true) {
        Board::Led::on();
        Board::delay_ms(500);

        Board::Led::off();
        Board::delay_ms(500);
    }

    return 0;
}

// Weak symbols for startup code
extern "C" {
    void SystemInit() {
        // Optional: Configure clocks here
        // For now, running on default HSI (8MHz)
    }
}
