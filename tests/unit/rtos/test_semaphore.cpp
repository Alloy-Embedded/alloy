/// Unit Tests for RTOS Semaphores
///
/// Tests binary and counting semaphores for task synchronization,
/// ISR signaling, resource pooling, and edge cases.

#include <gtest/gtest.h>
#include "rtos/semaphore.hpp"
#include "rtos/rtos.hpp"
#include "hal/host/systick.hpp"
#include <thread>
#include <atomic>
#include <chrono>

using namespace alloy;
using namespace alloy::rtos;

// Test fixture
class SemaphoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto result = hal::host::SystemTick::init();
        ASSERT_TRUE(result.is_ok());
    }
};

// ============================================================================
// Test 1: Binary Semaphore - Creation and Initial State
// ============================================================================

TEST_F(SemaphoreTest, BinarySemaphoreCreationDefault) {
    // Given: A binary semaphore with default construction
    BinarySemaphore sem;

    // Then: Should start at 0 (taken)
    EXPECT_EQ(sem.count(), 0u);
    EXPECT_FALSE(sem.is_available());
}

TEST_F(SemaphoreTest, BinarySemaphoreCreationWithInitialValue) {
    // Given: Binary semaphores with different initial values
    BinarySemaphore sem0(0);
    BinarySemaphore sem1(1);
    BinarySemaphore sem_large(100);  // Should be clamped to 1

    // Then: Values should be correct
    EXPECT_EQ(sem0.count(), 0u);
    EXPECT_EQ(sem1.count(), 1u);
    EXPECT_EQ(sem_large.count(), 1u) << "Binary semaphore should clamp to 1";
}

// ============================================================================
// Test 2: Binary Semaphore - Give and Take
// ============================================================================

TEST_F(SemaphoreTest, BinarySemaphoreBasicGiveTake) {
    // Given: A binary semaphore starting at 0
    BinarySemaphore sem(0);

    // When: Giving the semaphore
    sem.give();

    // Then: Count should be 1
    EXPECT_EQ(sem.count(), 1u);
    EXPECT_TRUE(sem.is_available());

    // When: Taking the semaphore
    bool taken = sem.try_take();

    // Then: Should succeed and count becomes 0
    EXPECT_TRUE(taken);
    EXPECT_EQ(sem.count(), 0u);
    EXPECT_FALSE(sem.is_available());
}

TEST_F(SemaphoreTest, BinarySemaphoreTryTakeWhenEmpty) {
    // Given: An empty binary semaphore
    BinarySemaphore sem(0);

    // When: Trying to take
    bool taken = sem.try_take();

    // Then: Should fail
    EXPECT_FALSE(taken);
    EXPECT_EQ(sem.count(), 0u);
}

TEST_F(SemaphoreTest, BinarySemaphoreMultipleGives) {
    // Given: A binary semaphore
    BinarySemaphore sem(0);

    // When: Giving multiple times
    sem.give();
    sem.give();
    sem.give();

    // Then: Count should still be 1 (binary property)
    EXPECT_EQ(sem.count(), 1u);

    // When: Taking once
    bool taken = sem.try_take();

    // Then: Should succeed
    EXPECT_TRUE(taken);
    EXPECT_EQ(sem.count(), 0u);

    // When: Trying to take again
    taken = sem.try_take();

    // Then: Should fail (only one token despite multiple gives)
    EXPECT_FALSE(taken);
}

// ============================================================================
// Test 3: Binary Semaphore - Producer/Consumer Signaling
// ============================================================================

TEST_F(SemaphoreTest, BinarySemaphoreProducerConsumerSignaling) {
    // Given: A binary semaphore for signaling
    BinarySemaphore data_ready(0);
    std::atomic<int> data{0};
    std::atomic<int> items_consumed{0};

    const int ITEMS_TO_PRODUCE = 50;

    // When: Producer signals, consumer waits
    std::thread producer([&]() {
        for (int i = 1; i <= ITEMS_TO_PRODUCE; i++) {
            data.store(i);
            data_ready.give();  // Signal data ready
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    std::thread consumer([&]() {
        for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
            // Wait for data with timeout
            while (!data_ready.take(100)) {
                // Keep trying
            }
            int value = data.load();
            EXPECT_GT(value, 0);
            items_consumed++;
        }
    });

    producer.join();
    consumer.join();

    // Then: All items should be consumed
    EXPECT_EQ(items_consumed.load(), ITEMS_TO_PRODUCE);
}

// ============================================================================
// Test 4: Counting Semaphore - Creation and Limits
// ============================================================================

TEST_F(SemaphoreTest, CountingSemaphoreCreation) {
    // Given: Counting semaphores with different max counts
    CountingSemaphore<5> sem5;
    CountingSemaphore<10> sem10(7);
    CountingSemaphore<255> sem_max(255);

    // Then: Initial values should be correct
    EXPECT_EQ(sem5.count(), 0u);
    EXPECT_EQ(sem5.max_count(), 5u);

    EXPECT_EQ(sem10.count(), 7u);
    EXPECT_EQ(sem10.max_count(), 10u);

    EXPECT_EQ(sem_max.count(), 255u);
    EXPECT_EQ(sem_max.max_count(), 255u);
}

TEST_F(SemaphoreTest, CountingSemaphoreInitialValueClamped) {
    // Given: Counting semaphore with initial value > max
    CountingSemaphore<5> sem(100);

    // Then: Should be clamped to max
    EXPECT_EQ(sem.count(), 5u);
    EXPECT_EQ(sem.max_count(), 5u);
}

// ============================================================================
// Test 5: Counting Semaphore - Give and Take
// ============================================================================

TEST_F(SemaphoreTest, CountingSemaphoreBasicGiveTake) {
    // Given: A counting semaphore
    CountingSemaphore<10> sem(0);

    // When: Giving multiple times
    sem.give();
    EXPECT_EQ(sem.count(), 1u);

    sem.give();
    EXPECT_EQ(sem.count(), 2u);

    sem.give();
    EXPECT_EQ(sem.count(), 3u);

    // When: Taking
    EXPECT_TRUE(sem.try_take());
    EXPECT_EQ(sem.count(), 2u);

    EXPECT_TRUE(sem.try_take());
    EXPECT_EQ(sem.count(), 1u);

    EXPECT_TRUE(sem.try_take());
    EXPECT_EQ(sem.count(), 0u);

    // Then: Should be empty
    EXPECT_FALSE(sem.try_take());
}

TEST_F(SemaphoreTest, CountingSemaphoreMaxLimit) {
    // Given: A counting semaphore at max count
    CountingSemaphore<5> sem(5);

    // When: Trying to give more
    sem.give();
    sem.give();
    sem.give();

    // Then: Should stay at max
    EXPECT_EQ(sem.count(), 5u);

    // When: Taking all tokens
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(sem.try_take());
    }

    // Then: Should be empty
    EXPECT_EQ(sem.count(), 0u);
    EXPECT_FALSE(sem.try_take());
}

// ============================================================================
// Test 6: Counting Semaphore - Resource Pool Pattern
// ============================================================================

TEST_F(SemaphoreTest, CountingSemaphoreResourcePool) {
    // Given: Resource pool with 5 resources
    CountingSemaphore<5> pool(5);
    std::atomic<int> max_concurrent_users{0};
    std::atomic<int> current_users{0};

    const int NUM_WORKERS = 10;
    const int OPERATIONS_PER_WORKER = 20;

    // When: Multiple workers compete for resources
    auto worker = [&]() {
        for (int i = 0; i < OPERATIONS_PER_WORKER; i++) {
            if (pool.try_take()) {
                // Got resource
                int users = ++current_users;

                // Track max concurrent users
                int current_max = max_concurrent_users.load();
                while (users > current_max) {
                    max_concurrent_users.compare_exchange_weak(current_max, users);
                }

                // Simulate work
                std::this_thread::sleep_for(std::chrono::microseconds(10));

                // Release resource
                --current_users;
                pool.give();
            }
        }
    };

    std::vector<std::thread> workers;
    for (int i = 0; i < NUM_WORKERS; i++) {
        workers.emplace_back(worker);
    }

    for (auto& w : workers) {
        w.join();
    }

    // Then: Max concurrent users should not exceed pool size
    EXPECT_LE(max_concurrent_users.load(), 5);

    // And: All resources should be returned
    EXPECT_EQ(pool.count(), 5u);
}

// ============================================================================
// Test 7: Semaphore Timeout Behavior
// ============================================================================

TEST_F(SemaphoreTest, BinarySemaphoreTakeWithTimeout) {
    // Given: An empty binary semaphore
    BinarySemaphore sem(0);

    // When: Taking with short timeout
    auto start = std::chrono::steady_clock::now();
    bool taken = sem.take(50);  // 50ms timeout
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Then: Should timeout
    EXPECT_FALSE(taken);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 45);
}

TEST_F(SemaphoreTest, CountingSemaphoreTakeWithTimeout) {
    // Given: An empty counting semaphore
    CountingSemaphore<5> sem(0);

    // When: Taking with short timeout
    auto start = std::chrono::steady_clock::now();
    bool taken = sem.take(50);  // 50ms timeout
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Then: Should timeout
    EXPECT_FALSE(taken);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 45);
}

// ============================================================================
// Test 8: Multiple Semaphores Independence
// ============================================================================

TEST_F(SemaphoreTest, MultipleSemaphoresAreIndependent) {
    // Given: Multiple binary semaphores
    BinarySemaphore sem1(0);
    BinarySemaphore sem2(0);
    BinarySemaphore sem3(1);

    // When: Manipulating them independently
    sem1.give();

    // Then: Each should have independent state
    EXPECT_TRUE(sem1.is_available());
    EXPECT_FALSE(sem2.is_available());
    EXPECT_TRUE(sem3.is_available());

    sem1.try_take();
    sem3.try_take();

    EXPECT_FALSE(sem1.is_available());
    EXPECT_FALSE(sem2.is_available());
    EXPECT_FALSE(sem3.is_available());
}

TEST_F(SemaphoreTest, MultipleCountingSemaphoresAreIndependent) {
    // Given: Multiple counting semaphores
    CountingSemaphore<5> sem1(2);
    CountingSemaphore<10> sem2(5);
    CountingSemaphore<3> sem3(0);

    // When: Manipulating them
    sem1.give();
    sem2.try_take();
    sem3.give();

    // Then: Each should have correct independent count
    EXPECT_EQ(sem1.count(), 3u);
    EXPECT_EQ(sem2.count(), 4u);
    EXPECT_EQ(sem3.count(), 1u);
}

// ============================================================================
// Test 9: ISR-Like Signaling Pattern
// ============================================================================

TEST_F(SemaphoreTest, ISRLikeSignalingPattern) {
    // Given: Semaphore for ISR → Task signaling
    BinarySemaphore isr_signal(0);
    std::atomic<int> events_processed{0};
    std::atomic<bool> stop{false};

    const int EVENTS_TO_GENERATE = 100;

    // Simulated ISR (separate thread)
    std::thread isr_simulator([&]() {
        for (int i = 0; i < EVENTS_TO_GENERATE; i++) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            isr_signal.give();  // Signal event from "ISR"
        }
        stop = true;
    });

    // Task waiting for ISR signals
    std::thread task([&]() {
        while (!stop.load() || isr_signal.is_available()) {
            if (isr_signal.take(10)) {  // Wait for signal
                events_processed++;
            }
        }
    });

    isr_simulator.join();
    task.join();

    // Then: All events should be processed
    EXPECT_GE(events_processed.load(), EVENTS_TO_GENERATE - 5);  // Allow some tolerance
}

// ============================================================================
// Test 10: Rate Limiting Pattern
// ============================================================================

TEST_F(SemaphoreTest, RateLimitingPattern) {
    // Given: Counting semaphore for rate limiting (tokens per second)
    CountingSemaphore<10> rate_limiter(10);
    std::atomic<int> operations_completed{0};

    const int OPERATIONS_TO_TRY = 50;

    // When: Trying many operations
    for (int i = 0; i < OPERATIONS_TO_TRY; i++) {
        if (rate_limiter.try_take()) {
            operations_completed++;
        }
    }

    // Then: Should be limited by semaphore count
    EXPECT_EQ(operations_completed.load(), 10);
    EXPECT_EQ(rate_limiter.count(), 0u);

    // When: Refilling tokens
    for (int i = 0; i < 5; i++) {
        rate_limiter.give();
    }

    // Then: Should have more capacity
    EXPECT_EQ(rate_limiter.count(), 5u);
}

// ============================================================================
// Test 11: Stress Test - High Frequency Signaling
// ============================================================================

TEST_F(SemaphoreTest, StressTestHighFrequencySignaling) {
    // Given: Binary semaphore with rapid signaling
    BinarySemaphore sem(0);
    std::atomic<int> signals_sent{0};
    std::atomic<int> signals_received{0};
    std::atomic<bool> done{false};

    const int SIGNALS_TO_SEND = 1000;

    // Rapid producer
    std::thread producer([&]() {
        for (int i = 0; i < SIGNALS_TO_SEND; i++) {
            sem.give();
            signals_sent++;
        }
        done = true;
    });

    // Rapid consumer
    std::thread consumer([&]() {
        while (!done.load() || sem.is_available()) {
            if (sem.try_take()) {
                signals_received++;
            }
        }
    });

    producer.join();
    consumer.join();

    // Then: Should have processed signals (may miss some due to binary property)
    EXPECT_GT(signals_received.load(), 0);
    EXPECT_LE(signals_received.load(), signals_sent.load());
}

// ============================================================================
// Test 12: Counting Semaphore Stress Test
// ============================================================================

TEST_F(SemaphoreTest, StressTestCountingSemaphorePool) {
    // Given: Resource pool
    CountingSemaphore<20> pool(20);
    std::atomic<int> successful_acquisitions{0};
    std::atomic<int> in_use{0};
    std::atomic<int> max_in_use{0};

    const int NUM_THREADS = 10;
    const int OPERATIONS = 100;

    auto worker = [&]() {
        for (int i = 0; i < OPERATIONS; i++) {
            if (pool.try_take()) {
                successful_acquisitions++;
                int current = ++in_use;

                // Track maximum
                int current_max = max_in_use.load();
                while (current > current_max) {
                    max_in_use.compare_exchange_weak(current_max, current);
                }

                std::this_thread::sleep_for(std::chrono::microseconds(1));

                --in_use;
                pool.give();
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

    // Then: Max in use should not exceed pool size
    EXPECT_LE(max_in_use.load(), 20);
    EXPECT_EQ(pool.count(), 20u);  // All returned
}

// ============================================================================
// Test 13: Memory Footprint
// ============================================================================

TEST_F(SemaphoreTest, SemaphoreSizeIsReasonable) {
    // When: Checking sizes
    size_t binary_size = sizeof(BinarySemaphore);
    size_t counting_size = sizeof(CountingSemaphore<10>);

    // Then: Should be compact (documented: 12 bytes binary, 16 bytes counting)
    EXPECT_LE(binary_size, 24u) << "BinarySemaphore should be compact";
    EXPECT_LE(counting_size, 32u) << "CountingSemaphore should be compact";
}

// ============================================================================
// Test 14: Different MaxCount Values
// ============================================================================

TEST_F(SemaphoreTest, DifferentMaxCountValues) {
    // Given: Counting semaphores with various max counts
    CountingSemaphore<1> tiny(1);      // Effectively binary
    CountingSemaphore<8> small(8);
    CountingSemaphore<64> medium(64);
    CountingSemaphore<255> large(255);

    // Then: All should respect their limits
    EXPECT_EQ(tiny.max_count(), 1u);
    EXPECT_EQ(small.max_count(), 8u);
    EXPECT_EQ(medium.max_count(), 64u);
    EXPECT_EQ(large.max_count(), 255u);

    // When: Testing behavior
    tiny.give();
    tiny.give();  // Should not exceed 1
    EXPECT_EQ(tiny.count(), 1u);

    for (int i = 0; i < 10; i++) {
        small.give();
    }
    EXPECT_EQ(small.count(), 8u);  // Capped at max
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
