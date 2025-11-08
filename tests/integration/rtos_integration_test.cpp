/// RTOS Integration Tests
///
/// Comprehensive tests that exercise all RTOS primitives together
/// in realistic multi-task scenarios. Tests complex interactions
/// between Task, Queue, Mutex, Semaphore, EventFlags, and Scheduler.

#include <catch2/catch_test_macros.hpp>
#include "rtos/rtos.hpp"
#include "rtos/queue.hpp"
#include "rtos/mutex.hpp"
#include "rtos/semaphore.hpp"
#include "rtos/event.hpp"
#include "hal/host/systick.hpp"
#include <thread>
#include <atomic>
#include <chrono>

using namespace alloy;
using namespace alloy::rtos;

// ============================================================================
// Integration Test 1: Producer-Consumer with Multiple IPC Primitives
// ============================================================================

TEST_CASE("ProducerConsumerWithAllIPCPrimitives", "[rtos][integration]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Complete producer-consumer system with all IPC types
    struct SensorData {
        core::u32 timestamp;
        core::u16 temperature;
        core::u16 humidity;
    };

    Queue<SensorData, 8> data_queue;
    Mutex console_mutex;
    BinarySemaphore data_ready_sem;
    EventFlags system_events;

    constexpr core::u32 EVENT_PRODUCER_READY = (1 << 0);
    constexpr core::u32 EVENT_CONSUMER_READY = (1 << 1);
    constexpr core::u32 EVENT_PROCESSING_DONE = (1 << 2);

    std::atomic<int> items_produced{0};
    std::atomic<int> items_consumed{0};
    std::atomic<bool> stop{false};

    const int ITEMS_TO_PRODUCE = 20;

    // Producer task
    auto producer_func = [&]() {
        system_events.set(EVENT_PRODUCER_READY);

        for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
            SensorData data;
            data.timestamp = systick::micros();
            data.temperature = 20 + (i % 10);
            data.humidity = 50 + (i % 30);

            {
                LockGuard guard(console_mutex);
                // Simulated sensor reading
            }

            if (data_queue.send(data, 1000)) {
                items_produced++;
                data_ready_sem.give();
            }

            RTOS::delay(10);
        }

        stop = true;
    };

    // Consumer task
    auto consumer_func = [&]() {
        system_events.set(EVENT_CONSUMER_READY);

        // Wait for producer to be ready
        system_events.wait_any(EVENT_PRODUCER_READY, 1000);

        while (!stop.load() || items_consumed.load() < ITEMS_TO_PRODUCE) {
            if (data_ready_sem.take(100)) {
                SensorData data;
                if (data_queue.receive(data, 100)) {
                    {
                        LockGuard guard(console_mutex);
                        // Simulated processing
                    }
                    items_consumed++;
                }
            }
        }

        system_events.set(EVENT_PROCESSING_DONE);
    };

    // When: Running producer and consumer tasks
    Task<1024, Priority::High> producer_task(producer_func, "Producer");
    Task<1024, Priority::Normal> consumer_task(consumer_func, "Consumer");

    // Wait for completion
    core::u32 done = system_events.wait_any(EVENT_PROCESSING_DONE, 5000);

    // Then: All items should be produced and consumed
    REQUIRE(items_produced.load(), ITEMS_TO_PRODUCE);
    REQUIRE(items_consumed.load(), ITEMS_TO_PRODUCE);
    REQUIRE(done, 0u) << "Processing should complete";
}

// ============================================================================
// Integration Test 2: Multi-Task Synchronization
// ============================================================================

TEST_CASE("MultiTaskSynchronizationWithEventFlags", "[rtos][integration]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Multiple tasks coordinating with event flags
    EventFlags sync_events;

    constexpr core::u32 TASK1_READY = (1 << 0);
    constexpr core::u32 TASK2_READY = (1 << 1);
    constexpr core::u32 TASK3_READY = (1 << 2);
    constexpr core::u32 ALL_TASKS_READY = TASK1_READY | TASK2_READY | TASK3_READY;

    std::atomic<int> synchronized_executions{0};
    std::atomic<int> task1_count{0};
    std::atomic<int> task2_count{0};
    std::atomic<int> task3_count{0};

    const int ITERATIONS = 5;

    auto task1_func = [&]() {
        for (int i = 0; i < ITERATIONS; i++) {
            RTOS::delay(5);
            sync_events.set(TASK1_READY);

            if (sync_events.wait_all(ALL_TASKS_READY, 1000)) {
                task1_count++;
                synchronized_executions++;
                sync_events.clear(ALL_TASKS_READY);
            }
        }
    };

    auto task2_func = [&]() {
        for (int i = 0; i < ITERATIONS; i++) {
            RTOS::delay(6);
            sync_events.set(TASK2_READY);

            if (sync_events.wait_all(ALL_TASKS_READY, 1000)) {
                task2_count++;
                sync_events.clear(ALL_TASKS_READY);
            }
        }
    };

    auto task3_func = [&]() {
        for (int i = 0; i < ITERATIONS; i++) {
            RTOS::delay(7);
            sync_events.set(TASK3_READY);

            if (sync_events.wait_all(ALL_TASKS_READY, 1000)) {
                task3_count++;
                sync_events.clear(ALL_TASKS_READY);
            }
        }
    };

    // When: Running synchronized tasks
    Task<1024, Priority::Normal> task1(task1_func, "SyncTask1");
    Task<1024, Priority::Normal> task2(task2_func, "SyncTask2");
    Task<1024, Priority::Normal> task3(task3_func, "SyncTask3");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Then: All tasks should have synchronized
    REQUIRE(synchronized_executions.load(), 0);
    REQUIRE(task1_count.load(), 0);
    REQUIRE(task2_count.load(), 0);
    REQUIRE(task3_count.load(), 0);
}

// ============================================================================
// Integration Test 3: Resource Pool Management
// ============================================================================

TEST_CASE("ResourcePoolWithSemaphoreAndMutex", "[rtos][integration]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Resource pool managed by semaphore, access protected by mutex
    CountingSemaphore<5> pool(5);
    Mutex resource_mutex;

    std::atomic<int> max_concurrent{0};
    std::atomic<int> current_active{0};
    std::atomic<int> successful_uses{0};

    const int NUM_WORKERS = 8;
    const int OPERATIONS_PER_WORKER = 10;

    auto worker_func = [&]() {
        for (int i = 0; i < OPERATIONS_PER_WORKER; i++) {
            // Try to get resource from pool
            if (pool.take(100)) {
                // Got resource - use it
                {
                    LockGuard guard(resource_mutex);
                    int active = ++current_active;

                    // Track max concurrent
                    int current_max = max_concurrent.load();
                    while (active > current_max) {
                        max_concurrent.compare_exchange_weak(current_max, active);
                    }

                    successful_uses++;
                }

                // Simulate work
                RTOS::delay(2);

                {
                    LockGuard guard(resource_mutex);
                    --current_active;
                }

                // Return resource to pool
                pool.give();
            }
        }
    };

    // When: Multiple workers compete for resources
    std::vector<std::unique_ptr<Task<1024, Priority::Normal>>> workers;
    for (int i = 0; i < NUM_WORKERS; i++) {
        workers.push_back(
            std::make_unique<Task<1024, Priority::Normal>>(worker_func, "Worker")
        );
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Then: Concurrency should not exceed pool size
    REQUIRE(max_concurrent.load(), 5);
    REQUIRE(successful_uses.load(), 0);
    REQUIRE(current_active.load(), 0); // All resources returned
}

// ============================================================================
// Integration Test 4: Priority Inversion Prevention
// ============================================================================

TEST_CASE("PriorityInheritancePreventsInversion", "[rtos][integration]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Mutex with priority inheritance
    Mutex shared_mutex;
    std::atomic<int> execution_order{0};
    std::atomic<int> low_priority_got_lock{0};
    std::atomic<int> high_priority_got_lock{0};

    // Low priority task holds mutex for a while
    auto low_priority_func = [&]() {
        RTOS::delay(10);

        if (shared_mutex.lock(1000)) {
            low_priority_got_lock = ++execution_order;
            RTOS::delay(100);  // Hold mutex for a while
            shared_mutex.unlock();
        }
    };

    // High priority task tries to get mutex
    auto high_priority_func = [&]() {
        RTOS::delay(50);

        if (shared_mutex.lock(1000)) {
            high_priority_got_lock = ++execution_order;
            shared_mutex.unlock();
        }
    };

    // When: Running tasks with priority inheritance
    Task<1024, Priority::Low> low_task(low_priority_func, "LowTask");
    Task<1024, Priority::High> high_task(high_priority_func, "HighTask");

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Then: Both tasks should eventually get the lock
    REQUIRE(low_priority_got_lock.load(), 0);
    REQUIRE(high_priority_got_lock.load(), 0);

    // Low priority should get lock first (it started first)
    REQUIRE(low_priority_got_lock.load(), high_priority_got_lock.load());
}

// ============================================================================
// Integration Test 5: Complex Data Flow Pipeline
// ============================================================================

TEST_CASE("DataProcessingPipeline", "[rtos][integration]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Multi-stage data processing pipeline
    Queue<core::u32, 8> stage1_to_stage2;
    Queue<core::u32, 8> stage2_to_stage3;
    BinarySemaphore stage1_done;
    BinarySemaphore stage2_done;
    EventFlags pipeline_events;

    constexpr core::u32 STAGE1_COMPLETE = (1 << 0);
    constexpr core::u32 STAGE2_COMPLETE = (1 << 1);
    constexpr core::u32 STAGE3_COMPLETE = (1 << 2);

    std::atomic<int> stage3_processed{0};

    const int DATA_ITEMS = 15;

    // Stage 1: Data generator
    auto stage1_func = [&]() {
        for (int i = 0; i < DATA_ITEMS; i++) {
            stage1_to_stage2.send(i * 10, 1000);
            RTOS::delay(5);
        }
        pipeline_events.set(STAGE1_COMPLETE);
        stage1_done.give();
    };

    // Stage 2: Data processor
    auto stage2_func = [&]() {
        stage1_done.take(2000);  // Wait for stage 1 to start

        for (int i = 0; i < DATA_ITEMS; i++) {
            core::u32 data;
            if (stage1_to_stage2.receive(data, 1000)) {
                core::u32 processed = data + 5;  // Process
                stage2_to_stage3.send(processed, 1000);
            }
        }
        pipeline_events.set(STAGE2_COMPLETE);
        stage2_done.give();
    };

    // Stage 3: Data consumer
    auto stage3_func = [&]() {
        stage2_done.take(3000);  // Wait for stage 2 to start

        for (int i = 0; i < DATA_ITEMS; i++) {
            core::u32 data;
            if (stage2_to_stage3.receive(data, 1000)) {
                stage3_processed++;
            }
        }
        pipeline_events.set(STAGE3_COMPLETE);
    };

    // When: Running pipeline stages
    Task<1024, Priority::Normal> stage1(stage1_func, "Stage1");
    Task<1024, Priority::Normal> stage2(stage2_func, "Stage2");
    Task<1024, Priority::Low> stage3(stage3_func, "Stage3");

    // Wait for pipeline to complete
    core::u32 result = pipeline_events.wait_all(
        STAGE1_COMPLETE | STAGE2_COMPLETE | STAGE3_COMPLETE,
        5000
    );

    // Then: All data should flow through pipeline
    REQUIRE(result, 0u) << "Pipeline should complete";
    REQUIRE(stage3_processed.load(), DATA_ITEMS);
}

// ============================================================================
// Integration Test 6: Stress Test - All Primitives Under Load
// ============================================================================

TEST_CASE("StressTestAllPrimitivesUnderLoad", "[rtos][integration]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: System using all RTOS primitives simultaneously
    Queue<core::u32, 16> shared_queue;
    Mutex shared_mutex;
    CountingSemaphore<10> resource_pool(10);
    EventFlags control_events;

    std::atomic<int> queue_ops{0};
    std::atomic<int> mutex_locks{0};
    std::atomic<int> semaphore_takes{0};
    std::atomic<bool> stress_complete{false};

    const int NUM_STRESS_TASKS = 6;
    const int OPERATIONS_PER_TASK = 20;

    auto stress_worker = [&](int task_id) {
        for (int i = 0; i < OPERATIONS_PER_TASK; i++) {
            // Queue operation
            if (shared_queue.try_send(task_id)) {
                queue_ops++;
            }

            core::u32 value;
            if (shared_queue.try_receive(value)) {
                queue_ops++;
            }

            // Mutex operation
            {
                LockGuard guard(shared_mutex, 10);
                if (guard.is_locked()) {
                    mutex_locks++;
                }
            }

            // Semaphore operation
            if (resource_pool.try_take()) {
                semaphore_takes++;
                RTOS::delay(1);
                resource_pool.give();
            }

            // Event flags
            control_events.set(1 << (task_id % 8));

            RTOS::delay(2);
        }
    };

    // When: Running stress test
    std::vector<std::unique_ptr<Task<1024, Priority::Normal>>> stress_tasks;
    for (int i = 0; i < NUM_STRESS_TASKS; i++) {
        stress_tasks.push_back(
            std::make_unique<Task<1024, Priority::Normal>>(
                [&stress_worker, i]() { stress_worker(i); },
                "StressWorker"
            )
        );
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    stress_complete = true;

    // Then: All operations should have executed
    REQUIRE(queue_ops.load(), 0);
    REQUIRE(mutex_locks.load(), 0);
    REQUIRE(semaphore_takes.load(), 0);
}

// ============================================================================
// Integration Test 7: Deadlock Avoidance
// ============================================================================

TEST_CASE("TimeoutsPreventDeadlocks", "[rtos][integration]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Two mutexes that could cause deadlock without timeouts
    Mutex mutex_a;
    Mutex mutex_b;

    std::atomic<int> task1_timeouts{0};
    std::atomic<int> task2_timeouts{0};
    std::atomic<int> successful_dual_locks{0};

    const int ATTEMPTS = 20;

    // Task 1: Tries to lock A then B
    auto task1_func = [&]() {
        for (int i = 0; i < ATTEMPTS; i++) {
            if (mutex_a.lock(10)) {
                RTOS::delay(1);  // Increase chance of contention
                if (mutex_b.lock(10)) {
                    successful_dual_locks++;
                    mutex_b.unlock();
                } else {
                    task1_timeouts++;
                }
                mutex_a.unlock();
            }
            RTOS::delay(1);
        }
    };

    // Task 2: Tries to lock B then A (opposite order - potential deadlock)
    auto task2_func = [&]() {
        for (int i = 0; i < ATTEMPTS; i++) {
            if (mutex_b.lock(10)) {
                RTOS::delay(1);
                if (mutex_a.lock(10)) {
                    successful_dual_locks++;
                    mutex_a.unlock();
                } else {
                    task2_timeouts++;
                }
                mutex_b.unlock();
            }
            RTOS::delay(1);
        }
    };

    // When: Running tasks
    Task<1024, Priority::Normal> task1(task1_func, "Task1");
    Task<1024, Priority::Normal> task2(task2_func, "Task2");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Then: Timeouts should prevent deadlock
    // Some operations should succeed, some should timeout (no deadlock)
    REQUIRE(successful_dual_locks.load(), 0) << "Some should succeed";
    REQUIRE(task1_timeouts.load() + task2_timeouts.load(), 0) << "Some should timeout";
}

// ============================================================================
// Main
// ============================================================================


