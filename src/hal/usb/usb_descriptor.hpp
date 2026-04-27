#pragma once

// Compile-time USB descriptor builder.
//
// `device_descriptor`, `configuration_header`, `interface_descriptor`,
// `endpoint_descriptor`, and `string_descriptor` each return a `constexpr
// std::array<std::byte, N>`. Compose larger trees via `concat_descriptors(...)`
// — the result is a flat byte array that can be `static constexpr` in a class
// driver and handed to the controller as a `std::span<const std::byte>` at
// `GET_DESCRIPTOR` time.
//
// Zero runtime allocation. The total size is computed at compile time and
// `static_assert`-checked against 65535 bytes (the USB `wTotalLength` limit).

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "hal/usb/usb_setup_packet.hpp"
#include "hal/usb/usb_types.hpp"

namespace alloy::hal::usb::descriptor {

using ::alloy::hal::usb::DescriptorType;

inline constexpr std::uint8_t kDeviceDescriptorLength = 18u;
inline constexpr std::uint8_t kConfigDescriptorLength = 9u;
inline constexpr std::uint8_t kInterfaceDescriptorLength = 9u;
inline constexpr std::uint8_t kEndpointDescriptorLength = 7u;

namespace detail {

[[nodiscard]] constexpr auto byte(std::uint32_t v) -> std::byte {
    return static_cast<std::byte>(v & 0xFFu);
}

[[nodiscard]] constexpr auto lo(std::uint16_t v) -> std::byte {
    return static_cast<std::byte>(v & 0xFFu);
}

[[nodiscard]] constexpr auto hi(std::uint16_t v) -> std::byte {
    return static_cast<std::byte>((v >> 8u) & 0xFFu);
}

}  // namespace detail

/// Device descriptor — exactly 18 bytes per USB 2.0 §9.6.1.
struct DeviceDescriptor {
    std::uint16_t usb_release_bcd = 0x0200u;  ///< 0x0200 = USB 2.0
    std::uint8_t  device_class = 0u;
    std::uint8_t  device_subclass = 0u;
    std::uint8_t  device_protocol = 0u;
    std::uint8_t  ep0_max_packet_size = 64u;  ///< Always 64 on full-speed.
    std::uint16_t vendor_id = 0u;
    std::uint16_t product_id = 0u;
    std::uint16_t device_release_bcd = 0x0100u;
    std::uint8_t  manufacturer_string_index = 0u;
    std::uint8_t  product_string_index = 0u;
    std::uint8_t  serial_string_index = 0u;
    std::uint8_t  num_configurations = 1u;
};

[[nodiscard]] constexpr auto device_descriptor(const DeviceDescriptor& d)
    -> std::array<std::byte, kDeviceDescriptorLength> {
    using detail::byte;
    using detail::hi;
    using detail::lo;
    return {{
        byte(kDeviceDescriptorLength),
        byte(static_cast<std::uint8_t>(DescriptorType::Device)),
        lo(d.usb_release_bcd), hi(d.usb_release_bcd),
        byte(d.device_class), byte(d.device_subclass), byte(d.device_protocol),
        byte(d.ep0_max_packet_size),
        lo(d.vendor_id), hi(d.vendor_id),
        lo(d.product_id), hi(d.product_id),
        lo(d.device_release_bcd), hi(d.device_release_bcd),
        byte(d.manufacturer_string_index), byte(d.product_string_index),
        byte(d.serial_string_index),
        byte(d.num_configurations),
    }};
}

/// Configuration descriptor *header* — 9 bytes. Add interface + endpoint
/// descriptors after it; `wTotalLength` is patched in `concat_descriptors`.
struct ConfigDescriptor {
    std::uint16_t total_length = 0u;       ///< Filled by `concat_descriptors`.
    std::uint8_t  num_interfaces = 1u;
    std::uint8_t  configuration_value = 1u;
    std::uint8_t  configuration_string_index = 0u;
    std::uint8_t  attributes = 0xC0u;       ///< D7=1 (req'd) D6=self-powered.
    std::uint8_t  max_power_2ma_units = 50u;///< 50 = 100 mA.
};

[[nodiscard]] constexpr auto config_descriptor(const ConfigDescriptor& c)
    -> std::array<std::byte, kConfigDescriptorLength> {
    using detail::byte;
    using detail::hi;
    using detail::lo;
    return {{
        byte(kConfigDescriptorLength),
        byte(static_cast<std::uint8_t>(DescriptorType::Configuration)),
        lo(c.total_length), hi(c.total_length),
        byte(c.num_interfaces),
        byte(c.configuration_value),
        byte(c.configuration_string_index),
        byte(c.attributes),
        byte(c.max_power_2ma_units),
    }};
}

/// Interface descriptor — 9 bytes per USB 2.0 §9.6.5.
struct InterfaceDescriptor {
    std::uint8_t interface_number = 0u;
    std::uint8_t alternate_setting = 0u;
    std::uint8_t num_endpoints = 0u;
    std::uint8_t interface_class = 0u;
    std::uint8_t interface_subclass = 0u;
    std::uint8_t interface_protocol = 0u;
    std::uint8_t interface_string_index = 0u;
};

[[nodiscard]] constexpr auto interface_descriptor(const InterfaceDescriptor& i)
    -> std::array<std::byte, kInterfaceDescriptorLength> {
    using detail::byte;
    return {{
        byte(kInterfaceDescriptorLength),
        byte(static_cast<std::uint8_t>(DescriptorType::Interface)),
        byte(i.interface_number),
        byte(i.alternate_setting),
        byte(i.num_endpoints),
        byte(i.interface_class),
        byte(i.interface_subclass),
        byte(i.interface_protocol),
        byte(i.interface_string_index),
    }};
}

/// Endpoint descriptor — 7 bytes per USB 2.0 §9.6.6.
struct EndpointDescriptorSpec {
    EndpointAddress address{};
    EndpointType    type = EndpointType::Bulk;
    std::uint16_t   max_packet_size = 64u;
    std::uint8_t    interval = 0u;          ///< Polling interval (frames). 0 for bulk.
};

[[nodiscard]] constexpr auto endpoint_descriptor(const EndpointDescriptorSpec& e)
    -> std::array<std::byte, kEndpointDescriptorLength> {
    using detail::byte;
    using detail::hi;
    using detail::lo;
    return {{
        byte(kEndpointDescriptorLength),
        byte(static_cast<std::uint8_t>(DescriptorType::Endpoint)),
        byte(e.address.raw),
        byte(static_cast<std::uint8_t>(e.type)),
        lo(e.max_packet_size), hi(e.max_packet_size),
        byte(e.interval),
    }};
}

// ── concatenation helpers ──────────────────────────────────────────────────

namespace detail {

template <std::size_t N>
[[nodiscard]] constexpr auto concat_one(const std::array<std::byte, N>& a)
    -> std::array<std::byte, N> {
    return a;
}

template <std::size_t Na, std::size_t Nb>
[[nodiscard]] constexpr auto concat_two(const std::array<std::byte, Na>& a,
                                        const std::array<std::byte, Nb>& b)
    -> std::array<std::byte, Na + Nb> {
    std::array<std::byte, Na + Nb> out{};
    for (std::size_t i = 0; i < Na; ++i) {
        out[i] = a[i];
    }
    for (std::size_t i = 0; i < Nb; ++i) {
        out[Na + i] = b[i];
    }
    return out;
}

}  // namespace detail

/// Concatenate any number of descriptor byte arrays into one flat
/// `std::array<std::byte, total>`. After concatenation, the
/// configuration-descriptor's `wTotalLength` field (offset 2-3) is patched to
/// the resulting total length so the host receives a self-consistent tree.
template <std::size_t N0>
[[nodiscard]] constexpr auto concat_descriptors(const std::array<std::byte, N0>& a)
    -> std::array<std::byte, N0> {
    return a;
}

template <std::size_t N0, std::size_t N1, std::size_t... Ns>
[[nodiscard]] constexpr auto concat_descriptors(const std::array<std::byte, N0>& a,
                                                const std::array<std::byte, N1>& b,
                                                const std::array<std::byte, Ns>&... rest)
    -> std::array<std::byte, N0 + N1 + (0u + ... + Ns)> {
    return concat_descriptors(detail::concat_two(a, b), rest...);
}

/// Patch the `wTotalLength` of the configuration descriptor in-place. Call this
/// after `concat_descriptors` produces the full configuration tree (config
/// header + interfaces + endpoints) so the host receives the correct size in
/// the GET_DESCRIPTOR response.
template <std::size_t N>
constexpr void patch_config_total_length(std::array<std::byte, N>& tree) {
    static_assert(N >= kConfigDescriptorLength,
                  "Tree must contain at least one configuration descriptor");
    tree[2] = detail::lo(static_cast<std::uint16_t>(N));
    tree[3] = detail::hi(static_cast<std::uint16_t>(N));
}

/// String descriptor 0 — supported language IDs. Only English (US) by default.
[[nodiscard]] constexpr auto string_descriptor_languages()
    -> std::array<std::byte, 4> {
    using detail::byte;
    return {{
        byte(4u),
        byte(static_cast<std::uint8_t>(DescriptorType::String)),
        byte(0x09u), byte(0x04u),  // 0x0409 = English (US)
    }};
}

}  // namespace alloy::hal::usb::descriptor
