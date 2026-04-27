#pragma once

// USB DFU 1.1 (Device Firmware Upgrade) class driver.
//
// Two modes per spec (USB DFU 1.1, §3 + §6):
//   - Runtime mode: the device is in its normal operational role (CDC-ACM,
//     HID, vendor) and exposes a DFU interface advertising the
//     bcdDFU/wTransferSize fields. On DFU_DETACH, the device disconnects,
//     re-enumerates as a DFU-mode-only device (or jumps to a bootloader).
//   - DFU mode: the device only exposes the DFU interface. DFU_DNLOAD writes
//     firmware blocks; DFU_GETSTATE / DFU_GETSTATUS poll progress;
//     DFU_DNLOAD with wLength=0 finalises and triggers verification.
//
// `DfuRuntime` exposes only the runtime interface (Detach handling).
// `DfuDownload<FlashBackend>` is the bootloader-side downloader: it owns a
// FlashBackend and commits each DFU_DNLOAD block to flash.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/usb/dfu/flash_backend.hpp"
#include "hal/usb/usb_descriptor.hpp"
#include "hal/usb/usb_device_controller.hpp"
#include "hal/usb/usb_setup_packet.hpp"

namespace alloy::drivers::usb::dfu {

namespace usb = alloy::hal::usb;
namespace desc = alloy::hal::usb::descriptor;

inline constexpr std::uint8_t kDfuInterfaceClass    = 0xFEu;
inline constexpr std::uint8_t kDfuInterfaceSubclass = 0x01u;
inline constexpr std::uint8_t kDfuProtocolRuntime   = 0x01u;
inline constexpr std::uint8_t kDfuProtocolDfuMode   = 0x02u;

inline constexpr std::uint8_t kDfuRequestDetach   = 0x00u;
inline constexpr std::uint8_t kDfuRequestDnload   = 0x01u;
inline constexpr std::uint8_t kDfuRequestUpload   = 0x02u;
inline constexpr std::uint8_t kDfuRequestGetStatus = 0x03u;
inline constexpr std::uint8_t kDfuRequestClrStatus = 0x04u;
inline constexpr std::uint8_t kDfuRequestGetState  = 0x05u;
inline constexpr std::uint8_t kDfuRequestAbort     = 0x06u;

/// DFU device state machine (USB DFU 1.1 §6.1.2). Subset relevant for the
/// runtime + downloader paths.
enum class DfuState : std::uint8_t {
    AppIdle = 0u,
    AppDetach = 1u,
    DfuIdle = 2u,
    DfuDnloadSync = 3u,
    DfuDnBusy = 4u,
    DfuDnloadIdle = 5u,
    DfuManifestSync = 6u,
    DfuManifest = 7u,
    DfuManifestWaitReset = 8u,
    DfuUploadIdle = 9u,
    DfuError = 10u,
};

enum class DfuStatus : std::uint8_t {
    Ok = 0x00u,
    ErrTarget = 0x01u,
    ErrFile = 0x02u,
    ErrWrite = 0x03u,
    ErrErase = 0x04u,
    ErrCheckErased = 0x05u,
    ErrProg = 0x06u,
    ErrVerify = 0x07u,
    ErrAddress = 0x08u,
    ErrNotDone = 0x09u,
    ErrFirmware = 0x0Au,
    ErrVendor = 0x0Bu,
    ErrUsbR = 0x0Cu,
    ErrPor = 0x0Du,
    ErrUnknown = 0x0Eu,
    ErrStalledPkt = 0x0Fu,
};

/// 9-byte DFU functional descriptor (USB DFU 1.1 §4.1.3).
struct DfuFunctionalSpec {
    std::uint8_t  attributes = 0x07u;       ///< CanDownload | CanUpload | ManifestationTolerant.
    std::uint16_t detach_timeout_ms = 1000u;
    std::uint16_t transfer_size = 1024u;
    std::uint16_t bcd_dfu = 0x0110u;
};

[[nodiscard]] constexpr auto dfu_functional_descriptor(const DfuFunctionalSpec& spec)
    -> std::array<std::byte, 9> {
    auto byte = [](std::uint32_t v) { return static_cast<std::byte>(v & 0xFFu); };
    auto lo   = [](std::uint16_t v) { return static_cast<std::byte>(v & 0xFFu); };
    auto hi   = [](std::uint16_t v) { return static_cast<std::byte>((v >> 8u) & 0xFFu); };
    return {{
        byte(9u),
        byte(0x21u),                       // bDescriptorType = DFU_FUNCTIONAL
        byte(spec.attributes),
        lo(spec.detach_timeout_ms), hi(spec.detach_timeout_ms),
        lo(spec.transfer_size), hi(spec.transfer_size),
        lo(spec.bcd_dfu), hi(spec.bcd_dfu),
    }};
}

// ── Runtime mode — handles DFU_DETACH only ─────────────────────────────────

/// DFU Runtime state — added as an additional interface to a non-DFU
/// configuration (alongside CDC-ACM, HID, vendor). The host can issue
/// DFU_DETACH which signals the application to switch into DFU mode (typical
/// implementation: jump to bootloader).
template <usb::UsbDeviceController Controller>
class DfuRuntime {
   public:
    using detach_callback = void (*)(void* user_data);

    explicit DfuRuntime(Controller& controller) : controller_(&controller) {}

    void on_detach(detach_callback fn, void* user_data) {
        detach_cb_ = fn;
        detach_user_data_ = user_data;
    }

    [[nodiscard]] auto handle_class_request(const usb::SetupPacket& setup,
                                            std::span<const std::byte>)
        -> core::Result<std::span<const std::byte>, core::ErrorCode> {
        if (!setup.is_class() ||
            setup.recipient() != usb::RequestRecipient::Interface) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        switch (setup.bRequest) {
            case kDfuRequestDetach:
                state_ = DfuState::AppDetach;
                if (detach_cb_ != nullptr) {
                    detach_cb_(detach_user_data_);
                }
                return core::Ok(std::span<const std::byte>{});
            case kDfuRequestGetStatus:
                return get_status_response();
            case kDfuRequestGetState: {
                state_response_[0] = static_cast<std::byte>(state_);
                return core::Ok(std::span<const std::byte>{state_response_.data(),
                                                            state_response_.size()});
            }
            default:
                return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto state() const -> DfuState { return state_; }

   private:
    [[nodiscard]] auto get_status_response()
        -> core::Result<std::span<const std::byte>, core::ErrorCode> {
        get_status_response_[0] = static_cast<std::byte>(DfuStatus::Ok);
        get_status_response_[1] = static_cast<std::byte>(0u);  // bwPollTimeout LSB
        get_status_response_[2] = static_cast<std::byte>(0u);
        get_status_response_[3] = static_cast<std::byte>(0u);
        get_status_response_[4] = static_cast<std::byte>(state_);
        get_status_response_[5] = static_cast<std::byte>(0u);
        return core::Ok(std::span<const std::byte>{get_status_response_.data(),
                                                    get_status_response_.size()});
    }

    Controller* controller_ = nullptr;
    DfuState state_ = DfuState::AppIdle;
    detach_callback detach_cb_ = nullptr;
    void*           detach_user_data_ = nullptr;
    std::array<std::byte, 6> get_status_response_{};
    std::array<std::byte, 1> state_response_{};
};

// ── DFU Download mode — bootloader path ────────────────────────────────────

/// Bootloader-side downloader. Receives DFU_DNLOAD blocks and commits them
/// to a FlashBackend. After the host signals end-of-transfer (DFU_DNLOAD with
/// wLength=0), the downloader transitions through DfuManifest →
/// DfuManifestWaitReset; the application is expected to issue a system reset
/// to load the new firmware.
template <usb::UsbDeviceController Controller, FlashBackend Flash>
class DfuDownload {
   public:
    DfuDownload(Controller& controller, Flash& flash, std::uint32_t base_address,
                std::uint32_t region_size)
        : controller_(&controller),
          flash_(&flash),
          base_address_(base_address),
          region_size_(region_size) {}

    [[nodiscard]] auto handle_class_request(const usb::SetupPacket& setup,
                                            std::span<const std::byte> data_out)
        -> core::Result<std::span<const std::byte>, core::ErrorCode> {
        if (!setup.is_class() ||
            setup.recipient() != usb::RequestRecipient::Interface) {
            return core::Err(core::ErrorCode::NotSupported);
        }

        switch (setup.bRequest) {
            case kDfuRequestDnload:
                return handle_dnload(setup, data_out);
            case kDfuRequestGetStatus:
                return get_status_response();
            case kDfuRequestGetState: {
                state_response_[0] = static_cast<std::byte>(state_);
                return core::Ok(std::span<const std::byte>{state_response_.data(),
                                                            state_response_.size()});
            }
            case kDfuRequestClrStatus:
                if (state_ == DfuState::DfuError) {
                    state_ = DfuState::DfuIdle;
                    status_ = DfuStatus::Ok;
                }
                return core::Ok(std::span<const std::byte>{});
            case kDfuRequestAbort:
                state_ = DfuState::DfuIdle;
                bytes_written_ = 0u;
                return core::Ok(std::span<const std::byte>{});
            default:
                return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto state()  const -> DfuState  { return state_; }
    [[nodiscard]] auto status() const -> DfuStatus { return status_; }
    [[nodiscard]] auto bytes_written() const -> std::uint32_t { return bytes_written_; }

   private:
    [[nodiscard]] auto handle_dnload(const usb::SetupPacket& setup,
                                     std::span<const std::byte> data_out)
        -> core::Result<std::span<const std::byte>, core::ErrorCode> {
        // wLength == 0 → end-of-transfer marker.
        if (setup.wLength == 0u) {
            state_ = DfuState::DfuManifestSync;
            return core::Ok(std::span<const std::byte>{});
        }

        // First block — erase the target region before writing.
        if (state_ == DfuState::DfuIdle) {
            if (auto r = flash_->erase(base_address_, region_size_); !r.is_ok()) {
                state_  = DfuState::DfuError;
                status_ = DfuStatus::ErrErase;
                return core::Err(r.err());
            }
            bytes_written_ = 0u;
        }

        if (bytes_written_ + data_out.size() > region_size_) {
            state_  = DfuState::DfuError;
            status_ = DfuStatus::ErrAddress;
            return core::Err(core::ErrorCode::OutOfRange);
        }

        if (auto r = flash_->write(base_address_ + bytes_written_, data_out); !r.is_ok()) {
            state_  = DfuState::DfuError;
            status_ = DfuStatus::ErrWrite;
            return core::Err(r.err());
        }

        bytes_written_ += static_cast<std::uint32_t>(data_out.size());
        state_ = DfuState::DfuDnloadIdle;
        return core::Ok(std::span<const std::byte>{});
    }

    [[nodiscard]] auto get_status_response()
        -> core::Result<std::span<const std::byte>, core::ErrorCode> {
        // Manifest → wait-reset transition is the canonical "host saw the
        // status response, we can now reset" hand-off.
        if (state_ == DfuState::DfuManifestSync) {
            state_ = DfuState::DfuManifestWaitReset;
        }
        get_status_response_[0] = static_cast<std::byte>(status_);
        get_status_response_[1] = static_cast<std::byte>(0u);
        get_status_response_[2] = static_cast<std::byte>(0u);
        get_status_response_[3] = static_cast<std::byte>(0u);
        get_status_response_[4] = static_cast<std::byte>(state_);
        get_status_response_[5] = static_cast<std::byte>(0u);
        return core::Ok(std::span<const std::byte>{get_status_response_.data(),
                                                    get_status_response_.size()});
    }

    Controller* controller_ = nullptr;
    Flash*      flash_ = nullptr;
    std::uint32_t base_address_ = 0u;
    std::uint32_t region_size_  = 0u;
    DfuState  state_  = DfuState::DfuIdle;
    DfuStatus status_ = DfuStatus::Ok;
    std::uint32_t bytes_written_ = 0u;
    std::array<std::byte, 6> get_status_response_{};
    std::array<std::byte, 1> state_response_{};
};

}  // namespace alloy::drivers::usb::dfu
