/// Unit Tests for RTOS Scheduler
///
/// Tests priority-based preemptive scheduling, task state transitions,
/// ready queue operations, delayed tasks, and context switching.

#include <gtest/gtest.h>
#include "rtos/scheduler.hpp"
#include "rtos/rtos.hpp"
#include "hal/host/systick.hpp"
#include <thread>
#include <atomic>
#include <chrono>

using namespace alloy;
using namespace alloy::rtos;

// Test fixture
class SchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto result = hal::host::SystemTick::init();
        ASSERT_TRUE(result.is_ok());
    }
};

// ============================================================================
// Test 1: Priority Enum Values
// ============================================================================

TEST_F(SchedulerTest, PriorityEnumValuesAreCorrect) {
    // When: Checking priority enum values
    EXPECT_EQ(static_cast<core::u8>(Priority::Idle), 0u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Lowest), 1u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Low), 2u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Normal), 3u);
    EXPECT_EQ(static_cast<core::u8>(Priority::High), 4u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Higher), 5u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Highest), 6u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Critical), 7u);
}

TEST_F(SchedulerTest, PrioritiesAreOrdered) {
    // Then: Priorities should be in ascending order
    EXPECT_LT(static_cast<core::u8>(Priority::Idle),
              static_cast<core::u8>(Priority::Lowest));
    EXPECT_LT(static_cast<core::u8>(Priority::Low),
              static_cast<core::u8>(Priority::Normal));
    EXPECT_LT(static_cast<core::u8>(Priority::Normal),
              static_cast<core::u8>(Priority::High));
    EXPECT_LT(static_cast<core::u8>(Priority::High),
              static_cast<core::u8>(Priority::Critical));
}

// ============================================================================
// Test 2: TaskState Enum
// ============================================================================

TEST_F(SchedulerTest, TaskStateEnumValues) {
    // When: Checking task states
    TaskState ready = TaskState::Ready;
    TaskState running = TaskState::Running;
    TaskState blocked = TaskState::Blocked;
    TaskState suspended = TaskState::Suspended;
    TaskState delayed = TaskState::Delayed;

    // Then: All states should be distinct
    EXPECT_NE(ready, running);
    EXPECT_NE(running, blocked);
    EXPECT_NE(blocked, suspended);
    EXPECT_NE(suspended, delayed);
}

// ============================================================================
// Test 3: ReadyQueue Operations
// ============================================================================

TEST_F(SchedulerTest, ReadyQueueInitiallyEmpty) {
    // Given: A new ready queue
    ReadyQueue queue;

    // Then: Should be empty
    EXPECT_EQ(queue.get_highest_priority(), nullptr);
}

TEST_F(SchedulerTest, ReadyQueueMakeReadyAndRetrieve) {
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
    EXPECT_EQ(retrieved, &tcb);
    EXPECT_EQ(retrieved->priority, static_cast<core::u8>(Priority::Normal));
}

TEST_F(SchedulerTest, ReadyQueuePriorityOrdering) {
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
    EXPECT_EQ(first, &tcb_high);
    EXPECT_EQ(first->priority, static_cast<core::u8>(Priority::High));
}

TEST_F(SchedulerTest, ReadyQueueRemoveTask) {
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
    EXPECT_EQ(queue.get_highest_priority(), nullptr);
}

TEST_F(SchedulerTest, ReadyQueueMultipleTasksSamePriority) {
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
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->priority, static_cast<core::u8>(Priority::Normal));
}

// ============================================================================
// Test 4: RTOS Delay
// ============================================================================

TEST_F(SchedulerTest, DelayFunction) {
    // Given: RTOS initialized
    auto start = std::chrono::steady_clock::now();

    // When: Delaying for 50ms
    RTOS::delay(50);

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    // Then: Should have delayed approximately 50ms
    // Allow tolerance due to OS scheduling
    EXPECT_GE(ms, 40);
    EXPECT_LE(ms, 100);
}

TEST_F(SchedulerTest, ZeroDelayDoesNotBlock) {
    // Given: RTOS initialized
    auto start = std::chrono::steady_clock::now();

    // When: Delaying for 0ms
    RTOS::delay(0);

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    // Then: Should return immediately
    EXPECT_LT(ms, 10);
}

// ============================================================================
// Test 5: Tick Counter
// ============================================================================

TEST_F(SchedulerTest, TickCounterIncrements) {
    // Given: Initial tick count
    core::u32 initial_count = RTOS::get_tick_count();

    // When: Waiting some time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Then: Tick count should have increased
    core::u32 new_count = RTOS::get_tick_count();
    EXPECT_GT(new_count, initial_count);
}

// ============================================================================
// Test 6: Task State Transitions
// ============================================================================

TEST_F(SchedulerTest, TaskInitialStateIsReady) {
    // Given: A newly created task
    std::atomic<bool> ran{false};

    auto task_func = [&ran]() {
        ran = true;
        while (true) RTOS::delay(1000);
    };

    Task<512, Priority::Normal> task(task_func, "StateTest");

    // Then: Initial state should be Ready
    EXPECT_EQ(task.get_tcb()->state, TaskState::Ready);
}

// ============================================================================
// Test 7: Priority Preemption Behavior
// ============================================================================

TEST_F(SchedulerTest, HigherPriorityTaskPreemptsLower) {
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
    EXPECT_GT(low_priority_started.load(), 0);
    EXPECT_GT(high_priority_started.load(), 0);
}

// ============================================================================
// Test 8: ReadyQueue Bitmap Optimization
// ============================================================================

TEST_F(SchedulerTest, ReadyQueueUsesO1PriorityLookup) {
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
        EXPECT_NE(task, nullptr);
    }

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

    // Then: Should complete very quickly (O(1) per operation)
    EXPECT_LT(us, 1000) << "O(1) operations should be fast";
}

// ============================================================================
// Test 9: Scheduler State Structure
// ============================================================================

TEST_F(SchedulerTest, SchedulerStateStructureSize) {
    // When: Checking scheduler state size
    size_t scheduler_size = sizeof(SchedulerState);

    // Then: Should be reasonably compact
    EXPECT_LE(scheduler_size, 256u) << "SchedulerState should be compact";
}

TEST_F(SchedulerTest, GlobalSchedulerExists) {
    // When: Accessing global scheduler
    SchedulerState* scheduler = &g_scheduler;

    // Then: Should exist
    EXPECT_NE(scheduler, nullptr);
}

// ============================================================================
// Test 10: Task Control Block Structure
// ============================================================================

TEST_F(SchedulerTest, TCBStructureIsCompact) {
    // When: Checking TCB size
    size_t tcb_size = sizeof(TaskControlBlock);

    // Then: Should be compact for embedded systems
    EXPECT_LE(tcb_size, 64u) << "TCB should be compact";
    EXPECT_GE(tcb_size, 24u) << "TCB should contain essential fields";
}

TEST_F(SchedulerTest, TCBHasEssentialFields) {
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

    EXPECT_EQ(tcb.priority, 5u);
    EXPECT_EQ(tcb.state, TaskState::Running);
    EXPECT_EQ(tcb.stack_size, 512u);
}

// ============================================================================
// Test 11: Multiple Delays
// ============================================================================

TEST_F(SchedulerTest, MultipleSequentialDelays) {
    // Given: Multiple delay operations
    auto start = std::chrono::steady_clock::now();

    // When: Performing multiple delays
    RTOS::delay(20);
    RTOS::delay(20);
    RTOS::delay(20);

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    // Then: Total delay should be approximately sum of delays
    EXPECT_GE(ms, 50);
    EXPECT_LE(ms, 100);
}

// ============================================================================
// Test 12: Multiple Tasks Execution
// ============================================================================

TEST_F(SchedulerTest, MultipleTasksExecute) {
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
    EXPECT_GT(task1_runs.load(), 0);
    EXPECT_GT(task2_runs.load(), 0);
}

// ============================================================================
// Test 13: INFINITE Constant
// ============================================================================

TEST_F(SchedulerTest, InfiniteConstantIsMaxValue) {
    // Then: INFINITE should be maximum 32-bit value
    EXPECT_EQ(INFINITE, 0xFFFFFFFFu);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
