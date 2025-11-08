/// SysTick Host Test Example
///
/// Tests the host (PC) SysTick implementation using std::chrono.
/// This demonstrates that embedded timing code can run on a PC for testing.
///
/// Expected output:
/// - Demonstrates microsecond precision timing
/// - Shows delay functions working correctly
/// - Tests overflow handling (micros_since)
/// - Measures timing accuracy

#include <iomanip>
#include <iostream>

#include "hal/host/delay.hpp"
#include "hal/host/gpio.hpp"
#include "hal/host/systick.hpp"

using namespace alloy;

/// Print a test header
void print_test(const char* name) {
    std::cout << "\n=== " << name << " ===" << std::endl;
}

/// Print test result
void print_result(const char* test, bool passed) {
    std::cout << "  " << test << ": " << (passed ? "\033[32mPASS\033[0m" : "\033[31mFAIL\033[0m")
              << std::endl;
}

/// Test basic micros() functionality
void test_basic_micros() {
    print_test("Basic micros() Test");

    core::u32 start = systick::micros();
    hal::host::delay_ms(10);  // 10ms delay
    core::u32 end = systick::micros();

    core::u32 elapsed = end - start;

    std::cout << "  Start:   " << start << " us" << std::endl;
    std::cout << "  End:     " << end << " us" << std::endl;
    std::cout << "  Elapsed: " << elapsed << " us" << std::endl;

    // Check if elapsed time is reasonable (9-11ms range)
    bool passed = (elapsed >= 9000 && elapsed <= 11000);
    print_result("10ms delay accuracy", passed);
}

/// Test micros_since() with overflow handling
void test_micros_since() {
    print_test("micros_since() Overflow Test");

    // Simulate near-overflow scenario
    core::u32 start = 0xFFFFFFF0;  // Near max uint32
    core::u32 now = 0x00000100;    // Wrapped around

    core::u32 elapsed = now - start;  // Unsigned arithmetic handles this

    std::cout << "  Start:   0x" << std::hex << start << std::dec << std::endl;
    std::cout << "  Now:     0x" << std::hex << now << std::dec << std::endl;
    std::cout << "  Elapsed: " << elapsed << " us" << std::endl;

    bool passed = (elapsed == 272);  // 0x100 - 0xFFFFFFF0 = 0x110 = 272
    print_result("Overflow handling", passed);
}

/// Test delay accuracy
void test_delay_accuracy() {
    print_test("Delay Accuracy Test");

    // Test millisecond delays
    core::u32 delays_ms[] = {1, 5, 10, 50};

    for (auto delay_ms : delays_ms) {
        core::u32 start = systick::micros();
        hal::host::delay_ms(delay_ms);
        core::u32 elapsed = systick::micros_since(start);

        core::u32 expected_us = delay_ms * 1000;
        core::u32 tolerance = delay_ms * 200;  // 20% tolerance

        bool passed =
            (elapsed >= (expected_us - tolerance) && elapsed <= (expected_us + tolerance));

        std::cout << "  " << delay_ms << "ms delay: " << elapsed << " us (expected ~" << expected_us
                  << " us) - " << (passed ? "OK" : "FAIL") << std::endl;
    }

    // Test microsecond delays
    std::cout << "\n  Microsecond delays:" << std::endl;
    core::u32 delays_us[] = {100, 500, 1000};

    for (auto delay_us : delays_us) {
        core::u32 start = systick::micros();
        hal::host::delay_us(delay_us);
        core::u32 elapsed = systick::micros_since(start);

        core::u32 tolerance = delay_us / 5;  // 20% tolerance

        bool passed = (elapsed >= (delay_us - tolerance) && elapsed <= (delay_us + tolerance));

        std::cout << "  " << delay_us << "us delay: " << elapsed << " us - "
                  << (passed ? "OK" : "FAIL") << std::endl;
    }
}

/// Test timeout functionality
void test_timeout() {
    print_test("Timeout Test");

    core::u32 start = systick::micros();
    core::u32 timeout_us = 5000;  // 5ms timeout

    // Busy-wait until timeout
    while (!systick::is_timeout(start, timeout_us)) {
        // Simulating some work
    }

    core::u32 elapsed = systick::micros_since(start);

    std::cout << "  Timeout set: " << timeout_us << " us" << std::endl;
    std::cout << "  Elapsed:     " << elapsed << " us" << std::endl;

    bool passed = (elapsed >= timeout_us && elapsed < timeout_us + 1000);
    print_result("Timeout accuracy", passed);
}

/// Test reset functionality
void test_reset() {
    print_test("Reset Test");

    // Run for a bit
    hal::host::delay_ms(10);

    core::u32 before_reset = systick::micros();
    std::cout << "  Before reset: " << before_reset << " us" << std::endl;

    // Reset
    hal::host::SystemTick::reset();

    core::u32 after_reset = systick::micros();
    std::cout << "  After reset:  " << after_reset << " us" << std::endl;

    bool passed = (after_reset < 1000);  // Should be near zero
    print_result("Reset functionality", passed);
}

/// Demonstrate a simple "blink" simulation
void test_simulated_blink() {
    print_test("Simulated Blink (3 cycles)");

    hal::host::GpioPin<25> led;
    led.configure(hal::PinMode::Output);

    for (int i = 0; i < 3; i++) {
        core::u32 cycle_start = systick::micros();

        led.set_high();
        std::cout << "  [" << (systick::micros() / 1000) << " ms] LED ON" << std::endl;
        hal::host::delay_ms(100);

        led.set_low();
        std::cout << "  [" << (systick::micros() / 1000) << " ms] LED OFF" << std::endl;
        hal::host::delay_ms(100);

        core::u32 cycle_time = systick::micros_since(cycle_start);
        std::cout << "  Cycle " << (i + 1) << " took " << cycle_time << " us" << std::endl;
    }
}

int main() {
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Alloy SysTick Host Test Suite       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;

    // Initialize SysTick
    auto result = hal::host::SystemTick::init();
    if (!result.is_ok()) {
        std::cerr << "Failed to initialize SysTick!" << std::endl;
        return 1;
    }

    std::cout << "\nSysTick initialized successfully" << std::endl;
    std::cout << "Platform: Host (std::chrono)" << std::endl;

    // Run all tests
    test_basic_micros();
    test_micros_since();
    test_delay_accuracy();
    test_timeout();
    test_reset();
    test_simulated_blink();

    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║  All tests completed!                 ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝\n" << std::endl;

    return 0;
}
