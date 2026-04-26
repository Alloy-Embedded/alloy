#pragma once

#include <atomic>
#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "runtime/time.hpp"

namespace alloy::runtime::event {

template <typename Tag>
class completion {
   public:
    using tag_type = Tag;

    static auto signal() -> void {
#if ALLOY_SINGLE_CORE
        state().signaled.store(true, std::memory_order_relaxed);
        std::atomic_signal_fence(std::memory_order_release);
#else
        state().signaled.store(true, std::memory_order_release);
#endif
    }

    static auto reset() -> void { state().signaled.store(false, std::memory_order_relaxed); }

    [[nodiscard]] static auto ready() -> bool {
#if ALLOY_SINGLE_CORE
        std::atomic_signal_fence(std::memory_order_acquire);
        return state().signaled.load(std::memory_order_relaxed);
#else
        return state().signaled.load(std::memory_order_acquire);
#endif
    }

    template <typename TimeSource>
    [[nodiscard]] static auto wait_until(time::Deadline deadline)
        -> core::Result<void, core::ErrorCode> {
        while (!ready()) {
            if (deadline.expired(TimeSource::now())) {
                return core::Err(core::ErrorCode::Timeout);
            }
        }
        return core::Ok();
    }

    template <typename TimeSource>
    [[nodiscard]] static auto wait_for(time::Duration duration)
        -> core::Result<void, core::ErrorCode> {
        return wait_until<TimeSource>(TimeSource::deadline_after(duration));
    }

   private:
    struct state_type {
        std::atomic<bool> signaled{false};
    };

    [[nodiscard]] static auto state() -> state_type& {
        static state_type value{};
        return value;
    }
};

template <auto Value>
struct tag_constant {
    static constexpr auto value = Value;
};

}  // namespace alloy::runtime::event
