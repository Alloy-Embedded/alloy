/// @file device/concepts.hpp
/// Concepts for alloy.device.v2.1 flat-struct peripheral types.
///
/// Every peripheral struct produced by alloy-codegen v2.1 looks like:
///
///   struct usart1 {
///     static constexpr uintptr_t   kBaseAddress = 0x40013800u;
///     static constexpr const char* kName        = "usart1";
///     static constexpr const char* kRccEnable   = "rcc.apbenr2.usart1en";
///     static constexpr const char* kRccReset    = "rcc.apbrstr2.usart1rst";
///     static constexpr const char* kBus         = "APB2";
///     static constexpr unsigned    kInstance     = 1u;
///     ...
///   };
///
/// Use these concepts as template constraints on HAL driver templates that
/// accept a peripheral type directly instead of a PeripheralId enumerator.
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace alloy::device {

// ---------------------------------------------------------------------------
// Base concept — satisfied by any v2.1 peripheral struct
// ---------------------------------------------------------------------------

/// Minimal peripheral: has a base address and a human-readable name.
template <typename P>
concept PeripheralSpec = requires {
    { P::kBaseAddress } -> std::convertible_to<std::uintptr_t>;
    { P::kName } -> std::convertible_to<const char*>;
};

// ---------------------------------------------------------------------------
// Optional-field refinements
// ---------------------------------------------------------------------------

/// Peripheral with RCC clock-gate and reset bits encoded as dotted paths.
/// e.g. kRccEnable = "rcc.apbenr2.usart1en"
template <typename P>
concept ClockablePeripheral = PeripheralSpec<P> && requires {
    { P::kRccEnable } -> std::convertible_to<const char*>;
    { P::kRccReset } -> std::convertible_to<const char*>;
};

/// Peripheral whose bus domain is known (APB1, APB2, AHB …).
template <typename P>
concept BusedPeripheral = PeripheralSpec<P> && requires {
    { P::kBus } -> std::convertible_to<const char*>;
};

/// Peripheral with a kernel-clock mux path.
/// e.g. kKernelClockMux = "rcc.ccipr.usart1sel"
template <typename P>
concept MuxedPeripheral = PeripheralSpec<P> && requires {
    { P::kKernelClockMux } -> std::convertible_to<const char*>;
};

/// Peripheral with an instance number (e.g. kInstance = 1 for USART1).
template <typename P>
concept NumberedPeripheral = PeripheralSpec<P> && requires {
    { P::kInstance } -> std::convertible_to<unsigned>;
};

/// Full v2.1 peripheral spec — all standard fields present.
template <typename P>
concept FullPeripheralSpec =
    ClockablePeripheral<P> && BusedPeripheral<P> && NumberedPeripheral<P>;

// ---------------------------------------------------------------------------
// Signal / pin refinements (optional, present on peripherals with I/O)
// ---------------------------------------------------------------------------

/// Peripheral that declares its signal names and count.
template <typename P>
concept SignaledPeripheral = PeripheralSpec<P> && requires {
    { P::kSignalCount } -> std::convertible_to<std::size_t>;
    P::kSignals;  // const char*[]
};

}  // namespace alloy::device
