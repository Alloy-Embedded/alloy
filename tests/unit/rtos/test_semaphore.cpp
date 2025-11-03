/// Unit Tests for RTOS Semaphores
///
/// Tests binary and counting semaphores for synchronization using Catch2

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_section_info.hpp>
#include "rtos/semaphore.hpp"
#include "hal/host/systick.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace alloy;
using namespace alloy::rtos;

// ============================================================================
// Test 1: Binary Semaphore Basic Operations
// ============================================================================

TEST_CASE("Binary semaphore basic operations", "[semaphore][binary][basic]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Binary semaphore starts with initial value") {
        BinarySemaphore sem(0);
        REQUIRE(sem.count() == 0);

        BinarySemaphore sem_one(1);
        REQUIRE(sem_one.count() == 1);
    }

    SECTION("give increments semaphore to 1") {
        BinarySemaphore sem(0);
        sem.give();
        REQUIRE(sem.count() == 1);
    }

    SECTION("Multiple gives keep count at 1 (binary property)") {
        BinarySemaphore sem(0);
        sem.give();
        sem.give();
        sem.give();
        REQUIRE(sem.count() == 1);  // Binary: max is 1
    }

    SECTION("try_take succeeds when count is 1") {
        BinarySemaphore sem(1);
        REQUIRE(sem.try_take());
        REQUIRE(sem.count() == 0);
    }

    SECTION("try_take fails when count is 0") {
        BinarySemaphore sem(0);
        REQUIRE_FALSE(sem.try_take());
        REQUIRE(sem.count() == 0);
    }
}

// ============================================================================
// Test 2: Counting Semaphore Basic Operations
// ============================================================================

TEST_CASE("Counting semaphore basic operations", "[semaphore][counting][basic]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Counting semaphore starts with initial value") {
        CountingSemaphore<10> sem(0);
        REQUIRE(sem.count() == 0);

        CountingSemaphore<10> sem_five(5);
        REQUIRE(sem_five.count() == 5);
    }

    SECTION("give increments count") {
        CountingSemaphore<10> sem(0);
        sem.give();
        REQUIRE(sem.count() == 1);
        sem.give();
        REQUIRE(sem.count() == 2);
    }

    SECTION("give respects max count") {
        CountingSemaphore<3> sem(3);
        sem.give();  // At max, should not increment
        REQUIRE(sem.count() == 3);  // Still at max
    }

    SECTION("try_take decrements count") {
        CountingSemaphore<10> sem(5);
        REQUIRE(sem.try_take());
        REQUIRE(sem.count() == 4);
    }

    SECTION("try_take fails when count is zero") {
        CountingSemaphore<10> sem(0);
        REQUIRE_FALSE(sem.try_take());
    }
}

// ============================================================================
// Test 3: Semaphore Timeouts
// ============================================================================

TEST_CASE("Semaphore timeout behavior", "[semaphore][timeout]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("try_take is non-blocking") {
        BinarySemaphore sem(0);

        auto start = std::chrono::steady_clock::now();
        REQUIRE_FALSE(sem.try_take());  // Should fail immediately
        auto duration = std::chrono::steady_clock::now() - start;

        // Should return immediately (within 50ms)
        REQUIRE(duration < std::chrono::milliseconds(50));
    }

    // NOTE: Semaphore timeout is handled by task blocking, not try_take()
}

// ============================================================================
// Test 4: Multi-threaded Synchronization
// ============================================================================

TEST_CASE("Semaphore multi-threaded synchronization", "[semaphore][threading]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Binary semaphore synchronizes threads") {
        BinarySemaphore sem(0);
        std::atomic<bool> thread_ran{false};

        std::thread worker([&]() {
            sem.take();  // Wait for signal
            thread_ran = true;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        REQUIRE_FALSE(thread_ran);  // Thread should be waiting

        sem.give();  // Signal thread
        worker.join();

        REQUIRE(thread_ran);
    }

    SECTION("Counting semaphore limits concurrent access") {
        const int max_concurrent = 3;
        CountingSemaphore<max_concurrent> sem(max_concurrent);
        std::atomic<int> current_count{0};
        std::atomic<int> max_seen{0};

        std::vector<std::thread> threads;
        for (int i = 0; i < 10; i++) {
            threads.emplace_back([&]() {
                sem.take();

                int count = ++current_count;
                if (count > max_seen) max_seen = count;

                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                --current_count;
                sem.give();
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        REQUIRE(max_seen <= max_concurrent);
        REQUIRE(current_count == 0);
    }
}

// ============================================================================
// Test 5: Edge Cases
// ============================================================================

TEST_CASE("Semaphore edge cases", "[semaphore][edge-cases]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Binary semaphore max value is 1") {
        BinarySemaphore sem(0);
        for (int i = 0; i < 100; i++) {
            sem.give();
        }
        REQUIRE(sem.count() == 1);
    }

    SECTION("Counting semaphore respects max") {
        CountingSemaphore<5> sem(0);
        for (int i = 0; i < 5; i++) {
            sem.give();
        }
        REQUIRE(sem.count() == 5);

        // Giving when at max should not increment
        sem.give();
        REQUIRE(sem.count() == 5);
    }

    SECTION("Take and give cycle") {
        BinarySemaphore sem(1);
        for (int i = 0; i < 100; i++) {
            REQUIRE(sem.try_take());
            sem.give();
        }
        REQUIRE(sem.count() == 1);
    }
}

// ============================================================================
// Test 6: Compile-Time Properties
// ============================================================================

TEST_CASE("Semaphore compile-time properties", "[semaphore][compile-time]") {
    SECTION("BinarySemaphore is not copyable") {
        REQUIRE(!std::is_copy_constructible_v<BinarySemaphore>);
        REQUIRE(!std::is_copy_assignable_v<BinarySemaphore>);
    }

    SECTION("CountingSemaphore is not copyable") {
        REQUIRE(!std::is_copy_constructible_v<CountingSemaphore<10>>);
        REQUIRE(!std::is_copy_assignable_v<CountingSemaphore<10>>);
    }

    SECTION("Max count is enforced at compile time") {
        CountingSemaphore<5> sem(5);
        REQUIRE(sem.count() == 5);
        // Max is enforced by template parameter
        SUCCEED("Max count template works");
    }
}
