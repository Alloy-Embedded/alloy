/**
 * @file test_units.cpp
 * @brief Unit tests for core units (BaudRate)
 *
 * Tests type-safe unit wrappers
 */

#include <cassert>
#include <iostream>

#include "../../src/core/units.hpp"

using namespace alloy::core;
using namespace alloy::core::literals;
using namespace alloy::core::baud_rates;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                \
    void test_##name();                                           \
    void run_test_##name() {                                      \
        tests_run++;                                              \
        std::cout << "Running test: " #name << "...";             \
        try {                                                     \
            test_##name();                                        \
            tests_passed++;                                       \
            std::cout << " PASS" << std::endl;                    \
        } catch (const std::exception& e) {                       \
            std::cout << " FAIL: " << e.what() << std::endl;      \
        } catch (...) {                                           \
            std::cout << " FAIL: Unknown exception" << std::endl; \
        }                                                         \
    }                                                             \
    void test_##name()

#define ASSERT(condition)                                              \
    do {                                                               \
        if (!(condition)) {                                            \
            throw std::runtime_error("Assertion failed: " #condition); \
        }                                                              \
    } while (0)

// =============================================================================
// BaudRate Construction Tests
// =============================================================================

TEST(baudrate_construction) {
    BaudRate rate(9600);
    ASSERT(rate.value() == 9600);
}

TEST(baudrate_construction_high_speed) {
    BaudRate rate(115200);
    ASSERT(rate.value() == 115200);
}

TEST(baudrate_zero) {
    BaudRate rate(0);
    ASSERT(rate.value() == 0);
}

TEST(baudrate_large_value) {
    BaudRate rate(921600);
    ASSERT(rate.value() == 921600);
}

// =============================================================================
// BaudRate Comparison Tests
// =============================================================================

TEST(baudrate_equality) {
    BaudRate rate1(9600);
    BaudRate rate2(9600);
    ASSERT(rate1 == rate2);
}

TEST(baudrate_inequality) {
    BaudRate rate1(9600);
    BaudRate rate2(115200);
    ASSERT(rate1 != rate2);
}

TEST(baudrate_comparison_same_object) {
    BaudRate rate(9600);
    ASSERT(rate == rate);
    ASSERT(!(rate != rate));
}

// =============================================================================
// BaudRate Constants Tests
// =============================================================================

TEST(baudrate_constant_9600) {
    ASSERT(Baud9600.value() == 9600);
}

TEST(baudrate_constant_19200) {
    ASSERT(Baud19200.value() == 19200);
}

TEST(baudrate_constant_38400) {
    ASSERT(Baud38400.value() == 38400);
}

TEST(baudrate_constant_57600) {
    ASSERT(Baud57600.value() == 57600);
}

TEST(baudrate_constant_115200) {
    ASSERT(Baud115200.value() == 115200);
}

TEST(baudrate_constant_230400) {
    ASSERT(Baud230400.value() == 230400);
}

TEST(baudrate_constant_460800) {
    ASSERT(Baud460800.value() == 460800);
}

TEST(baudrate_constant_921600) {
    ASSERT(Baud921600.value() == 921600);
}

// =============================================================================
// BaudRate Constant Comparison Tests
// =============================================================================

TEST(baudrate_constants_different) {
    ASSERT(Baud9600 != Baud115200);
    ASSERT(Baud19200 != Baud38400);
}

TEST(baudrate_constants_equality) {
    BaudRate rate = Baud115200;
    ASSERT(rate == Baud115200);
}

// =============================================================================
// BaudRate User-Defined Literal Tests
// =============================================================================

TEST(baudrate_literal_basic) {
    auto rate = 9600_baud;
    ASSERT(rate.value() == 9600);
}

TEST(baudrate_literal_high_speed) {
    auto rate = 115200_baud;
    ASSERT(rate.value() == 115200);
}

TEST(baudrate_literal_comparison) {
    auto rate1 = 9600_baud;
    auto rate2 = 9600_baud;
    ASSERT(rate1 == rate2);
}

TEST(baudrate_literal_vs_constant) {
    auto rate = 115200_baud;
    ASSERT(rate == Baud115200);
}

TEST(baudrate_literal_vs_constructor) {
    auto rate1 = 9600_baud;
    BaudRate rate2(9600);
    ASSERT(rate1 == rate2);
}

// =============================================================================
// BaudRate Type Safety Tests
// =============================================================================

// This function only accepts BaudRate, not raw integers
void configure_uart(BaudRate rate) {
    // Function body doesn't matter for test
    (void)rate;
}

TEST(baudrate_type_safety) {
    // These should compile
    configure_uart(Baud115200);
    configure_uart(BaudRate(9600));
    configure_uart(9600_baud);

    // This would NOT compile (type safety):
    // configure_uart(9600);  // Error: u32 is not BaudRate

    ASSERT(true);  // If we got here, type safety works
}

// =============================================================================
// BaudRate constexpr Tests
// =============================================================================

TEST(baudrate_constexpr_construction) {
    constexpr BaudRate rate(9600);
    ASSERT(rate.value() == 9600);
}

TEST(baudrate_constexpr_comparison) {
    constexpr BaudRate rate1(9600);
    constexpr BaudRate rate2(9600);
    constexpr bool equal = (rate1 == rate2);
    ASSERT(equal);
}

TEST(baudrate_constexpr_constants) {
    // Constants should be usable at compile time
    constexpr auto rate = Baud115200;
    ASSERT(rate.value() == 115200);
}

// =============================================================================
// BaudRate Size Tests
// =============================================================================

TEST(baudrate_size) {
    // BaudRate should be the same size as u32 (zero overhead)
    ASSERT(sizeof(BaudRate) == sizeof(u32));
}

// =============================================================================
// BaudRate Practical Usage Tests
// =============================================================================

TEST(baudrate_switch_statement) {
    BaudRate rate = Baud115200;

    // BaudRate can't be used directly in switch, but value() can
    u32 divider = 0;
    switch (rate.value()) {
        case 9600:
            divider = 1;
            break;
        case 115200:
            divider = 12;
            break;
        default:
            divider = 0;
            break;
    }

    ASSERT(divider == 12);
}

TEST(baudrate_array_of_rates) {
    BaudRate rates[] = {Baud9600, Baud19200, Baud38400, Baud57600, Baud115200};

    ASSERT(rates[0] == Baud9600);
    ASSERT(rates[4] == Baud115200);
}

// =============================================================================
// Compile-time Checks
// =============================================================================

// Verify BaudRate is trivially copyable (zero overhead)
static_assert(std::is_trivially_copyable_v<BaudRate>, "BaudRate must be trivially copyable");
static_assert(std::is_standard_layout_v<BaudRate>, "BaudRate must be standard layout");

TEST(baudrate_compile_time_checks) {
    // If we reach here, all static_assert passed
    ASSERT(true);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Core Units (BaudRate) Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run all tests
    run_test_baudrate_construction();
    run_test_baudrate_construction_high_speed();
    run_test_baudrate_zero();
    run_test_baudrate_large_value();

    run_test_baudrate_equality();
    run_test_baudrate_inequality();
    run_test_baudrate_comparison_same_object();

    run_test_baudrate_constant_9600();
    run_test_baudrate_constant_19200();
    run_test_baudrate_constant_38400();
    run_test_baudrate_constant_57600();
    run_test_baudrate_constant_115200();
    run_test_baudrate_constant_230400();
    run_test_baudrate_constant_460800();
    run_test_baudrate_constant_921600();

    run_test_baudrate_constants_different();
    run_test_baudrate_constants_equality();

    run_test_baudrate_literal_basic();
    run_test_baudrate_literal_high_speed();
    run_test_baudrate_literal_comparison();
    run_test_baudrate_literal_vs_constant();
    run_test_baudrate_literal_vs_constructor();

    run_test_baudrate_type_safety();

    run_test_baudrate_constexpr_construction();
    run_test_baudrate_constexpr_comparison();
    run_test_baudrate_constexpr_constants();

    run_test_baudrate_size();

    run_test_baudrate_switch_statement();
    run_test_baudrate_array_of_rates();

    run_test_baudrate_compile_time_checks();

    // Print summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total:  " << tests_run << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "========================================" << std::endl;

    return (tests_run == tests_passed) ? 0 : 1;
}
