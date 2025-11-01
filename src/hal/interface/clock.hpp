/// Platform-agnostic Clock configuration interface
///
/// Defines Clock concepts and configuration types for all platforms.

#ifndef ALLOY_HAL_INTERFACE_CLOCK_HPP
#define ALLOY_HAL_INTERFACE_CLOCK_HPP

#include "core/error.hpp"
#include "core/types.hpp"
#include <concepts>

namespace alloy::hal {

/// Clock source selection
enum class ClockSource : core::u8 {
    InternalRC,         ///< Internal RC oscillator (less accurate, no external components)
    ExternalCrystal,    ///< External crystal oscillator (high accuracy)
    ExternalClock,      ///< External clock input
    Pll                 ///< PLL (Phase-Locked Loop) - multiplies clock frequency
};

/// Peripheral identifier for clock control
enum class Peripheral : core::u16 {
    // GPIO ports
    GpioA   = 0x0000,
    GpioB   = 0x0001,
    GpioC   = 0x0002,
    GpioD   = 0x0003,
    GpioE   = 0x0004,
    GpioF   = 0x0005,

    // Communication peripherals
    Uart1   = 0x0100,
    Uart2   = 0x0101,
    Uart3   = 0x0102,
    Uart4   = 0x0103,
    I2c1    = 0x0110,
    I2c2    = 0x0111,
    Spi1    = 0x0120,
    Spi2    = 0x0121,
    Spi3    = 0x0122,

    // Timers
    Timer1  = 0x0200,
    Timer2  = 0x0201,
    Timer3  = 0x0202,
    Timer4  = 0x0203,
    Timer5  = 0x0204,
    SysTick = 0x0210,  ///< System tick timer for microsecond time tracking

    // Analog peripherals
    Adc1    = 0x0300,
    Adc2    = 0x0301,
    Dac     = 0x0310,

    // DMA
    Dma1    = 0x0400,
    Dma2    = 0x0401
};

/// PLL (Phase-Locked Loop) configuration
///
/// Used to multiply input clock frequency to achieve higher system speeds.
struct PllConfig {
    ClockSource input_source;        ///< PLL input source (internal RC or external crystal)
    core::u32 input_frequency_hz;    ///< Input frequency in Hz
    core::u8 multiplier;             ///< PLL multiplier (e.g., 9 for 8MHz * 9 = 72MHz)
    core::u8 divider;                ///< PLL divider (typically 1 or 2)

    /// Constructor with default configuration
    constexpr PllConfig(ClockSource src = ClockSource::ExternalCrystal,
                       core::u32 input_hz = 8000000,
                       core::u8 mul = 9,
                       core::u8 div = 1)
        : input_source(src), input_frequency_hz(input_hz),
          multiplier(mul), divider(div) {}

    /// Calculate output frequency
    constexpr core::u32 output_frequency() const {
        return (input_frequency_hz * multiplier) / divider;
    }
};

/// System clock configuration
///
/// Comprehensive clock configuration including source, PLL, and bus dividers.
struct ClockConfig {
    ClockSource source;              ///< Main clock source
    core::u32 crystal_frequency_hz;  ///< External crystal frequency (if used)
    core::u8 pll_multiplier;         ///< PLL multiplier (0 = PLL disabled)
    core::u8 pll_divider;            ///< PLL divider
    core::u8 ahb_divider;            ///< AHB bus divider (1, 2, 4, 8, ...)
    core::u8 apb1_divider;           ///< APB1 bus divider
    core::u8 apb2_divider;           ///< APB2 bus divider
    core::u32 target_frequency_hz;   ///< Target system frequency (for auto-calculation)

    /// Constructor with default configuration (internal RC, no PLL)
    constexpr ClockConfig(ClockSource src = ClockSource::InternalRC,
                         core::u32 crystal_hz = 8000000,
                         core::u8 pll_mul = 0,
                         core::u8 pll_div = 1,
                         core::u8 ahb_div = 1,
                         core::u8 apb1_div = 1,
                         core::u8 apb2_div = 1,
                         core::u32 target_hz = 0)
        : source(src), crystal_frequency_hz(crystal_hz),
          pll_multiplier(pll_mul), pll_divider(pll_div),
          ahb_divider(ahb_div), apb1_divider(apb1_div),
          apb2_divider(apb2_div), target_frequency_hz(target_hz) {}
};

/// System clock device concept
///
/// Defines the interface that all system clock implementations must satisfy.
/// Uses Result<T, ErrorCode> for all operations that can fail.
///
/// Error codes specific to Clock:
/// - ErrorCode::PllLockFailed: PLL failed to lock/stabilize
/// - ErrorCode::ClockInvalidFrequency: Requested frequency invalid or out of range
/// - ErrorCode::ClockSourceNotReady: Clock source not ready/stable
/// - ErrorCode::InvalidParameter: Invalid configuration parameter
/// - ErrorCode::NotSupported: Feature not supported by hardware
template<typename T>
concept SystemClock = requires(T device, const T const_device,
                               ClockConfig config,
                               PllConfig pll_config,
                               Peripheral peripheral,
                               core::u32 frequency_hz) {
    /// Configure system clock
    ///
    /// Configures the main system clock including source selection, PLL,
    /// and bus dividers. This function automatically adjusts flash latency
    /// for the target frequency.
    ///
    /// @param config System clock configuration
    /// @return Ok on success, error code on failure
    { device.configure(config) } -> std::same_as<core::Result<void>>;

    /// Configure PLL
    ///
    /// Advanced PLL configuration with explicit control over multiplier and divider.
    ///
    /// @param config PLL configuration
    /// @return Ok on success, error code on failure
    { device.configure_pll(pll_config) } -> std::same_as<core::Result<void>>;

    /// Set system frequency
    ///
    /// High-level API to set system frequency. Implementation calculates
    /// optimal PLL/divider settings automatically.
    ///
    /// @param frequency_hz Desired system frequency in Hz
    /// @return Ok on success, error code on failure
    { device.set_frequency(frequency_hz) } -> std::same_as<core::Result<void>>;

    /// Get current system frequency
    ///
    /// @return Current system clock frequency in Hz
    { const_device.get_frequency() } -> std::same_as<core::u32>;

    /// Get AHB bus frequency
    ///
    /// @return Current AHB bus frequency in Hz
    { const_device.get_ahb_frequency() } -> std::same_as<core::u32>;

    /// Get APB1 bus frequency
    ///
    /// @return Current APB1 bus frequency in Hz
    { const_device.get_apb1_frequency() } -> std::same_as<core::u32>;

    /// Get APB2 bus frequency
    ///
    /// @return Current APB2 bus frequency in Hz
    { const_device.get_apb2_frequency() } -> std::same_as<core::u32>;

    /// Get specific peripheral clock frequency
    ///
    /// Returns the clock frequency for a specific peripheral.
    /// Useful for calculating baud rates, timer prescalers, etc.
    ///
    /// @param peripheral Peripheral identifier
    /// @return Peripheral clock frequency in Hz
    { const_device.get_peripheral_frequency(peripheral) } -> std::same_as<core::u32>;

    /// Enable peripheral clock
    ///
    /// Enables clock for a specific peripheral. Must be called before
    /// using the peripheral.
    ///
    /// @param peripheral Peripheral to enable
    /// @return Ok on success, error code on failure
    { device.enable_peripheral(peripheral) } -> std::same_as<core::Result<void>>;

    /// Disable peripheral clock
    ///
    /// Disables clock for a specific peripheral to save power.
    ///
    /// @param peripheral Peripheral to disable
    /// @return Ok on success, error code on failure
    { device.disable_peripheral(peripheral) } -> std::same_as<core::Result<void>>;

    /// Set flash latency
    ///
    /// Adjusts flash wait states for the current system frequency.
    /// Called automatically by configure() and set_frequency().
    ///
    /// @param frequency_hz System frequency in Hz
    /// @return Ok on success, error code on failure
    { device.set_flash_latency(frequency_hz) } -> std::same_as<core::Result<void>>;
};

/// Helper function to calculate required flash latency
///
/// Different MCU speeds require different flash wait states.
/// This is a generic implementation; vendor-specific implementations
/// should override with actual specifications.
///
/// @param frequency_hz System frequency in Hz
/// @return Number of wait states required
inline constexpr core::u8 calculate_flash_latency(core::u32 frequency_hz) {
    // Generic conservative values (adjust for specific MCU)
    if (frequency_hz <= 24000000) return 0;  // 0 wait states up to 24MHz
    if (frequency_hz <= 48000000) return 1;  // 1 wait state up to 48MHz
    if (frequency_hz <= 72000000) return 2;  // 2 wait states up to 72MHz
    return 3;  // 3 wait states above 72MHz
}

/// Helper function to calculate PLL multiplier for target frequency
///
/// @param input_frequency_hz PLL input frequency in Hz
/// @param target_frequency_hz Desired output frequency in Hz
/// @return PLL multiplier value
inline constexpr core::u8 calculate_pll_multiplier(core::u32 input_frequency_hz,
                                                   core::u32 target_frequency_hz) {
    if (input_frequency_hz == 0) return 0;
    core::u8 multiplier = static_cast<core::u8>(target_frequency_hz / input_frequency_hz);
    // Clamp to typical PLL multiplier range (2-16)
    if (multiplier < 2) multiplier = 2;
    if (multiplier > 16) multiplier = 16;
    return multiplier;
}

/// Helper function to validate clock frequency
///
/// Checks if frequency is within typical MCU limits.
/// Vendor-specific implementations should use actual MCU limits.
///
/// @param frequency_hz Frequency to validate
/// @param max_frequency_hz Maximum allowed frequency
/// @return true if valid, false if out of range
inline constexpr bool is_frequency_valid(core::u32 frequency_hz, core::u32 max_frequency_hz = 168000000) {
    // Most embedded MCUs operate between 1MHz and 200MHz
    return (frequency_hz >= 1000000 && frequency_hz <= max_frequency_hz);
}

/// Helper function to calculate bus frequency with divider
///
/// @param system_frequency_hz System clock frequency in Hz
/// @param divider Bus divider (1, 2, 4, 8, 16, ...)
/// @return Bus frequency in Hz
inline constexpr core::u32 calculate_bus_frequency(core::u32 system_frequency_hz, core::u8 divider) {
    if (divider == 0) divider = 1;  // Prevent division by zero
    return system_frequency_hz / divider;
}

} // namespace alloy::hal

#endif // ALLOY_HAL_INTERFACE_CLOCK_HPP
