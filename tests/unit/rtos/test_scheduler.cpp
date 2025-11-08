/// Unit Tests for RTOS Scheduler
///
/// Tests priority-based preemptive scheduling, task state transitions,
/// ready queue operations, delayed tasks, and context switching.

#include <atomic>
#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "hal/host/systick.hpp"

#include "rtos/rtos.hpp"
#include "rtos/scheduler.hpp"

using namespace alloy;
using namespace alloy::rtos;

// ============================================================================
// Test 1: Priority Enum Values
// ============================================================================

TEST_CASE("PriorityEnumValuesAreCorrect", "[rtos][scheduler]") {
    // When: Checking priority enum values
    REQUIRE(static_cast<core::u8>(Priority::Idle), 0u);
    REQUIRE(static_cast<core::u8>(Priority::Lowest), 1u);
    REQUIRE(static_cast<core::u8>(Priority::Low), 2u);
    REQUIRE(static_cast<core::u8>(Priority::Normal), 3u);
    REQUIRE(static_cast<core::u8>(Priority::High), 4u);
    REQUIRE(static_cast<core::u8>(Priority::Higher), 5u);
    REQUIRE(static_cast<core::u8>(Priority::Highest), 6u);
    REQUIRE(static_cast<core::u8>(Priority::Critical), 7u);
}

TEST_CASE("PrioritiesAreOrdered", "[rtos][scheduler]") {
    // Then: Priorities should be in ascending order
    REQUIRE(static_cast<core::u8>(Priority::Idle), static_cast<core::u8>(Priority::Lowest));
    REQUIRE(static_cast<core::u8>(Priority::Low), static_cast<core::u8>(Priority::Normal));
    REQUIRE(static_cast<core::u8>(Priority::Normal), static_cast<core::u8>(Priority::High));
    REQUIRE(static_cast<core::u8>(Priority::High), static_cast<core::u8>(Priority::Critical));
}

// ============================================================================
// Test 2: TaskState Enum
// ============================================================================

TEST_CASE("TaskStateEnumValues", "[rtos][scheduler]") {
    // When: Checking task states
    TaskState ready = TaskState::Ready;
    TaskState running = TaskState::Running;
    TaskState blocked = TaskState::Blocked;
    TaskState suspended = TaskState::Suspended;
    TaskState delayed = TaskState::Delayed;

    // Then: All states should be distinct
    REQUIRE(ready, running);
    REQUIRE(running, blocked);
    REQUIRE(blocked, suspended);
    REQUIRE(suspended, delayed);
}

// ============================================================================
// Test 3: ReadyQueue Operations
// ============================================================================

TEST_CASE("ReadyQueueInitiallyEmpty", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A new ready queue
    ReadyQueue queue;

    // Then: Should be empty
    REQUIRE(queue.get_highest_priority(), nullptr);
}

TEST_CASE("ReadyQueueMakeReadyAndRetrieve", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A ready queue and a task
    ReadyQueue queue;

    // Create a task control block (simplified for testing)
    TaskControlBlock tcb;
    tcb.priority = static_cast<core::u8>(Priority::Normal);
    tcb.state = TaskState::Ready;
    tcb.next = nullptr;
    tcb.name = "TestTask";

    // When: Making task ready
    queue.make_ready(&tcb);

    // Then: Should be retrievable
    TaskControlBlock* retrieved = queue.get_highest_priority();
    REQUIRE(retrieved, &tcb);
    REQUIRE(retrieved->priority, static_cast<core::u8>(Priority::Normal));
}

TEST_CASE("ReadyQueuePriorityOrdering", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Ready queue with multiple tasks
    ReadyQueue queue;

    TaskControlBlock tcb_low;
    tcb_low.priority = static_cast<core::u8>(Priority::Low);
    tcb_low.state = TaskState::Ready;
    tcb_low.next = nullptr;
    tcb_low.name = "LowPriority";

    TaskControlBlock tcb_high;
    tcb_high.priority = static_cast<core::u8>(Priority::High);
    tcb_high.state = TaskState::Ready;
    tcb_high.next = nullptr;
    tcb_high.name = "HighPriority";

    TaskControlBlock tcb_normal;
    tcb_normal.priority = static_cast<core::u8>(Priority::Normal);
    tcb_normal.state = TaskState::Ready;
    tcb_normal.next = nullptr;
    tcb_normal.name = "NormalPriority";

    // When: Adding tasks in random order
    queue.make_ready(&tcb_low);
    queue.make_ready(&tcb_high);
    queue.make_ready(&tcb_normal);

    // Then: Should retrieve highest priority first
    TaskControlBlock* first = queue.get_highest_priority();
    REQUIRE(first, &tcb_high);
    REQUIRE(first->priority, static_cast<core::u8>(Priority::High));
}

TEST_CASE("ReadyQueueRemoveTask", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Ready queue with a task
    ReadyQueue queue;

    TaskControlBlock tcb;
    tcb.priority = static_cast<core::u8>(Priority::Normal);
    tcb.state = TaskState::Ready;
    tcb.next = nullptr;
    tcb.name = "TestTask";

    queue.make_ready(&tcb);

    // When: Removing the task
    queue.make_not_ready(&tcb);

    // Then: Queue should be empty
    REQUIRE(queue.get_highest_priority(), nullptr);
}

TEST_CASE("ReadyQueueMultipleTasksSamePriority", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Ready queue with multiple tasks at same priority
    ReadyQueue queue;

    TaskControlBlock tcb1, tcb2, tcb3;

    tcb1.priority = static_cast<core::u8>(Priority::Normal);
    tcb1.state = TaskState::Ready;
    tcb1.next = nullptr;
    tcb1.name = "Task1";

    tcb2.priority = static_cast<core::u8>(Priority::Normal);
    tcb2.state = TaskState::Ready;
    tcb2.next = nullptr;
    tcb2.name = "Task2";

    tcb3.priority = static_cast<core::u8>(Priority::Normal);
    tcb3.state = TaskState::Ready;
    tcb3.next = nullptr;
    tcb3.name = "Task3";

    // When: Adding tasks
    queue.make_ready(&tcb1);
    queue.make_ready(&tcb2);
    queue.make_ready(&tcb3);

    // Then: Should get one of them (FIFO order at same priority)
    TaskControlBlock* retrieved = queue.get_highest_priority();
    REQUIRE(retrieved, nullptr);
    REQUIRE(retrieved->priority, static_cast<core::u8>(Priority::Normal));
}

// ============================================================================
// Test 4: RTOS Delay
// ============================================================================

TEST_CASE("DelayFunction", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: RTOS initialized
    auto start = std::chrono::steady_clock::now();

    // When: Delaying for 50ms
    RTOS::delay(50);

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    // Then: Should have delayed approximately 50ms
    // Allow tolerance due to OS scheduling
    REQUIRE(ms, 40);
    REQUIRE(ms, 100);
}

TEST_CASE("ZeroDelayDoesNotBlock", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: RTOS initialized
    auto start = std::chrono::steady_clock::now();

    // When: Delaying for 0ms
    RTOS::delay(0);

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    // Then: Should return immediately
    REQUIRE(ms, 10);
}

// ============================================================================
// Test 5: Tick Counter
// ============================================================================

TEST_CASE("TickCounterIncrements", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Initial tick count
    core::u32 initial_count = RTOS::get_tick_count();

    // When: Waiting some time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Then: Tick count should have increased
    core::u32 new_count = RTOS::get_tick_count();
    REQUIRE(new_count, initial_count);
}

// ============================================================================
// Test 6: Task State Transitions
// ============================================================================

TEST_CASE("TaskInitialStateIsReady", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A newly created task
    std::atomic<bool> ran{false};

    auto task_func = [&ran]() {
        ran = true;
        while (true)
            RTOS::delay(1000);
    };

    Task<512, Priority::Normal> task(task_func, "StateTest");

    // Then: Initial state should be Ready
    REQUIRE(task.get_tcb()->state, TaskState::Ready);
}

// ============================================================================
// Test 7: Priority Preemption Behavior
// ============================================================================

TEST_CASE("HigherPriorityTaskPreemptsLower", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Two tasks - one high priority, one low priority
    std::atomic<int> execution_order{0};
    std::atomic<int> low_priority_started{0};
    std::atomic<int> high_priority_started{0};

    auto low_priority_func = [&]() {
        low_priority_started = ++execution_order;
        for (int i = 0; i < 5; i++) {
            RTOS::delay(10);
        }
    };

    auto high_priority_func = [&]() {
        high_priority_started = ++execution_order;
        RTOS::delay(10);
    };

    // When: Creating tasks (high priority should preempt)
    Task<512, Priority::Low> low_task(low_priority_func, "LowTask");

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    Task<512, Priority::High> high_task(high_priority_func, "HighTask");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Note: In host implementation, exact preemption timing may vary
    // We're mainly testing that both tasks can execute
    REQUIRE(low_priority_started.load(), 0);
    REQUIRE(high_priority_started.load(), 0);
}

// ============================================================================
// Test 8: ReadyQueue Bitmap Optimization
// ============================================================================

TEST_CASE("ReadyQueueUsesO1PriorityLookup", "[rtos][scheduler]") {
    // This test verifies the O(1) priority bitmap optimization
    // by ensuring quick lookups even with many tasks

    ReadyQueue queue;
    std::vector<TaskControlBlock> tasks(100);

    // When: Adding 100 tasks at various priorities
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; i++) {
        tasks[i].priority = i % 8;  // Distribute across all 8 priorities
        tasks[i].state = TaskState::Ready;
        tasks[i].next = nullptr;
        tasks[i].name = "Task";
        queue.make_ready(&tasks[i]);
    }

    // Retrieve highest priority 100 times
    for (int i = 0; i < 100; i++) {
        TaskControlBlock* task = queue.get_highest_priority();
        REQUIRE(task, nullptr);
    }

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

    // Then: Should complete very quickly (O(1) per operation)
    REQUIRE(us, 1000) << "O(1) operations should be fast";
}

// ============================================================================
// Test 9: Scheduler State Structure
// ============================================================================

TEST_CASE("SchedulerStateStructureSize", "[rtos][scheduler]") {
    // When: Checking scheduler state size
    size_t scheduler_size = sizeof(SchedulerState);

    // Then: Should be reasonably compact
    REQUIRE(scheduler_size, 256u) << "SchedulerState should be compact";
}

TEST_CASE("GlobalSchedulerExists", "[rtos][scheduler]") {
    // When: Accessing global scheduler
    SchedulerState* scheduler = &g_scheduler;

    // Then: Should exist
    REQUIRE(scheduler, nullptr);
}

// ============================================================================
// Test 10: Task Control Block Structure
// ============================================================================

TEST_CASE("TCBStructureIsCompact", "[rtos][scheduler]") {
    // When: Checking TCB size
    size_t tcb_size = sizeof(TaskControlBlock);

    // Then: Should be compact for embedded systems
    REQUIRE(tcb_size, 64u) << "TCB should be compact";
    REQUIRE(tcb_size, 24u) << "TCB should contain essential fields";
}

TEST_CASE("TCBHasEssentialFields", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A task control block
    TaskControlBlock tcb;

    // Then: Should have all essential fields accessible
    tcb.priority = 5;
    tcb.state = TaskState::Running;
    tcb.stack_base = nullptr;
    tcb.stack_pointer = nullptr;
    tcb.stack_size = 512;
    tcb.wake_time = 0;
    tcb.next = nullptr;
    tcb.name = "Test";

    REQUIRE(tcb.priority, 5u);
    REQUIRE(tcb.state, TaskState::Running);
    REQUIRE(tcb.stack_size, 512u);
}

// ============================================================================
// Test 11: Multiple Delays
// ============================================================================

TEST_CASE("MultipleSequentialDelays", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Multiple delay operations
    auto start = std::chrono::steady_clock::now();

    // When: Performing multiple delays
    RTOS::delay(20);
    RTOS::delay(20);
    RTOS::delay(20);

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    // Then: Total delay should be approximately sum of delays
    REQUIRE(ms, 50);
    REQUIRE(ms, 100);
}

// ============================================================================
// Test 12: Multiple Tasks Execution
// ============================================================================

TEST_CASE("MultipleTasksExecute", "[rtos][scheduler]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Multiple tasks
    std::atomic<int> task1_runs{0};
    std::atomic<int> task2_runs{0};

    auto task1_func = [&task1_runs]() {
        for (int i = 0; i < 3; i++) {
            task1_runs++;
            RTOS::delay(20);
        }
    };

    auto task2_func = [&task2_runs]() {
        for (int i = 0; i < 3; i++) {
            task2_runs++;
            RTOS::delay(20);
        }
    };

    // When: Creating tasks
    Task<512, Priority::Normal> task1(task1_func, "Task1");
    Task<512, Priority::Normal> task2(task2_func, "Task2");

    // Wait for tasks to run
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Then: Both tasks should execute
    REQUIRE(task1_runs.load(), 0);
    REQUIRE(task2_runs.load(), 0);
}

// ============================================================================
// Test 13: INFINITE Constant
// ============================================================================

TEST_CASE("InfiniteConstantIsMaxValue", "[rtos][scheduler]") {
    // Then: INFINITE should be maximum 32-bit value
    REQUIRE(INFINITE, 0xFFFFFFFFu);
}

// ============================================================================
// Main
// ============================================================================
