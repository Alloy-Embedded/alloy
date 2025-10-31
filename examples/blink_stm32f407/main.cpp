/// Blink LED Example for STM32F407VG Discovery
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
/// Hardware: STM32F407VG Discovery board
/// LED: PD12 (Green, built-in LED)

#include "stm32f407vg/board.hpp"

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
