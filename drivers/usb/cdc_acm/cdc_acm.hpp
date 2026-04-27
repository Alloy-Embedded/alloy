#pragma once

// CDC-ACM (USB Communications Device Class — Abstract Control Model) driver.
//
// Implements the host-visible "/dev/ttyACM0" virtual COM port pattern via a
// `UsbDeviceController` backend. The driver is templated over the controller
// type so it composes with any backend that satisfies the concept.
//
// The class driver:
//   - Builds the standard CDC-ACM descriptor tree (Communication Class +
//     Data Class interfaces, two interfaces total, three endpoints: 1 INT
//     notify + 1 BULK IN + 1 BULK OUT).
//   - Handles the three class-specific control requests required by every
//     OS host driver (SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE).
//   - Exposes `write(span)` / `read(span)` returning `core::Result` so the
//     CDC-ACM endpoint can drop into any `ByteStream`-shaped consumer (Modbus
//     RTU over USB, log-print sink, …) the same way a UART would.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/usb/usb_descriptor.hpp"
#include "hal/usb/usb_device_controller.hpp"
#include "hal/usb/usb_setup_packet.hpp"
#include "hal/usb/usb_types.hpp"

namespace alloy::drivers::usb::cdc_acm {

namespace usb = alloy::hal::usb;
namespace desc = alloy::hal::usb::descriptor;

// ── CDC class constants ─────────────────────────────────────────────────────

inline constexpr std::uint8_t kCdcInterfaceClass        = 0x02u;  // Communications Class
inline constexpr std::uint8_t kCdcInterfaceSubclassAcm  = 0x02u;  // Abstract Control Model
inline constexpr std::uint8_t kCdcInterfaceProtocolAtCmd = 0x01u; // Common AT commands
inline constexpr std::uint8_t kDataInterfaceClass       = 0x0Au;  // CDC Data Class

inline constexpr std::uint8_t kCdcRequestSetLineCoding        = 0x20u;
inline constexpr std::uint8_t kCdcRequestGetLineCoding        = 0x21u;
inline constexpr std::uint8_t kCdcRequestSetControlLineState  = 0x22u;
inline constexpr std::uint8_t kCdcRequestSendBreak            = 0x23u;

inline constexpr std::uint8_t kCdcCsInterfaceDescriptor = 0x24u;

inline constexpr std::uint8_t kCdcFunctionalHeader      = 0x00u;
inline constexpr std::uint8_t kCdcFunctionalCallMgmt    = 0x01u;
inline constexpr std::uint8_t kCdcFunctionalAcm         = 0x02u;
inline constexpr std::uint8_t kCdcFunctionalUnion       = 0x06u;

/// Line coding state — sent by the host via SET_LINE_CODING, reported back
/// via GET_LINE_CODING. CDC-ACM does NOT impose this on the bulk endpoints
/// (USB has no baud rate); applications can use the value to drive a real
/// UART pass-through if desired.
struct LineCoding {
    std::uint32_t baud_rate = 115'200u;
    std::uint8_t  stop_bits = 0u;     ///< 0=1, 1=1.5, 2=2 stop bits.
    std::uint8_t  parity_type = 0u;   ///< 0=None, 1=Odd, 2=Even, 3=Mark, 4=Space.
    std::uint8_t  data_bits = 8u;     ///< 5, 6, 7, 8, or 16.
};

/// Control line state — DTR / RTS bits the host sets via
/// SET_CONTROL_LINE_STATE (`wValue` bits 0/1 respectively).
struct ControlLineState {
    bool dtr = false;
    bool rts = false;
};

/// Default endpoint addresses used by the descriptor builder. Override via
/// `Endpoints` template parameter if your controller's FIFO layout demands
/// different numbers.
struct DefaultEndpoints {
    static constexpr auto notify = usb::EndpointAddress::make(1, usb::EndpointDirection::In);
    static constexpr auto data_in  = usb::EndpointAddress::make(2, usb::EndpointDirection::In);
    static constexpr auto data_out = usb::EndpointAddress::make(2, usb::EndpointDirection::Out);
    static constexpr std::uint16_t bulk_max_packet_size = 64u;
    static constexpr std::uint16_t notify_max_packet_size = 8u;
};

namespace detail {

[[nodiscard]] constexpr auto byte(std::uint32_t v) -> std::byte {
    return static_cast<std::byte>(v & 0xFFu);
}

[[nodiscard]] constexpr auto cdc_header_descriptor() -> std::array<std::byte, 5> {
    return {{byte(5u), byte(kCdcCsInterfaceDescriptor), byte(kCdcFunctionalHeader),
             byte(0x10u), byte(0x01u)}};  // bcdCDC = 0x0110
}

[[nodiscard]] constexpr auto cdc_call_mgmt_descriptor() -> std::array<std::byte, 5> {
    return {{byte(5u), byte(kCdcCsInterfaceDescriptor), byte(kCdcFunctionalCallMgmt),
             byte(0x00u), byte(0x01u)}};  // capabilities=0, data interface=1
}

[[nodiscard]] constexpr auto cdc_acm_descriptor() -> std::array<std::byte, 4> {
    return {{byte(4u), byte(kCdcCsInterfaceDescriptor), byte(kCdcFunctionalAcm),
             byte(0x02u)}};  // capabilities = SET/GET_LINE_CODING + SET_CTRL_LINE_STATE
}

[[nodiscard]] constexpr auto cdc_union_descriptor() -> std::array<std::byte, 5> {
    return {{byte(5u), byte(kCdcCsInterfaceDescriptor), byte(kCdcFunctionalUnion),
             byte(0u), byte(1u)}};  // master=if0, slave=if1
}

}  // namespace detail

/// Build the full CDC-ACM configuration descriptor tree. Output is a flat
/// `std::array<std::byte, N>` with `wTotalLength` already patched to N.
template <typename Endpoints = DefaultEndpoints>
[[nodiscard]] constexpr auto build_configuration_descriptor() {
    constexpr auto cfg = desc::config_descriptor({
        .total_length = 0u,
        .num_interfaces = 2u,
        .configuration_value = 1u,
        .configuration_string_index = 0u,
        .attributes = 0xC0u,
        .max_power_2ma_units = 50u,
    });

    constexpr auto cdc_if = desc::interface_descriptor({
        .interface_number = 0u,
        .alternate_setting = 0u,
        .num_endpoints = 1u,
        .interface_class = kCdcInterfaceClass,
        .interface_subclass = kCdcInterfaceSubclassAcm,
        .interface_protocol = kCdcInterfaceProtocolAtCmd,
        .interface_string_index = 0u,
    });

    constexpr auto cdc_header   = detail::cdc_header_descriptor();
    constexpr auto cdc_callmgmt = detail::cdc_call_mgmt_descriptor();
    constexpr auto cdc_acm      = detail::cdc_acm_descriptor();
    constexpr auto cdc_union    = detail::cdc_union_descriptor();

    constexpr auto notify_ep = desc::endpoint_descriptor({
        .address = Endpoints::notify,
        .type = usb::EndpointType::Interrupt,
        .max_packet_size = Endpoints::notify_max_packet_size,
        .interval = 16u,
    });

    constexpr auto data_if = desc::interface_descriptor({
        .interface_number = 1u,
        .alternate_setting = 0u,
        .num_endpoints = 2u,
        .interface_class = kDataInterfaceClass,
        .interface_subclass = 0u,
        .interface_protocol = 0u,
        .interface_string_index = 0u,
    });

    constexpr auto in_ep = desc::endpoint_descriptor({
        .address = Endpoints::data_in,
        .type = usb::EndpointType::Bulk,
        .max_packet_size = Endpoints::bulk_max_packet_size,
        .interval = 0u,
    });

    constexpr auto out_ep = desc::endpoint_descriptor({
        .address = Endpoints::data_out,
        .type = usb::EndpointType::Bulk,
        .max_packet_size = Endpoints::bulk_max_packet_size,
        .interval = 0u,
    });

    auto tree = desc::concat_descriptors(cfg, cdc_if, cdc_header, cdc_callmgmt,
                                         cdc_acm, cdc_union, notify_ep,
                                         data_if, in_ep, out_ep);
    desc::patch_config_total_length(tree);
    return tree;
}

/// CDC-ACM class driver, templated over the USB device controller backend.
/// Controllers must satisfy `UsbDeviceController`. The driver owns the
/// line-coding state and the small response buffer used to answer
/// `GET_LINE_CODING`.
template <usb::UsbDeviceController Controller, typename Endpoints = DefaultEndpoints>
class CdcAcm {
   public:
    using controller_type = Controller;

    explicit CdcAcm(Controller& controller) : controller_(&controller) {}

    /// Open the CDC-ACM endpoints on the underlying controller. Must be
    /// called after the host has assigned a device address and selected the
    /// configuration; typical call site is the SET_CONFIGURATION handler.
    [[nodiscard]] auto configure() -> core::Result<void, core::ErrorCode> {
        if (auto r = controller_->enable_endpoint(
                Endpoints::notify, usb::EndpointType::Interrupt,
                Endpoints::notify_max_packet_size);
            !r.is_ok()) {
            return r;
        }
        if (auto r = controller_->enable_endpoint(
                Endpoints::data_in, usb::EndpointType::Bulk,
                Endpoints::bulk_max_packet_size);
            !r.is_ok()) {
            return r;
        }
        if (auto r = controller_->enable_endpoint(
                Endpoints::data_out, usb::EndpointType::Bulk,
                Endpoints::bulk_max_packet_size);
            !r.is_ok()) {
            return r;
        }
        configured_ = true;
        return core::Ok();
    }

    /// Send `tx` on the CDC bulk-IN endpoint. Returns `Ok` once the data has
    /// been queued; the actual bus transfer completes asynchronously.
    [[nodiscard]] auto write(std::span<const std::byte> tx)
        -> core::Result<void, core::ErrorCode> {
        if (!configured_) {
            return core::Err(core::ErrorCode::NotInitialized);
        }
        return controller_->write(Endpoints::data_in, tx);
    }

    /// Read up to `rx.size()` bytes from the CDC bulk-OUT endpoint. Returns
    /// the actual byte count.
    [[nodiscard]] auto read(std::span<std::byte> rx)
        -> core::Result<std::size_t, core::ErrorCode> {
        if (!configured_) {
            return core::Err(core::ErrorCode::NotInitialized);
        }
        return controller_->read(Endpoints::data_out, rx);
    }

    [[nodiscard]] auto is_connected() const -> bool { return connected_; }
    [[nodiscard]] auto line_coding() const -> const LineCoding& { return line_coding_; }
    [[nodiscard]] auto control_line_state() const -> const ControlLineState& {
        return control_line_state_;
    }

    /// Handle a class-specific setup packet. Returns `Ok(span)` to provide a
    /// device-to-host response payload, or `Err(NotSupported)` to STALL.
    /// Wire this from the controller's SetupHandler when
    /// `setup.is_class() && setup.recipient() == Interface`.
    [[nodiscard]] auto handle_class_request(const usb::SetupPacket& setup,
                                            std::span<const std::byte> data_out)
        -> core::Result<std::span<const std::byte>, core::ErrorCode> {
        if (!setup.is_class() ||
            setup.recipient() != usb::RequestRecipient::Interface) {
            return core::Err(core::ErrorCode::NotSupported);
        }

        switch (setup.bRequest) {
            case kCdcRequestSetLineCoding: {
                if (data_out.size() < sizeof(LineCoding) - 1u) {
                    return core::Err(core::ErrorCode::InvalidParameter);
                }
                line_coding_.baud_rate =
                    static_cast<std::uint32_t>(data_out[0]) |
                    (static_cast<std::uint32_t>(data_out[1]) << 8u) |
                    (static_cast<std::uint32_t>(data_out[2]) << 16u) |
                    (static_cast<std::uint32_t>(data_out[3]) << 24u);
                line_coding_.stop_bits   = static_cast<std::uint8_t>(data_out[4]);
                line_coding_.parity_type = static_cast<std::uint8_t>(data_out[5]);
                line_coding_.data_bits   = static_cast<std::uint8_t>(data_out[6]);
                return core::Ok(std::span<const std::byte>{});  // status stage only
            }
            case kCdcRequestGetLineCoding: {
                line_coding_response_[0] = static_cast<std::byte>(line_coding_.baud_rate & 0xFFu);
                line_coding_response_[1] = static_cast<std::byte>((line_coding_.baud_rate >> 8u) & 0xFFu);
                line_coding_response_[2] = static_cast<std::byte>((line_coding_.baud_rate >> 16u) & 0xFFu);
                line_coding_response_[3] = static_cast<std::byte>((line_coding_.baud_rate >> 24u) & 0xFFu);
                line_coding_response_[4] = static_cast<std::byte>(line_coding_.stop_bits);
                line_coding_response_[5] = static_cast<std::byte>(line_coding_.parity_type);
                line_coding_response_[6] = static_cast<std::byte>(line_coding_.data_bits);
                return core::Ok(std::span<const std::byte>{line_coding_response_.data(),
                                                            line_coding_response_.size()});
            }
            case kCdcRequestSetControlLineState: {
                control_line_state_.dtr = (setup.wValue & 0x01u) != 0u;
                control_line_state_.rts = (setup.wValue & 0x02u) != 0u;
                connected_ = control_line_state_.dtr;
                return core::Ok(std::span<const std::byte>{});  // status stage only
            }
            case kCdcRequestSendBreak: {
                // Most applications no-op; record the duration for later inspection.
                last_break_duration_ms_ = setup.wValue;
                return core::Ok(std::span<const std::byte>{});
            }
            default:
                return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto last_break_duration_ms() const -> std::uint16_t {
        return last_break_duration_ms_;
    }

   private:
    Controller* controller_ = nullptr;
    bool configured_ = false;
    bool connected_ = false;
    LineCoding line_coding_{};
    ControlLineState control_line_state_{};
    std::uint16_t last_break_duration_ms_ = 0u;
    std::array<std::byte, 7> line_coding_response_{};
};

}  // namespace alloy::drivers::usb::cdc_acm
