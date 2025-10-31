/// Blink LED Example for Arduino Zero
///
/// This example demonstrates the modern C++20 board API inspired by modm.
/// The board pre-defines commonly used peripherals that users can access
/// directly without needing to know pin numbers.
///
/// Architecture:
///   User Code (main.cpp)
///     ↓ uses Board::LedControl
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
/// - Simple API (Board::LedControl::on())
/// - Portable (same code works on different boards)
///
/// Hardware: Arduino Zero board (ATSAMD21G18)
/// LED: PA17 (Yellow LED marked "L")

#include "arduino_zero/board.hpp"

int main() {
    // Initialize LED
    Board::LedControl::init();

    // Blink LED forever - ultra simple!
    while (true) {
        Board::LedControl::on();
        Board::delay_ms(500);

        Board::LedControl::off();
        Board::delay_ms(500);
    }

    return 0;
}
