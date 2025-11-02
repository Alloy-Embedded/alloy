/// Unit Tests for RTOS Queue (Message Queue)
///
/// Tests type-safe FIFO message queue for inter-task communication.
/// Covers send/receive, blocking, timeouts, full/empty conditions, and edge cases.

#include <gtest/gtest.h>
#include "rtos/queue.hpp"
#include "hal/host/systick.hpp"
#include <thread>
#include <atomic>

using namespace alloy;
using namespace alloy::rtos;

// Test message types
struct SimpleMessage {
    core::u32 id;
    core::i16 value;
};

struct LargeMessage {
    core::u32 timestamp;
    core::u8 data[64];
    core::u16 checksum;
};

// Test fixture
class QueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto result = hal::host::SystemTick::init();
        ASSERT_TRUE(result.is_ok());
    }
};

// ============================================================================
// Test 1: Queue Creation and Capacity
// ============================================================================

TEST_F(QueueTest, QueueCreation) {
    // Given: A queue with capacity 8
    Queue<SimpleMessage, 8> queue;

    // Then: Should be empty initially
    EXPECT_EQ(queue.count(), 0u);
    EXPECT_EQ(queue.available(), 8u);
    EXPECT_EQ(queue.capacity(), 8u);
    EXPECT_TRUE(queue.is_empty());
    EXPECT_FALSE(queue.is_full());
}

TEST_F(QueueTest, DifferentCapacities) {
    // Given: Queues with different capacities (must be power of 2)
    Queue<core::u32, 2> tiny;
    Queue<core::u32, 4> small;
    Queue<core::u32, 16> medium;
    Queue<core::u32, 64> large;
    Queue<core::u32, 256> xlarge;

    // Then: Capacities should match
    EXPECT_EQ(tiny.capacity(), 2u);
    EXPECT_EQ(small.capacity(), 4u);
    EXPECT_EQ(medium.capacity(), 16u);
    EXPECT_EQ(large.capacity(), 64u);
    EXPECT_EQ(xlarge.capacity(), 256u);
}

// ============================================================================
// Test 2: Basic Send and Receive
// ============================================================================

TEST_F(QueueTest, SendAndReceiveSingleMessage) {
    // Given: An empty queue
    Queue<SimpleMessage, 8> queue;

    SimpleMessage msg_in = {42, 100};

    // When: Sending a message
    bool sent = queue.try_send(msg_in);

    // Then: Send should succeed
    EXPECT_TRUE(sent);
    EXPECT_EQ(queue.count(), 1u);
    EXPECT_FALSE(queue.is_empty());

    // When: Receiving the message
    SimpleMessage msg_out;
    bool received = queue.try_receive(msg_out);

    // Then: Receive should succeed with correct data
    EXPECT_TRUE(received);
    EXPECT_EQ(msg_out.id, 42u);
    EXPECT_EQ(msg_out.value, 100);
    EXPECT_TRUE(queue.is_empty());
}

TEST_F(QueueTest, SendAndReceiveMultipleMessages) {
    // Given: An empty queue
    Queue<core::u32, 8> queue;

    // When: Sending multiple messages
    for (core::u32 i = 0; i < 5; i++) {
        EXPECT_TRUE(queue.try_send(i * 10));
    }

    // Then: Count should be correct
    EXPECT_EQ(queue.count(), 5u);

    // When: Receiving messages
    for (core::u32 i = 0; i < 5; i++) {
        core::u32 value;
        EXPECT_TRUE(queue.try_receive(value));
        EXPECT_EQ(value, i * 10);
    }

    // Then: Queue should be empty
    EXPECT_TRUE(queue.is_empty());
}

// ============================================================================
// Test 3: FIFO Order
// ============================================================================

TEST_F(QueueTest, FIFOOrdering) {
    // Given: A queue with messages
    Queue<core::u32, 16> queue;

    const core::u32 NUM_MESSAGES = 10;
    for (core::u32 i = 1; i <= NUM_MESSAGES; i++) {
        queue.try_send(i);
    }

    // When: Receiving messages
    // Then: Should receive in FIFO order (first in, first out)
    for (core::u32 i = 1; i <= NUM_MESSAGES; i++) {
        core::u32 value;
        EXPECT_TRUE(queue.try_receive(value));
        EXPECT_EQ(value, i) << "Messages should be received in FIFO order";
    }
}

// ============================================================================
// Test 4: Full Queue Behavior
// ============================================================================

TEST_F(QueueTest, QueueFullDetection) {
    // Given: A small queue
    Queue<core::u32, 4> queue;

    // When: Filling the queue
    for (core::u32 i = 0; i < 4; i++) {
        EXPECT_TRUE(queue.try_send(i));
    }

    // Then: Should be full
    EXPECT_TRUE(queue.is_full());
    EXPECT_EQ(queue.count(), 4u);
    EXPECT_EQ(queue.available(), 0u);

    // And: Further sends should fail
    EXPECT_FALSE(queue.try_send(999));
}

TEST_F(QueueTest, SendToFullQueueFails) {
    // Given: A full queue
    Queue<core::u32, 2> queue;
    queue.try_send(1);
    queue.try_send(2);

    // When: Trying to send to full queue
    bool sent = queue.try_send(3);

    // Then: Should fail
    EXPECT_FALSE(sent);
    EXPECT_EQ(queue.count(), 2u);
}

// ============================================================================
// Test 5: Empty Queue Behavior
// ============================================================================

TEST_F(QueueTest, ReceiveFromEmptyQueueFails) {
    // Given: An empty queue
    Queue<core::u32, 8> queue;

    // When: Trying to receive from empty queue
    core::u32 value;
    bool received = queue.try_receive(value);

    // Then: Should fail
    EXPECT_FALSE(received);
}

// ============================================================================
// Test 6: Circular Buffer Wraparound
// ============================================================================

TEST_F(QueueTest, CircularBufferWraparound) {
    // Given: A queue
    Queue<core::u32, 4> queue;

    // When: Repeatedly send and receive (causes wraparound)
    for (int cycle = 0; cycle < 20; cycle++) {
        // Fill queue
        for (core::u32 i = 0; i < 4; i++) {
            EXPECT_TRUE(queue.try_send(cycle * 10 + i));
        }

        // Empty queue
        for (core::u32 i = 0; i < 4; i++) {
            core::u32 value;
            EXPECT_TRUE(queue.try_receive(value));
            EXPECT_EQ(value, cycle * 10 + i);
        }

        EXPECT_TRUE(queue.is_empty());
    }
}

// ============================================================================
// Test 7: Peek Functionality
// ============================================================================

TEST_F(QueueTest, PeekDoesNotRemoveMessage) {
    // Given: A queue with a message
    Queue<core::u32, 8> queue;
    queue.try_send(42);

    // When: Peeking multiple times
    core::u32 value1, value2, value3;
    EXPECT_TRUE(queue.try_peek(value1));
    EXPECT_TRUE(queue.try_peek(value2));
    EXPECT_TRUE(queue.try_peek(value3));

    // Then: All peeks should return same value
    EXPECT_EQ(value1, 42u);
    EXPECT_EQ(value2, 42u);
    EXPECT_EQ(value3, 42u);

    // And: Message should still be in queue
    EXPECT_EQ(queue.count(), 1u);

    // When: Actually receiving
    core::u32 value_out;
    EXPECT_TRUE(queue.try_receive(value_out));

    // Then: Should get the same value and queue becomes empty
    EXPECT_EQ(value_out, 42u);
    EXPECT_TRUE(queue.is_empty());
}

// ============================================================================
// Test 8: Large Message Types
// ============================================================================

TEST_F(QueueTest, LargeMessageTypes) {
    // Given: A queue for large messages
    Queue<LargeMessage, 4> queue;

    LargeMessage msg_in;
    msg_in.timestamp = 12345;
    for (int i = 0; i < 64; i++) {
        msg_in.data[i] = static_cast<core::u8>(i);
    }
    msg_in.checksum = 0xABCD;

    // When: Sending and receiving
    EXPECT_TRUE(queue.try_send(msg_in));

    LargeMessage msg_out;
    EXPECT_TRUE(queue.try_receive(msg_out));

    // Then: Data should match exactly
    EXPECT_EQ(msg_out.timestamp, 12345u);
    EXPECT_EQ(msg_out.checksum, 0xABCDu);
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(msg_out.data[i], static_cast<core::u8>(i));
    }
}

// ============================================================================
// Test 9: Available Space
// ============================================================================

TEST_F(QueueTest, AvailableSpaceTracking) {
    // Given: A queue
    Queue<core::u32, 8> queue;

    // Then: Initially all space available
    EXPECT_EQ(queue.available(), 8u);

    // When: Adding messages
    queue.try_send(1);
    EXPECT_EQ(queue.available(), 7u);

    queue.try_send(2);
    EXPECT_EQ(queue.available(), 6u);

    queue.try_send(3);
    EXPECT_EQ(queue.available(), 5u);

    // When: Removing a message
    core::u32 value;
    queue.try_receive(value);
    EXPECT_EQ(queue.available(), 6u);
}

// ============================================================================
// Test 10: Count Tracking
// ============================================================================

TEST_F(QueueTest, CountTracking) {
    // Given: A queue
    Queue<core::u32, 16> queue;

    // Then: Initially count is 0
    EXPECT_EQ(queue.count(), 0u);

    // When: Adding messages
    for (core::u32 i = 0; i < 10; i++) {
        queue.try_send(i);
        EXPECT_EQ(queue.count(), i + 1);
    }

    // When: Removing messages
    for (core::u32 i = 0; i < 10; i++) {
        core::u32 value;
        queue.try_receive(value);
        EXPECT_EQ(queue.count(), 10 - i - 1);
    }

    EXPECT_EQ(queue.count(), 0u);
}

// ============================================================================
// Test 11: Type Safety
// ============================================================================

struct TypeA { int x; };
struct TypeB { float y; };

TEST_F(QueueTest, TypeSafety) {
    // Given: Queues of different types
    Queue<TypeA, 4> queue_a;
    Queue<TypeB, 4> queue_b;

    TypeA msg_a = {42};
    TypeB msg_b = {3.14f};

    // When: Sending type-specific messages
    EXPECT_TRUE(queue_a.try_send(msg_a));
    EXPECT_TRUE(queue_b.try_send(msg_b));

    // Then: Each queue should maintain its type
    TypeA out_a;
    TypeB out_b;

    EXPECT_TRUE(queue_a.try_receive(out_a));
    EXPECT_TRUE(queue_b.try_receive(out_b));

    EXPECT_EQ(out_a.x, 42);
    EXPECT_FLOAT_EQ(out_b.y, 3.14f);
}

// ============================================================================
// Test 12: Reset/Clear
// ============================================================================

TEST_F(QueueTest, ResetClearsQueue) {
    // Given: A queue with messages
    Queue<core::u32, 8> queue;
    for (core::u32 i = 0; i < 5; i++) {
        queue.try_send(i);
    }

    EXPECT_EQ(queue.count(), 5u);

    // When: Resetting the queue
    queue.reset();

    // Then: Should be empty
    EXPECT_EQ(queue.count(), 0u);
    EXPECT_TRUE(queue.is_empty());
    EXPECT_EQ(queue.available(), 8u);
}

// ============================================================================
// Test 13: Edge Case - Single Element Queue
// ============================================================================

TEST_F(QueueTest, SingleElementQueue) {
    // Given: A queue with capacity 2 (minimum)
    Queue<core::u32, 2> queue;

    // When: Repeatedly using the queue
    for (int i = 0; i < 100; i++) {
        EXPECT_TRUE(queue.try_send(i));
        EXPECT_EQ(queue.count(), 1u);

        core::u32 value;
        EXPECT_TRUE(queue.try_receive(value));
        EXPECT_EQ(value, static_cast<core::u32>(i));
        EXPECT_TRUE(queue.is_empty());
    }
}

// ============================================================================
// Test 14: Stress Test - Many Operations
// ============================================================================

TEST_F(QueueTest, StressTestManyOperations) {
    // Given: A queue
    Queue<core::u32, 64> queue;

    const int ITERATIONS = 1000;

    // When: Performing many send/receive operations
    for (int i = 0; i < ITERATIONS; i++) {
        EXPECT_TRUE(queue.try_send(i));

        if (i % 2 == 0) {  // Periodically drain some
            core::u32 value;
            EXPECT_TRUE(queue.try_receive(value));
        }
    }

    // Then: Queue should have predictable state
    EXPECT_EQ(queue.count(), ITERATIONS / 2);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
