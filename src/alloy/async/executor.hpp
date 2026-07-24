// alloy::async::executor — the cooperative, heap-less coroutine runtime. It is
// the async sibling of alloy::scheduler (sched.hpp): tasks are coroutines, wakes
// come from ISRs, and everything runs on one stack with no preemption.
//
// Two wake sources:
//   - schedule(h): ISR-safe ready-queue push (drivers call it from their ISR via
//     an event; see awaiter.hpp). Resumes happen in THREAD context only.
//   - sleep timers: a co_await sleep(ms) registers a deadline; run_once() wakes
//     it when uptime_ms() passes the deadline.
//
// executor_core holds all the logic and is NON-templated so event/sleep awaiters
// can reach the running executor via executor_core::the() without knowing its
// queue size. executor<MaxReady> just supplies the ready-queue storage. A single
// instance is registered; constructing a second traps (most firmware has one).

#pragma once

#include <coroutine>
#include <cstddef>
#include <cstdint>

#include "alloy/arch/cpu.hpp"
#include "alloy/arch/irq.hpp"
#include "alloy/async/task.hpp"
#include "alloy/time.hpp"

namespace alloy::async {

// A pending sleep: an intrusive node the sleep awaiter owns on its coroutine
// frame and links into the executor's timer list while suspended.
struct timer_node {
    std::coroutine_handle<> waiter{};
    std::uint32_t deadline_ms = 0;
    timer_node* next = nullptr;
    bool armed = false;
};

class executor_core {
    std::coroutine_handle<>* ready_;
    std::size_t cap_, head_ = 0, tail_ = 0, count_ = 0;
    timer_node* timers_ = nullptr;

    static inline executor_core* instance_ = nullptr;

protected:
    executor_core(std::coroutine_handle<>* buf, std::size_t cap) : ready_(buf), cap_(cap) {
        if (instance_ != nullptr) {
            __builtin_trap();  // v1 supports a single executor instance
        }
        instance_ = this;
    }

public:
    ~executor_core() { instance_ = nullptr; }
    executor_core(const executor_core&) = delete;
    executor_core& operator=(const executor_core&) = delete;

    // The registered instance, for event/sleep awaiters. Traps if none exists.
    static executor_core& the() {
        if (instance_ == nullptr) {
            __builtin_trap();
        }
        return *instance_;
    }

    // ISR-safe wake: push a handle onto the ready queue. A full queue traps
    // (honest hard fail) rather than dropping the wake — size MaxReady correctly
    // (each live task is enqueued at most once, so >= task count cannot overflow).
    void schedule(std::coroutine_handle<> h) {
        auto tp = std::coroutine_handle<task::promise_type>::from_address(h.address());
        const arch::irq_state s = arch::irq_save();
        if (tp.promise().queued) {
            arch::irq_restore(s);
            return;  // already queued — idempotent (double-spawn / double-wake safe)
        }
        if (count_ >= cap_) {
            arch::irq_restore(s);
            __builtin_trap();  // ready-queue overflow — raise MaxReady
        }
        tp.promise().queued = true;
        ready_[tail_] = h;
        tail_ = (tail_ + 1) % cap_;
        ++count_;
        arch::irq_restore(s);
    }

    // Register a sleep timer (called by the sleep awaiter, thread context — the
    // timer list is touched only from thread context, so it needs no mask).
    void arm_timer(timer_node& t) {
        t.next = timers_;
        t.armed = true;
        timers_ = &t;
    }

    // Unlink a timer early (a sleep awaiter destroyed before its deadline, e.g.
    // its task was destroyed mid-sleep). No-op if already fired/unlinked.
    void cancel_timer(timer_node& t) {
        if (!t.armed) {
            return;
        }
        for (timer_node** link = &timers_; *link != nullptr; link = &(*link)->next) {
            if (*link == &t) {
                *link = t.next;
                break;
            }
        }
        t.armed = false;
    }

    // Start a task: initial_suspend parked it at the opening brace, so enqueue
    // its handle for the first resume.
    void spawn(const task& t) { schedule(t.handle); }

    // One superstep (THREAD context only): wake elapsed timers, then resume every
    // ready handle once, retiring any that finished. Returns handles resumed.
    std::size_t run_once() {
        wake_expired_timers(alloy::uptime_ms());
        std::size_t ran = 0;
        for (;;) {
            std::coroutine_handle<> h{};
            const arch::irq_state s = arch::irq_save();
            if (count_ > 0) {
                h = ready_[head_];
                head_ = (head_ + 1) % cap_;
                --count_;
                // Clear queued so a wake that arrives after resume can re-enqueue.
                std::coroutine_handle<task::promise_type>::from_address(h.address())
                    .promise()
                    .queued = false;
            }
            arch::irq_restore(s);
            if (!h) {
                break;
            }
            h.resume();
            ++ran;
            if (h.done()) {
                retire(h);
            }
        }
        return ran;
    }

    // On-target forever loop: run a superstep, then sleep until the next ISR.
    // WFI/WAITI returns immediately if a wake is already pending, so no wake is
    // lost in the window between run_once() and idle().
    [[noreturn]] void run() {
        for (;;) {
            run_once();
            arch::idle();
        }
    }

    [[nodiscard]] std::size_t ready_count() const { return count_; }

private:
    // Destroy a finished coroutine and mark its storage reusable together (read
    // the flag before destroy() invalidates the frame; the flag lives in the
    // task_storage, which is separate memory and stays valid).
    static void retire(std::coroutine_handle<> h) {
        auto tp = std::coroutine_handle<task::promise_type>::from_address(h.address());
        bool* in_use = tp.promise().storage_in_use;
        h.destroy();
        if (in_use != nullptr) {
            *in_use = false;
        }
    }

    void wake_expired_timers(std::uint32_t now) {
        timer_node** link = &timers_;
        while (*link != nullptr) {
            timer_node* t = *link;
            // Wrap-safe "now has reached deadline": the unsigned difference read
            // as signed is >= 0 once now is at/past the deadline, negative while
            // still pending (correct across a counter wrap for deadlines < 2^31).
            if (t->armed && static_cast<std::int32_t>(now - t->deadline_ms) >= 0) {
                *link = t->next;  // unlink before waking — the task may re-arm it
                t->armed = false;
                auto w = t->waiter;
                t->waiter = {};
                if (w) {
                    schedule(w);
                }
            } else {
                link = &t->next;
            }
        }
    }
};

// Concrete executor: supplies the ready-queue storage. Pick MaxReady >= the
// number of concurrent tasks so schedule() can never overflow.
template <std::size_t MaxReady = 8>
class executor : public executor_core {
    std::coroutine_handle<> buf_[MaxReady]{};

public:
    executor() : executor_core(buf_, MaxReady) {}
};

}  // namespace alloy::async
