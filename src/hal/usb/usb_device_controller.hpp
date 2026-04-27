#pragma once

// `UsbDeviceController` C++20 concept. A backend (STM32 USB FS, nRF52 USBD,
// SAME70 UOTGHS, …) satisfies this concept by exposing the canonical USB
// device-MAC operations: configure, connect, enable_endpoint, write, read,
// stall, set_address, plus a `service()` pump for ISR-style backends that
// surface events through a poll loop.
//
// The concept lets the descriptor builder, class drivers (CDC-ACM / DFU /
// HID), and example code be written once against the concept and compiled
// against any conforming backend with no virtual dispatch.

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/usb/usb_setup_packet.hpp"
#include "hal/usb/usb_types.hpp"

namespace alloy::hal::usb {

/// Event handler signature — backends call this from their ISR (or service
/// loop) to surface bus events to the application. Implementations must keep
/// the handler short — they're called from interrupt context on most
/// hardware.
using EventHandler = void (*)(UsbEvent event, void* user_data);

/// Setup-packet handler signature — backends call this when an EP0 setup
/// packet arrives. Returning `Ok(span)` provides the IN-direction payload;
/// returning `Err(NotSupported)` causes the controller to STALL the request.
using SetupHandler = core::Result<std::span<const std::byte>, core::ErrorCode> (*)(
    const SetupPacket& setup, void* user_data);

template <typename T>
concept UsbDeviceController = requires(T& controller,
                                       UsbSpeed speed,
                                       EndpointAddress ep,
                                       EndpointType type,
                                       std::size_t mps,
                                       std::span<const std::byte> tx,
                                       std::span<std::byte> rx,
                                       std::uint8_t address,
                                       EventHandler event_handler,
                                       SetupHandler setup_handler,
                                       void* user_data) {
    /// Configure the controller and install the event + setup handlers.
    /// Must be called before `connect()`.
    {
        controller.configure(speed, event_handler, setup_handler, user_data)
    } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Pull the device's pull-up so the host enumerates it.
    { controller.connect() } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Drop the pull-up (graceful disconnect).
    { controller.disconnect() } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Open an endpoint with the requested type and max packet size.
    {
        controller.enable_endpoint(ep, type, mps)
    } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Close an endpoint (de-allocate FIFO, NAK further transfers).
    {
        controller.disable_endpoint(ep)
    } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Queue a write on an IN endpoint. Returns `Ok` once the controller has
    /// accepted the buffer; the actual transfer-complete event is signalled
    /// through `EventHandler(UsbEvent::TransferComplete)`.
    { controller.write(ep, tx) } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Read pending data from an OUT endpoint into the buffer. Returns the
    /// number of bytes actually read.
    {
        controller.read(ep, rx)
    } -> std::same_as<core::Result<std::size_t, core::ErrorCode>>;

    /// Stall an endpoint (halt feature). Cleared either by the host
    /// (CLEAR_FEATURE) or by the application via `unstall`.
    { controller.stall(ep) } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Clear a stall condition.
    { controller.unstall(ep) } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Set the device's USB address (post-SET_ADDRESS).
    {
        controller.set_address(address)
    } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Service the controller — polls and dispatches events. No-op on
    /// fully-interrupt-driven backends; required on poll-mode backends. Must
    /// be safe to call from a main loop or RTOS task.
    { controller.service() } -> std::same_as<core::Result<void, core::ErrorCode>>;
};

}  // namespace alloy::hal::usb
