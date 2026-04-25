#pragma once

// Static priority levels for the cooperative task scheduler.
//
// Cooperative scheduling means tasks switch only at user-visible co_await
// points; priority controls which ready task the scheduler resumes next, never
// what preempts whom. There are exactly four levels by design:
//
//   high   -- "must respond before the others", e.g. a button debouncer
//   normal -- the default for application tasks
//   low    -- background work that gives way to anything else
//   idle   -- only runs when nothing else is ready
//
// FIFO order is preserved within a level. No priority inheritance, no aging,
// no boosting. Cooperative => no priority inversion is possible.

#include <cstdint>

namespace alloy::tasks {

enum class Priority : std::uint8_t {
    High = 0,
    Normal = 1,
    Low = 2,
    Idle = 3,
};

inline constexpr std::size_t kPriorityCount = 4u;

[[nodiscard]] constexpr auto to_index(Priority p) noexcept -> std::size_t {
    return static_cast<std::size_t>(p);
}

}  // namespace alloy::tasks
