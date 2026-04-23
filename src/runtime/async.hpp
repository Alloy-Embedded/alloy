#pragma once

#include <cstdint>
#include <utility>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "time.hpp"

namespace alloy::runtime::async {

enum class poll_status : std::uint8_t {
    pending = 0u,
    ready = 1u,
};

template <typename CompletionToken>
class operation {
   public:
    using completion_type = CompletionToken;

    [[nodiscard]] auto ready() const -> bool { return completion_type::ready(); }

    [[nodiscard]] auto poll() const -> poll_status {
        return ready() ? poll_status::ready : poll_status::pending;
    }

    template <typename TimeSource>
    [[nodiscard]] auto wait_until(time::Deadline deadline) const
        -> core::Result<void, core::ErrorCode> {
        return completion_type::template wait_until<TimeSource>(deadline);
    }

    template <typename TimeSource>
    [[nodiscard]] auto wait_for(time::Duration duration) const
        -> core::Result<void, core::ErrorCode> {
        return completion_type::template wait_for<TimeSource>(duration);
    }

    template <typename Callback>
    auto notify_if_ready(Callback&& callback) const -> bool {
        if (!ready()) {
            return false;
        }
        std::forward<Callback>(callback)();
        return true;
    }
};

template <typename CompletionToken>
[[nodiscard]] constexpr auto bind() -> operation<CompletionToken> {
    return operation<CompletionToken>{};
}

}  // namespace alloy::runtime::async
