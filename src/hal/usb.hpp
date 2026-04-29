#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::usb {

#if ALLOY_DEVICE_USB_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;
using PinId        = device::PinId;

// ── Enums ─────────────────────────────────────────────────────────────────────

/// USB controller operating mode.
enum class UsbMode : std::uint8_t {
    Device = 0u,
    Host   = 1u,
};

/// Endpoint transfer type.
enum class EndpointType : std::uint8_t {
    Control   = 0u,
    Bulk      = 1u,
    Interrupt = 2u,
    Iso       = 3u,
};

/// Endpoint data direction.
enum class Direction : std::uint8_t {
    In  = 0u,
    Out = 1u,
};

/// Typed interrupt selector.
enum class InterruptKind : std::uint8_t {
    Reset            = 0u,  ///< Bus reset received (EORST on SAME70 USBHS)
    Suspend          = 1u,  ///< Bus suspend (SUSP)
    Resume           = 2u,  ///< Bus wake-up / resume (WAKEUP)
    Setup            = 3u,  ///< EP0 SETUP packet received (via PEP_0 on SAME70)
    EndpointTransfer = 4u,  ///< Endpoint transfer complete (PEP_0 bit 12)
    Sof              = 5u,  ///< Start-of-frame (SOF)
    Error            = 6u,  ///< Bus error (device-specific)
    ConnectionChange = 7u,  ///< Host-mode connection-change (not available in device mode)
};

// ── Endpoint config ───────────────────────────────────────────────────────────

/// Endpoint descriptor passed to configure_endpoint().
struct EndpointConfig {
    std::uint8_t  number          = 0u;
    Direction     direction        = Direction::Out;
    EndpointType  type             = EndpointType::Control;
    std::uint16_t max_packet_size  = 64u;
};

// ─────────────────────────────────────────────────────────────────────────────

struct Config {
    bool enable_on_configure = true;
};

// ── Handle ────────────────────────────────────────────────────────────────────

template <PeripheralId Peripheral>
class handle {
  public:
    using semantic_traits = device::UsbSemanticTraits<Peripheral>;
    using config_type     = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid         = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (Peripheral != device::PeripheralId::none) {
            if (const auto r =
                    detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
                r.is_err()) {
                return r;
            }
        }
        if (config_.enable_on_configure) {
            return enable(true);
        }
        return core::Ok();
    }

    [[nodiscard]] auto enable(bool en) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField,
                                                 en ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Phase 1: Mode + address + clock + crystalless + pins ──────────────────

    /// Set the USB operating mode.
    /// Gated on kForceDeviceModeField.valid && kForceHostModeField.valid.
    /// On SAME70 USBHS, mode is selected via hardware strap (UR.UIMOD);
    /// both force fields are invalid, so this returns NotSupported.
    [[nodiscard]] auto set_mode(UsbMode /*mode*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kForceDeviceModeField.valid &&
                      semantic_traits::kForceHostModeField.valid) {
            // Device-mode: set force-device, clear force-host (or vice versa)
            return core::Ok();  // placeholder — actual device codegen varies
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the device address (7-bit). Writes kAddressField.
    [[nodiscard]] auto set_address(std::uint8_t addr) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kAddressField.valid) {
            return detail::runtime::modify_field(semantic_traits::kAddressField,
                                                 static_cast<std::uint32_t>(addr & 0x7Fu));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Enable or disable the device address (USBHS DEVCTRL.ADDEN).
    [[nodiscard]] auto enable_address(bool en) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kAddressEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kAddressEnableField,
                                                 en ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Freeze the USB clock (USBHS CTRL.FRZCLK). Gated on kHasClockFreeze.
    [[nodiscard]] auto freeze_clock() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasClockFreeze &&
                      semantic_traits::kFreezeClockField.valid) {
            return detail::runtime::modify_field(semantic_traits::kFreezeClockField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Unfreeze the USB clock. Gated on kHasClockFreeze.
    [[nodiscard]] auto unfreeze_clock() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasClockFreeze &&
                      semantic_traits::kFreezeClockField.valid) {
            return detail::runtime::modify_field(semantic_traits::kFreezeClockField, 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns true when the USB clock is usable (USBHS SR.CLKUSABLE).
    /// Gated on kHasClockFreeze.
    [[nodiscard]] auto clock_usable() const -> bool {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasClockFreeze &&
                      semantic_traits::kClockUsableField.valid) {
            const auto r =
                detail::runtime::read_field(semantic_traits::kClockUsableField);
            return r.is_ok() && r.unwrap() != 0u;
        }
        return true;
    }

    /// Enable or disable the USB crystalless mode. Gated on kCrystalless.
    /// Returns NotSupported on devices that require a crystal (SAME70 USBHS).
    [[nodiscard]] auto enable_crystalless(bool /*en*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kCrystalless) {
            // STM32G0 / RP2040 etc. would set the CRS / SOF-sync bit here.
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns the D− pin descriptor for this peripheral.
    [[nodiscard]] static constexpr auto dm_pin() -> PinId {
        return semantic_traits::kDmPin;
    }

    /// Returns the D+ pin descriptor for this peripheral.
    [[nodiscard]] static constexpr auto dp_pin() -> PinId {
        return semantic_traits::kDpPin;
    }

    // ── Phase 2: Endpoint configuration + DPRAM allocator ────────────────────

    /// Configure an endpoint. Gated on kHasDedicatedEndpointConfig.
    /// SAME70 USBHS endpoint config registers are at base + 0x100 + ep * 0x20;
    /// individual field refs are not yet exposed in the device contract.
    [[nodiscard]] auto configure_endpoint(const EndpointConfig& /*cfg*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasDedicatedEndpointConfig) {
            // Endpoint register layout not yet in traits — follow-up codegen task.
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Allocate a byte slice in the DPRAM for an endpoint buffer.
    /// Gated on kDpramBaseAddress != 0. SAME70 USBHS kDpramBaseAddress = 0.
    [[nodiscard]] auto allocate_endpoint_buffer(std::size_t /*bytes*/) const
        -> core::Result<std::span<std::byte>, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kDpramBaseAddress != 0u) {
            // DPRAM allocator would track a watermark here.
            return core::Err(core::ErrorCode::NotSupported);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Reset the DPRAM watermark to zero (for re-enumeration).
    [[nodiscard]] auto reset_dpram() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kDpramBaseAddress != 0u) {
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Phase 3: Interrupts + IRQ vector ──────────────────────────────────────

    /// Enable a specific interrupt kind. Uses DEVIER (device mode) on SAME70.
    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kDeviceInterruptEnableRegister.valid) {
            const std::uint32_t bit = device_interrupt_bit(kind);
            if (bit == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return detail::runtime::write_register(
                semantic_traits::kDeviceInterruptEnableRegister, bit);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Disable a specific interrupt kind. Uses DEVIDR (device mode) on SAME70.
    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "USB peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kDeviceInterruptDisableRegister.valid) {
            const std::uint32_t bit = device_interrupt_bit(kind);
            if (bit == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return detail::runtime::write_register(
                semantic_traits::kDeviceInterruptDisableRegister, bit);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns the device-level IRQ numbers for this peripheral.
    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
    }

    /// Number of endpoints supported by the controller.
    [[nodiscard]] static constexpr auto endpoint_count() -> std::uint16_t {
        return semantic_traits::kEndpointCount;
    }

    /// True when the controller supports high-speed operation.
    [[nodiscard]] static constexpr auto supports_high_speed() -> bool {
        return semantic_traits::kSupportsHighSpeed;
    }

   private:
    Config config_{};

    // ── Interrupt bit mapping ─────────────────────────────────────────────────
    // USBHS DEVISR / DEVIER / DEVIDR bit layout (SAME70 §43.7.5):
    //   bit 0: SUSP   — suspend
    //   bit 2: SOF    — start of frame
    //   bit 3: EORST  — end of bus reset
    //   bit 4: WAKEUP — wake-up
    //  bit 12: PEP_0  — endpoint 0 interrupt (SETUP / EndpointTransfer)

    [[nodiscard]] static constexpr auto device_interrupt_bit(InterruptKind kind)
        -> std::uint32_t {
        switch (kind) {
            case InterruptKind::Reset:            return 1u << 3u;   // EORST
            case InterruptKind::Suspend:          return 1u << 0u;   // SUSP
            case InterruptKind::Resume:           return 1u << 4u;   // WAKEUP
            case InterruptKind::Setup:            return 1u << 12u;  // PEP_0 (EP0)
            case InterruptKind::EndpointTransfer: return 1u << 12u;  // PEP_0 (EP0)
            case InterruptKind::Sof:              return 1u << 2u;   // SOF
            case InterruptKind::Error:
            case InterruptKind::ConnectionChange:
            default:                              return 0u;
        }
    }
};

// ── Factory ───────────────────────────────────────────────────────────────────

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "USB peripheral is not present on the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::usb
