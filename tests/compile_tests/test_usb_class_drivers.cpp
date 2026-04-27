// Compile test: instantiate every USB class driver against the host-side
// MockUsbController, exercise the public surface, and pin a few state
// transitions. Acts as a sanity check that the driver headers + the concept
// + the mock controller compose without surprises.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

#include "drivers/usb/cdc_acm/cdc_acm.hpp"
#include "drivers/usb/dfu/dfu.hpp"
#include "drivers/usb/dfu/flash_backend.hpp"
#include "drivers/usb/hid/hid.hpp"
#include "hal/usb/backends/mock_usb_controller.hpp"

namespace {

namespace usb = alloy::hal::usb;
using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;

// ── Mock flash backend satisfying drivers::usb::dfu::FlashBackend ──────────
struct MockFlash {
    [[nodiscard]] auto erase(std::uint32_t, std::size_t) -> ResultVoid {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto write(std::uint32_t, std::span<const std::byte>) -> ResultVoid {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto read(std::uint32_t, std::span<std::byte>) -> ResultVoid {
        return alloy::core::Ok();
    }
};

static_assert(alloy::drivers::usb::dfu::FlashBackend<MockFlash>);

// ── Type aliases ───────────────────────────────────────────────────────────
using Mock = alloy::hal::usb::backends::MockUsbController;
using CdcAcm = alloy::drivers::usb::cdc_acm::CdcAcm<Mock>;
using DfuRuntime = alloy::drivers::usb::dfu::DfuRuntime<Mock>;
using DfuDownload = alloy::drivers::usb::dfu::DfuDownload<Mock, MockFlash>;
using HidKeyboard = alloy::drivers::usb::hid::HidDevice<Mock, 8u>;

}  // namespace

[[maybe_unused]] void compile_usb_class_drivers() {
    Mock mock;
    [[maybe_unused]] const auto cfg = mock.configure(
        usb::UsbSpeed::Full, nullptr, nullptr, nullptr);

    // CDC-ACM ----------------------------------------------------------------
    CdcAcm cdc{mock};
    [[maybe_unused]] const auto cdc_cfg = cdc.configure();
    std::array<std::byte, 4> tx_buf{};
    [[maybe_unused]] const auto cdc_w = cdc.write(tx_buf);
    std::array<std::byte, 4> rx_buf{};
    [[maybe_unused]] const auto cdc_r = cdc.read(rx_buf);
    static_cast<void>(cdc.is_connected());
    static_cast<void>(cdc.line_coding());
    static_cast<void>(cdc.control_line_state());

    // CDC-ACM SET_LINE_CODING handler --------------------------------------
    constexpr usb::SetupPacket kSetLineCoding{
        .bmRequestType = 0x21u,
        .bRequest = alloy::drivers::usb::cdc_acm::kCdcRequestSetLineCoding,
        .wValue = 0u,
        .wIndex = 0u,
        .wLength = 7u,
    };
    constexpr std::array<std::byte, 7> kCodingPayload{{
        std::byte{0x00}, std::byte{0xC2}, std::byte{0x01}, std::byte{0x00},  // 115200 baud
        std::byte{0x00}, std::byte{0x00}, std::byte{0x08},
    }};
    [[maybe_unused]] const auto cdc_setup =
        cdc.handle_class_request(kSetLineCoding, kCodingPayload);

    // DFU runtime ----------------------------------------------------------
    DfuRuntime dfu_rt{mock};
    constexpr usb::SetupPacket kDfuDetach{
        .bmRequestType = 0x21u,
        .bRequest = alloy::drivers::usb::dfu::kDfuRequestDetach,
        .wValue = 1000u,
    };
    [[maybe_unused]] const auto rt_response = dfu_rt.handle_class_request(kDfuDetach, {});
    static_cast<void>(dfu_rt.state());

    // DFU downloader ------------------------------------------------------
    MockFlash flash;
    DfuDownload dl{mock, flash, 0x08000000u, 0x00010000u};
    static_cast<void>(dl.state());
    static_cast<void>(dl.status());
    static_cast<void>(dl.bytes_written());

    // HID --------------------------------------------------------------------
    HidKeyboard hid{
        mock,
        std::span<const std::byte>{
            alloy::drivers::usb::hid::kBootKeyboardReportDescriptor},
        usb::EndpointAddress::make(1, usb::EndpointDirection::In),
    };
    [[maybe_unused]] const auto hid_cfg = hid.configure();
    constexpr std::array<std::byte, 8> kHidReport{};
    [[maybe_unused]] const auto hid_send = hid.send_report(kHidReport);
}
