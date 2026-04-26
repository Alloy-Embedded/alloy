// TSan-compatible unit test for CrossCoreChannel<T,N>.
//
// Two std::threads simulate a dual-core scenario: producer pushes 1M items,
// consumer pops them. All ordering is via acquire/release atomics — TSan
// should report zero data races.
//
// Run with:  cmake ... -DENABLE_TSAN=ON

#include <atomic>
#include <cstdint>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "runtime/cross_core_channel.hpp"

using alloy::tasks::CrossCoreChannel;

TEST_CASE("CrossCoreChannel: 1M items, zero drops, correct sum", "[cross_core][ordering]") {
    static constexpr std::size_t kCapacity = 1024;
    static constexpr std::uint64_t kItems = 1'000'000;

    CrossCoreChannel<std::uint32_t, kCapacity> ch;

    std::uint64_t received_sum = 0;
    std::uint64_t expected_sum = 0;
    for (std::uint64_t i = 0; i < kItems; ++i) expected_sum += i;

    std::atomic<bool> done{false};

    std::thread consumer([&] {
        std::uint64_t count = 0;
        while (count < kItems) {
            if (auto v = ch.try_pop()) {
                received_sum += *v;
                ++count;
            }
        }
        done.store(true, std::memory_order_release);
    });

    std::thread producer([&] {
        for (std::uint64_t i = 0; i < kItems; ++i) {
            while (!ch.try_push(static_cast<std::uint32_t>(i))) {
                // spin until consumer drains a slot
            }
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(done.load(std::memory_order_acquire));
    REQUIRE(received_sum == expected_sum);
}

TEST_CASE("CrossCoreChannel: size and empty", "[cross_core]") {
    CrossCoreChannel<int, 8> ch;

    REQUIRE(ch.empty());
    REQUIRE(ch.size() == 0u);

    REQUIRE(ch.try_push(10));
    REQUIRE(ch.try_push(20));

    REQUIRE_FALSE(ch.empty());
    REQUIRE(ch.size() == 2u);

    REQUIRE(ch.try_pop().has_value());
    REQUIRE(ch.size() == 1u);

    REQUIRE(ch.try_pop().has_value());
    REQUIRE(ch.empty());
}

TEST_CASE("CrossCoreChannel: drop counter on full ring", "[cross_core]") {
    CrossCoreChannel<int, 4> ch;

    REQUIRE(ch.try_push(1));
    REQUIRE(ch.try_push(2));
    REQUIRE(ch.try_push(3));
    REQUIRE_FALSE(ch.try_push(4));  // ring full (Lamport: capacity-1 usable)
    REQUIRE(ch.drops() == 1u);
}
