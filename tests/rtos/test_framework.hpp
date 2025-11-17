/// Alloy RTOS Test Framework
///
/// Lightweight test framework for embedded RTOS testing.
///
/// Features:
/// - Minimal overhead (suitable for embedded)
/// - UART output for test results
/// - Assertion macros
/// - Test suite organization
/// - Performance measurement
///
/// Usage:
/// ```cpp
/// TEST_CASE("Queue send/receive") {
///     Queue<uint32_t, 8> queue;
///
///     TEST_ASSERT(queue.send(42, 1000).is_ok());
///
///     auto result = queue.receive(1000);
///     TEST_ASSERT(result.is_ok());
///     TEST_ASSERT_EQUAL(result.unwrap(), 42);
/// }
/// ```

#ifndef ALLOY_RTOS_TEST_FRAMEWORK_HPP
#define ALLOY_RTOS_TEST_FRAMEWORK_HPP

#include <cstddef>
#include <cstdio>

#include "core/types.hpp"
#include "hal/interface/systick.hpp"

namespace alloy::rtos::test {

// ============================================================================
// Test Statistics
// ============================================================================

struct TestStats {
    core::u32 total_tests{0};
    core::u32 passed_tests{0};
    core::u32 failed_tests{0};
    core::u32 skipped_tests{0};

    void reset() {
        total_tests = 0;
        passed_tests = 0;
        failed_tests = 0;
        skipped_tests = 0;
    }

    core::u8 pass_percentage() const {
        if (total_tests == 0) return 0;
        return (passed_tests * 100) / total_tests;
    }
};

// ============================================================================
// Global Test Context
// ============================================================================

struct TestContext {
    TestStats stats;
    const char* current_suite{nullptr};
    const char* current_test{nullptr};
    bool test_failed{false};

    static TestContext& instance() {
        static TestContext ctx;
        return ctx;
    }
};

// ============================================================================
// Test Macros
// ============================================================================

/// Begin test suite
#define TEST_SUITE(name) \
    namespace { \
        struct TestSuite_##name { \
            static void run_all_tests(); \
        }; \
    } \
    void test_suite_##name() { \
        auto& ctx = alloy::rtos::test::TestContext::instance(); \
        ctx.current_suite = #name; \
        printf("\n=== Test Suite: %s ===\n", #name); \
        TestSuite_##name::run_all_tests(); \
    } \
    void TestSuite_##name::run_all_tests()

/// Define a test case
#define TEST_CASE(name) \
    static void test_##name(); \
    namespace { \
        struct TestRunner_##name { \
            TestRunner_##name() { \
                auto& ctx = alloy::rtos::test::TestContext::instance(); \
                ctx.current_test = #name; \
                ctx.test_failed = false; \
                ctx.stats.total_tests++; \
                \
                printf("  [TEST] %s ... ", #name); \
                \
                core::u32 start = alloy::hal::SysTick::micros(); \
                test_##name(); \
                core::u32 duration = alloy::hal::SysTick::micros() - start; \
                \
                if (!ctx.test_failed) { \
                    ctx.stats.passed_tests++; \
                    printf("PASS (%lu µs)\n", duration); \
                } else { \
                    ctx.stats.failed_tests++; \
                    printf("FAIL (%lu µs)\n", duration); \
                } \
            } \
        }; \
        static TestRunner_##name test_runner_##name; \
    } \
    static void test_##name()

/// Assert that condition is true
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            auto& ctx = alloy::rtos::test::TestContext::instance(); \
            ctx.test_failed = true; \
            printf("\n    ASSERTION FAILED: %s\n", #condition); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

/// Assert equality
#define TEST_ASSERT_EQUAL(actual, expected) \
    do { \
        auto _actual = (actual); \
        auto _expected = (expected); \
        if (_actual != _expected) { \
            auto& ctx = alloy::rtos::test::TestContext::instance(); \
            ctx.test_failed = true; \
            printf("\n    ASSERTION FAILED: Expected %s == %s\n", #actual, #expected); \
            printf("    Actual: %lu, Expected: %lu\n", \
                   static_cast<unsigned long>(_actual), \
                   static_cast<unsigned long>(_expected)); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

/// Assert not equal
#define TEST_ASSERT_NOT_EQUAL(actual, expected) \
    do { \
        auto _actual = (actual); \
        auto _expected = (expected); \
        if (_actual == _expected) { \
            auto& ctx = alloy::rtos::test::TestContext::instance(); \
            ctx.test_failed = true; \
            printf("\n    ASSERTION FAILED: Expected %s != %s\n", #actual, #expected); \
            printf("    Both values: %lu\n", static_cast<unsigned long>(_actual)); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

/// Assert that Result is Ok
#define TEST_ASSERT_OK(result) \
    do { \
        auto _result = (result); \
        if (_result.is_err()) { \
            auto& ctx = alloy::rtos::test::TestContext::instance(); \
            ctx.test_failed = true; \
            printf("\n    ASSERTION FAILED: Expected Ok, got Err\n"); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

/// Assert that Result is Err
#define TEST_ASSERT_ERR(result) \
    do { \
        auto _result = (result); \
        if (_result.is_ok()) { \
            auto& ctx = alloy::rtos::test::TestContext::instance(); \
            ctx.test_failed = true; \
            printf("\n    ASSERTION FAILED: Expected Err, got Ok\n"); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

/// Assert within range
#define TEST_ASSERT_IN_RANGE(value, min, max) \
    do { \
        auto _value = (value); \
        auto _min = (min); \
        auto _max = (max); \
        if (_value < _min || _value > _max) { \
            auto& ctx = alloy::rtos::test::TestContext::instance(); \
            ctx.test_failed = true; \
            printf("\n    ASSERTION FAILED: Value out of range\n"); \
            printf("    Value: %lu, Min: %lu, Max: %lu\n", \
                   static_cast<unsigned long>(_value), \
                   static_cast<unsigned long>(_min), \
                   static_cast<unsigned long>(_max)); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

/// Skip test
#define TEST_SKIP() \
    do { \
        auto& ctx = alloy::rtos::test::TestContext::instance(); \
        ctx.stats.skipped_tests++; \
        printf("SKIP\n"); \
        return; \
    } while (0)

// ============================================================================
// Test Utilities
// ============================================================================

/// Performance measurement helper
class PerfTimer {
public:
    PerfTimer() : start_(hal::SysTick::micros()) {}

    core::u32 elapsed_us() const {
        return hal::SysTick::micros() - start_;
    }

    core::u32 elapsed_ms() const {
        return elapsed_us() / 1000;
    }

    void reset() {
        start_ = hal::SysTick::micros();
    }

private:
    core::u32 start_;
};

/// Print test summary
inline void print_test_summary() {
    auto& ctx = TestContext::instance();

    printf("\n");
    printf("=====================================\n");
    printf("Test Summary\n");
    printf("=====================================\n");
    printf("Total:   %lu tests\n", ctx.stats.total_tests);
    printf("Passed:  %lu tests (%u%%)\n",
           ctx.stats.passed_tests,
           ctx.stats.pass_percentage());
    printf("Failed:  %lu tests\n", ctx.stats.failed_tests);
    printf("Skipped: %lu tests\n", ctx.stats.skipped_tests);
    printf("=====================================\n");

    if (ctx.stats.failed_tests == 0) {
        printf("Result: ALL TESTS PASSED ✓\n");
    } else {
        printf("Result: SOME TESTS FAILED ✗\n");
    }
    printf("=====================================\n");
}

/// Reset test statistics
inline void reset_test_stats() {
    TestContext::instance().stats.reset();
}

}  // namespace alloy::rtos::test

#endif  // ALLOY_RTOS_TEST_FRAMEWORK_HPP
