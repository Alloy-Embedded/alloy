#pragma once

// USB setup packet — the 8-byte structure sent by the host on every control
// transfer. Layout matches USB 2.0 §9.3.

#include <cstdint>

namespace alloy::hal::usb {

/// `bmRequestType` recipient (low 5 bits of the byte).
enum class RequestRecipient : std::uint8_t {
    Device = 0u,
    Interface = 1u,
    Endpoint = 2u,
    Other = 3u,
};

/// `bmRequestType` request type (bits 6:5 of the byte).
enum class RequestType : std::uint8_t {
    Standard = 0u,
    Class = 1u,
    Vendor = 2u,
    Reserved = 3u,
};

/// USB standard request codes (USB 2.0 §9.4).
enum class StandardRequest : std::uint8_t {
    GetStatus = 0u,
    ClearFeature = 1u,
    SetFeature = 3u,
    SetAddress = 5u,
    GetDescriptor = 6u,
    SetDescriptor = 7u,
    GetConfiguration = 8u,
    SetConfiguration = 9u,
    GetInterface = 10u,
    SetInterface = 11u,
    SynchFrame = 12u,
};

/// Standard descriptor types (`wValue` high byte for GET_DESCRIPTOR).
enum class DescriptorType : std::uint8_t {
    Device = 1u,
    Configuration = 2u,
    String = 3u,
    Interface = 4u,
    Endpoint = 5u,
    DeviceQualifier = 6u,
    OtherSpeedConfig = 7u,
    InterfacePower = 8u,
    Hid = 0x21u,
    HidReport = 0x22u,
};

/// 8-byte USB setup packet — wire-layout struct. The descriptor offsets match
/// USB 2.0 §9.3 exactly; the typed accessors below decode the bitfields in
/// `bmRequestType`.
struct SetupPacket {
    std::uint8_t  bmRequestType = 0u;
    std::uint8_t  bRequest = 0u;
    std::uint16_t wValue = 0u;
    std::uint16_t wIndex = 0u;
    std::uint16_t wLength = 0u;

    [[nodiscard]] constexpr auto recipient() const -> RequestRecipient {
        return static_cast<RequestRecipient>(bmRequestType & 0x1Fu);
    }
    [[nodiscard]] constexpr auto type() const -> RequestType {
        return static_cast<RequestType>((bmRequestType >> 5u) & 0x03u);
    }
    [[nodiscard]] constexpr auto host_to_device() const -> bool {
        return (bmRequestType & 0x80u) == 0u;
    }
    [[nodiscard]] constexpr auto device_to_host() const -> bool {
        return (bmRequestType & 0x80u) != 0u;
    }

    [[nodiscard]] constexpr auto is_standard() const -> bool {
        return type() == RequestType::Standard;
    }
    [[nodiscard]] constexpr auto is_class() const -> bool {
        return type() == RequestType::Class;
    }
    [[nodiscard]] constexpr auto is_vendor() const -> bool {
        return type() == RequestType::Vendor;
    }

    /// `wValue` high byte — descriptor type for GET_DESCRIPTOR.
    [[nodiscard]] constexpr auto descriptor_type() const -> std::uint8_t {
        return static_cast<std::uint8_t>(wValue >> 8u);
    }
    /// `wValue` low byte — descriptor index.
    [[nodiscard]] constexpr auto descriptor_index() const -> std::uint8_t {
        return static_cast<std::uint8_t>(wValue & 0xFFu);
    }
};

static_assert(sizeof(SetupPacket) == 8u, "USB setup packet must be exactly 8 bytes");

}  // namespace alloy::hal::usb
