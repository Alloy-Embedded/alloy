// Host-level coverage for the canonical runtime async-model completion+timeout
// primitive.
//
// Proves the two scenarios cited by openspec change
// `make-async-model-real-and-usable` spec runtime-validation:
//   - completion fires before the deadline  -> wait_for returns Ok
//   - completion never fires before deadline -> wait_for returns Err(Timeout)
//
// These tests intentionally do not touch MMIO. The canonical completion
// primitive (`runtime::event::completion`) is portable and testable with a
// deterministic mock time source. A separate host-MMIO scenario covers
// DMA-driven completion signalling on real descriptor metadata.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include "core/error_code.hpp"
#include "runtime/event.hpp"
#include "runtime/time.hpp"

namespace {

struct SuccessTag {};
struct TimeoutTag {};
struct RecoveryTag {};

using SuccessCompletion = alloy::runtime::event::completion<SuccessTag>;
using TimeoutCompletion = alloy::runtime::event::completion<TimeoutTag>;
using RecoveryCompletion = alloy::runtime::event::completion<RecoveryTag>;

// Deterministic time source: each call to now() advances the internal clock
// by `step_micros`. Deadline checks therefore make progress every time the
// event loop polls — no real wall-clock dependency.
class MockTime {
   public:
    static void reset(std::uint64_t start_micros = 0, std::uint64_t step = 1000u) {
        state().micros = start_micros;
        state().step_micros = step;
    }

    [[nodiscard]] static auto now() -> alloy::runtime::time::Instant {
        const auto current = state().micros;
        state().micros += state().step_micros;
        return alloy::runtime::time::Instant::from_micros(current);
    }

    [[nodiscard]] static auto deadline_after(alloy::runtime::time::Duration d)
        -> alloy::runtime::time::Deadline {
        return alloy::runtime::time::Deadline::at(now() + d);
    }

   private:
    struct state_type {
        std::uint64_t micros = 0;
        std::uint64_t step_micros = 1000u;  // 1 ms per poll by default
    };

    [[nodiscard]] static auto state() -> state_type& {
        static state_type s{};
        return s;
    }
};

}  // namespace

TEST_CASE(
    "runtime async canonical path reports completion when signal fires before deadline",
    "[unit][async][completion]") {
    MockTime::reset(/*start_micros=*/0, /*step=*/1000u);
    SuccessCompletion::reset();

    // Completion fires first; wait_for must observe it and return Ok even
    // with a generous deadline.
    SuccessCompletion::signal();

    const auto result =
        SuccessCompletion::wait_for<MockTime>(alloy::runtime::time::Duration::from_millis(50));

    REQUIRE(result.is_ok());
}

TEST_CASE(
    "runtime async canonical path reports Timeout when signal never fires",
    "[unit][async][completion][timeout]") {
    MockTime::reset(/*start_micros=*/0, /*step=*/1000u);
    TimeoutCompletion::reset();

    // Signal is deliberately withheld. Each poll advances the mock clock by
    // 1ms; with a 5ms deadline, wait_for must give up and return Err(Timeout)
    // rather than spinning forever or reporting success.
    const auto result =
        TimeoutCompletion::wait_for<MockTime>(alloy::runtime::time::Duration::from_millis(5));

    REQUIRE(result.is_err());
    REQUIRE(result.error() == alloy::core::ErrorCode::Timeout);
    REQUIRE_FALSE(TimeoutCompletion::ready());
}

TEST_CASE(
    "runtime async canonical path is recoverable after a timeout",
    "[unit][async][completion][recovery]") {
    MockTime::reset(/*start_micros=*/0, /*step=*/1000u);
    RecoveryCompletion::reset();

    // First wait: signal withheld, deadline elapses, Timeout is reported.
    const auto first =
        RecoveryCompletion::wait_for<MockTime>(alloy::runtime::time::Duration::from_millis(5));
    REQUIRE(first.is_err());
    REQUIRE(first.error() == alloy::core::ErrorCode::Timeout);

    // Same token, same in-flight operation. Signal fires, the user waits
    // again with a fresh deadline, and the canonical path keeps working:
    // completion is still observable, no undefined state left behind.
    RecoveryCompletion::signal();
    const auto second =
        RecoveryCompletion::wait_for<MockTime>(alloy::runtime::time::Duration::from_millis(50));
    REQUIRE(second.is_ok());
    REQUIRE(RecoveryCompletion::ready());
}
