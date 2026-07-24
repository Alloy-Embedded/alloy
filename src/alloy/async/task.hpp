// alloy::async::task — a heap-less C++20 coroutine task for the cooperative
// executor. The coroutine frame lives in a caller-provided task_storage<N>
// (embassy-style inline storage), never on the heap: the plain operator new is
// DELETED, so a task declared without a storage argument is a COMPILE ERROR and
// there is provably no heap-allocation path (verified: a full -nostdlib link on
// Cortex-M0+/M7 and Xtensa LX6 pulls in zero malloc/_sbrk/operator-new).
//
// Frame overflow (storage too small for the compiler's real frame) is caught at
// COMPILE TIME under -O1/-Os/-O2 by an __attribute__((error)) guard that folds
// away when the frame fits; at -O0/-Og it degrades to a runtime __builtin_trap
// on first spawn. No toolchain exposes a coroutine frame size as a constant
// expression (GCC has no __builtin_coro_size; Clang's is not constexpr), so a
// real static_assert is impossible — this attribute idiom is the portable
// substitute and honors alloy's "errors at compile time" contract.

#pragma once

#include <coroutine>
#include <cstddef>
#include <cstdint>

namespace alloy::async {

// Emitted only when the frame provably exceeds N; the error attribute then turns
// the surviving call into a hard compile error naming the fix. (Linux
// BUILD_BUG_ON idiom, portable across every GCC/Clang alloy targets.)
[[noreturn]] __attribute__((error(
    "alloy::async: coroutine frame does not fit its task_storage<N> — increase N "
    "(the instantiation backtrace above names the task)")))
void frame_does_not_fit();

// Diagnostic hook: the last frame size operator new saw. Read by the
// alloy-frame-audit tool / debug builds to size task_storage tightly.
inline std::size_t g_last_frame_size = 0;

// Inline frame storage for exactly one task. Bytes is the user's frame budget;
// the default is comfortably above the realistic 40–72 B range so beginners
// never pick a number. `in_use` guards against reusing storage under a live task.
template <std::size_t Bytes = 256>
struct task_storage {
    alignas(alignof(std::max_align_t)) unsigned char buf[Bytes];
    bool in_use = false;

    static constexpr std::size_t capacity = Bytes;
    void release() noexcept { in_use = false; }
};

// The coroutine return object. `spawn()` starts it (initial_suspend parks it at
// the opening brace); it suspends at every co_await and at the closing brace so
// the executor can observe done() and retire() it.
struct task {
    struct promise_type {
        // Back-pointer to the storage's in_use flag so the executor can release
        // it on retire(). C++ constructs the promise with the coroutine's own
        // arguments when a matching constructor exists, so we capture the flag
        // here without threading it through every call site.
        bool* storage_in_use = nullptr;

        template <std::size_t N, class... Rest>
        explicit promise_type(task_storage<N>& store, Rest&...) noexcept
            : storage_in_use(&store.in_use) {}

        // Frame allocates from the task_storage passed as the coroutine's FIRST
        // argument. Extra coroutine parameters bind to Rest&&... (lvalues).
        //
        // NOT noexcept on purpose: a noexcept operator new obliges a
        // get_return_object_on_allocation_failure() and makes the compiler emit
        // a null-check branch. We never return null (oversize traps), so the
        // plain form is leaner and honest.
        template <std::size_t N, class... Rest>
        void* operator new(std::size_t sz, task_storage<N>& store, Rest&&...) {
            g_last_frame_size = sz;
            // -O1+: `sz` folds to a literal, so an oversize frame is a known-true
            // constant and the error() call survives DCE → compile error.
            if (__builtin_constant_p(sz > N) && sz > N) {
                frame_does_not_fit();
            }
            // -O0/-Og backstop (guard above inactive there): honest hard fail.
            if (sz > N || store.in_use) {
                __builtin_trap();
            }
            store.in_use = true;
            return store.buf;
        }

        // Coroutine destroy() lands here; the frame is static storage, so there
        // is nothing to free. Lifecycle (marking the storage reusable) is the
        // executor's retire() job, paired with handle.destroy().
        void operator delete(void*) noexcept {}

        // DELETED: a task without a storage argument must not compile. This is
        // the proof there is no heap path — global ::operator new is also
        // suppressed because the promise declares its own operator new.
        static void* operator new(std::size_t) = delete;

        task get_return_object() noexcept {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { __builtin_trap(); }  // -fno-exceptions
    };

    std::coroutine_handle<promise_type> handle{};
};

}  // namespace alloy::async
