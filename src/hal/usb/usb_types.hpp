#pragma once

// alloy::hal::usb common types — speed, endpoint addressing, transfer kinds,
// runtime events. Concept-only header; no peripheral-specific dependencies.

#include <cstdint>

namespace alloy::hal::usb {

enum class UsbSpeed : std::uint8_t {
    Low = 0u,    ///< 1.5 Mbps (USB 1.x). Rarely used on modern MCUs.
    Full = 1u,   ///< 12 Mbps (USB 1.1). The default on STM32 USB FS / nRF52 USBD.
    High = 2u,   ///< 480 Mbps (USB 2.0). STM32 OTG HS, SAME70 UOTGHS.
    Super = 3u,  ///< 5 Gbps (USB 3.0). No supported MCU yet.
};

enum class EndpointDirection : std::uint8_t {
    Out = 0u,  ///< Host → device. The "OUT" name follows the host viewpoint.
    In  = 1u,  ///< Device → host.
};

enum class EndpointType : std::uint8_t {
    Control = 0u,
    Isochronous = 1u,
    Bulk = 2u,
    Interrupt = 3u,
};

/// Endpoint address: 7-bit number + direction bit, encoded the same way as the
/// `bEndpointAddress` byte in the USB endpoint descriptor.
///
///   bit 7   : direction (0 = OUT, 1 = IN)
///   bits 6-4: reserved (0)
///   bits 3-0: endpoint number (0..15)
struct EndpointAddress {
    std::uint8_t raw = 0u;

    [[nodiscard]] constexpr auto number() const -> std::uint8_t {
        return raw & 0x0Fu;
    }
    [[nodiscard]] constexpr auto direction() const -> EndpointDirection {
        return (raw & 0x80u) != 0u ? EndpointDirection::In : EndpointDirection::Out;
    }

    [[nodiscard]] static constexpr auto make(std::uint8_t number, EndpointDirection dir)
        -> EndpointAddress {
        const auto dir_bit = dir == EndpointDirection::In ? 0x80u : 0x00u;
        return EndpointAddress{static_cast<std::uint8_t>((number & 0x0Fu) | dir_bit)};
    }

    constexpr auto operator==(const EndpointAddress&) const -> bool = default;
};

inline constexpr auto kControlEp0Out = EndpointAddress::make(0u, EndpointDirection::Out);
inline constexpr auto kControlEp0In  = EndpointAddress::make(0u, EndpointDirection::In);

/// Runtime events surfaced from a controller backend to the application via
/// the `configure(event_handler)` callback.
enum class UsbEvent : std::uint8_t {
    Reset,             ///< Bus reset detected.
    Suspend,           ///< Bus is in suspended state (no SOF for >3 ms).
    Resume,            ///< Bus is resuming from suspend.
    SetupReceived,     ///< Setup packet has arrived on EP0.
    TransferComplete,  ///< Endpoint transfer (IN or OUT) completed.
    Error,             ///< Backend-side error condition (FIFO overrun, CRC, …).
};

/// USB control transfer stages, exposed so descriptor handlers can react to
/// host status-stage acknowledgement.
enum class ControlStage : std::uint8_t {
    Setup,
    DataIn,
    DataOut,
    StatusIn,
    StatusOut,
};

}  // namespace alloy::hal::usb
