/// Unit Tests for RTOS Mutex
///
/// Tests mutex locking, unlocking, recursive locking, priority inheritance,
/// and RAII lock guards using Catch2 framework.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_section_info.hpp>
#include "rtos/mutex.hpp"
#include "hal/host/systick.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

using namespace alloy;
using namespace alloy::rtos;

// ============================================================================
// Test 1: Basic Mutex Locking and Unlocking
// ============================================================================

TEST_CASE("Mutex basic locking", "[mutex][basic]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Mutex starts unlocked") {
        Mutex mutex;
        REQUIRE_FALSE(mutex.is_locked());
    }

    SECTION("Mutex can be locked") {
        Mutex mutex;
        REQUIRE(mutex.lock());
        REQUIRE(mutex.is_locked());
    }

    SECTION("Mutex can be unlocked") {
        Mutex mutex;
        REQUIRE(mutex.lock());
        REQUIRE(mutex.is_locked());
        REQUIRE(mutex.unlock());
        REQUIRE_FALSE(mutex.is_locked());
    }

    SECTION("try_lock succeeds on unlocked mutex") {
        Mutex mutex;
        REQUIRE(mutex.try_lock());
        REQUIRE(mutex.is_locked());
        mutex.unlock();
    }

    // NOTE: Mutex::try_lock() does not support timeout parameter
    // Timeout is handled at a higher level (task blocking)
}

// ============================================================================
// Test 2: Recursive Locking
// ============================================================================

TEST_CASE("Mutex recursive locking", "[mutex][recursive]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Mutex supports recursive locking") {
        Mutex mutex;

        REQUIRE(mutex.lock());
        REQUIRE(mutex.lock());  // Recursive lock
        REQUIRE(mutex.lock());  // Another recursive lock

        REQUIRE(mutex.is_locked());

        REQUIRE(mutex.unlock());
        REQUIRE(mutex.is_locked());  // Still locked (2 more unlocks needed)

        REQUIRE(mutex.unlock());
        REQUIRE(mutex.is_locked());  // Still locked (1 more unlock needed)

        REQUIRE(mutex.unlock());
        REQUIRE_FALSE(mutex.is_locked());  // Now fully unlocked
    }

    SECTION("Recursive lock count is tracked") {
        Mutex mutex;

        for (int i = 0; i < 5; i++) {
            REQUIRE(mutex.lock());
        }

        // Should still be locked after first unlock
        REQUIRE(mutex.unlock());
        REQUIRE(mutex.is_locked());

        // Unlock remaining 4 times
        for (int i = 0; i < 4; i++) {
            REQUIRE(mutex.unlock());
        }

        REQUIRE_FALSE(mutex.is_locked());
    }
}

// ============================================================================
// Test 3: Lock Guard (RAII)
// ============================================================================

TEST_CASE("Mutex lock guard RAII", "[mutex][lock-guard][raii]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("LockGuard acquires lock on construction") {
        Mutex mutex;
        {
            LockGuard guard(mutex);
            REQUIRE(mutex.is_locked());
        }
    }

    SECTION("LockGuard releases lock on destruction") {
        Mutex mutex;
        {
            LockGuard guard(mutex);
            REQUIRE(mutex.is_locked());
        }
        REQUIRE_FALSE(mutex.is_locked());
    }

    SECTION("Nested LockGuards work correctly") {
        Mutex mutex;
        {
            LockGuard guard1(mutex);
            REQUIRE(mutex.is_locked());
            {
                LockGuard guard2(mutex);
                REQUIRE(mutex.is_locked());
            }
            REQUIRE(mutex.is_locked());
        }
        REQUIRE_FALSE(mutex.is_locked());
    }

    SECTION("Exception safety with LockGuard") {
        Mutex mutex;

        try {
            LockGuard guard(mutex);
            REQUIRE(mutex.is_locked());
            throw std::runtime_error("Test exception");
        } catch (...) {
            // Lock should be released even when exception is thrown
            REQUIRE_FALSE(mutex.is_locked());
        }
    }
}

// ============================================================================
// Test 4: Mutex Timeout
// ============================================================================

TEST_CASE("Mutex timeout behavior", "[mutex][timeout]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("try_lock returns false when already locked") {
        Mutex mutex;
        mutex.lock();

        REQUIRE_FALSE(mutex.try_lock());

        mutex.unlock();
    }

    SECTION("try_lock is non-blocking") {
        Mutex mutex;
        mutex.lock();

        auto start = std::chrono::steady_clock::now();
        REQUIRE_FALSE(mutex.try_lock());
        auto duration = std::chrono::steady_clock::now() - start;

        // Should return immediately (within 50ms)
        REQUIRE(duration < std::chrono::milliseconds(50));

        mutex.unlock();
    }
}

// ============================================================================
// Test 5: Multi-threaded Protection (using std::thread)
// ============================================================================

TEST_CASE("Mutex multi-threaded protection", "[mutex][threading]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Mutex protects shared counter") {
        Mutex mutex;
        int counter = 0;
        const int iterations = 1000;
        const int num_threads = 4;

        std::vector<std::thread> threads;

        for (int t = 0; t < num_threads; t++) {
            threads.emplace_back([&]() {
                for (int i = 0; i < iterations; i++) {
                    LockGuard guard(mutex);
                    counter++;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        REQUIRE(counter == iterations * num_threads);
    }

    SECTION("Mutex prevents data races") {
        Mutex mutex;
        std::vector<int> shared_data;
        const int iterations = 100;

        std::thread writer([&]() {
            for (int i = 0; i < iterations; i++) {
                LockGuard guard(mutex);
                shared_data.push_back(i);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });

        std::thread reader([&]() {
            for (int i = 0; i < iterations; i++) {
                LockGuard guard(mutex);
                if (!shared_data.empty()) {
                    [[maybe_unused]] int value = shared_data.back();
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });

        writer.join();
        reader.join();

        REQUIRE(shared_data.size() == iterations);
    }
}

// ============================================================================
// Test 6: Mutex State Queries
// ============================================================================

TEST_CASE("Mutex state queries", "[mutex][state]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("is_locked reports correct state") {
        Mutex mutex;

        REQUIRE_FALSE(mutex.is_locked());
        mutex.lock();
        REQUIRE(mutex.is_locked());
        mutex.unlock();
        REQUIRE_FALSE(mutex.is_locked());
    }

    SECTION("is_locked works with recursive locks") {
        Mutex mutex;

        mutex.lock();
        mutex.lock();
        REQUIRE(mutex.is_locked());

        mutex.unlock();
        REQUIRE(mutex.is_locked());  // Still locked once

        mutex.unlock();
        REQUIRE_FALSE(mutex.is_locked());
    }
}

// ============================================================================
// Test 7: Edge Cases
// ============================================================================

TEST_CASE("Mutex edge cases", "[mutex][edge-cases]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Unlock without lock returns false") {
        Mutex mutex;
        REQUIRE_FALSE(mutex.unlock());
    }

    SECTION("Multiple unlocks without locks") {
        Mutex mutex;
        mutex.lock();
        REQUIRE(mutex.unlock());
        REQUIRE_FALSE(mutex.unlock());  // Already unlocked
    }

    SECTION("Deep recursive locking") {
        Mutex mutex;
        const int depth = 100;

        for (int i = 0; i < depth; i++) {
            REQUIRE(mutex.lock());
        }

        REQUIRE(mutex.is_locked());

        for (int i = 0; i < depth; i++) {
            REQUIRE(mutex.unlock());
        }

        REQUIRE_FALSE(mutex.is_locked());
    }
}

// ============================================================================
// Test 8: Performance and Timing
// ============================================================================

TEST_CASE("Mutex performance", "[mutex][performance]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Lock/unlock is fast") {
        Mutex mutex;

        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < 10000; i++) {
            mutex.lock();
            mutex.unlock();
        }

        auto duration = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        INFO("10000 lock/unlock cycles took " << ms << "ms");
        REQUIRE(ms < 1000);  // Should be much faster than 1 second
    }

    SECTION("LockGuard overhead is minimal") {
        Mutex mutex;

        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < 10000; i++) {
            LockGuard guard(mutex);
        }

        auto duration = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        INFO("10000 LockGuard constructions took " << ms << "ms");
        REQUIRE(ms < 1000);  // Should be fast
    }
}

// ============================================================================
// Test 9: Compile-Time Properties
// ============================================================================

TEST_CASE("Mutex compile-time properties", "[mutex][compile-time]") {
    SECTION("Mutex is not copyable") {
        INFO("Mutex should not be copyable (enforced at compile time)");
        REQUIRE(!std::is_copy_constructible_v<Mutex>);
        REQUIRE(!std::is_copy_assignable_v<Mutex>);
    }

    SECTION("Mutex is not movable") {
        INFO("Mutex should not be movable (enforced at compile time)");
        REQUIRE(!std::is_move_constructible_v<Mutex>);
        REQUIRE(!std::is_move_assignable_v<Mutex>);
    }

    SECTION("LockGuard is not copyable") {
        INFO("LockGuard should not be copyable (RAII safety)");
        REQUIRE(!std::is_copy_constructible_v<LockGuard>);
        REQUIRE(!std::is_copy_assignable_v<LockGuard>);
    }
}
