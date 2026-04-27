// Compile test: pin the alloy::hal::usb::UsbDeviceController C++20 concept and
// verify the host-side MockUsbController satisfies it. A "fake" minimal
// controller below documents the smallest legal implementation.

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/usb/backends/mock_usb_controller.hpp"
#include "hal/usb/usb_device_controller.hpp"

namespace {

namespace usb = alloy::hal::usb;
using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;
using ResultSize = alloy::core::Result<std::size_t, alloy::core::ErrorCode>;

// ── Smallest legal implementation of the concept ─────────────────────────────
struct MinimalController {
    [[nodiscard]] auto configure(usb::UsbSpeed, usb::EventHandler, usb::SetupHandler,
                                 void*) -> ResultVoid {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto connect()    -> ResultVoid { return alloy::core::Ok(); }
    [[nodiscard]] auto disconnect() -> ResultVoid { return alloy::core::Ok(); }
    [[nodiscard]] auto enable_endpoint(usb::EndpointAddress, usb::EndpointType,
                                       std::size_t) -> ResultVoid {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto disable_endpoint(usb::EndpointAddress) -> ResultVoid {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto write(usb::EndpointAddress, std::span<const std::byte>)
        -> ResultVoid { return alloy::core::Ok(); }
    [[nodiscard]] auto read(usb::EndpointAddress, std::span<std::byte>) -> ResultSize {
        return alloy::core::Ok(std::size_t{0u});
    }
    [[nodiscard]] auto stall(usb::EndpointAddress)   -> ResultVoid { return alloy::core::Ok(); }
    [[nodiscard]] auto unstall(usb::EndpointAddress) -> ResultVoid { return alloy::core::Ok(); }
    [[nodiscard]] auto set_address(std::uint8_t)     -> ResultVoid { return alloy::core::Ok(); }
    [[nodiscard]] auto service()                     -> ResultVoid { return alloy::core::Ok(); }
};

static_assert(usb::UsbDeviceController<MinimalController>,
              "MinimalController must satisfy UsbDeviceController");
static_assert(usb::UsbDeviceController<alloy::hal::usb::backends::MockUsbController>,
              "MockUsbController must satisfy UsbDeviceController");

// ── EndpointAddress packing checks ────────────────────────────────────────────
static_assert(usb::EndpointAddress::make(2u, usb::EndpointDirection::In).raw == 0x82u);
static_assert(usb::EndpointAddress::make(3u, usb::EndpointDirection::Out).raw == 0x03u);
static_assert(usb::EndpointAddress::make(2u, usb::EndpointDirection::In).number() == 2u);
static_assert(usb::EndpointAddress::make(2u, usb::EndpointDirection::In).direction()
              == usb::EndpointDirection::In);

}  // namespace

[[maybe_unused]] void compile_usb_controller_api() {
    alloy::hal::usb::backends::MockUsbController mock;
    [[maybe_unused]] const auto cfg = mock.configure(usb::UsbSpeed::Full, nullptr, nullptr, nullptr);
    [[maybe_unused]] const auto conn = mock.connect();
    [[maybe_unused]] const auto en = mock.enable_endpoint(
        usb::EndpointAddress::make(1, usb::EndpointDirection::In),
        usb::EndpointType::Bulk, 64u);
}
