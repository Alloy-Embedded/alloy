#pragma once

// Dual-core cooperative scheduler.
//
// `SharedScheduler<N, B>` owns two `Scheduler<N, B>` instances — one per
// core — and a TAS spinlock that serialises spawn() calls for
// `CoreAffinity::Any` tasks.
//
// Spawn routing:
//   CoreAffinity::Core0  → core0 scheduler directly (no lock needed)
//   CoreAffinity::Core1  → core1 scheduler directly (no lock needed)
//   CoreAffinity::Any    → round-robin between the two, under spinlock
//
// Tick: each core calls tick(core_id) from its own loop. Only that core's
// scheduler is driven; there is no cross-core task migration after spawn.
//
// Single-core fallback: if ALLOY_SINGLE_CORE is defined, CoreAffinity is
// accepted but ignored — all tasks go into the single Scheduler instance.

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "core/result.hpp"
#include "runtime/tasks/scheduler.hpp"

namespace alloy::tasks {

enum class CoreAffinity : std::uint8_t {
    Core0,
    Core1,
    Any,
};

// TAS (test-and-set) spinlock. Byte-sized flag; alignas(64) separates it
// from the scheduler state to prevent false sharing on the cache line.
class TasSpinlock {
   public:
    void lock() noexcept {
        while (flag_.exchange(1u, std::memory_order_acquire) != 0u) {
            // spin — yield hint on x86, nop elsewhere
        }
    }
    void unlock() noexcept { flag_.store(0u, std::memory_order_release); }
    [[nodiscard]] auto try_lock() noexcept -> bool {
        return flag_.exchange(1u, std::memory_order_acquire) == 0u;
    }

   private:
    std::atomic<std::uint8_t> flag_{0};
};

template <std::size_t MaxTasksPerCore, std::size_t MaxFrameBytes>
class SharedScheduler {
   public:
    SharedScheduler() noexcept = default;

    SharedScheduler(const SharedScheduler&) = delete;
    auto operator=(const SharedScheduler&) -> SharedScheduler& = delete;

    using SchedType = Scheduler<MaxTasksPerCore, MaxFrameBytes>;

    // Spawn a task with core affinity.
    //
    // Core0/Core1 spawns are lock-free — they go directly into the pinned
    // scheduler. Any spawns take the spinlock to safely round-robin.
    template <typename Fn>
    auto spawn(Fn&& factory, Priority prio = Priority::Normal,
               CoreAffinity affinity = CoreAffinity::Any,
               CancellationToken token = CancellationToken::make())
        -> core::Result<TaskId, SpawnError> {
        switch (affinity) {
            case CoreAffinity::Core0:
                return core0_.spawn(std::forward<Fn>(factory), prio, token);
            case CoreAffinity::Core1:
                return core1_.spawn(std::forward<Fn>(factory), prio, token);
            case CoreAffinity::Any: {
                lock_.lock();
                const auto target = next_any_core_++ & 1u;
                lock_.unlock();
                return (target == 0u)
                    ? core0_.spawn(std::forward<Fn>(factory), prio, token)
                    : core1_.spawn(std::forward<Fn>(factory), prio, token);
            }
        }
        return core0_.spawn(std::forward<Fn>(factory), prio, token);
    }

    // Drive one tick on `core_id` (0 or 1). Returns true while the chosen
    // scheduler has live tasks. Call from the main loop of the corresponding core.
    auto tick(std::size_t core_id) -> bool {
        return (core_id == 0u) ? core0_.tick() : core1_.tick();
    }

    void set_time_source(SchedulerBase::TimeFn fn) noexcept {
        core0_.set_time_source(fn);
        core1_.set_time_source(fn);
    }

    // Direct access for tests / diagnostics.
    [[nodiscard]] auto core0() noexcept -> SchedType& { return core0_; }
    [[nodiscard]] auto core1() noexcept -> SchedType& { return core1_; }

   private:
    SchedType core0_;
    SchedType core1_;

    // Separate cache line from the two schedulers.
    alignas(64) TasSpinlock lock_{};
    std::size_t next_any_core_{0};
};

}  // namespace alloy::tasks
