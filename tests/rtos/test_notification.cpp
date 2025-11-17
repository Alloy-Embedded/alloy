/// Unit tests for RTOS TaskNotification

#include "tests/rtos/test_framework.hpp"
#include "rtos/task_notification.hpp"
#include "rtos/rtos.hpp"

using namespace alloy;
using namespace alloy::rtos;
using namespace alloy::rtos::test;

// Test tasks
static TaskControlBlock test_task_tcb;
static core::u32 notification_received = 0;

void notification_receiver_task() {
    while (1) {
        auto result = TaskNotification::wait(1000);
        if (result.is_ok()) {
            notification_received = result.unwrap();
        }
        RTOS::yield();
    }
}

TEST_SUITE(TaskNotification) {
    TEST_CASE(notification_state_initial) {
        // Note: This test assumes notification state is accessible
        // In practice, we test through the API

        TEST_ASSERT(!TaskNotification::is_pending());
        TEST_ASSERT_EQUAL(TaskNotification::peek(), 0);
    }

    TEST_CASE(notification_set_bits) {
        TaskNotification::clear().unwrap();

        // Set bit 0
        auto r1 = TaskNotification::notify(
            &test_task_tcb,
            0x01,
            NotifyAction::SetBits
        );
        TEST_ASSERT_OK(r1);
        TEST_ASSERT_EQUAL(r1.unwrap(), 0);  // Previous value was 0

        // Set bit 1
        auto r2 = TaskNotification::notify(
            &test_task_tcb,
            0x02,
            NotifyAction::SetBits
        );
        TEST_ASSERT_OK(r2);
        TEST_ASSERT_EQUAL(r2.unwrap(), 0x01);  // Previous value was 0x01

        // Value should be 0x03 (bits 0 and 1)
        TEST_ASSERT_EQUAL(TaskNotification::peek(), 0x03);

        TaskNotification::clear();
    }

    TEST_CASE(notification_increment) {
        TaskNotification::clear().unwrap();

        // Increment by 1
        auto r1 = TaskNotification::notify(
            &test_task_tcb,
            1,
            NotifyAction::Increment
        );
        TEST_ASSERT_OK(r1);
        TEST_ASSERT_EQUAL(r1.unwrap(), 0);

        // Increment by 5
        auto r2 = TaskNotification::notify(
            &test_task_tcb,
            5,
            NotifyAction::Increment
        );
        TEST_ASSERT_OK(r2);
        TEST_ASSERT_EQUAL(r2.unwrap(), 1);

        // Value should be 6
        TEST_ASSERT_EQUAL(TaskNotification::peek(), 6);

        TaskNotification::clear();
    }

    TEST_CASE(notification_overwrite) {
        TaskNotification::clear().unwrap();

        // Set initial value
        TaskNotification::notify(
            &test_task_tcb,
            100,
            NotifyAction::Overwrite
        ).unwrap();

        TEST_ASSERT_EQUAL(TaskNotification::peek(), 100);

        // Overwrite with new value
        auto r = TaskNotification::notify(
            &test_task_tcb,
            200,
            NotifyAction::Overwrite
        );
        TEST_ASSERT_OK(r);
        TEST_ASSERT_EQUAL(r.unwrap(), 100);  // Previous value

        TEST_ASSERT_EQUAL(TaskNotification::peek(), 200);

        TaskNotification::clear();
    }

    TEST_CASE(notification_overwrite_if_empty) {
        TaskNotification::clear().unwrap();

        // Should succeed on empty
        auto r1 = TaskNotification::notify(
            &test_task_tcb,
            42,
            NotifyAction::OverwriteIfEmpty
        );
        TEST_ASSERT_OK(r1);

        // Should fail if already pending
        auto r2 = TaskNotification::notify(
            &test_task_tcb,
            99,
            NotifyAction::OverwriteIfEmpty
        );
        TEST_ASSERT_ERR(r2);
        TEST_ASSERT_EQUAL(r2.unwrap_err(), RTOSError::QueueFull);

        // Value should still be 42
        TEST_ASSERT_EQUAL(TaskNotification::peek(), 42);

        TaskNotification::clear();
    }

    TEST_CASE(notification_clear_modes) {
        TaskNotification::clear().unwrap();

        // Set a value
        TaskNotification::notify(
            &test_task_tcb,
            0xFF,
            NotifyAction::Overwrite
        ).unwrap();

        TEST_ASSERT_EQUAL(TaskNotification::peek(), 0xFF);

        // Clear
        auto result = TaskNotification::clear();
        TEST_ASSERT_OK(result);
        TEST_ASSERT_EQUAL(result.unwrap(), 0xFF);  // Previous value

        TEST_ASSERT_EQUAL(TaskNotification::peek(), 0);
        TEST_ASSERT(!TaskNotification::is_pending());
    }

    TEST_CASE(notification_try_wait_empty) {
        TaskNotification::clear().unwrap();

        // Try to wait on empty notification
        auto result = TaskNotification::try_wait();
        TEST_ASSERT_ERR(result);
        TEST_ASSERT_EQUAL(result.unwrap_err(), RTOSError::QueueEmpty);
    }

    TEST_CASE(notification_try_wait_success) {
        TaskNotification::clear().unwrap();

        // Set notification
        TaskNotification::notify(
            &test_task_tcb,
            123,
            NotifyAction::Overwrite
        ).unwrap();

        // Try wait should succeed
        auto result = TaskNotification::try_wait();
        TEST_ASSERT_OK(result);
        TEST_ASSERT_EQUAL(result.unwrap(), 123);

        // Should be cleared (default mode)
        TEST_ASSERT_EQUAL(TaskNotification::peek(), 0);
    }

    TEST_CASE(notification_pending_count) {
        TaskNotification::clear().unwrap();

        TEST_ASSERT(!TaskNotification::is_pending());

        // Send notification
        TaskNotification::notify(
            &test_task_tcb,
            42,
            NotifyAction::Overwrite
        ).unwrap();

        TEST_ASSERT(TaskNotification::is_pending());

        // Clear
        TaskNotification::clear();

        TEST_ASSERT(!TaskNotification::is_pending());
    }

    TEST_CASE(notification_performance_notify) {
        TaskNotification::clear().unwrap();

        PerfTimer timer;

        // Measure 1000 notify operations
        for (core::u32 i = 0; i < 1000; i++) {
            TaskNotification::notify(
                &test_task_tcb,
                i,
                NotifyAction::Overwrite
            ).unwrap();
            TaskNotification::clear();
        }

        core::u32 elapsed = timer.elapsed_us();
        core::u32 avg = elapsed / 1000;

        printf("\n    Average notify: %lu µs\n", avg);

        // Should be very fast (<1µs)
        TEST_ASSERT_IN_RANGE(avg, 0, 1);
    }

    TEST_CASE(notification_event_flags) {
        TaskNotification::clear().unwrap();

        // Simulate multiple event sources
        TaskNotification::notify(
            &test_task_tcb,
            0x01,  // Event 1
            NotifyAction::SetBits
        ).unwrap();

        TaskNotification::notify(
            &test_task_tcb,
            0x04,  // Event 3
            NotifyAction::SetBits
        ).unwrap();

        TaskNotification::notify(
            &test_task_tcb,
            0x10,  // Event 5
            NotifyAction::SetBits
        ).unwrap();

        // All flags should be set
        core::u32 flags = TaskNotification::peek();
        TEST_ASSERT_EQUAL(flags, 0x15);  // 0x01 | 0x04 | 0x10

        // Check individual flags
        TEST_ASSERT(flags & 0x01);  // Event 1 set
        TEST_ASSERT(!(flags & 0x02));  // Event 2 not set
        TEST_ASSERT(flags & 0x04);  // Event 3 set
        TEST_ASSERT(!(flags & 0x08));  // Event 4 not set
        TEST_ASSERT(flags & 0x10);  // Event 5 set

        TaskNotification::clear();
    }

    TEST_CASE(notification_counting_semaphore) {
        TaskNotification::clear().unwrap();

        // Use as counting semaphore
        for (int i = 0; i < 5; i++) {
            TaskNotification::notify(
                &test_task_tcb,
                1,  // Increment count
                NotifyAction::Increment
            ).unwrap();
        }

        // Should have count of 5
        TEST_ASSERT_EQUAL(TaskNotification::peek(), 5);

        // "Take" semaphore 3 times
        for (int i = 0; i < 3; i++) {
            auto result = TaskNotification::try_wait(NotifyClearMode::NoClear);
            TEST_ASSERT_OK(result);

            core::u32 count = result.unwrap();
            TEST_ASSERT(count > 0);

            // Decrement
            TaskNotification::notify(
                &test_task_tcb,
                static_cast<core::u32>(-1),  // Decrement
                NotifyAction::Increment
            ).unwrap();
        }

        // Should have count of 2
        TEST_ASSERT_EQUAL(TaskNotification::peek(), 2);

        TaskNotification::clear();
    }
}
