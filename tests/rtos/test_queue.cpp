/// Unit tests for RTOS Queue

#include "tests/rtos/test_framework.hpp"
#include "rtos/queue.hpp"
#include "rtos/rtos.hpp"

using namespace alloy;
using namespace alloy::rtos;
using namespace alloy::rtos::test;

TEST_SUITE(Queue) {
    TEST_CASE(queue_construction) {
        Queue<core::u32, 8> queue;

        TEST_ASSERT(queue.is_empty());
        TEST_ASSERT(!queue.is_full());
        TEST_ASSERT_EQUAL(queue.available(), 0);
        TEST_ASSERT_EQUAL(queue.capacity(), 8);
    }

    TEST_CASE(queue_send_receive_basic) {
        Queue<core::u32, 8> queue;

        // Send a value
        auto send_result = queue.try_send(42);
        TEST_ASSERT_OK(send_result);

        // Queue should have one item
        TEST_ASSERT(!queue.is_empty());
        TEST_ASSERT_EQUAL(queue.available(), 1);

        // Receive the value
        auto recv_result = queue.try_receive();
        TEST_ASSERT_OK(recv_result);
        TEST_ASSERT_EQUAL(recv_result.unwrap(), 42);

        // Queue should be empty again
        TEST_ASSERT(queue.is_empty());
        TEST_ASSERT_EQUAL(queue.available(), 0);
    }

    TEST_CASE(queue_fifo_order) {
        Queue<core::u32, 8> queue;

        // Send multiple values
        for (core::u32 i = 1; i <= 5; i++) {
            TEST_ASSERT_OK(queue.try_send(i * 10));
        }

        TEST_ASSERT_EQUAL(queue.available(), 5);

        // Receive in FIFO order
        for (core::u32 i = 1; i <= 5; i++) {
            auto result = queue.try_receive();
            TEST_ASSERT_OK(result);
            TEST_ASSERT_EQUAL(result.unwrap(), i * 10);
        }

        TEST_ASSERT(queue.is_empty());
    }

    TEST_CASE(queue_full_detection) {
        Queue<core::u32, 4> queue;

        // Fill the queue
        for (core::u32 i = 0; i < 4; i++) {
            TEST_ASSERT_OK(queue.try_send(i));
        }

        TEST_ASSERT(queue.is_full());
        TEST_ASSERT_EQUAL(queue.available(), 4);

        // Try to send to full queue
        auto result = queue.try_send(999);
        TEST_ASSERT_ERR(result);
        TEST_ASSERT_EQUAL(result.unwrap_err(), RTOSError::QueueFull);
    }

    TEST_CASE(queue_empty_detection) {
        Queue<core::u32, 8> queue;

        // Try to receive from empty queue
        auto result = queue.try_receive();
        TEST_ASSERT_ERR(result);
        TEST_ASSERT_EQUAL(result.unwrap_err(), RTOSError::QueueEmpty);
    }

    TEST_CASE(queue_capacity_boundary) {
        Queue<core::u32, 2> queue;

        // Send to capacity
        TEST_ASSERT_OK(queue.try_send(1));
        TEST_ASSERT_OK(queue.try_send(2));
        TEST_ASSERT(queue.is_full());

        // Can't send more
        TEST_ASSERT_ERR(queue.try_send(3));

        // Receive one
        auto result = queue.try_receive();
        TEST_ASSERT_OK(result);
        TEST_ASSERT_EQUAL(result.unwrap(), 1);

        // Now we can send again
        TEST_ASSERT_OK(queue.try_send(3));
    }

    TEST_CASE(queue_struct_message) {
        struct Message {
            core::u32 id;
            core::u16 value;
            core::u8 flags;
        };

        static_assert(IPCMessage<Message>, "Message should satisfy IPCMessage");

        Queue<Message, 4> queue;

        // Send structured message
        Message msg{.id = 123, .value = 456, .flags = 0xAB};
        TEST_ASSERT_OK(queue.try_send(msg));

        // Receive and verify
        auto result = queue.try_receive();
        TEST_ASSERT_OK(result);

        Message received = result.unwrap();
        TEST_ASSERT_EQUAL(received.id, 123);
        TEST_ASSERT_EQUAL(received.value, 456);
        TEST_ASSERT_EQUAL(received.flags, 0xAB);
    }

    TEST_CASE(queue_wraparound) {
        Queue<core::u32, 4> queue;

        // Fill queue
        for (core::u32 i = 0; i < 4; i++) {
            TEST_ASSERT_OK(queue.try_send(i));
        }

        // Receive 2
        for (core::u32 i = 0; i < 2; i++) {
            auto result = queue.try_receive();
            TEST_ASSERT_OK(result);
            TEST_ASSERT_EQUAL(result.unwrap(), i);
        }

        // Send 2 more (wraparound)
        TEST_ASSERT_OK(queue.try_send(100));
        TEST_ASSERT_OK(queue.try_send(101));

        TEST_ASSERT(queue.is_full());

        // Verify order
        TEST_ASSERT_EQUAL(queue.try_receive().unwrap(), 2);
        TEST_ASSERT_EQUAL(queue.try_receive().unwrap(), 3);
        TEST_ASSERT_EQUAL(queue.try_receive().unwrap(), 100);
        TEST_ASSERT_EQUAL(queue.try_receive().unwrap(), 101);

        TEST_ASSERT(queue.is_empty());
    }

    TEST_CASE(queue_performance_send_receive) {
        Queue<core::u32, 16> queue;

        PerfTimer timer;

        // Measure 100 send/receive cycles
        for (core::u32 i = 0; i < 100; i++) {
            queue.try_send(i).unwrap();
            queue.try_receive().unwrap();
        }

        core::u32 elapsed = timer.elapsed_us();
        core::u32 avg_per_cycle = elapsed / 100;

        printf("\n    Average send+receive: %lu µs\n", avg_per_cycle);

        // Should be fast (<10µs per cycle)
        TEST_ASSERT_IN_RANGE(avg_per_cycle, 0, 10);
    }
}
