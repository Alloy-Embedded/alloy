// Compile test: build full USB descriptor trees at constexpr time and pin
// their byte-exact size. Catches accidental layout changes in the descriptor
// builder.

#include <array>
#include <cstddef>
#include <cstdint>

#include "drivers/usb/cdc_acm/cdc_acm.hpp"
#include "drivers/usb/dfu/dfu.hpp"
#include "drivers/usb/hid/hid.hpp"
#include "hal/usb/usb_descriptor.hpp"

namespace {

namespace desc = alloy::hal::usb::descriptor;

// ── Device descriptor: exactly 18 bytes per USB 2.0 §9.6.1 ─────────────────
constexpr auto kDeviceDescriptor = desc::device_descriptor({
    .vendor_id = 0xCAFEu,
    .product_id = 0xBEEFu,
    .num_configurations = 1u,
});
static_assert(kDeviceDescriptor.size() == 18u);
static_assert(static_cast<std::uint8_t>(kDeviceDescriptor[0]) == 18u);
static_assert(static_cast<std::uint8_t>(kDeviceDescriptor[1]) == 1u);  // Device
static_assert(static_cast<std::uint8_t>(kDeviceDescriptor[8])  == 0xFEu);
static_assert(static_cast<std::uint8_t>(kDeviceDescriptor[9])  == 0xCAu);
static_assert(static_cast<std::uint8_t>(kDeviceDescriptor[10]) == 0xEFu);
static_assert(static_cast<std::uint8_t>(kDeviceDescriptor[11]) == 0xBEu);

// ── CDC-ACM full configuration tree ────────────────────────────────────────
//   config(9) + iface(9) + cdc_header(5) + cdc_callmgmt(5) + cdc_acm(4) +
//   cdc_union(5) + notify_ep(7) + iface(9) + in_ep(7) + out_ep(7) = 67 bytes
constexpr auto kCdcConfig =
    alloy::drivers::usb::cdc_acm::build_configuration_descriptor();
static_assert(kCdcConfig.size() == 67u);
// wTotalLength is patched to N at offset 2-3.
static_assert(static_cast<std::uint8_t>(kCdcConfig[2]) == 67u);
static_assert(static_cast<std::uint8_t>(kCdcConfig[3]) == 0u);

// ── DFU functional descriptor: 9 bytes ─────────────────────────────────────
constexpr auto kDfuFunctional = alloy::drivers::usb::dfu::dfu_functional_descriptor({
    .attributes = 0x07u,
    .detach_timeout_ms = 1000u,
    .transfer_size = 1024u,
    .bcd_dfu = 0x0110u,
});
static_assert(kDfuFunctional.size() == 9u);

// ── HID descriptor: 9 bytes; boot keyboard report descriptor: 63 bytes ─────
constexpr auto kHidDesc = alloy::drivers::usb::hid::hid_descriptor({
    .bcd_hid = 0x0111u,
    .country_code = 0u,
    .report_descriptor_length = 63u,
});
static_assert(kHidDesc.size() == 9u);
static_assert(alloy::drivers::usb::hid::kBootKeyboardReportDescriptor.size() == 63u);

// ── Device descriptor + concat ─────────────────────────────────────────────
// Ensure concat_descriptors compiles for arbitrary fold expressions.
constexpr auto kCombined = desc::concat_descriptors(kDeviceDescriptor, kHidDesc);
static_assert(kCombined.size() == 27u);

}  // namespace

[[maybe_unused]] void compile_usb_descriptor_api() {
    static_cast<void>(kDeviceDescriptor);
    static_cast<void>(kCdcConfig);
    static_cast<void>(kDfuFunctional);
    static_cast<void>(kHidDesc);
}
