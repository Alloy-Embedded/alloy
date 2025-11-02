/// Unit Tests for RTOS Mutex (Mutual Exclusion)
///
/// Tests mutual exclusion, priority inheritance, recursive locking, timeouts,
/// RAII lock guards, and multi-task scenarios.

#include <gtest/gtest.h>
#include "rtos/mutex.hpp"
#include "rtos/rtos.hpp"
#include "hal/host/systick.hpp"
#include <thread>
#include <atomic>
#include <chrono>

using namespace alloy;
using namespace alloy::rtos;

// Test fixture
class MutexTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto result = hal::host::SystemTick::init();
        ASSERT_TRUE(result.is_ok());
    }
};

// ============================================================================
// Test 1: Mutex Creation and Initial State
// ============================================================================

TEST_F(MutexTest, MutexCreation) {
    // Given: A newly created mutex
    Mutex mutex;

    // Then: Should be unlocked with no owner
    EXPECT_FALSE(mutex.is_locked());
    EXPECT_EQ(mutex.owner(), nullptr);
}

// ============================================================================
// Test 2: Basic Lock and Unlock
// ============================================================================

TEST_F(MutexTest, BasicLockUnlock) {
    // Given: An unlocked mutex
    Mutex mutex;

    // When: Locking the mutex
    bool locked = mutex.try_lock();

    // Then: Should succeed
    EXPECT_TRUE(locked);
    EXPECT_TRUE(mutex.is_locked());
    EXPECT_NE(mutex.owner(), nullptr);

    // When: Unlocking the mutex
    bool unlocked = mutex.unlock();

    // Then: Should succeed and be free
    EXPECT_TRUE(unlocked);
    EXPECT_FALSE(mutex.is_locked());
    EXPECT_EQ(mutex.owner(), nullptr);
}

TEST_F(MutexTest, TryLockWhenAlreadyLocked) {
    // Given: A locked mutex
    Mutex mutex;
    mutex.try_lock();

    // When: Another task tries to lock (simulate by trying again)
    // Note: In single-threaded test, we own it, so this tests recursive lock
    bool locked_again = mutex.try_lock();

    // Then: Should succeed (recursive lock)
    EXPECT_TRUE(locked_again);
    EXPECT_TRUE(mutex.is_locked());
}

// ============================================================================
// Test 3: Recursive Locking
// ============================================================================

TEST_F(MutexTest, RecursiveLocking) {
    // Given: A mutex
    Mutex mutex;

    // When: Locking multiple times by same task
    EXPECT_TRUE(mutex.lock());
    EXPECT_TRUE(mutex.lock());  // Recursive
    EXPECT_TRUE(mutex.lock());  // Recursive
    EXPECT_TRUE(mutex.lock());  // Recursive

    // Then: Should be locked
    EXPECT_TRUE(mutex.is_locked());

    // When: Unlocking (must unlock same number of times)
    EXPECT_TRUE(mutex.unlock());
    EXPECT_TRUE(mutex.is_locked());  // Still locked

    EXPECT_TRUE(mutex.unlock());
    EXPECT_TRUE(mutex.is_locked());  // Still locked

    EXPECT_TRUE(mutex.unlock());
    EXPECT_TRUE(mutex.is_locked());  // Still locked

    EXPECT_TRUE(mutex.unlock());
    EXPECT_FALSE(mutex.is_locked());  // Now free
}

// ============================================================================
// Test 4: RAII Lock Guard
// ============================================================================

TEST_F(MutexTest, LockGuardBasicUsage) {
    // Given: A mutex
    Mutex mutex;

    // When: Using LockGuard in a scope
    {
        LockGuard guard(mutex);

        // Then: Mutex should be locked
        EXPECT_TRUE(guard.is_locked());
        EXPECT_TRUE(mutex.is_locked());
    }

    // Then: Mutex should be automatically unlocked
    EXPECT_FALSE(mutex.is_locked());
}

TEST_F(MutexTest, LockGuardNestedScopes) {
    // Given: A mutex
    Mutex mutex;

    // When: Using nested LockGuards (tests recursive locking)
    {
        LockGuard guard1(mutex);
        EXPECT_TRUE(mutex.is_locked());

        {
            LockGuard guard2(mutex);
            EXPECT_TRUE(mutex.is_locked());

            {
                LockGuard guard3(mutex);
                EXPECT_TRUE(mutex.is_locked());
            }

            EXPECT_TRUE(mutex.is_locked());
        }

        EXPECT_TRUE(mutex.is_locked());
    }

    // Then: Should be fully unlocked
    EXPECT_FALSE(mutex.is_locked());
}

// ============================================================================
// Test 5: Mutual Exclusion (Shared Counter)
// ============================================================================

TEST_F(MutexTest, MutualExclusionProtectsSharedData) {
    // Given: Shared counter and mutex
    Mutex mutex;
    std::atomic<int> shared_counter{0};
    std::atomic<int> critical_section_count{0};

    // When: Multiple threads increment counter
    const int NUM_THREADS = 4;
    const int INCREMENTS_PER_THREAD = 100;

    auto worker = [&]() {
        for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
            LockGuard guard(mutex);

            // Critical section - should only be accessed by one thread
            critical_section_count++;
            EXPECT_EQ(critical_section_count.load(), 1)
                << "Critical section entered by multiple threads!";

            int temp = shared_counter.load();
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            shared_counter.store(temp + 1);

            critical_section_count--;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Then: Counter should have exact expected value
    EXPECT_EQ(shared_counter.load(), NUM_THREADS * INCREMENTS_PER_THREAD);
}

// ============================================================================
// Test 6: Lock with Timeout
// ============================================================================

TEST_F(MutexTest, LockWithTimeoutSucceeds) {
    // Given: An unlocked mutex
    Mutex mutex;

    // When: Locking with timeout
    bool locked = mutex.lock(1000);

    // Then: Should succeed quickly
    EXPECT_TRUE(locked);
    EXPECT_TRUE(mutex.is_locked());

    mutex.unlock();
}

TEST_F(MutexTest, LockWithTimeoutOnLockedMutex) {
    // Given: A locked mutex
    Mutex mutex;
    mutex.lock();

    // When: Another thread tries to lock with short timeout
    std::atomic<bool> timeout_occurred{false};
    std::atomic<bool> lock_acquired{false};

    std::thread other([&]() {
        bool locked = mutex.lock(50);  // 50ms timeout
        if (locked) {
            lock_acquired = true;
            mutex.unlock();
        } else {
            timeout_occurred = true;
        }
    });

    other.join();

    // Then: Should timeout (mutex still held by main thread)
    EXPECT_TRUE(timeout_occurred.load());
    EXPECT_FALSE(lock_acquired.load());

    mutex.unlock();
}

// ============================================================================
// Test 7: Owner Tracking
// ============================================================================

TEST_F(MutexTest, OwnerIsCorrectlyTracked) {
    // Given: A mutex
    Mutex mutex;
    TaskControlBlock* initial_owner = nullptr;

    // When: Locking
    mutex.lock();
    TaskControlBlock* locked_owner = mutex.owner();

    // Then: Should have an owner
    EXPECT_NE(locked_owner, nullptr);

    // When: Unlocking
    mutex.unlock();
    TaskControlBlock* unlocked_owner = mutex.owner();

    // Then: Should have no owner
    EXPECT_EQ(unlocked_owner, nullptr);
}

// ============================================================================
// Test 8: Unlock by Non-Owner (Error Case)
// ============================================================================

TEST_F(MutexTest, UnlockByDifferentThreadFails) {
    // Given: A mutex locked by one thread
    Mutex mutex;
    mutex.lock();

    // When: Another thread tries to unlock
    std::atomic<bool> unlock_succeeded{false};
    std::atomic<bool> unlock_failed{false};

    std::thread other([&]() {
        bool result = mutex.unlock();
        if (result) {
            unlock_succeeded = true;
        } else {
            unlock_failed = true;
        }
    });

    other.join();

    // Then: Unlock should fail (not owner)
    EXPECT_TRUE(unlock_failed.load());
    EXPECT_FALSE(unlock_succeeded.load());

    // And: Mutex should still be locked
    EXPECT_TRUE(mutex.is_locked());

    // Cleanup
    mutex.unlock();
}

// ============================================================================
// Test 9: Multiple Mutexes
// ============================================================================

TEST_F(MutexTest, MultipleIndependentMutexes) {
    // Given: Multiple mutexes
    Mutex mutex1, mutex2, mutex3;

    // When: Locking all of them
    EXPECT_TRUE(mutex1.lock());
    EXPECT_TRUE(mutex2.lock());
    EXPECT_TRUE(mutex3.lock());

    // Then: All should be independently locked
    EXPECT_TRUE(mutex1.is_locked());
    EXPECT_TRUE(mutex2.is_locked());
    EXPECT_TRUE(mutex3.is_locked());

    // When: Unlocking in different order
    EXPECT_TRUE(mutex2.unlock());
    EXPECT_TRUE(mutex1.unlock());
    EXPECT_TRUE(mutex3.unlock());

    // Then: All should be unlocked
    EXPECT_FALSE(mutex1.is_locked());
    EXPECT_FALSE(mutex2.is_locked());
    EXPECT_FALSE(mutex3.is_locked());
}

// ============================================================================
// Test 10: Lock Guard Exception Safety
// ============================================================================

TEST_F(MutexTest, LockGuardUnlocksOnException) {
    // Given: A mutex
    Mutex mutex;

    // When: Exception thrown with LockGuard
    try {
        LockGuard guard(mutex);
        EXPECT_TRUE(mutex.is_locked());
        throw std::runtime_error("Test exception");
    } catch (...) {
        // Expected
    }

    // Then: Mutex should be unlocked despite exception
    EXPECT_FALSE(mutex.is_locked());
}

// ============================================================================
// Test 11: Stress Test - High Contention
// ============================================================================

TEST_F(MutexTest, StressTestHighContention) {
    // Given: Shared resource and mutex
    Mutex mutex;
    std::atomic<core::u32> shared_value{0};

    const int NUM_THREADS = 8;
    const int OPERATIONS_PER_THREAD = 500;

    // When: Many threads compete for mutex
    auto worker = [&]() {
        for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
            if (mutex.lock(100)) {
                core::u32 temp = shared_value.load();
                temp++;
                shared_value.store(temp);
                mutex.unlock();
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Then: Value should reflect all successful locks
    // (May be less than total if timeouts occurred)
    EXPECT_GT(shared_value.load(), 0u);
    EXPECT_LE(shared_value.load(), NUM_THREADS * OPERATIONS_PER_THREAD);
}

// ============================================================================
// Test 12: Producer-Consumer Pattern
// ============================================================================

TEST_F(MutexTest, ProducerConsumerPattern) {
    // Given: Shared buffer and mutex
    Mutex mutex;
    std::vector<int> buffer;
    std::atomic<bool> done{false};

    const int ITEMS_TO_PRODUCE = 100;

    // When: Producer adds items, consumer removes them
    std::thread producer([&]() {
        for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
            LockGuard guard(mutex);
            buffer.push_back(i);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        done = true;
    });

    std::thread consumer([&]() {
        int items_consumed = 0;
        while (items_consumed < ITEMS_TO_PRODUCE) {
            LockGuard guard(mutex);
            if (!buffer.empty()) {
                buffer.erase(buffer.begin());
                items_consumed++;
            }
        }
    });

    producer.join();
    consumer.join();

    // Then: Buffer should be empty
    EXPECT_TRUE(buffer.empty());
}

// ============================================================================
// Test 13: Rapid Lock/Unlock Cycles
// ============================================================================

TEST_F(MutexTest, RapidLockUnlockCycles) {
    // Given: A mutex
    Mutex mutex;

    // When: Rapidly locking and unlocking
    const int CYCLES = 10000;
    for (int i = 0; i < CYCLES; i++) {
        EXPECT_TRUE(mutex.lock());
        EXPECT_TRUE(mutex.is_locked());
        EXPECT_TRUE(mutex.unlock());
        EXPECT_FALSE(mutex.is_locked());
    }

    // Then: Should still work correctly
    EXPECT_FALSE(mutex.is_locked());
}

// ============================================================================
// Test 14: TryLock vs Lock Behavior
// ============================================================================

TEST_F(MutexTest, TryLockVsLockBehavior) {
    // Given: A locked mutex
    Mutex mutex;
    mutex.lock();

    // When: try_lock is called on locked mutex
    // (Note: Same task, so recursive lock will succeed)
    bool try_result = mutex.try_lock();

    // Then: Should succeed (recursive)
    EXPECT_TRUE(try_result);

    // Unlock twice (recursive)
    mutex.unlock();
    mutex.unlock();

    // When: try_lock on free mutex
    bool try_result_free = mutex.try_lock();

    // Then: Should succeed
    EXPECT_TRUE(try_result_free);
    EXPECT_TRUE(mutex.is_locked());

    mutex.unlock();
}

// ============================================================================
// Test 15: Memory Footprint
// ============================================================================

TEST_F(MutexTest, MutexSizeIsReasonable) {
    // When: Checking mutex size
    size_t mutex_size = sizeof(Mutex);
    size_t guard_size = sizeof(LockGuard);

    // Then: Should be compact (documented as 20 bytes for Mutex)
    EXPECT_LE(mutex_size, 32u) << "Mutex should be compact for embedded systems";
    EXPECT_LE(guard_size, 16u) << "LockGuard should be minimal overhead";
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
