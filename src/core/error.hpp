#ifndef ALLOY_CORE_ERROR_HPP
#define ALLOY_CORE_ERROR_HPP

#include "types.hpp"
#include <utility>
#include <cassert>

/// Error handling for Alloy framework
///
/// Provides deterministic error handling without exceptions using a Rust-inspired
/// Result<T, ErrorCode> type. This enables type-safe error handling with zero
/// runtime overhead in embedded systems.

namespace alloy::core {

/// Standard error codes for Alloy operations
///
/// Each error represents a specific failure condition that can occur during
/// HAL operations. ErrorCode::Ok (value 0) represents success.
enum class ErrorCode : u8 {
    Ok = 0,              ///< Operation succeeded (not an error)
    InvalidParameter,    ///< Invalid parameter passed to function
    Timeout,             ///< Operation timed out
    Busy,                ///< Resource is busy, try again later
    NotSupported,        ///< Operation not supported on this platform
    HardwareError,       ///< Hardware-level error occurred
    OutOfRange,          ///< Value out of valid range
    NotInitialized,      ///< Component not initialized
    AlreadyInitialized,  ///< Component already initialized
    BufferFull,          ///< Buffer is full, cannot accept more data
    BufferEmpty,         ///< Buffer is empty, no data available
    CommunicationError,  ///< Communication protocol error
    ChecksumError,       ///< Data checksum/CRC mismatch

    // I2C-specific errors
    I2cNack,             ///< I2C NACK received (device not responding)
    I2cBusBusy,          ///< I2C bus is busy (another master active)
    I2cArbitrationLost,  ///< I2C arbitration lost (multi-master conflict)

    // ADC-specific errors
    AdcCalibrationFailed, ///< ADC calibration failed
    AdcOverrun,          ///< ADC data overrun (conversion too fast)
    AdcConversionTimeout, ///< ADC conversion did not complete in time

    // DMA-specific errors
    DmaTransferError,    ///< DMA transfer error occurred
    DmaAlignmentError,   ///< DMA address alignment error
    DmaChannelBusy,      ///< DMA channel is already in use

    // Clock-specific errors
    PllLockFailed,       ///< PLL failed to lock/stabilize
    ClockInvalidFrequency, ///< Requested frequency is invalid or out of range
    ClockSourceNotReady, ///< Clock source not ready/stable

    Unknown              ///< Unknown error occurred
};

/// Result type for operations that can fail
///
/// Similar to Rust's Result<T, E>, this type forces explicit error handling
/// without using exceptions. It contains either a success value (T) or an
/// error code (ErrorCode), but never both.
///
/// Memory layout: Uses a union to minimize memory footprint. The size is
/// max(sizeof(T), sizeof(ErrorCode)) + 1 byte for the discriminator.
///
/// Example usage:
/// \code
/// Result<u32> read_register() {
///     if (hardware_ready()) {
///         return Result<u32>::ok(read_value());
///     }
///     return Result<u32>::error(ErrorCode::Timeout);
/// }
///
/// auto result = read_register();
/// if (result.is_ok()) {
///     u32 value = result.value();
///     // use value...
/// } else {
///     ErrorCode err = result.error();
///     // handle error...
/// }
/// \endcode
template<typename T>
class Result {
public:
    /// Create a successful Result containing a value
    ///
    /// @param value The success value to store
    /// @return Result in success state
    static Result ok(T value) {
        Result r;
        r.has_value_ = true;
        new (&r.value_) T(std::move(value));
        return r;
    }

    /// Create a failed Result containing an error code
    ///
    /// @param code The error code describing the failure
    /// @return Result in error state
    static Result error(ErrorCode code) {
        Result r;
        r.has_value_ = false;
        r.error_ = code;
        return r;
    }

    /// Destructor - destroys the contained value if present
    ~Result() {
        if (has_value_) {
            value_.~T();
        }
    }

    /// Copy constructor
    Result(const Result& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) T(other.value_);
        } else {
            error_ = other.error_;
        }
    }

    /// Move constructor
    Result(Result&& other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) T(std::move(other.value_));
        } else {
            error_ = other.error_;
        }
    }

    /// Copy assignment
    Result& operator=(const Result& other) {
        if (this != &other) {
            if (has_value_) {
                value_.~T();
            }
            has_value_ = other.has_value_;
            if (has_value_) {
                new (&value_) T(other.value_);
            } else {
                error_ = other.error_;
            }
        }
        return *this;
    }

    /// Move assignment
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            if (has_value_) {
                value_.~T();
            }
            has_value_ = other.has_value_;
            if (has_value_) {
                new (&value_) T(std::move(other.value_));
            } else {
                error_ = other.error_;
            }
        }
        return *this;
    }

    /// Check if Result contains a success value
    ///
    /// @return true if Result contains a value, false if it contains an error
    [[nodiscard]] bool is_ok() const noexcept {
        return has_value_;
    }

    /// Check if Result contains an error
    ///
    /// @return true if Result contains an error, false if it contains a value
    [[nodiscard]] bool is_error() const noexcept {
        return !has_value_;
    }

    /// Get the success value
    ///
    /// @pre is_ok() must be true
    /// @return Reference to the contained value
    /// @warning Undefined behavior if called when is_error() == true
    [[nodiscard]] T& value() & {
        assert(has_value_ && "Cannot access value() on error Result");
        return value_;
    }

    /// Get the success value (const version)
    ///
    /// @pre is_ok() must be true
    /// @return Const reference to the contained value
    /// @warning Undefined behavior if called when is_error() == true
    [[nodiscard]] const T& value() const & {
        assert(has_value_ && "Cannot access value() on error Result");
        return value_;
    }

    /// Get the success value (rvalue version)
    ///
    /// @pre is_ok() must be true
    /// @return Moved value
    /// @warning Undefined behavior if called when is_error() == true
    [[nodiscard]] T&& value() && {
        assert(has_value_ && "Cannot access value() on error Result");
        return std::move(value_);
    }

    /// Get the error code
    ///
    /// @pre is_error() must be true
    /// @return The error code
    /// @warning Undefined behavior if called when is_ok() == true
    [[nodiscard]] ErrorCode error() const noexcept {
        assert(!has_value_ && "Cannot access error() on success Result");
        return error_;
    }

    /// Get value or default
    ///
    /// Returns the contained value if present, otherwise returns the provided default.
    ///
    /// @param default_value Value to return if Result contains an error
    /// @return The contained value or default_value
    [[nodiscard]] T value_or(T default_value) const & {
        return has_value_ ? value_ : default_value;
    }

    /// Get value or default (rvalue version)
    [[nodiscard]] T value_or(T default_value) && {
        return has_value_ ? std::move(value_) : default_value;
    }

private:
    /// Private default constructor
    Result() : has_value_(false), error_(ErrorCode::Unknown) {}

    bool has_value_;

    union {
        T value_;
        ErrorCode error_;
    };
};

/// Specialization for void return (operation that can fail but returns no value)
///
/// Example:
/// \code
/// Result<void> initialize() {
///     if (hardware_init_success()) {
///         return Result<void>::ok();
///     }
///     return Result<void>::error(ErrorCode::HardwareError);
/// }
/// \endcode
template<>
class Result<void> {
public:
    /// Create a successful Result (no value)
    static Result ok() {
        Result r;
        r.has_value_ = true;
        r.error_ = ErrorCode::Ok;
        return r;
    }

    /// Create a failed Result with error code
    static Result error(ErrorCode code) {
        Result r;
        r.has_value_ = false;
        r.error_ = code;
        return r;
    }

    /// Check if operation succeeded
    [[nodiscard]] bool is_ok() const noexcept {
        return has_value_;
    }

    /// Check if operation failed
    [[nodiscard]] bool is_error() const noexcept {
        return !has_value_;
    }

    /// Get the error code
    ///
    /// @pre is_error() must be true
    [[nodiscard]] ErrorCode error() const noexcept {
        assert(!has_value_ && "Cannot access error() on success Result");
        return error_;
    }

private:
    Result() : has_value_(false), error_(ErrorCode::Unknown) {}

    bool has_value_;
    ErrorCode error_;
};

} // namespace alloy::core

#endif // ALLOY_CORE_ERROR_HPP
