/// SysTick Demo Example
///
/// This example demonstrates the SysTick timer functionality across all platforms.
/// It shows how to use the microsecond timer for precise timing measurements.
///
/// Features demonstrated:
/// - Reading current time with micros()
/// - Measuring elapsed time with micros_since()
/// - Timeout checking with is_timeout()
/// - Accurate LED blink timing
///
/// The SysTick timer provides:
/// - 1 microsecond resolution
/// - 32-bit counter (wraps after ~71 minutes)
/// - Zero overhead when not used
/// - Thread-safe reads
///
/// This example will blink the LED with precisely measured timing.

#include "board.hpp"

int main() {
    // Initialize board (includes SysTick auto-init)
    Board::initialize();
    Board::Led::init();

    // Test 1: Demonstrate basic timing
    // Blink LED every 500ms with precise timing measurement
    while (true) {
        // Record start time
        core::u32 start = alloy::systick::micros();

        // Turn LED on
        Board::Led::on();

        // Wait 500ms using timeout check
        while (!alloy::systick::is_timeout(start, 500000)) {
            // Do nothing - could do other work here
        }

        // Measure actual elapsed time
        core::u32 elapsed_on = alloy::systick::micros_since(start);

        // Record start time for off period
        start = alloy::systick::micros();

        // Turn LED off
        Board::Led::off();

        // Wait another 500ms
        while (!alloy::systick::is_timeout(start, 500000)) {
            // Do nothing
        }

        // Measure off period
        core::u32 elapsed_off = alloy::systick::micros_since(start);

        // In a real application, you could print these values:
        // printf("LED on: %u us, LED off: %u us\n", elapsed_on, elapsed_off);
        // Expected: ~500000 microseconds each (±1% accuracy)
    }

    return 0;
}
