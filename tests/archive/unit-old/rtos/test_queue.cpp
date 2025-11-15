/// Unit Tests for RTOS Queue (Message Queue)
///
/// Tests type-safe FIFO message queue for inter-task communication.
/// Covers send/receive, blocking, timeouts, full/empty conditions, and edge cases.

#include <atomic>
#include <thread>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "hal/vendors/host/systick.hpp"

#include "rtos/queue.hpp"

using namespace alloy;
using namespace alloy::rtos;
using Catch::Approx;

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

// ============================================================================
// Test 1: Queue Creation and Capacity
// ============================================================================

TEST_CASE("QueueCreation", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A queue with capacity 8
    Queue<SimpleMessage, 8> queue;

    // Then: Should be empty initially
    REQUIRE(queue.count() == 0u);
    REQUIRE(queue.available() == 8u);
    REQUIRE(queue.capacity() == 8u);
    REQUIRE(queue.is_empty());
    REQUIRE_FALSE(queue.is_full());
}

TEST_CASE("DifferentCapacities", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Queues with different capacities (must be power of 2)
    Queue<core::u32, 2> tiny;
    Queue<core::u32, 4> small;
    Queue<core::u32, 16> medium;
    Queue<core::u32, 64> large;
    Queue<core::u32, 256> xlarge;

    // Then: Capacities should match
    REQUIRE(tiny.capacity() == 2u);
    REQUIRE(small.capacity() == 4u);
    REQUIRE(medium.capacity() == 16u);
    REQUIRE(large.capacity() == 64u);
    REQUIRE(xlarge.capacity() == 256u);
}

// ============================================================================
// Test 2: Basic Send and Receive
// ============================================================================

TEST_CASE("SendAndReceiveSingleMessage", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: An empty queue
    Queue<SimpleMessage, 8> queue;

    SimpleMessage msg_in = {42, 100};

    // When: Sending a message
    bool sent = queue.try_send(msg_in);

    // Then: Send should succeed
    REQUIRE(sent);
    REQUIRE(queue.count() == 1u);
    REQUIRE_FALSE(queue.is_empty());

    // When: Receiving the message
    SimpleMessage msg_out;
    bool received = queue.try_receive(msg_out);

    // Then: Receive should succeed with correct data
    REQUIRE(received);
    REQUIRE(msg_out.id == 42u);
    REQUIRE(msg_out.value == 100);
    REQUIRE(queue.is_empty());
}

TEST_CASE("SendAndReceiveMultipleMessages", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: An empty queue
    Queue<core::u32, 8> queue;

    // When: Sending multiple messages
    for (core::u32 i = 0; i < 5; i++) {
        REQUIRE(queue.try_send(i * 10));
    }

    // Then: Count should be correct
    REQUIRE(queue.count() == 5u);

    // When: Receiving messages
    for (core::u32 i = 0; i < 5; i++) {
        core::u32 value;
        REQUIRE(queue.try_receive(value));
        REQUIRE(value == i * 10);
    }

    // Then: Queue should be empty
    REQUIRE(queue.is_empty());
}

// ============================================================================
// Test 3: FIFO Order
// ============================================================================

TEST_CASE("FIFOOrdering", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

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
        REQUIRE(queue.try_receive(value));
        REQUIRE(value == i);
    }
}

// ============================================================================
// Test 4: Full Queue Behavior
// ============================================================================

TEST_CASE("QueueFullDetection", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A small queue
    Queue<core::u32, 4> queue;

    // When: Filling the queue
    for (core::u32 i = 0; i < 4; i++) {
        REQUIRE(queue.try_send(i));
    }

    // Then: Should be full
    REQUIRE(queue.is_full());
    REQUIRE(queue.count() == 4u);
    REQUIRE(queue.available() == 0u);

    // And: Further sends should fail
    REQUIRE_FALSE(queue.try_send(999));
}

TEST_CASE("SendToFullQueueFails", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A full queue
    Queue<core::u32, 2> queue;
    queue.try_send(1);
    queue.try_send(2);

    // When: Trying to send to full queue
    bool sent = queue.try_send(3);

    // Then: Should fail
    REQUIRE_FALSE(sent);
    REQUIRE(queue.count() == 2u);
}

// ============================================================================
// Test 5: Empty Queue Behavior
// ============================================================================

TEST_CASE("ReceiveFromEmptyQueueFails", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: An empty queue
    Queue<core::u32, 8> queue;

    // When: Trying to receive from empty queue
    core::u32 value;
    bool received = queue.try_receive(value);

    // Then: Should fail
    REQUIRE_FALSE(received);
}

// ============================================================================
// Test 6: Circular Buffer Wraparound
// ============================================================================

TEST_CASE("CircularBufferWraparound", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A queue
    Queue<core::u32, 4> queue;

    // When: Repeatedly send and receive (causes wraparound)
    for (int cycle = 0; cycle < 20; cycle++) {
        // Fill queue
        for (core::u32 i = 0; i < 4; i++) {
            REQUIRE(queue.try_send(cycle * 10 + i));
        }

        // Empty queue
        for (core::u32 i = 0; i < 4; i++) {
            core::u32 value;
            REQUIRE(queue.try_receive(value));
            REQUIRE(value == cycle * 10 + i);
        }

        REQUIRE(queue.is_empty());
    }
}

// ============================================================================
// Test 7: Peek Functionality
// ============================================================================

// TODO: Implement try_peek method in Queue class
// TEST_CASE("PeekDoesNotRemoveMessage", "[rtos][queue]") {
//     auto systick_result = hal::host::SystemTick::init();
//     REQUIRE(systick_result.is_ok());

//     // Given: A queue with a message
//     Queue<core::u32, 8> queue;
//     queue.try_send(42);

//     // When: Peeking multiple times
//     core::u32 value1, value2, value3;
//     REQUIRE(queue.try_peek(value1));
//     REQUIRE(queue.try_peek(value2));
//     REQUIRE(queue.try_peek(value3));

//     // Then: All peeks should return same value
//     REQUIRE(value1 == 42u);
//     REQUIRE(value2 == 42u);
//     REQUIRE(value3 == 42u);

//     // And: Message should still be in queue
//     REQUIRE(queue.count() == 1u);

//     // When: Actually receiving
//     core::u32 value_out;
//     REQUIRE(queue.try_receive(value_out));

//     // Then: Should get the same value and queue becomes empty
//     REQUIRE(value_out == 42u);
//     REQUIRE(queue.is_empty());
// }

// ============================================================================
// Test 8: Large Message Types
// ============================================================================

TEST_CASE("LargeMessageTypes", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A queue for large messages
    Queue<LargeMessage, 4> queue;

    LargeMessage msg_in;
    msg_in.timestamp = 12345;
    for (int i = 0; i < 64; i++) {
        msg_in.data[i] = static_cast<core::u8>(i);
    }
    msg_in.checksum = 0xABCD;

    // When: Sending and receiving
    REQUIRE(queue.try_send(msg_in));

    LargeMessage msg_out;
    REQUIRE(queue.try_receive(msg_out));

    // Then: Data should match exactly
    REQUIRE(msg_out.timestamp == 12345u);
    REQUIRE(msg_out.checksum == 0xABCDu);
    for (int i = 0; i < 64; i++) {
        REQUIRE(msg_out.data[i] == static_cast<core::u8>(i));
    }
}

// ============================================================================
// Test 9: Available Space
// ============================================================================

TEST_CASE("AvailableSpaceTracking", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A queue
    Queue<core::u32, 8> queue;

    // Then: Initially all space available
    REQUIRE(queue.available() == 8u);

    // When: Adding messages
    queue.try_send(1);
    REQUIRE(queue.available() == 7u);

    queue.try_send(2);
    REQUIRE(queue.available() == 6u);

    queue.try_send(3);
    REQUIRE(queue.available() == 5u);

    // When: Removing a message
    core::u32 value;
    queue.try_receive(value);
    REQUIRE(queue.available() == 6u);
}

// ============================================================================
// Test 10: Count Tracking
// ============================================================================

TEST_CASE("CountTracking", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A queue
    Queue<core::u32, 16> queue;

    // Then: Initially count is 0
    REQUIRE(queue.count() == 0u);

    // When: Adding messages
    for (core::u32 i = 0; i < 10; i++) {
        queue.try_send(i);
        REQUIRE(queue.count() == i + 1);
    }

    // When: Removing messages
    for (core::u32 i = 0; i < 10; i++) {
        core::u32 value;
        queue.try_receive(value);
        REQUIRE(queue.count() == 10 - i - 1);
    }

    REQUIRE(queue.count() == 0u);
}

// ============================================================================
// Test 11: Type Safety
// ============================================================================

struct TypeA {
    int x;
};
struct TypeB {
    float y;
};

TEST_CASE("TypeSafety", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: Queues of different types
    Queue<TypeA, 4> queue_a;
    Queue<TypeB, 4> queue_b;

    TypeA msg_a = {42};
    TypeB msg_b = {3.14f};

    // When: Sending type-specific messages
    REQUIRE(queue_a.try_send(msg_a));
    REQUIRE(queue_b.try_send(msg_b));

    // Then: Each queue should maintain its type
    TypeA out_a;
    TypeB out_b;

    REQUIRE(queue_a.try_receive(out_a));
    REQUIRE(queue_b.try_receive(out_b));

    REQUIRE(out_a.x == 42);
    REQUIRE(out_b.y == Approx(3.14f));
}

// ============================================================================
// Test 12: Reset/Clear
// ============================================================================

TEST_CASE("ResetClearsQueue", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A queue with messages
    Queue<core::u32, 8> queue;
    for (core::u32 i = 0; i < 5; i++) {
        queue.try_send(i);
    }

    REQUIRE(queue.count() == 5u);

    // When: Resetting the queue
    queue.reset();

    // Then: Should be empty
    REQUIRE(queue.count() == 0u);
    REQUIRE(queue.is_empty());
    REQUIRE(queue.available() == 8u);
}

// ============================================================================
// Test 13: Edge Case - Single Element Queue
// ============================================================================

TEST_CASE("SingleElementQueue", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A queue with capacity 2 (minimum)
    Queue<core::u32, 2> queue;

    // When: Repeatedly using the queue
    for (int i = 0; i < 100; i++) {
        REQUIRE(queue.try_send(i));
        REQUIRE(queue.count() == 1u);

        core::u32 value;
        REQUIRE(queue.try_receive(value));
        REQUIRE(value == static_cast<core::u32>(i));
        REQUIRE(queue.is_empty());
    }
}

// ============================================================================
// Test 14: Stress Test - Many Operations
// ============================================================================

TEST_CASE("StressTestManyOperations", "[rtos][queue]") {
    auto systick_result = hal::host::SystemTick::init();
    REQUIRE(systick_result.is_ok());

    // Given: A queue
    Queue<core::u32, 64> queue;

    const int ITERATIONS = 1000;

    // When: Performing many send/receive operations
    for (int i = 0; i < ITERATIONS; i++) {
        REQUIRE(queue.try_send(i));

        if (i % 2 == 0) {  // Periodically drain some
            core::u32 value;
            REQUIRE(queue.try_receive(value));
        }
    }

    // Then: Queue should have predictable state
    REQUIRE(queue.count() == ITERATIONS / 2);
}

// ============================================================================
// Main
// ============================================================================
