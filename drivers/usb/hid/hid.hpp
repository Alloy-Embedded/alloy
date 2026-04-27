#pragma once

// USB HID 1.11 (Human Interface Device) class driver.
//
// HidDevice<Controller, ReportSize> sends fixed-size input reports on an
// interrupt-IN endpoint and optionally accepts output reports on an
// interrupt-OUT endpoint. The host reads the report descriptor (a
// compile-time `constexpr std::array`) via GET_DESCRIPTOR(HidReport), which
// describes the report layout in HID Usage IDs.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/usb/usb_descriptor.hpp"
#include "hal/usb/usb_device_controller.hpp"
#include "hal/usb/usb_setup_packet.hpp"

namespace alloy::drivers::usb::hid {

namespace usb = alloy::hal::usb;
namespace desc = alloy::hal::usb::descriptor;

inline constexpr std::uint8_t kHidInterfaceClass    = 0x03u;
inline constexpr std::uint8_t kHidInterfaceSubclass = 0x00u;  // No subclass.
inline constexpr std::uint8_t kHidProtocolNone      = 0x00u;
inline constexpr std::uint8_t kHidProtocolKeyboard  = 0x01u;
inline constexpr std::uint8_t kHidProtocolMouse     = 0x02u;

inline constexpr std::uint8_t kHidRequestGetReport   = 0x01u;
inline constexpr std::uint8_t kHidRequestGetIdle     = 0x02u;
inline constexpr std::uint8_t kHidRequestGetProtocol = 0x03u;
inline constexpr std::uint8_t kHidRequestSetReport   = 0x09u;
inline constexpr std::uint8_t kHidRequestSetIdle     = 0x0Au;
inline constexpr std::uint8_t kHidRequestSetProtocol = 0x0Bu;

inline constexpr std::uint8_t kHidDescriptorTypeHid    = 0x21u;
inline constexpr std::uint8_t kHidDescriptorTypeReport = 0x22u;

/// 9-byte HID descriptor (HID 1.11 §6.2.1) — pointer to the report
/// descriptor + its length. `bcdHID` defaults to 0x0111 (HID 1.11).
struct HidDescriptorSpec {
    std::uint16_t bcd_hid = 0x0111u;
    std::uint8_t  country_code = 0u;
    std::uint16_t report_descriptor_length = 0u;
};

[[nodiscard]] constexpr auto hid_descriptor(const HidDescriptorSpec& spec)
    -> std::array<std::byte, 9> {
    auto byte = [](std::uint32_t v) { return static_cast<std::byte>(v & 0xFFu); };
    auto lo   = [](std::uint16_t v) { return static_cast<std::byte>(v & 0xFFu); };
    auto hi   = [](std::uint16_t v) { return static_cast<std::byte>((v >> 8u) & 0xFFu); };
    return {{
        byte(9u),
        byte(kHidDescriptorTypeHid),
        lo(spec.bcd_hid), hi(spec.bcd_hid),
        byte(spec.country_code),
        byte(1u),                                     // bNumDescriptors = 1
        byte(kHidDescriptorTypeReport),
        lo(spec.report_descriptor_length), hi(spec.report_descriptor_length),
    }};
}

/// HID class driver. The application provides:
///   - A compile-time report descriptor (e.g. `kBootKeyboardReportDescriptor`).
///   - A fixed-size input-report buffer (`InputReportSize` template parameter).
///
/// Output reports (host → device) are optional; if the application wants to
/// receive them, it registers a callback via `set_output_callback`.
template <usb::UsbDeviceController Controller,
          std::size_t InputReportSize,
          std::size_t OutputReportSize = 0u>
class HidDevice {
   public:
    using output_callback = void (*)(std::span<const std::byte> report, void* user_data);

    HidDevice(Controller& controller,
              std::span<const std::byte> report_descriptor,
              usb::EndpointAddress in_endpoint,
              usb::EndpointAddress out_endpoint = {})
        : controller_(&controller),
          report_descriptor_(report_descriptor),
          in_endpoint_(in_endpoint),
          out_endpoint_(out_endpoint) {}

    [[nodiscard]] auto configure() -> core::Result<void, core::ErrorCode> {
        if (auto r = controller_->enable_endpoint(in_endpoint_,
                                                  usb::EndpointType::Interrupt,
                                                  InputReportSize);
            !r.is_ok()) {
            return r;
        }
        if constexpr (OutputReportSize > 0u) {
            if (auto r = controller_->enable_endpoint(out_endpoint_,
                                                      usb::EndpointType::Interrupt,
                                                      OutputReportSize);
                !r.is_ok()) {
                return r;
            }
        }
        configured_ = true;
        return core::Ok();
    }

    /// Send an input report (`InputReportSize` bytes) on the interrupt-IN
    /// endpoint. The report contents follow the layout declared in the
    /// report descriptor.
    [[nodiscard]] auto send_report(std::span<const std::byte> report)
        -> core::Result<void, core::ErrorCode> {
        if (!configured_) {
            return core::Err(core::ErrorCode::NotInitialized);
        }
        if (report.size() != InputReportSize) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        return controller_->write(in_endpoint_, report);
    }

    void set_output_callback(output_callback fn, void* user_data) {
        output_cb_ = fn;
        output_user_data_ = user_data;
    }

    /// Forward a received output report to the application callback. Wire
    /// this from the controller's transfer-complete event for the HID OUT
    /// endpoint.
    void deliver_output_report(std::span<const std::byte> report) {
        if (output_cb_ != nullptr) {
            output_cb_(report, output_user_data_);
        }
    }

    /// Handle HID class-specific setup. Returns the report descriptor for
    /// GET_DESCRIPTOR(Report), no-op-Ok for SET_IDLE / SET_PROTOCOL,
    /// `NotSupported` (→ STALL) otherwise.
    [[nodiscard]] auto handle_class_request(const usb::SetupPacket& setup,
                                            std::span<const std::byte>)
        -> core::Result<std::span<const std::byte>, core::ErrorCode> {
        // GET_DESCRIPTOR(Report) is a Standard request to the interface,
        // not a Class request — the application's setup-handler routes it
        // here when the descriptor type is HID Report.
        if (setup.is_standard() &&
            setup.recipient() == usb::RequestRecipient::Interface &&
            setup.bRequest == static_cast<std::uint8_t>(usb::StandardRequest::GetDescriptor) &&
            setup.descriptor_type() == kHidDescriptorTypeReport) {
            return core::Ok(std::span<const std::byte>{report_descriptor_});
        }

        if (!setup.is_class() ||
            setup.recipient() != usb::RequestRecipient::Interface) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        switch (setup.bRequest) {
            case kHidRequestSetIdle:
                idle_rate_ = static_cast<std::uint8_t>(setup.wValue >> 8u);
                return core::Ok(std::span<const std::byte>{});
            case kHidRequestSetProtocol:
                protocol_ = static_cast<std::uint8_t>(setup.wValue & 0xFFu);
                return core::Ok(std::span<const std::byte>{});
            case kHidRequestGetIdle:
                idle_response_[0] = static_cast<std::byte>(idle_rate_);
                return core::Ok(std::span<const std::byte>{idle_response_.data(),
                                                            idle_response_.size()});
            case kHidRequestGetProtocol:
                protocol_response_[0] = static_cast<std::byte>(protocol_);
                return core::Ok(std::span<const std::byte>{protocol_response_.data(),
                                                            protocol_response_.size()});
            default:
                return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto idle_rate() const -> std::uint8_t { return idle_rate_; }
    [[nodiscard]] auto protocol()  const -> std::uint8_t { return protocol_; }

   private:
    Controller* controller_ = nullptr;
    std::span<const std::byte> report_descriptor_;
    usb::EndpointAddress in_endpoint_{};
    usb::EndpointAddress out_endpoint_{};
    bool configured_ = false;
    std::uint8_t idle_rate_ = 0u;
    std::uint8_t protocol_  = 1u;  // 1 = Report protocol (default)
    output_callback output_cb_ = nullptr;
    void*           output_user_data_ = nullptr;
    std::array<std::byte, 1> idle_response_{};
    std::array<std::byte, 1> protocol_response_{};
};

// ── Common HID report descriptors ──────────────────────────────────────────

/// HID Boot-keyboard report descriptor (HID 1.11 Appendix B.1). 8-byte
/// reports: 1 byte modifier mask, 1 byte reserved, 6 bytes keycodes.
inline constexpr std::array<std::byte, 63> kBootKeyboardReportDescriptor = {{
    std::byte{0x05}, std::byte{0x01},        // Usage Page (Generic Desktop)
    std::byte{0x09}, std::byte{0x06},        // Usage (Keyboard)
    std::byte{0xA1}, std::byte{0x01},        // Collection (Application)
    std::byte{0x05}, std::byte{0x07},        //   Usage Page (Keyboard/Keypad)
    std::byte{0x19}, std::byte{0xE0},        //   Usage Minimum (Left Ctrl)
    std::byte{0x29}, std::byte{0xE7},        //   Usage Maximum (Right GUI)
    std::byte{0x15}, std::byte{0x00},        //   Logical Minimum (0)
    std::byte{0x25}, std::byte{0x01},        //   Logical Maximum (1)
    std::byte{0x75}, std::byte{0x01},        //   Report Size (1)
    std::byte{0x95}, std::byte{0x08},        //   Report Count (8)
    std::byte{0x81}, std::byte{0x02},        //   Input (Data, Var, Abs)
    std::byte{0x95}, std::byte{0x01},        //   Report Count (1)
    std::byte{0x75}, std::byte{0x08},        //   Report Size (8)
    std::byte{0x81}, std::byte{0x01},        //   Input (Const) — reserved
    std::byte{0x95}, std::byte{0x05},        //   Report Count (5) — LEDs
    std::byte{0x75}, std::byte{0x01},        //   Report Size (1)
    std::byte{0x05}, std::byte{0x08},        //   Usage Page (LEDs)
    std::byte{0x19}, std::byte{0x01},        //   Usage Minimum (NumLock)
    std::byte{0x29}, std::byte{0x05},        //   Usage Maximum (Kana)
    std::byte{0x91}, std::byte{0x02},        //   Output (Data, Var, Abs)
    std::byte{0x95}, std::byte{0x01},        //   Report Count (1)
    std::byte{0x75}, std::byte{0x03},        //   Report Size (3) — padding
    std::byte{0x91}, std::byte{0x01},        //   Output (Const)
    std::byte{0x95}, std::byte{0x06},        //   Report Count (6) — keycodes
    std::byte{0x75}, std::byte{0x08},        //   Report Size (8)
    std::byte{0x15}, std::byte{0x00},        //   Logical Minimum (0)
    std::byte{0x25}, std::byte{0x65},        //   Logical Maximum (101)
    std::byte{0x05}, std::byte{0x07},        //   Usage Page (Keyboard/Keypad)
    std::byte{0x19}, std::byte{0x00},        //   Usage Minimum (0)
    std::byte{0x29}, std::byte{0x65},        //   Usage Maximum (101)
    std::byte{0x81}, std::byte{0x00},        //   Input (Data, Array)
    std::byte{0xC0},                          // End Collection
}};

}  // namespace alloy::drivers::usb::hid
