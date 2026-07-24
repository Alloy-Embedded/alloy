// Value-or-error return type — the fallible-call ergonomics every peripheral
// and service wants (`if (auto r = adc.read()) use(*r);`). A hand-rolled,
// -fno-exceptions-safe cousin of std::expected: it mirrors the surface
// (has_value / value_or / and_then / transform) but a misused checked value()
// __builtin_trap()s instead of throwing — the house guard (see the uart
// double-open trap). Honest-error convention matches the concepts: a bool is
// the v1 error on buses; Result carries a code where callers want the reason.
//
// `Result` keeps a capital R by request: the one deliberate exception to
// alloy's all-lowercase-type rule, so it reads like std::expected at the call.

#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

namespace alloy {

// Catch-all error code. A subsystem may pass its own enum as Result's E.
enum class error : std::uint8_t {
    ok = 0,
    fail,          // unspecified failure
    timeout,
    io,            // bus / transport error
    nack,          // no acknowledge (I2C / comm)
    again,         // not ready / would block
    invalid,       // bad argument or state
    unsupported,
    out_of_range,
    overflow,
    busy,
};

template <class T, class E = error>
class Result {
    static_assert(std::is_default_constructible_v<T>,
                  "Result<T> stores T by value; T must be default-constructible");
    T value_{};
    E error_{};
    bool has_{false};

public:
    using value_type = T;
    using error_type = E;

    constexpr Result(const T& v) : value_(v), has_(true) {}        // NOLINT: implicit by design
    constexpr Result(T&& v) : value_(std::move(v)), has_(true) {}  // NOLINT
    constexpr Result(E e) : error_(e) {}                           // NOLINT

    [[nodiscard]] constexpr bool has_value() const { return has_; }
    [[nodiscard]] constexpr explicit operator bool() const { return has_; }
    [[nodiscard]] constexpr E error() const { return error_; }

    // Unchecked access (std::expected style) — undefined if !has_value().
    [[nodiscard]] constexpr const T& operator*() const { return value_; }
    [[nodiscard]] constexpr T& operator*() { return value_; }
    [[nodiscard]] constexpr const T* operator->() const { return &value_; }
    [[nodiscard]] constexpr T* operator->() { return &value_; }

    // Checked access — traps (never throws) on the error path.
    [[nodiscard]] constexpr const T& value() const {
        if (!has_) { __builtin_trap(); }
        return value_;
    }
    [[nodiscard]] constexpr T& value() {
        if (!has_) { __builtin_trap(); }
        return value_;
    }

    template <class U>
    [[nodiscard]] constexpr T value_or(U&& fallback) const {
        return has_ ? value_ : static_cast<T>(std::forward<U>(fallback));
    }

    // Monadic: f(T) -> Result<U,E>; the error propagates unchanged.
    template <class F>
    [[nodiscard]] constexpr auto and_then(F&& f) const {
        using R = std::invoke_result_t<F, const T&>;
        return has_ ? f(value_) : R(error_);
    }
    // Monadic: f(T) -> U; wraps into Result<U,E>, error propagates.
    template <class F>
    [[nodiscard]] constexpr auto transform(F&& f) const {
        using U = std::invoke_result_t<F, const T&>;
        return has_ ? Result<U, E>(f(value_)) : Result<U, E>(error_);
    }
};

// void payload: success-or-error, no value.
template <class E>
class Result<void, E> {
    E error_{};
    bool has_{true};

public:
    using value_type = void;
    using error_type = E;

    constexpr Result() = default;                        // success
    constexpr Result(E e) : error_(e), has_(false) {}    // NOLINT

    [[nodiscard]] constexpr bool has_value() const { return has_; }
    [[nodiscard]] constexpr explicit operator bool() const { return has_; }
    [[nodiscard]] constexpr E error() const { return error_; }
};

// Reads well at return sites: `return alloy::ok();` / `return alloy::error::nack;`.
template <class E = error>
[[nodiscard]] constexpr Result<void, E> ok() {
    return {};
}

}  // namespace alloy
