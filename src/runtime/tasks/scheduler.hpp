#pragma once

// Cooperative coroutine scheduler. See openspec/specs/runtime-tasks/spec.md for
// the contract; this header is the user-facing entry point.
//
// Single-stack, single-threaded per instance. Tasks switch only at co_await
// points the user can see. Four static priorities, FIFO within a level. Pool-
// allocated coroutine frames -- no heap.
//
// v1 intentionally narrows scope:
//   - Task return type is void only (a future change adds Task<T>).
//   - Awaiters: delay, yield_now. (any_of, all_of, on(event), until(predicate)
//     come in follow-up commits per tasks.md.)
//   - cancellation_token is present as a primitive but only awaiters opt in
//     to check it; the chained-token machinery is a follow-up.

#include <array>
#include <atomic>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

#include "core/result.hpp"
#include "runtime/tasks/event.hpp"
#include "runtime/tasks/pool.hpp"
#include "runtime/tasks/priority.hpp"
#include "runtime/time.hpp"

namespace alloy::tasks {

// --- forward declarations ----------------------------------------------------

class SchedulerBase;
struct TaskPromise;
class Task;

// --- ID + error types --------------------------------------------------------

struct TaskId {
    std::uint16_t index = 0xFFFFu;
    [[nodiscard]] constexpr auto valid() const noexcept -> bool { return index != 0xFFFFu; }
};

enum class SpawnError : std::uint8_t {
    PoolFull,        // every frame slot is occupied
    FrameTooLarge,   // requested size > BytesPerSlot
    SlotTableFull,   // pool had room but the scheduler's task table is saturated
};

enum class Cancelled : std::uint8_t { Yes };

// --- cancellation token (v1: simple atomic flag) -----------------------------

class CancellationToken {
   public:
    /// Default-constructed token is a "no-op" -- never reports cancellation.
    /// Use `CancellationToken::make()` to obtain a real token.
    constexpr CancellationToken() noexcept = default;
    explicit constexpr CancellationToken(std::atomic<bool>* f) noexcept : flag_(f) {}

    void request() noexcept {
        if (flag_) flag_->store(true, std::memory_order_release);
    }
    [[nodiscard]] auto requested() const noexcept -> bool {
        return flag_ && flag_->load(std::memory_order_acquire);
    }

    static auto make() -> CancellationToken {
        // Tokens are POD-like handles to a flag stored in static storage. Each
        // call hands out the next slot (round-robin). The cursor is plain
        // size_t rather than atomic because Cortex-M0+ has no hardware
        // atomic-add and the scheduler is single-threaded by design -- ISRs
        // call `request()` on an existing token, never `make()`.
        static std::array<std::atomic<bool>, 16> flags{};
        static std::size_t next = 0;
        const auto idx = next++ % flags.size();
        flags[idx].store(false, std::memory_order_relaxed);
        return CancellationToken{&flags[idx]};
    }

   private:
    std::atomic<bool>* flag_ = nullptr;
};

// --- promise type ------------------------------------------------------------

struct TaskPromise {
    enum class State : std::uint8_t {
        Ready,
        WaitingDelay,
        WaitingEvent,
        WaitingPredicate,
        Done,
    };

    SchedulerBase* sched = nullptr;
    Priority priority = Priority::Normal;
    State state = State::Ready;
    runtime::time::Instant wake_at{};
    Event* pending_event = nullptr;  // valid when state == WaitingEvent
    // Predicate poll closure (valid when state == WaitingPredicate). The
    // scheduler calls predicate_poll(predicate_data) each tick; on true the
    // task transitions back to Ready. The closure object lives in the
    // coroutine frame, so the lifetime is automatically tied to the await.
    using PredicatePoll = bool (*)(void*) noexcept;
    PredicatePoll predicate_poll = nullptr;
    void* predicate_data = nullptr;
    CancellationToken token = CancellationToken{nullptr};
    bool token_observed_cancel = false;

    // operator new/delete are routed through a thread_local pool pointer the
    // scheduler installs around `spawn`. Anywhere else, allocation fails.
    static auto active_pool() noexcept -> void*&;
    static auto active_slot_size() noexcept -> std::size_t&;

    [[nodiscard]] auto get_return_object() noexcept -> Task;
    auto initial_suspend() noexcept -> std::suspend_always { return {}; }
    auto final_suspend() noexcept -> std::suspend_always {
        state = State::Done;
        return {};
    }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}  // -fno-exceptions: never reached

    static auto get_return_object_on_allocation_failure() noexcept -> Task;

    void* operator new(std::size_t size) noexcept {
        auto*& pool = active_pool();
        if (pool == nullptr) return nullptr;  // not in a spawn frame
        // Let the pool's allocate decide. It records oversize requests in
        // `last_oversize_request_` so the scheduler can tell `FrameTooLarge`
        // apart from `PoolFull` after `get_return_object_on_allocation_failure`
        // produces an empty `Task`.
        using AllocFn = void* (*)(void*, std::size_t);
        auto* alloc_fn = reinterpret_cast<AllocFn>(active_alloc_fn());
        return alloc_fn ? alloc_fn(pool, size) : nullptr;
    }

    void operator delete(void* p) noexcept {
        auto*& pool = active_pool();
        using FreeFn = void (*)(void*, void*);
        auto* free_fn = reinterpret_cast<FreeFn>(active_free_fn());
        if (free_fn && p) free_fn(pool, p);
    }

    static auto active_alloc_fn() noexcept -> void*&;
    static auto active_free_fn() noexcept -> void*&;
};

// --- Task<void> --------------------------------------------------------------

class Task {
   public:
    using promise_type = TaskPromise;

    Task() = default;
    explicit Task(std::coroutine_handle<TaskPromise> h) noexcept : handle_(h) {}

    Task(const Task&) = delete;
    auto operator=(const Task&) -> Task& = delete;

    Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
    auto operator=(Task&& other) noexcept -> Task& {
        if (this != &other) {
            destroy();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    ~Task() { destroy(); }

    [[nodiscard]] auto handle() const noexcept -> std::coroutine_handle<TaskPromise> {
        return handle_;
    }

    [[nodiscard]] auto valid() const noexcept -> bool { return handle_ != nullptr; }

    /// Detach the underlying coroutine_handle so the scheduler owns destruction.
    auto release() noexcept -> std::coroutine_handle<TaskPromise> {
        return std::exchange(handle_, {});
    }

   private:
    void destroy() noexcept {
        if (handle_) handle_.destroy();
        handle_ = {};
    }

    std::coroutine_handle<TaskPromise> handle_{};
};

inline auto TaskPromise::get_return_object() noexcept -> Task {
    return Task{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}
inline auto TaskPromise::get_return_object_on_allocation_failure() noexcept -> Task {
    return Task{};
}

// --- scheduler base (type-erased; the templated Scheduler holds the pool) ----

class SchedulerBase {
   public:
    SchedulerBase(const SchedulerBase&) = delete;
    auto operator=(const SchedulerBase&) -> SchedulerBase& = delete;

    /// Drive the scheduler one pass: scan tasks for ready transitions, resume
    /// the highest-priority ready task. Returns true while there is at least
    /// one undone task; the trivial `run()` loop calls `tick()` until false.
    auto tick() -> bool;

    /// Trivial loop. Returns when no task is left.
    void run() {
        while (tick()) {
        }
    }

    /// Stop the run loop; the next `tick` returns false.
    void request_stop() noexcept { stopped_ = true; }

    /// Time source. Tests can swap this for a mock; production uses
    /// runtime::time::Instant::now().
    using TimeFn = runtime::time::Instant (*)();
    void set_time_source(TimeFn fn) noexcept { time_fn_ = fn; }

   protected:
    explicit SchedulerBase(std::size_t max_tasks) noexcept : max_tasks_(max_tasks) {}
    ~SchedulerBase() = default;

    struct Slot {
        std::coroutine_handle<TaskPromise> handle{};
        bool occupied = false;
    };

    [[nodiscard]] virtual auto pool_alloc_fn() noexcept -> void* = 0;
    [[nodiscard]] virtual auto pool_free_fn() noexcept -> void* = 0;
    [[nodiscard]] virtual auto pool_ptr() noexcept -> void* = 0;
    [[nodiscard]] virtual auto pool_slot_bytes() const noexcept -> std::size_t = 0;
    [[nodiscard]] virtual auto last_oversize_request() const noexcept -> std::size_t = 0;
    [[nodiscard]] virtual auto slots_span() noexcept -> Slot* = 0;

    auto spawn_impl(Task&& task, Priority prio, CancellationToken token)
        -> core::Result<TaskId, SpawnError>;

    /// Current monotonic time per the configured source. Returns Instant{} (0)
    /// when no source has been installed; in that case `delay(...)` resolves
    /// on the very next tick, which is fine for tests that do not actually
    /// care about timing. Real applications must call `set_time_source(...)`
    /// with a board-provided function (typically wrapping
    /// `runtime::time::source<BoardSysTick>::now()`).
    auto now() const noexcept -> runtime::time::Instant {
        return time_fn_ ? time_fn_() : runtime::time::Instant{};
    }

    bool stopped_ = false;
    std::size_t max_tasks_;
    TimeFn time_fn_ = nullptr;

    friend struct TaskPromise;
    friend class DelayAwaiter;
    friend class YieldAwaiter;
};

// --- templated scheduler -----------------------------------------------------

template <std::size_t MaxTasks, std::size_t MaxFrameBytes>
class Scheduler final : public SchedulerBase {
   public:
    using Pool = FramePool<MaxTasks, MaxFrameBytes>;

    Scheduler() noexcept : SchedulerBase(MaxTasks) {}

    /// Spawn a task by calling a user-supplied factory while the scheduler's
    /// pool is installed. The factory is typically a lambda that calls a
    /// coroutine function: `sched.spawn([] { return blink(); }, priority::high)`.
    ///
    /// Spawning must take a factory rather than a pre-built `Task` because
    /// the C++ coroutine's `operator new` runs at the moment the coroutine
    /// function is called -- before any `spawn` body would otherwise execute.
    /// Installing the pool around the factory call is the only way the
    /// promise's allocator hook can find it.
    template <typename Fn>
    auto spawn(Fn&& factory, Priority prio = Priority::Normal,
               CancellationToken token = CancellationToken::make())
        -> core::Result<TaskId, SpawnError> {
        TaskPromise::active_pool() = &pool_;
        TaskPromise::active_slot_size() = MaxFrameBytes;
        TaskPromise::active_alloc_fn() = pool_alloc_fn();
        TaskPromise::active_free_fn() = pool_free_fn();
        Task task = factory();
        TaskPromise::active_pool() = nullptr;
        TaskPromise::active_slot_size() = 0;
        TaskPromise::active_alloc_fn() = nullptr;
        TaskPromise::active_free_fn() = nullptr;
        return spawn_impl(std::move(task), prio, token);
    }

    [[nodiscard]] auto pool() noexcept -> Pool& { return pool_; }

   protected:
    auto pool_alloc_fn() noexcept -> void* override {
        return reinterpret_cast<void*>(+[](void* p, std::size_t bytes) -> void* {
            return static_cast<Pool*>(p)->allocate(bytes);
        });
    }
    auto pool_free_fn() noexcept -> void* override {
        return reinterpret_cast<void*>(+[](void* p, void* ptr) {
            static_cast<Pool*>(p)->deallocate(ptr);
        });
    }
    auto pool_ptr() noexcept -> void* override { return &pool_; }
    auto pool_slot_bytes() const noexcept -> std::size_t override { return MaxFrameBytes; }
    auto last_oversize_request() const noexcept -> std::size_t override {
        return pool_.last_oversize_request();
    }
    auto slots_span() noexcept -> Slot* override { return slots_.data(); }

   private:
    Pool pool_;
    std::array<Slot, MaxTasks> slots_{};
};

// --- awaiters (declared here, defined in scheduler.cpp) ---------------------

class DelayAwaiter {
   public:
    explicit DelayAwaiter(runtime::time::Duration d) noexcept : duration_(d) {}
    [[nodiscard]] auto await_ready() const noexcept -> bool { return duration_.is_zero(); }
    void await_suspend(std::coroutine_handle<TaskPromise> h) noexcept;
    [[nodiscard]] auto await_resume() noexcept -> core::Result<void, Cancelled> {
        // The cancellation flag is owned by the promise so the scheduler can
        // toggle it from the outside while we're suspended; the awaiter just
        // observes it on resume.
        if (promise_ && promise_->token_observed_cancel) {
            promise_->token_observed_cancel = false;  // consume
            return core::Err(Cancelled::Yes);
        }
        return core::Ok();
    }

   private:
    runtime::time::Duration duration_;
    TaskPromise* promise_ = nullptr;
    friend class SchedulerBase;
};

class YieldAwaiter {
   public:
    [[nodiscard]] static auto await_ready() noexcept -> bool { return false; }
    void await_suspend(std::coroutine_handle<TaskPromise> h) noexcept;
    static void await_resume() noexcept {}
};

class OnEventAwaiter {
   public:
    explicit OnEventAwaiter(Event& e) noexcept : event_(&e) {}

    /// If the event is already signalled at await time, consume it and skip
    /// the suspension. Allows tight ISR -> task ping-pongs to stay efficient.
    [[nodiscard]] auto await_ready() noexcept -> bool { return event_->consume(); }

    void await_suspend(std::coroutine_handle<TaskPromise> h) noexcept;

    [[nodiscard]] auto await_resume() noexcept -> core::Result<void, Cancelled> {
        if (promise_ && promise_->token_observed_cancel) {
            promise_->token_observed_cancel = false;
            return core::Err(Cancelled::Yes);
        }
        return core::Ok();
    }

   private:
    Event* event_ = nullptr;
    TaskPromise* promise_ = nullptr;
    friend class SchedulerBase;
};

/// Suspend until a user-supplied predicate returns true. The scheduler calls
/// the predicate from the main loop on each tick; cheap, side-effect-free
/// callables are best (the predicate may be invoked many times before the
/// condition becomes true). The predicate runs in task / scheduler context,
/// not from any ISR.
class UntilAwaiter {
   public:
    using PollFn = bool (*)(void*) noexcept;

    UntilAwaiter(PollFn poll, void* data) noexcept : poll_(poll), data_(data) {}

    [[nodiscard]] auto await_ready() noexcept -> bool {
        // Short-circuit: if the predicate is already true, don't bother
        // suspending. The scheduler tick that observes the WaitingPredicate
        // state would do the same call anyway.
        return poll_ != nullptr && poll_(data_);
    }

    void await_suspend(std::coroutine_handle<TaskPromise> h) noexcept;

    [[nodiscard]] auto await_resume() noexcept -> core::Result<void, Cancelled> {
        if (promise_ && promise_->token_observed_cancel) {
            promise_->token_observed_cancel = false;
            return core::Err(Cancelled::Yes);
        }
        return core::Ok();
    }

   private:
    PollFn poll_ = nullptr;
    void* data_ = nullptr;
    TaskPromise* promise_ = nullptr;
    friend class SchedulerBase;
};

[[nodiscard]] inline auto delay(runtime::time::Duration d) noexcept -> DelayAwaiter {
    return DelayAwaiter{d};
}
[[nodiscard]] inline auto yield_now() noexcept -> YieldAwaiter { return {}; }
[[nodiscard]] inline auto on(Event& e) noexcept -> OnEventAwaiter { return OnEventAwaiter{e}; }

/// `until(predicate)` returns an awaiter that suspends until `predicate()`
/// returns true. The predicate is captured by reference; the user is
/// responsible for keeping its lifetime alive across the suspension. In
/// practice this means passing a lambda directly inside the co_await
/// expression, which the C++ coroutine machinery extends to the
/// suspend/resume cycle.
template <typename Predicate>
[[nodiscard]] auto until(Predicate&& pred) noexcept -> UntilAwaiter {
    using Decayed = std::remove_reference_t<Predicate>;
    static_assert(std::is_invocable_r_v<bool, Decayed&>,
                  "until() requires a callable returning bool");
    return UntilAwaiter{
        +[](void* p) noexcept -> bool { return (*static_cast<Decayed*>(p))(); },
        const_cast<void*>(static_cast<const void*>(std::addressof(pred))),
    };
}

}  // namespace alloy::tasks
