/// Unit Tests for RTOS Event Flags
///
/// Tests 32-bit event flags for multi-event synchronization,
/// wait_any, wait_all, set/clear operations, and complex scenarios.

#include <gtest/gtest.h>
#include "rtos/event.hpp"
#include "rtos/rtos.hpp"
#include "hal/host/systick.hpp"
#include <thread>
#include <atomic>
#include <chrono>

using namespace alloy;
using namespace alloy::rtos;

// Test fixture
class EventTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto result = hal::host::SystemTick::init();
        ASSERT_TRUE(result.is_ok());
    }

    // Common event flag definitions
    static constexpr core::u32 EVENT_FLAG_0  = (1 << 0);
    static constexpr core::u32 EVENT_FLAG_1  = (1 << 1);
    static constexpr core::u32 EVENT_FLAG_2  = (1 << 2);
    static constexpr core::u32 EVENT_FLAG_3  = (1 << 3);
    static constexpr core::u32 EVENT_FLAG_31 = (1u << 31);
};

// ============================================================================
// Test 1: Event Creation and Initial State
// ============================================================================

TEST_F(EventTest, EventCreationDefault) {
    // Given: An event flags object with default construction
    EventFlags events;

    // Then: All flags should be clear
    EXPECT_EQ(events.get(), 0u);
    EXPECT_FALSE(events.is_set(EVENT_FLAG_0));
    EXPECT_FALSE(events.is_any_set(0xFFFFFFFF));
}

TEST_F(EventTest, EventCreationWithInitialFlags) {
    // Given: Event flags with initial state
    EventFlags events(EVENT_FLAG_0 | EVENT_FLAG_2);

    // Then: Initial flags should be set
    EXPECT_EQ(events.get(), (EVENT_FLAG_0 | EVENT_FLAG_2));
    EXPECT_TRUE(events.is_set(EVENT_FLAG_0));
    EXPECT_TRUE(events.is_set(EVENT_FLAG_2));
    EXPECT_FALSE(events.is_set(EVENT_FLAG_1));
}

// ============================================================================
// Test 2: Set and Clear Operations
// ============================================================================

TEST_F(EventTest, SetSingleFlag) {
    // Given: Event flags
    EventFlags events;

    // When: Setting a single flag
    events.set(EVENT_FLAG_1);

    // Then: Only that flag should be set
    EXPECT_TRUE(events.is_set(EVENT_FLAG_1));
    EXPECT_EQ(events.get(), EVENT_FLAG_1);
}

TEST_F(EventTest, SetMultipleFlags) {
    // Given: Event flags
    EventFlags events;

    // When: Setting multiple flags
    events.set(EVENT_FLAG_0 | EVENT_FLAG_2 | EVENT_FLAG_3);

    // Then: All specified flags should be set
    EXPECT_TRUE(events.is_set(EVENT_FLAG_0));
    EXPECT_TRUE(events.is_set(EVENT_FLAG_2));
    EXPECT_TRUE(events.is_set(EVENT_FLAG_3));
    EXPECT_FALSE(events.is_set(EVENT_FLAG_1));
}

TEST_F(EventTest, SetIsAccumulative) {
    // Given: Event flags
    EventFlags events;

    // When: Setting flags in multiple calls
    events.set(EVENT_FLAG_0);
    events.set(EVENT_FLAG_1);
    events.set(EVENT_FLAG_2);

    // Then: All flags should be set (accumulative OR)
    EXPECT_TRUE(events.is_set(EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2));
    EXPECT_EQ(events.get(), (EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2));
}

TEST_F(EventTest, ClearSingleFlag) {
    // Given: Event flags with multiple flags set
    EventFlags events(EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2);

    // When: Clearing a single flag
    events.clear(EVENT_FLAG_1);

    // Then: Only that flag should be cleared
    EXPECT_TRUE(events.is_set(EVENT_FLAG_0));
    EXPECT_FALSE(events.is_set(EVENT_FLAG_1));
    EXPECT_TRUE(events.is_set(EVENT_FLAG_2));
}

TEST_F(EventTest, ClearMultipleFlags) {
    // Given: Event flags with all flags set
    EventFlags events(0xFFFFFFFF);

    // When: Clearing specific flags
    events.clear(EVENT_FLAG_0 | EVENT_FLAG_2);

    // Then: Only cleared flags should be 0
    EXPECT_FALSE(events.is_set(EVENT_FLAG_0));
    EXPECT_TRUE(events.is_set(EVENT_FLAG_1));
    EXPECT_FALSE(events.is_set(EVENT_FLAG_2));
    EXPECT_TRUE(events.is_set(EVENT_FLAG_3));
}

TEST_F(EventTest, SetAndClearAllFlags) {
    // Given: Event flags
    EventFlags events;

    // When: Setting all flags
    events.set(0xFFFFFFFF);

    // Then: All should be set
    EXPECT_EQ(events.get(), 0xFFFFFFFFu);

    // When: Clearing all flags
    events.clear(0xFFFFFFFF);

    // Then: All should be clear
    EXPECT_EQ(events.get(), 0u);
}

// ============================================================================
// Test 3: Query Operations (is_set, is_any_set)
// ============================================================================

TEST_F(EventTest, IsSetChecksAllFlags) {
    // Given: Event flags with some flags set
    EventFlags events(EVENT_FLAG_0 | EVENT_FLAG_1);

    // When: Checking if all specified flags are set
    EXPECT_TRUE(events.is_set(EVENT_FLAG_0));
    EXPECT_TRUE(events.is_set(EVENT_FLAG_1));
    EXPECT_TRUE(events.is_set(EVENT_FLAG_0 | EVENT_FLAG_1));

    // When: Checking flags that are not all set
    EXPECT_FALSE(events.is_set(EVENT_FLAG_0 | EVENT_FLAG_2));
    EXPECT_FALSE(events.is_set(EVENT_FLAG_2));
}

TEST_F(EventTest, IsAnySetChecksAnyFlag) {
    // Given: Event flags with some flags set
    EventFlags events(EVENT_FLAG_1);

    // When: Checking if any flag is set
    EXPECT_TRUE(events.is_any_set(EVENT_FLAG_1));
    EXPECT_TRUE(events.is_any_set(EVENT_FLAG_0 | EVENT_FLAG_1));
    EXPECT_TRUE(events.is_any_set(EVENT_FLAG_1 | EVENT_FLAG_2));

    // When: Checking flags that are all clear
    EXPECT_FALSE(events.is_any_set(EVENT_FLAG_0));
    EXPECT_FALSE(events.is_any_set(EVENT_FLAG_0 | EVENT_FLAG_2));
}

// ============================================================================
// Test 4: wait_any - Basic Functionality
// ============================================================================

TEST_F(EventTest, WaitAnyReturnsImmediatelyIfFlagSet) {
    // Given: Event flags with one flag set
    EventFlags events(EVENT_FLAG_1);

    // When: Waiting for any of multiple flags
    core::u32 result = events.wait_any(EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2, 0);

    // Then: Should return immediately with matching flag
    EXPECT_EQ(result, EVENT_FLAG_1);
}

TEST_F(EventTest, WaitAnyReturnsAllMatchingFlags) {
    // Given: Event flags with multiple flags set
    EventFlags events(EVENT_FLAG_0 | EVENT_FLAG_1);

    // When: Waiting for any of those flags
    core::u32 result = events.wait_any(EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2, 0);

    // Then: Should return all matching flags
    EXPECT_TRUE(result & EVENT_FLAG_0);
    EXPECT_TRUE(result & EVENT_FLAG_1);
    EXPECT_FALSE(result & EVENT_FLAG_2);
}

TEST_F(EventTest, WaitAnyWithAutoClearClearsMatchedFlags) {
    // Given: Event flags with flags set
    EventFlags events(EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2);

    // When: Waiting with auto_clear enabled
    core::u32 result = events.wait_any(EVENT_FLAG_1 | EVENT_FLAG_2, 0, true);

    // Then: Matched flags should be cleared
    EXPECT_NE(result, 0u);
    EXPECT_TRUE(events.is_set(EVENT_FLAG_0));  // Not cleared
    EXPECT_FALSE(events.is_set(EVENT_FLAG_1)); // Auto-cleared
    EXPECT_FALSE(events.is_set(EVENT_FLAG_2)); // Auto-cleared
}

// ============================================================================
// Test 5: wait_all - Basic Functionality
// ============================================================================

TEST_F(EventTest, WaitAllReturnsWhenAllFlagsSet) {
    // Given: Event flags with all required flags set
    EventFlags events(EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2);

    // When: Waiting for all flags
    core::u32 result = events.wait_all(EVENT_FLAG_0 | EVENT_FLAG_1, 0);

    // Then: Should succeed
    EXPECT_EQ(result, (EVENT_FLAG_0 | EVENT_FLAG_1));
}

TEST_F(EventTest, WaitAllFailsIfNotAllFlagsSet) {
    // Given: Event flags with only some flags set
    EventFlags events(EVENT_FLAG_0);

    // When: Waiting for all flags with short timeout
    core::u32 result = events.wait_all(EVENT_FLAG_0 | EVENT_FLAG_1, 10);

    // Then: Should timeout
    EXPECT_EQ(result, 0u);
}

TEST_F(EventTest, WaitAllWithAutoClearClearsFlags) {
    // Given: Event flags with all flags set
    EventFlags events(EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2);

    // When: Waiting with auto_clear
    core::u32 result = events.wait_all(EVENT_FLAG_0 | EVENT_FLAG_1, 0, true);

    // Then: Waited flags should be cleared
    EXPECT_NE(result, 0u);
    EXPECT_FALSE(events.is_set(EVENT_FLAG_0)); // Auto-cleared
    EXPECT_FALSE(events.is_set(EVENT_FLAG_1)); // Auto-cleared
    EXPECT_TRUE(events.is_set(EVENT_FLAG_2));  // Not waited for, not cleared
}

// ============================================================================
// Test 6: Multi-threaded Signaling
// ============================================================================

TEST_F(EventTest, MultiThreadedSetAndWaitAny) {
    // Given: Event flags and worker threads
    EventFlags events;
    std::atomic<bool> event_received{false};

    // Thread that waits for event
    std::thread waiter([&]() {
        core::u32 result = events.wait_any(EVENT_FLAG_1, 1000);
        if (result & EVENT_FLAG_1) {
            event_received = true;
        }
    });

    // Give waiter time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Thread that sets event
    std::thread setter([&]() {
        events.set(EVENT_FLAG_1);
    });

    waiter.join();
    setter.join();

    // Then: Event should have been received
    EXPECT_TRUE(event_received.load());
}

TEST_F(EventTest, MultiThreadedSetAndWaitAll) {
    // Given: Event flags
    EventFlags events;
    std::atomic<bool> all_received{false};

    const core::u32 ALL_FLAGS = EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2;

    // Thread waiting for all flags
    std::thread waiter([&]() {
        core::u32 result = events.wait_all(ALL_FLAGS, 1000);
        if (result == ALL_FLAGS) {
            all_received = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Threads setting flags incrementally
    std::thread setter1([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        events.set(EVENT_FLAG_0);
    });

    std::thread setter2([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        events.set(EVENT_FLAG_1);
    });

    std::thread setter3([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        events.set(EVENT_FLAG_2);
    });

    waiter.join();
    setter1.join();
    setter2.join();
    setter3.join();

    // Then: All flags should have been received
    EXPECT_TRUE(all_received.load());
}

// ============================================================================
// Test 7: All 32 Bits Independent
// ============================================================================

TEST_F(EventTest, All32BitsAreIndependent) {
    // Given: Event flags
    EventFlags events;

    // When: Setting each bit individually
    for (int i = 0; i < 32; i++) {
        core::u32 flag = (1u << i);
        events.set(flag);
        EXPECT_TRUE(events.is_set(flag)) << "Bit " << i << " should be set";
    }

    // Then: All 32 bits should be set
    EXPECT_EQ(events.get(), 0xFFFFFFFFu);

    // When: Clearing each bit individually
    for (int i = 0; i < 32; i++) {
        core::u32 flag = (1u << i);
        events.clear(flag);
        EXPECT_FALSE(events.is_set(flag)) << "Bit " << i << " should be clear";
    }

    // Then: All bits should be clear
    EXPECT_EQ(events.get(), 0u);
}

TEST_F(EventTest, HighestBit31Works) {
    // Given: Event flags
    EventFlags events;

    // When: Setting bit 31 (MSB)
    events.set(EVENT_FLAG_31);

    // Then: Should be set correctly
    EXPECT_TRUE(events.is_set(EVENT_FLAG_31));
    EXPECT_EQ(events.get(), EVENT_FLAG_31);

    // When: Waiting for bit 31
    core::u32 result = events.wait_any(EVENT_FLAG_31, 0);

    // Then: Should match
    EXPECT_EQ(result, EVENT_FLAG_31);
}

// ============================================================================
// Test 8: Complex Multi-Event Synchronization
// ============================================================================

TEST_F(EventTest, MultiSensorCoordinationPattern) {
    // Given: Sensors with individual ready flags
    EventFlags sensor_events;

    constexpr core::u32 SENSOR1_READY = (1 << 0);
    constexpr core::u32 SENSOR2_READY = (1 << 1);
    constexpr core::u32 SENSOR3_READY = (1 << 2);
    constexpr core::u32 ALL_SENSORS = SENSOR1_READY | SENSOR2_READY | SENSOR3_READY;

    std::atomic<int> fusions_completed{0};

    // Data fusion task - waits for all sensors
    std::thread fusion_task([&]() {
        for (int i = 0; i < 10; i++) {
            core::u32 result = sensor_events.wait_all(ALL_SENSORS, 1000, true);
            if (result == ALL_SENSORS) {
                fusions_completed++;
            }
        }
    });

    // Sensor tasks - set flags periodically
    std::thread sensor1([&]() {
        for (int i = 0; i < 10; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            sensor_events.set(SENSOR1_READY);
        }
    });

    std::thread sensor2([&]() {
        for (int i = 0; i < 10; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(6));
            sensor_events.set(SENSOR2_READY);
        }
    });

    std::thread sensor3([&]() {
        for (int i = 0; i < 10; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(7));
            sensor_events.set(SENSOR3_READY);
        }
    });

    fusion_task.join();
    sensor1.join();
    sensor2.join();
    sensor3.join();

    // Then: Should have completed some fusions
    EXPECT_GE(fusions_completed.load(), 5);
}

// ============================================================================
// Test 9: State Machine Pattern
// ============================================================================

TEST_F(EventTest, StateMachinePattern) {
    // Given: State machine with event-driven transitions
    EventFlags state_events;

    constexpr core::u32 EVENT_INIT      = (1 << 0);
    constexpr core::u32 EVENT_CONFIG    = (1 << 1);
    constexpr core::u32 EVENT_START     = (1 << 2);
    constexpr core::u32 EVENT_DATA      = (1 << 3);
    constexpr core::u32 EVENT_STOP      = (1 << 4);

    std::atomic<int> state{0};

    std::thread state_machine([&]() {
        // State 0: Wait for init
        if (state_events.wait_any(EVENT_INIT, 1000)) {
            state = 1;
        }

        // State 1: Wait for config
        if (state_events.wait_any(EVENT_CONFIG, 1000)) {
            state = 2;
        }

        // State 2: Wait for start
        if (state_events.wait_any(EVENT_START, 1000)) {
            state = 3;
        }

        // State 3: Process data
        if (state_events.wait_any(EVENT_DATA, 1000)) {
            state = 4;
        }

        // State 4: Wait for stop
        if (state_events.wait_any(EVENT_STOP, 1000)) {
            state = 5;
        }
    });

    // Drive state machine through transitions
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    state_events.set(EVENT_INIT);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    state_events.set(EVENT_CONFIG);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    state_events.set(EVENT_START);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    state_events.set(EVENT_DATA);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    state_events.set(EVENT_STOP);

    state_machine.join();

    // Then: Should have reached final state
    EXPECT_EQ(state.load(), 5);
}

// ============================================================================
// Test 10: Timeout Behavior
// ============================================================================

TEST_F(EventTest, WaitAnyTimeout) {
    // Given: Event flags with no flags set
    EventFlags events;

    // When: Waiting with timeout
    auto start = std::chrono::steady_clock::now();
    core::u32 result = events.wait_any(EVENT_FLAG_0, 50);
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Then: Should timeout and return 0
    EXPECT_EQ(result, 0u);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 45);
}

TEST_F(EventTest, WaitAllTimeout) {
    // Given: Event flags with partial match
    EventFlags events(EVENT_FLAG_0);

    // When: Waiting for all flags with timeout
    auto start = std::chrono::steady_clock::now();
    core::u32 result = events.wait_all(EVENT_FLAG_0 | EVENT_FLAG_1, 50);
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Then: Should timeout
    EXPECT_EQ(result, 0u);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 45);
}

// ============================================================================
// Test 11: Stress Test
// ============================================================================

TEST_F(EventTest, StressTestMultipleSettersAndWaiters) {
    // Given: Shared event flags
    EventFlags events;
    std::atomic<int> any_triggers{0};
    std::atomic<int> all_triggers{0};

    const int NUM_SETTERS = 4;
    const int NUM_WAITERS = 4;
    const int OPERATIONS = 50;

    // Setters - rapidly set random flags
    auto setter = [&](int id) {
        for (int i = 0; i < OPERATIONS; i++) {
            core::u32 flag = (1u << (id % 8));
            events.set(flag);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    };

    // Waiters - wait for various flag combinations
    auto waiter_any = [&]() {
        for (int i = 0; i < OPERATIONS / 2; i++) {
            core::u32 result = events.wait_any(0xFF, 10);
            if (result != 0) {
                any_triggers++;
            }
        }
    };

    auto waiter_all = [&]() {
        for (int i = 0; i < OPERATIONS / 4; i++) {
            core::u32 result = events.wait_all(EVENT_FLAG_0 | EVENT_FLAG_1, 20);
            if (result != 0) {
                all_triggers++;
            }
        }
    };

    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_SETTERS; i++) {
        threads.emplace_back(setter, i);
    }

    for (int i = 0; i < NUM_WAITERS; i++) {
        threads.emplace_back(waiter_any);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Then: Should have triggered many events
    EXPECT_GT(any_triggers.load(), 0);
}

// ============================================================================
// Test 12: Memory Footprint
// ============================================================================

TEST_F(EventTest, EventFlagsSizeIsReasonable) {
    // When: Checking size
    size_t event_size = sizeof(EventFlags);

    // Then: Should be compact (documented: 12 bytes)
    EXPECT_LE(event_size, 24u) << "EventFlags should be compact";
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
