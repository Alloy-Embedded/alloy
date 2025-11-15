/// Unit Tests for RTOS Event Flags
///
/// Tests 32-bit event flags for multi-event synchronization using Catch2

#include <atomic>
#include <chrono>
#include <thread>

#include <catch2/catch_section_info.hpp>
#include <catch2/catch_test_macros.hpp>

#include "hal/vendors/host/systick.hpp"

#include "rtos/event.hpp"

#include "core/types.hpp"

using namespace alloy;
using namespace alloy::rtos;

// Event flag definitions
constexpr core::u32 EVENT_FLAG_0 = (1u << 0);
constexpr core::u32 EVENT_FLAG_1 = (1u << 1);
constexpr core::u32 EVENT_FLAG_2 = (1u << 2);
constexpr core::u32 EVENT_FLAG_3 = (1u << 3);
constexpr core::u32 EVENT_ALL = EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2 | EVENT_FLAG_3;

// ============================================================================
// Test 1: EventFlags Basic Operations
// ============================================================================

TEST_CASE("EventFlags basic operations", "[event][basic]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("EventFlags start with initial value") {
        EventFlags events(0);
        REQUIRE(events.get() == 0);

        EventFlags events_init(EVENT_FLAG_0 | EVENT_FLAG_1);
        REQUIRE(events_init.get() == (EVENT_FLAG_0 | EVENT_FLAG_1));
    }

    SECTION("set adds flags") {
        EventFlags events(0);
        events.set(EVENT_FLAG_0);
        REQUIRE(events.get() == EVENT_FLAG_0);

        events.set(EVENT_FLAG_1);
        REQUIRE(events.get() == (EVENT_FLAG_0 | EVENT_FLAG_1));
    }

    SECTION("clear removes flags") {
        EventFlags events(EVENT_ALL);
        events.clear(EVENT_FLAG_0);
        REQUIRE((events.get() & EVENT_FLAG_0) == 0);
        REQUIRE((events.get() & EVENT_FLAG_1) != 0);
    }

    SECTION("clear can remove multiple flags") {
        EventFlags events(EVENT_ALL);
        events.clear(EVENT_FLAG_0 | EVENT_FLAG_1);
        REQUIRE((events.get() & (EVENT_FLAG_0 | EVENT_FLAG_1)) == 0);
        REQUIRE((events.get() & EVENT_FLAG_2) != 0);
    }
}

// ============================================================================
// Test 2: wait_any Operation
// ============================================================================

TEST_CASE("EventFlags wait_any", "[event][wait_any]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("wait_any returns immediately if any flag is set") {
        EventFlags events(EVENT_FLAG_0);

        auto start = std::chrono::steady_clock::now();
        core::u32 result = events.wait_any(EVENT_FLAG_0 | EVENT_FLAG_1, 0);
        auto duration = std::chrono::steady_clock::now() - start;

        REQUIRE(result == EVENT_FLAG_0);
        REQUIRE(duration < std::chrono::milliseconds(50));
    }

    SECTION("wait_any returns when one of multiple flags is set") {
        EventFlags events(EVENT_FLAG_1);
        core::u32 result = events.wait_any(EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2, 0);
        REQUIRE(result == EVENT_FLAG_1);
    }

    SECTION("wait_any returns 0 on timeout") {
        EventFlags events(0);
        core::u32 result = events.wait_any(EVENT_FLAG_0, 0);
        REQUIRE(result == 0);
    }
}

// ============================================================================
// Test 3: wait_all Operation
// ============================================================================

TEST_CASE("EventFlags wait_all", "[event][wait_all]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("wait_all returns when all requested flags are set") {
        EventFlags events(EVENT_FLAG_0 | EVENT_FLAG_1 | EVENT_FLAG_2);
        core::u32 result = events.wait_all(EVENT_FLAG_0 | EVENT_FLAG_1, 0);
        REQUIRE(result == (EVENT_FLAG_0 | EVENT_FLAG_1));
    }

    SECTION("wait_all returns 0 if not all flags are set") {
        EventFlags events(EVENT_FLAG_0);  // Only flag 0
        core::u32 result = events.wait_all(EVENT_FLAG_0 | EVENT_FLAG_1, 0);
        REQUIRE(result == 0);  // Flag 1 is missing
    }

    SECTION("wait_all with single flag") {
        EventFlags events(EVENT_FLAG_2);
        core::u32 result = events.wait_all(EVENT_FLAG_2, 0);
        REQUIRE(result == EVENT_FLAG_2);
    }
}

// ============================================================================
// Test 4: Auto-Clear Behavior
// ============================================================================

TEST_CASE("EventFlags auto-clear", "[event][auto-clear]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("wait_any with auto-clear removes matched flags") {
        EventFlags events(EVENT_FLAG_0 | EVENT_FLAG_1);
        core::u32 result = events.wait_any(EVENT_FLAG_0, 0, true);

        REQUIRE(result == EVENT_FLAG_0);
        REQUIRE((events.get() & EVENT_FLAG_0) == 0);  // Flag 0 cleared
        REQUIRE((events.get() & EVENT_FLAG_1) != 0);  // Flag 1 still set
    }

    SECTION("wait_all with auto-clear removes all matched flags") {
        EventFlags events(EVENT_ALL);
        core::u32 result = events.wait_all(EVENT_FLAG_0 | EVENT_FLAG_1, 0, true);

        REQUIRE(result == (EVENT_FLAG_0 | EVENT_FLAG_1));
        REQUIRE((events.get() & (EVENT_FLAG_0 | EVENT_FLAG_1)) == 0);
        REQUIRE((events.get() & EVENT_FLAG_2) != 0);  // Other flags remain
    }

    SECTION("No auto-clear leaves flags set") {
        EventFlags events(EVENT_FLAG_0);
        core::u32 result = events.wait_any(EVENT_FLAG_0, 0, false);

        REQUIRE(result == EVENT_FLAG_0);
        REQUIRE((events.get() & EVENT_FLAG_0) != 0);  // Still set
    }
}

// ============================================================================
// Test 5: Multi-threaded Synchronization
// ============================================================================

TEST_CASE("EventFlags multi-threaded synchronization", "[event][threading]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Thread waits for event to be set") {
        EventFlags events(0);
        std::atomic<bool> thread_completed{false};

        std::thread worker([&]() {
            core::u32 result = events.wait_any(EVENT_FLAG_0, 1000);
            if (result == EVENT_FLAG_0) {
                thread_completed = true;
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        REQUIRE_FALSE(thread_completed);

        events.set(EVENT_FLAG_0);
        worker.join();

        REQUIRE(thread_completed);
    }

    SECTION("Multiple threads wait for different events") {
        EventFlags events(0);
        std::atomic<int> completed{0};

        std::thread worker1([&]() {
            if (events.wait_any(EVENT_FLAG_0, 1000) == EVENT_FLAG_0) {
                completed++;
            }
        });

        std::thread worker2([&]() {
            if (events.wait_any(EVENT_FLAG_1, 1000) == EVENT_FLAG_1) {
                completed++;
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        events.set(EVENT_FLAG_0);
        events.set(EVENT_FLAG_1);

        worker1.join();
        worker2.join();

        REQUIRE(completed == 2);
    }
}

// ============================================================================
// Test 6: Bit Manipulation
// ============================================================================

TEST_CASE("EventFlags bit manipulation", "[event][bits]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("Can set all 32 bits") {
        EventFlags events(0);
        events.set(0xFFFFFFFF);
        REQUIRE(events.get() == 0xFFFFFFFF);
    }

    SECTION("Can clear all bits") {
        EventFlags events(0xFFFFFFFF);
        events.clear(0xFFFFFFFF);
        REQUIRE(events.get() == 0);
    }

    SECTION("Individual bit operations") {
        EventFlags events(0);

        for (int i = 0; i < 32; i++) {
            core::u32 flag = (1u << i);
            events.set(flag);
            REQUIRE((events.get() & flag) != 0);
        }

        REQUIRE(events.get() == 0xFFFFFFFF);
    }
}

// ============================================================================
// Test 7: Edge Cases
// ============================================================================

TEST_CASE("EventFlags edge cases", "[event][edge-cases]") {
    auto result = hal::host::SystemTick::init();
    REQUIRE(result.is_ok());

    SECTION("wait_any with no flags requested") {
        EventFlags events(EVENT_ALL);
        core::u32 result = events.wait_any(0, 0);
        REQUIRE(result == 0);
    }

    SECTION("wait_all with no flags requested") {
        EventFlags events(EVENT_ALL);
        core::u32 result = events.wait_all(0, 0);
        REQUIRE(result == 0);
    }

    SECTION("Rapid set and clear") {
        EventFlags events(0);
        for (int i = 0; i < 1000; i++) {
            events.set(EVENT_FLAG_0);
            events.clear(EVENT_FLAG_0);
        }
        REQUIRE(events.get() == 0);
    }
}

// ============================================================================
// Test 8: Compile-Time Properties
// ============================================================================

TEST_CASE("EventFlags compile-time properties", "[event][compile-time]") {
    SECTION("EventFlags is not copyable") {
        REQUIRE(!std::is_copy_constructible_v<EventFlags>);
        REQUIRE(!std::is_copy_assignable_v<EventFlags>);
    }

    SECTION("EventFlags uses 32-bit flags") {
        EventFlags events(0xFFFFFFFF);
        REQUIRE(events.get() == 0xFFFFFFFF);
        REQUIRE(sizeof(core::u32) == 4);
    }
}
