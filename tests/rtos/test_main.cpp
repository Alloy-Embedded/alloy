/// Alloy RTOS Test Runner
///
/// Main entry point for all RTOS tests.
///
/// Usage:
/// ```bash
/// # Build tests
/// cmake -B build -DBUILD_TESTS=ON
/// cmake --build build
///
/// # Run on board
/// ./build/tests/rtos_tests
/// ```

#include "tests/rtos/test_framework.hpp"
#include "rtos/rtos.hpp"
#include "board/board.hpp"

using namespace alloy;
using namespace alloy::rtos;
using namespace alloy::rtos::test;

// External test suite declarations
extern void test_suite_Queue();
extern void test_suite_TaskNotification();
extern void test_suite_StaticPool();

// Test runner task
void test_runner_task() {
    printf("\n");
    printf("=====================================\n");
    printf("Alloy RTOS Test Suite\n");
    printf("=====================================\n");
    printf("Platform: %s\n", BOARD_NAME);
    printf("C++ Standard: C++%ld\n", __cplusplus / 100 % 100);
    printf("Compiler: %s\n", __VERSION__);
    printf("=====================================\n");

    // Reset statistics
    reset_test_stats();

    // Run all test suites
    printf("\n>>> Running Core RTOS Tests...\n");

    test_suite_Queue();
    test_suite_TaskNotification();
    test_suite_StaticPool();

    // Print summary
    print_test_summary();

    // Hang or loop
    while (1) {
        RTOS::delay(1000);
    }
}

// Create test task
Task<2048, Priority::Normal, "TestRunner"> test_task(test_runner_task);

int main() {
    // Initialize board
    board::Board::initialize();

    printf("\n=== Alloy RTOS Tests Starting ===\n");

    // Start RTOS (tests run in task)
    RTOS::start();

    // Never returns
    return 0;
}
