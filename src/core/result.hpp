/**
 * @file result.hpp
 * @brief Result<T, E> type for error handling without exceptions
 *
 * Provides a Rust-inspired Result type for embedded systems where exceptions
 * are often disabled. Result<T, E> represents either a success value (Ok) or
 * an error value (Err).
 *
 * Example:
 * @code
 * Result<int, Error> divide(int a, int b) {
 *     if (b == 0) {
 *         return Err(Error::invalid_argument("Division by zero"));
 *     }
 *     return Ok(a / b);
 * }
 *
 * auto result = divide(10, 2);
 * if (result.is_ok()) {
 *     int value = result.unwrap();
 * }
 * @endcode
 */

#pragma once

#include <cassert>      // assert
#include <cstdlib>      // std::abort
#include <type_traits>  // std::enable_if, std::is_same
#include <utility>      // std::move, std::forward

namespace alloy {
namespace core {

// Forward declarations
template <typename T, typename E>
class Result;

namespace detail {
// Tag types for Ok and Err construction
template <typename T>
struct OkTag {
    T value;

    explicit OkTag(T&& v) : value(std::forward<T>(v)) {}
    explicit OkTag(const T& v) : value(v) {}
};

// Specialization for void
template <>
struct OkTag<void> {
    OkTag() = default;
};

template <typename E>
struct ErrTag {
    E error;

    explicit ErrTag(E&& e) : error(std::forward<E>(e)) {}
    explicit ErrTag(const E& e) : error(e) {}
};
}  // namespace detail

/**
 * @brief Factory function to create an Ok result
 * @tparam T The value type
 * @param value The success value
 * @return OkTag wrapping the value
 */
template <typename T>
constexpr detail::OkTag<T> Ok(T&& value) {
    return detail::OkTag<T>(std::forward<T>(value));
}

/**
 * @brief Factory function to create an Ok result for void type
 * @return OkTag for void
 */
inline constexpr detail::OkTag<void> Ok() {
    return detail::OkTag<void>();
}

/**
 * @brief Factory function to create an Err result
 * @tparam E The error type
 * @param error The error value
 * @return ErrTag wrapping the error
 */
template <typename E>
constexpr detail::ErrTag<E> Err(E&& error) {
    return detail::ErrTag<E>(std::forward<E>(error));
}

/**
 * @brief Result type representing either success (Ok) or failure (Err)
 * @tparam T The success value type
 * @tparam E The error type
 *
 * Result is a sum type that contains either a value of type T (success)
 * or a value of type E (error). This provides a way to handle errors
 * without exceptions, which is ideal for embedded systems.
 *
 * Memory layout: Uses a union to store either T or E, plus a bool flag.
 * Size: sizeof(Result<T,E>) = max(sizeof(T), sizeof(E)) + sizeof(bool) + padding
 */
template <typename T, typename E>
class Result {
   public:
    // Type aliases
    using value_type = T;
    using error_type = E;

    // Constructors

    /**
     * @brief Construct an Ok result from OkTag
     */
    Result(detail::OkTag<T>&& ok) : is_ok_(true) { new (&storage_.value) T(std::move(ok.value)); }

    Result(const detail::OkTag<T>& ok) : is_ok_(true) { new (&storage_.value) T(ok.value); }

    /**
     * @brief Construct an Err result from ErrTag
     */
    Result(detail::ErrTag<E>&& err) : is_ok_(false) {
        new (&storage_.error) E(std::move(err.error));
    }

    Result(const detail::ErrTag<E>& err) : is_ok_(false) { new (&storage_.error) E(err.error); }

    /**
     * @brief Copy constructor
     */
    Result(const Result& other) : is_ok_(other.is_ok_) {
        if (is_ok_) {
            new (&storage_.value) T(other.storage_.value);
        } else {
            new (&storage_.error) E(other.storage_.error);
        }
    }

    /**
     * @brief Move constructor
     */
    Result(Result&& other) noexcept : is_ok_(other.is_ok_) {
        if (is_ok_) {
            new (&storage_.value) T(std::move(other.storage_.value));
        } else {
            new (&storage_.error) E(std::move(other.storage_.error));
        }
    }

    /**
     * @brief Copy assignment
     */
    Result& operator=(const Result& other) {
        if (this != &other) {
            destroy();
            is_ok_ = other.is_ok_;
            if (is_ok_) {
                new (&storage_.value) T(other.storage_.value);
            } else {
                new (&storage_.error) E(other.storage_.error);
            }
        }
        return *this;
    }

    /**
     * @brief Move assignment
     */
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            destroy();
            is_ok_ = other.is_ok_;
            if (is_ok_) {
                new (&storage_.value) T(std::move(other.storage_.value));
            } else {
                new (&storage_.error) E(std::move(other.storage_.error));
            }
        }
        return *this;
    }

    /**
     * @brief Destructor
     */
    ~Result() { destroy(); }

    // Query methods

    /**
     * @brief Check if result is Ok
     * @return true if contains a value, false if contains an error
     */
    constexpr bool is_ok() const noexcept { return is_ok_; }

    /**
     * @brief Check if result is Err
     * @return true if contains an error, false if contains a value
     */
    constexpr bool is_err() const noexcept { return !is_ok_; }

    /**
     * @brief Boolean conversion operator
     * @return true if Ok, false if Err
     */
    explicit operator bool() const noexcept { return is_ok(); }

    // Value access methods

    /**
     * @brief Unwrap the value, aborting if Err
     * @return Reference to the contained value
     * @note Calls abort() if result is Err. Use only when you're certain result is Ok.
     */
    T& unwrap() & {
        if (!is_ok_) {
            // In embedded systems, we abort instead of throwing
            std::abort();
        }
        return storage_.value;
    }

    const T& unwrap() const& {
        if (!is_ok_) {
            std::abort();
        }
        return storage_.value;
    }

    T&& unwrap() && {
        if (!is_ok_) {
            std::abort();
        }
        return std::move(storage_.value);
    }

    /**
     * @brief Unwrap the value or return a default
     * @param default_value Value to return if result is Err
     * @return The contained value or the default
     */
    T unwrap_or(T default_value) const& { return is_ok_ ? storage_.value : default_value; }

    T unwrap_or(T default_value) && { return is_ok_ ? std::move(storage_.value) : default_value; }

    /**
     * @brief Unwrap with a custom panic message
     * @param msg Message to display before aborting (unused in release)
     * @return Reference to the contained value
     */
    T& expect(const char* msg) & {
        if (!is_ok_) {
            (void)msg;  // Unused in release builds
            std::abort();
        }
        return storage_.value;
    }

    const T& expect(const char* msg) const& {
        if (!is_ok_) {
            (void)msg;
            std::abort();
        }
        return storage_.value;
    }

    // Error access methods

    /**
     * @brief Get the error value (alias for unwrap_err)
     * @return Reference to the contained error
     * @note Calls abort() if result is Ok. Use only when you're certain result is Err.
     */
    E& err() & {
        if (is_ok_) {
            std::abort();
        }
        return storage_.error;
    }

    const E& err() const& {
        if (is_ok_) {
            std::abort();
        }
        return storage_.error;
    }

    E&& err() && {
        if (is_ok_) {
            std::abort();
        }
        return std::move(storage_.error);
    }

    /**
     * @brief Unwrap the error, aborting if Ok
     * @return Reference to the contained error
     */
    E& unwrap_err() & {
        if (is_ok_) {
            std::abort();
        }
        return storage_.error;
    }

    const E& unwrap_err() const& {
        if (is_ok_) {
            std::abort();
        }
        return storage_.error;
    }

    E&& unwrap_err() && {
        if (is_ok_) {
            std::abort();
        }
        return std::move(storage_.error);
    }

    // Monadic operations

    /**
     * @brief Map the value if Ok
     * @tparam F Function type T -> U
     * @param fn Function to apply to the value
     * @return Result<U, E> with mapped value or original error
     *
     * Example:
     * @code
     * Result<int, Error> r = Ok(5);
     * auto r2 = r.map([](int x) { return x * 2; }); // Ok(10)
     * @endcode
     */
    template <typename F>
    auto map(F&& fn) const& -> Result<decltype(fn(std::declval<T>())), E> {
        using U = decltype(fn(std::declval<T>()));
        if (is_ok_) {
            return Ok(fn(storage_.value));
        } else {
            return Result<U, E>(Err(E(storage_.error)));
        }
    }

    template <typename F>
    auto map(F&& fn) && -> Result<decltype(fn(std::declval<T>())), E> {
        using U = decltype(fn(std::declval<T>()));
        if (is_ok_) {
            return Ok(fn(std::move(storage_.value)));
        } else {
            return Result<U, E>(Err(std::move(storage_.error)));
        }
    }

    /**
     * @brief Chain operations that return Result
     * @tparam F Function type T -> Result<U, E>
     * @param fn Function to apply to the value
     * @return Result<U, E> from function or original error
     *
     * Example:
     * @code
     * auto divide = [](int x) -> Result<int, Error> {
     *     if (x == 0) return Err(Error::invalid_argument("div by zero"));
     *     return Ok(100 / x);
     * };
     * Result<int, Error> r = Ok(5);
     * auto r2 = r.and_then(divide); // Ok(20)
     * @endcode
     */
    template <typename F>
    auto and_then(F&& fn) const& -> decltype(fn(std::declval<T>())) {
        using ResultType = decltype(fn(std::declval<T>()));
        if (is_ok_) {
            return fn(storage_.value);
        } else {
            return ResultType(Err(E(storage_.error)));
        }
    }

    template <typename F>
    auto and_then(F&& fn) && -> decltype(fn(std::declval<T>())) {
        using ResultType = decltype(fn(std::declval<T>()));
        if (is_ok_) {
            return fn(std::move(storage_.value));
        } else {
            return ResultType(Err(std::move(storage_.error)));
        }
    }

    /**
     * @brief Recover from error by applying a function
     * @tparam F Function type E -> Result<T, F>
     * @param fn Function to apply to the error, must return Result<T, F>
     * @return Result<T, F> from function or original value
     *
     * Example:
     * @code
     * Result<int, Error> r = Err(Error::timeout());
     * auto r2 = r.or_else([](Error e) {
     *     // Try to recover from error
     *     return Ok(default_value);
     * });
     * @endcode
     */
    template <typename F>
    auto or_else(F&& fn) const& -> decltype(fn(std::declval<E>())) {
        using ResultType = decltype(fn(std::declval<E>()));
        if (is_ok_) {
            return ResultType(Ok(T(storage_.value)));
        } else {
            return fn(storage_.error);
        }
    }

    template <typename F>
    auto or_else(F&& fn) && -> decltype(fn(std::declval<E>())) {
        using ResultType = decltype(fn(std::declval<E>()));
        if (is_ok_) {
            return ResultType(Ok(std::move(storage_.value)));
        } else {
            return fn(std::move(storage_.error));
        }
    }

   private:
    void destroy() {
        if (is_ok_) {
            storage_.value.~T();
        } else {
            storage_.error.~E();
        }
    }

    union Storage {
        T value;
        E error;

        Storage() {}   // Union requires manual construction
        ~Storage() {}  // Union requires manual destruction
    } storage_;

    bool is_ok_;
};

/**
 * @brief Specialization of Result for void success type
 * @tparam E The error type
 *
 * This specialization handles operations that can fail but don't return a value on success.
 *
 * Example:
 * @code
 * Result<void, ErrorCode> initialize() {
 *     if (hardware_init_success()) {
 *         return Ok();
 *     }
 *     return Err(ErrorCode::HardwareError);
 * }
 * @endcode
 */
template <typename E>
class Result<void, E> {
   public:
    using error_type = E;

    // Constructors
    Result(const detail::OkTag<void>&) : is_ok_(true), error_() {}
    Result(const detail::ErrTag<E>& err) : is_ok_(false), error_(err.error) {}

    // No copy/move as we're keeping it simple for void type
    Result(const Result&) = default;
    Result& operator=(const Result&) = default;
    Result(Result&&) = default;
    Result& operator=(Result&&) = default;

    /**
     * @brief Check if the result is success
     * @return true if success, false if error
     */
    [[nodiscard]] constexpr bool is_ok() const noexcept { return is_ok_; }

    /**
     * @brief Check if the result is error
     * @return true if error, false if success
     */
    [[nodiscard]] constexpr bool is_err() const noexcept { return !is_ok_; }

    /**
     * @brief Get the error value (short alias)
     * @return Reference to the error
     * @pre is_err() must be true
     */
    [[nodiscard]] const E& err() const& {
        assert(!is_ok_ && "Cannot access err() on success Result");
        return error_;
    }

    /**
     * @brief Get the error value (move, short alias)
     * @return Error value
     * @pre is_err() must be true
     */
    [[nodiscard]] E&& err() && {
        assert(!is_ok_ && "Cannot access err() on success Result");
        return static_cast<E&&>(error_);
    }

    /**
     * @brief Get the error value (legacy alias for err)
     * @return Reference to the error
     * @pre is_err() must be true
     */
    [[nodiscard]] const E& error() const& {
        assert(!is_ok_ && "Cannot access error() on success Result");
        return error_;
    }

    /**
     * @brief Get the error value (move, legacy alias)
     * @return Error value
     * @pre is_err() must be true
     */
    [[nodiscard]] E&& error() && {
        assert(!is_ok_ && "Cannot access error() on success Result");
        return static_cast<E&&>(error_);
    }

    /**
     * @brief Assert that the result is Ok
     * @param msg Message to display if assertion fails
     * @pre is_ok() must be true
     */
    void expect(const char* msg) const {
        if (!is_ok_) {
            // In a real implementation, this would trigger a panic/assert
            assert(false && msg);
        }
    }

    /**
     * @brief Unwrap the result (asserts if error)
     * @pre is_ok() must be true
     */
    void unwrap() const { assert(is_ok_ && "Cannot unwrap error Result"); }

   private:
    bool is_ok_;
    E error_;  // Only valid when is_ok_ == false
};

}  // namespace core
}  // namespace alloy
