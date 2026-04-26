// Memory ordering tests for runtime channel primitives.
//
// Channel<T,N>: single-core ISR→task ring. Uses signal_fence (compiler barrier
// only). Tested here single-threaded — cross-thread use requires CrossCoreChannel.
//
// CrossCoreChannel<T,N>: acquire/release atomics, TSan-safe.
// Multi-threaded cross-core tests are in test_cross_core_channel.cpp.

#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "runtime/tasks/channel.hpp"

using alloy::tasks::Channel;

TEST_CASE("Channel: basic push/pop ordering", "[channel][ordering]") {
    Channel<std::uint32_t, 8> ch;

    REQUIRE(ch.empty());
    REQUIRE(ch.size() == 0u);

    REQUIRE(ch.try_push(10u));
    REQUIRE(ch.try_push(20u));
    REQUIRE(ch.try_push(30u));

    REQUIRE(ch.size() == 3u);

    auto v = ch.try_pop();
    REQUIRE(v.has_value());
    REQUIRE(*v == 10u);  // FIFO

    REQUIRE(ch.size() == 2u);
}

TEST_CASE("Channel: drop counter on full ring", "[channel][ordering]") {
    Channel<int, 4> ch;

    REQUIRE(ch.try_push(1));
    REQUIRE(ch.try_push(2));
    REQUIRE(ch.try_push(3));
    REQUIRE_FALSE(ch.try_push(4));  // full (Lamport: capacity-1 usable slots)
    REQUIRE(ch.drops() == 1u);
}

TEST_CASE("Channel: roundtrip 1024 items single-threaded", "[channel][ordering]") {
    static constexpr std::size_t kCapacity = 128;
    Channel<std::uint32_t, kCapacity> ch;

    std::uint32_t expected_sum = 0;
    std::uint32_t received_sum = 0;

    for (std::uint32_t i = 0; i < 1024u; ++i) {
        while (!ch.try_push(i)) {
            // drain before pushing more
            while (auto v = ch.try_pop()) received_sum += *v;
        }
        expected_sum += i;
    }
    while (auto v = ch.try_pop()) received_sum += *v;

    REQUIRE(received_sum == expected_sum);
}
