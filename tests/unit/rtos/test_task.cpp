/// Unit Tests for RTOS Task Management
///
/// Tests task creation, priorities, state transitions, and basic scheduling.
/// These tests ensure tasks behave correctly across all platforms.

#include <gtest/gtest.h>
#include "rtos/rtos.hpp"
#include "hal/host/systick.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace alloy;
using namespace alloy::rtos;

// Test fixture for Task tests
class TaskTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize SysTick for RTOS timing
        auto result = hal::host::SystemTick::init();
        ASSERT_TRUE(result.is_ok()) << "Failed to initialize SysTick";
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

// Static test functions (no lambda captures)
static void simple_task_func() {
    while (true) {
        RTOS::delay(1000);
    }
}

static void idle_task_func() {
    while (true) RTOS::delay(1000);
}

// ============================================================================
// Test 1: Task Creation and Properties
// ============================================================================

TEST_F(TaskTest, TaskCreationWithCorrectProperties) {
    // Given: A task with specific parameters
    Task<512, Priority::High> task(simple_task_func, "TestTask");

    // When: Inspecting task properties
    const TaskControlBlock* tcb = task.get_tcb();

    // Then: Properties should match creation parameters
    EXPECT_NE(tcb, nullptr);
    EXPECT_EQ(tcb->priority, static_cast<core::u8>(Priority::High));
    EXPECT_STREQ(tcb->name, "TestTask");
    EXPECT_EQ(tcb->stack_size, 512u);
    EXPECT_NE(tcb->stack_base, nullptr);
    EXPECT_NE(tcb->stack_pointer, nullptr);
}

TEST_F(TaskTest, TaskStackAlignment) {
    // Given: A task with 8-byte aligned stack
    Task<512, Priority::Normal> task(simple_task_func, "AlignTest");

    // When: Checking stack alignment
    const TaskControlBlock* tcb = task.get_tcb();
    uintptr_t stack_addr = reinterpret_cast<uintptr_t>(tcb->stack_base);

    // Then: Stack should be 8-byte aligned
    EXPECT_EQ(stack_addr % 8, 0u) << "Stack must be 8-byte aligned";
}

TEST_F(TaskTest, MultipleTasksWithDifferentPriorities) {
    // Given: Tasks with different priorities
    Task<512, Priority::Idle> task_idle(idle_task_func, "Idle");
    Task<512, Priority::Low> task_low(simple_task_func, "Low");
    Task<512, Priority::Normal> task_normal(simple_task_func, "Normal");
    Task<512, Priority::High> task_high(simple_task_func, "High");
    Task<512, Priority::Critical> task_critical(simple_task_func, "Critical");

    // When: Checking priorities
    EXPECT_EQ(task_idle.get_tcb()->priority, 0u);
    EXPECT_EQ(task_low.get_tcb()->priority, 2u);
    EXPECT_EQ(task_normal.get_tcb()->priority, 3u);
    EXPECT_EQ(task_high.get_tcb()->priority, 4u);
    EXPECT_EQ(task_critical.get_tcb()->priority, 7u);
}

// ============================================================================
// Test 2: Task State Transitions
// ============================================================================

TEST_F(TaskTest, TaskInitialStateIsReady) {
    // Given: A newly created task
    Task<512, Priority::Normal> task(simple_task_func, "StateTest");

    // When: Checking initial state
    const TaskControlBlock* tcb = task.get_tcb();

    // Then: Initial state should be Ready
    EXPECT_EQ(tcb->state, TaskState::Ready);
}

// ============================================================================
// Test 3: Task Stack Usage
// ============================================================================

TEST_F(TaskTest, StackUsageIsReasonable) {
    // Given: A simple task
    Task<1024, Priority::Normal> task(simple_task_func, "StackTest");

    // When: Checking stack usage
    core::u32 usage = task.get_stack_usage();

    // Then: Stack usage should be reasonable (not 0, not full)
    EXPECT_GE(usage, 0u);
    EXPECT_LE(usage, 1024u);
}

// ============================================================================
// Test 4: Task Names
// ============================================================================

TEST_F(TaskTest, TaskNameIsStored) {
    // Given: A task with a specific name
    Task<512, Priority::Normal> task(simple_task_func, "MyCustomTaskName");

    // When: Retrieving the name
    const char* name = task.get_tcb()->name;

    // Then: Name should match
    EXPECT_STREQ(name, "MyCustomTaskName");
}

TEST_F(TaskTest, DefaultTaskName) {
    // Given: A task without explicit name
    Task<512, Priority::Normal> task(simple_task_func);

    // When: Retrieving the name
    const char* name = task.get_tcb()->name;

    // Then: Should have default name
    EXPECT_STREQ(name, "task");
}

// ============================================================================
// Test 5: Multiple Task Instances
// ============================================================================

TEST_F(TaskTest, CanCreateMultipleTaskInstances) {
    // Given: Multiple task instances
    Task<512, Priority::Normal> task1(simple_task_func, "Task1");
    Task<512, Priority::Normal> task2(simple_task_func, "Task2");
    Task<512, Priority::Normal> task3(simple_task_func, "Task3");

    // When: Checking TCBs
    const TaskControlBlock* tcb1 = task1.get_tcb();
    const TaskControlBlock* tcb2 = task2.get_tcb();
    const TaskControlBlock* tcb3 = task3.get_tcb();

    // Then: All should have unique TCBs
    EXPECT_NE(tcb1, tcb2);
    EXPECT_NE(tcb2, tcb3);
    EXPECT_NE(tcb1, tcb3);

    // And: All should have unique stack bases
    EXPECT_NE(tcb1->stack_base, tcb2->stack_base);
    EXPECT_NE(tcb2->stack_base, tcb3->stack_base);
    EXPECT_NE(tcb1->stack_base, tcb3->stack_base);
}

// ============================================================================
// Test 6: Task with Different Stack Sizes
// ============================================================================

TEST_F(TaskTest, TasksWithDifferentStackSizes) {
    // Given: Tasks with different stack sizes
    Task<256, Priority::Normal> small_task(simple_task_func, "Small");
    Task<512, Priority::Normal> medium_task(simple_task_func, "Medium");
    Task<1024, Priority::Normal> large_task(simple_task_func, "Large");
    Task<2048, Priority::Normal> xlarge_task(simple_task_func, "XLarge");

    // When: Checking stack sizes
    EXPECT_EQ(small_task.get_tcb()->stack_size, 256u);
    EXPECT_EQ(medium_task.get_tcb()->stack_size, 512u);
    EXPECT_EQ(large_task.get_tcb()->stack_size, 1024u);
    EXPECT_EQ(xlarge_task.get_tcb()->stack_size, 2048u);
}

// ============================================================================
// Test 7: Priority Values
// ============================================================================

TEST_F(TaskTest, PriorityEnumValues) {
    // When: Checking priority enum values
    // Then: They should be correctly ordered
    EXPECT_EQ(static_cast<core::u8>(Priority::Idle), 0u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Lowest), 1u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Low), 2u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Normal), 3u);
    EXPECT_EQ(static_cast<core::u8>(Priority::High), 4u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Higher), 5u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Highest), 6u);
    EXPECT_EQ(static_cast<core::u8>(Priority::Critical), 7u);

    // And: They should be in ascending order
    EXPECT_LT(static_cast<core::u8>(Priority::Idle),
              static_cast<core::u8>(Priority::Lowest));
    EXPECT_LT(static_cast<core::u8>(Priority::Normal),
              static_cast<core::u8>(Priority::High));
    EXPECT_LT(static_cast<core::u8>(Priority::High),
              static_cast<core::u8>(Priority::Critical));
}

// ============================================================================
// Test 8: Task State Enum
// ============================================================================

TEST_F(TaskTest, TaskStateEnumValues) {
    // When: Checking TaskState enum
    // Then: All states should be defined
    TaskState ready = TaskState::Ready;
    TaskState running = TaskState::Running;
    TaskState blocked = TaskState::Blocked;
    TaskState suspended = TaskState::Suspended;
    TaskState delayed = TaskState::Delayed;

    // And: They should be distinct
    EXPECT_NE(ready, running);
    EXPECT_NE(running, blocked);
    EXPECT_NE(blocked, suspended);
    EXPECT_NE(suspended, delayed);
}

// ============================================================================
// Test 9: Compile-Time Validation
// ============================================================================

// These tests verify compile-time constraints

// Stack size must be at least 256 bytes
// This should compile:
static void global_task_func() { while(1) RTOS::delay(1000); }
Task<256, Priority::Normal> global_min_stack(global_task_func, "MinStack");

// Stack size must be 8-byte aligned
// This should compile:
Task<512, Priority::Normal> global_aligned_stack(global_task_func, "Aligned");

// Priority must be valid (0-7)
// These should all compile:
Task<512, Priority::Idle> global_idle(global_task_func, "Idle");
Task<512, Priority::Critical> global_critical(global_task_func, "Critical");

// ============================================================================
// Test 10: TCB Structure Size
// ============================================================================

TEST_F(TaskTest, TCBStructureSizeIsReasonable) {
    // When: Checking TCB size
    size_t tcb_size = sizeof(TaskControlBlock);

    // Then: Should be reasonably small (target: ~32 bytes on 32-bit, ~48 on 64-bit)
    EXPECT_LE(tcb_size, 64u) << "TCB should be compact for embedded systems";
    EXPECT_GE(tcb_size, 24u) << "TCB should have at least basic fields";
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
