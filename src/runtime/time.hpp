#pragma once

#include <cstdint>

#include "hal/systick.hpp"

namespace alloy::runtime::time {

class Duration {
   public:
    constexpr Duration() = default;

    [[nodiscard]] static constexpr auto from_micros(std::uint64_t micros) -> Duration {
        return Duration{micros};
    }

    [[nodiscard]] static constexpr auto from_millis(std::uint64_t millis) -> Duration {
        return Duration{millis * 1000u};
    }

    [[nodiscard]] static constexpr auto from_seconds(std::uint64_t seconds) -> Duration {
        return Duration{seconds * 1000u * 1000u};
    }

    [[nodiscard]] constexpr auto as_micros() const -> std::uint64_t { return micros_; }

    [[nodiscard]] constexpr auto as_millis() const -> std::uint64_t { return micros_ / 1000u; }

    [[nodiscard]] constexpr auto is_zero() const -> bool { return micros_ == 0u; }

    [[nodiscard]] friend constexpr auto operator<=>(Duration, Duration) = default;

    [[nodiscard]] friend constexpr auto operator+(Duration lhs, Duration rhs) -> Duration {
        return Duration{lhs.micros_ + rhs.micros_};
    }

    [[nodiscard]] friend constexpr auto operator-(Duration lhs, Duration rhs) -> Duration {
        return Duration{lhs.micros_ > rhs.micros_ ? (lhs.micros_ - rhs.micros_) : 0u};
    }

   private:
    explicit constexpr Duration(std::uint64_t micros) : micros_(micros) {}

    std::uint64_t micros_ = 0u;
};

class Instant {
   public:
    constexpr Instant() = default;

    [[nodiscard]] static constexpr auto from_micros(std::uint64_t micros) -> Instant {
        return Instant{micros};
    }

    [[nodiscard]] constexpr auto as_micros() const -> std::uint64_t { return micros_; }

    [[nodiscard]] constexpr auto elapsed_since(Instant earlier) const -> Duration {
        return Duration::from_micros(micros_ - earlier.micros_);
    }

    [[nodiscard]] friend constexpr auto operator<=>(Instant, Instant) = default;

    [[nodiscard]] friend constexpr auto operator+(Instant instant, Duration duration) -> Instant {
        return Instant{instant.micros_ + duration.as_micros()};
    }

    [[nodiscard]] friend constexpr auto operator-(Instant instant, Duration duration) -> Instant {
        return Instant{instant.micros_ > duration.as_micros() ? (instant.micros_ - duration.as_micros())
                                                              : 0u};
    }

    [[nodiscard]] friend constexpr auto operator-(Instant lhs, Instant rhs) -> Duration {
        return lhs.elapsed_since(rhs);
    }

   private:
    explicit constexpr Instant(std::uint64_t micros) : micros_(micros) {}

    std::uint64_t micros_ = 0u;
};

class Deadline {
   public:
    constexpr Deadline() = default;

    [[nodiscard]] static constexpr auto at(Instant instant) -> Deadline { return Deadline{instant}; }

    [[nodiscard]] constexpr auto instant() const -> Instant { return instant_; }

    [[nodiscard]] constexpr auto expired(Instant now) const -> bool { return now >= instant_; }

    [[nodiscard]] constexpr auto remaining(Instant now) const -> Duration {
        return expired(now) ? Duration{} : (instant_ - now);
    }

   private:
    explicit constexpr Deadline(Instant instant) : instant_(instant) {}

    Instant instant_{};
};

template <typename TickSource>
class source {
   public:
    using tick_source_type = TickSource;

    [[nodiscard]] static auto now() -> Instant {
        return Instant::from_micros(hal::SysTickTimer::micros<TickSource>());
    }

    [[nodiscard]] static auto uptime() -> Duration {
        return Duration::from_micros(hal::SysTickTimer::micros<TickSource>());
    }

    [[nodiscard]] static auto deadline_after(Duration duration) -> Deadline {
        return Deadline::at(now() + duration);
    }

    [[nodiscard]] static auto expired(Deadline deadline) -> bool { return deadline.expired(now()); }

    [[nodiscard]] static auto remaining(Deadline deadline) -> Duration {
        return deadline.remaining(now());
    }

    static auto sleep_for(Duration duration) -> void {
        hal::SysTickTimer::delay_us<TickSource>(static_cast<std::uint32_t>(duration.as_micros()));
    }

    static auto sleep_until(Deadline deadline) -> void {
        while (!deadline.expired(now())) {
        }
    }
};

}  // namespace alloy::runtime::time
