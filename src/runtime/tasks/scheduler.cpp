// Cooperative coroutine scheduler implementation. See scheduler.hpp for the
// public surface.

#include "runtime/tasks/scheduler.hpp"

namespace alloy::tasks {

// --- thread_local pool routing ----------------------------------------------
//
// The promise's operator new/delete need a pool to allocate from. The scheduler
// installs the pointers right before calling the coroutine factory and clears
// them after; outside that window, allocation fails fast (returns nullptr,
// which triggers get_return_object_on_allocation_failure).
//
// `thread_local` is not strictly necessary today (cooperative single-thread per
// scheduler) but it future-proofs against running multiple schedulers in
// preemptive contexts (e.g. one per FreeRTOS task on ESP32). With one pool per
// scheduler, the thread-local is a valid handle at any instant.

namespace {
thread_local void* g_pool = nullptr;
thread_local std::size_t g_slot = 0;
thread_local void* g_alloc_fn = nullptr;
thread_local void* g_free_fn = nullptr;
}  // namespace

auto TaskPromise::active_pool() noexcept -> void*& { return g_pool; }
auto TaskPromise::active_slot_size() noexcept -> std::size_t& { return g_slot; }
auto TaskPromise::active_alloc_fn() noexcept -> void*& { return g_alloc_fn; }
auto TaskPromise::active_free_fn() noexcept -> void*& { return g_free_fn; }

// --- spawn -------------------------------------------------------------------

auto SchedulerBase::spawn_impl(Task&& task, Priority prio, CancellationToken token)
    -> core::Result<TaskId, SpawnError> {
    if (!task.valid()) {
        // get_return_object_on_allocation_failure left handle empty.
        // The pool's last_oversize_request lets us tell which kind of failure
        // it was: oversize (one slot rejected for size) vs full (every slot
        // taken).
        return core::Err(last_oversize_request() > 0 ? SpawnError::FrameTooLarge
                                                      : SpawnError::PoolFull);
    }

    auto handle = task.release();
    auto& promise = handle.promise();
    promise.sched = this;
    promise.priority = prio;
    promise.state = TaskPromise::State::Ready;
    promise.token = token;

    // Find a free slot in the task table.
    auto* slots = slots_span();
    for (std::size_t i = 0; i < max_tasks_; ++i) {
        if (!slots[i].occupied) {
            slots[i].handle = handle;
            slots[i].occupied = true;
            return core::Ok(TaskId{static_cast<std::uint16_t>(i)});
        }
    }
    // Pool let us in but the slot table is full; destroy the just-allocated
    // coroutine to avoid leaking the frame.
    handle.destroy();
    return core::Err(SpawnError::SlotTableFull);
}

// --- tick --------------------------------------------------------------------
//
// One pass: scan blocked tasks, mark ready those whose condition is satisfied,
// pick the highest-priority ready task, resume it once. Returns true while at
// least one task is still occupied.

auto SchedulerBase::tick() -> bool {
    if (stopped_) return false;
    auto* slots = slots_span();
    const auto current = now();

    // 1. Wake any waiter whose condition is satisfied. Cancellation also
    //    counts as a wake-up; the awaiter inspects `token_observed_cancel`
    //    on resume and returns Cancelled.
    for (std::size_t i = 0; i < max_tasks_; ++i) {
        auto& slot = slots[i];
        if (!slot.occupied) continue;
        auto& promise = slot.handle.promise();
        const bool cancelled = promise.token.requested();
        if (promise.state == TaskPromise::State::WaitingDelay) {
            const bool deadline_passed = !(current < promise.wake_at);
            if (cancelled) promise.token_observed_cancel = true;
            if (deadline_passed || cancelled) {
                promise.state = TaskPromise::State::Ready;
            }
        } else if (promise.state == TaskPromise::State::WaitingEvent) {
            const bool fired =
                promise.pending_event != nullptr && promise.pending_event->consume();
            if (cancelled) promise.token_observed_cancel = true;
            if (fired || cancelled) {
                promise.state = TaskPromise::State::Ready;
                promise.pending_event = nullptr;
            }
        }
    }

    // 2. Find the highest-priority Ready task. Within a level, FIFO == lowest
    //    slot index that holds a ready task at that level.
    int chosen = -1;
    auto best_prio = Priority::Idle;
    bool any_occupied = false;
    for (std::size_t i = 0; i < max_tasks_; ++i) {
        auto& slot = slots[i];
        if (!slot.occupied) continue;
        any_occupied = true;
        auto& promise = slot.handle.promise();
        if (promise.state != TaskPromise::State::Ready) continue;
        if (chosen == -1 || to_index(promise.priority) < to_index(best_prio)) {
            chosen = static_cast<int>(i);
            best_prio = promise.priority;
        }
    }

    // 3. Reap any tasks that finished on their previous resume.
    for (std::size_t i = 0; i < max_tasks_; ++i) {
        auto& slot = slots[i];
        if (!slot.occupied) continue;
        if (slot.handle.promise().state == TaskPromise::State::Done) {
            slot.handle.destroy();
            slot.handle = {};
            slot.occupied = false;
        }
    }

    // 4. Resume the chosen task. Its co_await will move it back to a Waiting
    //    state, or final_suspend will set it to Done; either way the next tick
    //    picks the next thing.
    if (chosen >= 0) {
        slots[chosen].handle.resume();
        return true;
    }

    // No ready task this pass. If anything is still occupied (waiting), keep
    // ticking; otherwise the run loop is done.
    return any_occupied;
}

// --- awaiters ---------------------------------------------------------------

void DelayAwaiter::await_suspend(std::coroutine_handle<TaskPromise> h) noexcept {
    auto& p = h.promise();
    promise_ = &p;
    if (p.token.requested()) {
        // Already cancelled at the moment of suspension. Skip the wait; stay
        // Ready so tick() resumes us on the next pass and await_resume sees
        // the flag.
        p.token_observed_cancel = true;
        return;
    }
    p.state = TaskPromise::State::WaitingDelay;
    p.wake_at = p.sched->now() + duration_;
}

void YieldAwaiter::await_suspend(std::coroutine_handle<TaskPromise> h) noexcept {
    // yield_now is just "stay Ready". The scheduler picks next-best at the next
    // tick, which respects priority + FIFO-by-slot-index.
    h.promise().state = TaskPromise::State::Ready;
}

void OnEventAwaiter::await_suspend(std::coroutine_handle<TaskPromise> h) noexcept {
    auto& p = h.promise();
    promise_ = &p;
    if (p.token.requested()) {
        // Already cancelled at the moment of suspension.
        p.token_observed_cancel = true;
        return;
    }
    p.state = TaskPromise::State::WaitingEvent;
    p.pending_event = event_;
}

}  // namespace alloy::tasks
