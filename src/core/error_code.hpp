#ifndef ALLOY_CORE_ERROR_CODE_HPP
#define ALLOY_CORE_ERROR_CODE_HPP

#include "types.hpp"

/// Error code definitions for Alloy framework

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

} // namespace alloy::core

#endif // ALLOY_CORE_ERROR_CODE_HPP
