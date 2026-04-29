#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::sdmmc {

#if ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

// ── Enums + structs ───────────────────────────────────────────────────────────

/// SD/MMC data bus width.
/// On SAME70 HSMCI SDCR.SDCBUS: 0 = 1-bit, 2 = 4-bit, 3 = 8-bit.
enum class BusWidth : std::uint8_t {
    Bits1 = 0u,
    Bits4 = 2u,
    Bits8 = 3u,
};

/// SD command response type.
/// Maps directly to HSMCI CMDR.RSPTYP encoding.
enum class ResponseType : std::uint8_t {
    None      = 0u,  ///< No response
    Short     = 1u,  ///< 48-bit response (R1, R3, R6, R7)
    Long      = 2u,  ///< 136-bit response (R2)
    ShortBusy = 3u,  ///< 48-bit response with busy check (R1b)
};

/// Kernel clock source selector.
enum class KernelClockSource : std::uint8_t { Default = 0u };

/// Typed interrupt selector.
enum class InterruptKind : std::uint8_t {
    CommandComplete = 0u,  ///< Command sent, response received (CMDRDY)
    DataComplete    = 1u,  ///< Data transfer finished (XFRDONE)
    DataCrc         = 2u,  ///< Data CRC error (DCRCE)
    DataTimeout     = 3u,  ///< Data timeout (DTOE)
    CommandCrc      = 4u,  ///< Response CRC error (RCRCE)
    CommandTimeout  = 5u,  ///< Response timeout (RTOE)
    RxFifoFull      = 6u,  ///< RX buffer full (RXBUFF)
    TxFifoEmpty     = 7u,  ///< TX buffer empty (TXBUFE)
    CardBusy        = 8u,  ///< Card not-busy edge (NOTBUSY)
    CardDetect      = 9u,  ///< Card detect (not available on all controllers)
};

/// Command descriptor passed to send_command().
struct CommandConfig {
    std::uint8_t  index           = 0u;
    std::uint32_t argument        = 0u;
    ResponseType  response_type   = ResponseType::None;
    bool          wait_for_response = true;
};

/// Command response. `raw[0]` = RSPR0, `raw[1]` = RSPR1 … (136-bit R2 needs
/// all four). Populated only when the hardware exposes response registers via
/// field refs; otherwise zeroed (follow-up: add kResponseRegister0-3 to codegen).
struct Response {
    ResponseType                   type = ResponseType::None;
    std::array<std::uint32_t, 4>   raw{};
};

// ─────────────────────────────────────────────────────────────────────────────

struct Config {
    bool enable_on_configure = true;
};

// ── Handle ────────────────────────────────────────────────────────────────────

template <PeripheralId Peripheral>
class handle {
  public:
    using semantic_traits = device::SdmmcSemanticTraits<Peripheral>;
    using config_type     = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid         = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (Peripheral != device::PeripheralId::none) {
            if (const auto r =
                    detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
                r.is_err()) {
                return r;
            }
        }
        if (config_.enable_on_configure) {
            return enable();
        }
        return core::Ok();
    }

    [[nodiscard]] auto enable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto disable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kDisableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kDisableField, 1u);
        }
        if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto software_reset() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kSoftwareResetField.valid) {
            return detail::runtime::modify_field(semantic_traits::kSoftwareResetField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Phase 1: Bus config + clock + command primitives ──────────────────────

    /// Set the data bus width. BusWidth::Bits8 returns NotSupported on
    /// controllers that only support 4-bit (kSupports8Bit == false).
    [[nodiscard]] auto set_bus_width(BusWidth w) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kBusWidthField.valid) {
            if (w == BusWidth::Bits8 && !semantic_traits::kSupports8Bit) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            if (w == BusWidth::Bits4 && !semantic_traits::kSupports4Bit) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return detail::runtime::modify_field(semantic_traits::kBusWidthField,
                                                 static_cast<std::uint32_t>(w));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the clock divider (HSMCI MR.CLKDIV).
    /// f_sd = f_mck / (2 × (divider + 1)).
    [[nodiscard]] auto set_clock_divider(std::uint16_t divider) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kClockDividerField.valid) {
            return detail::runtime::modify_field(semantic_traits::kClockDividerField,
                                                 static_cast<std::uint32_t>(divider));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Select the SDMMC kernel clock source.
    /// Returns NotSupported on all current devices (no kKernelClockSelectorField).
    [[nodiscard]] auto set_kernel_clock_source(KernelClockSource /*src*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kKernelClockSourceOptions.size() == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Issue an SD/MMC command and optionally wait for the response.
    ///
    /// HSMCI note: CMDR is write-only and triggers the command on write.
    /// The argument register (ARGR) must be written first.
    /// Response registers (RSPR0–3) are not yet exposed in the device contract;
    /// Response::raw is zeroed pending a codegen extension (follow-up).
    [[nodiscard]] auto send_command(const CommandConfig& cmd) const
        -> core::Result<Response, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");

        if constexpr (semantic_traits::kArgumentRegister.valid &&
                      semantic_traits::kCommandRegister.valid) {
            // 1. Write argument
            if (const auto r = detail::runtime::write_register(
                    semantic_traits::kArgumentRegister, cmd.argument);
                !r.is_ok()) {
                return core::Err(r.unwrap_err());
            }

            // 2. Compose and write CMDR in one shot (write-only, triggers command)
            const std::uint32_t cmdr =
                (static_cast<std::uint32_t>(cmd.index) & 0x3Fu) |
                ((static_cast<std::uint32_t>(cmd.response_type) & 0x3u) << 6u);

            if (const auto r = detail::runtime::write_register(
                    semantic_traits::kCommandRegister, cmdr);
                !r.is_ok()) {
                return core::Err(r.unwrap_err());
            }

            // 3. Poll for command ready (CMDRDY)
            if (cmd.wait_for_response) {
                if constexpr (semantic_traits::kCommandReadyField.valid) {
                    bool ready = false;
                    for (std::size_t i = 0u; i < 0xFFFFu && !ready; ++i) {
                        const auto r =
                            detail::runtime::read_field(semantic_traits::kCommandReadyField);
                        ready = r.is_ok() && r.unwrap() != 0u;
                    }
                    if (!ready) {
                        return core::Err(core::ErrorCode::Timeout);
                    }
                }
            }

            // 4. Return response (RSPR0-3 not yet in traits — follow-up codegen task)
            return core::Ok(Response{cmd.response_type, {}});
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Phase 2: Block transfer + DMA + timeouts ──────────────────────────────

    /// Set the block size in bytes (HSMCI BLKR.BLKLEN).
    [[nodiscard]] auto set_block_size(std::uint16_t size) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kBlockLengthField.valid) {
            return detail::runtime::modify_field(semantic_traits::kBlockLengthField,
                                                 static_cast<std::uint32_t>(size));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the block count (HSMCI BLKR.BCNT).
    [[nodiscard]] auto set_block_count(std::uint16_t count) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kBlockCountField.valid) {
            return detail::runtime::modify_field(semantic_traits::kBlockCountField,
                                                 static_cast<std::uint32_t>(count));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Blocking multi-word read via the receive data register (HSMCI RDR).
    /// Polls kRxReadyField before each 32-bit word.
    /// Caller must have already sent a READ command with transfer enabled.
    [[nodiscard]] auto read_blocks(std::span<std::byte> buf) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kReadDataRegister.valid &&
                      semantic_traits::kRxReadyField.valid) {
            const std::size_t words = buf.size() / sizeof(std::uint32_t);
            for (std::size_t w = 0u; w < words; ++w) {
                bool ready = false;
                for (std::size_t i = 0u; i < 0xFFFFu && !ready; ++i) {
                    const auto r =
                        detail::runtime::read_field(semantic_traits::kRxReadyField);
                    ready = r.is_ok() && r.unwrap() != 0u;
                }
                if (!ready) {
                    return core::Err(core::ErrorCode::Timeout);
                }
                const auto r =
                    detail::runtime::read_register(semantic_traits::kReadDataRegister);
                if (!r.is_ok()) {
                    return core::Err(r.unwrap_err());
                }
                const std::uint32_t word = r.unwrap();
                const std::size_t byte_idx = w * sizeof(std::uint32_t);
                buf[byte_idx + 0u] = static_cast<std::byte>((word >>  0u) & 0xFFu);
                buf[byte_idx + 1u] = static_cast<std::byte>((word >>  8u) & 0xFFu);
                buf[byte_idx + 2u] = static_cast<std::byte>((word >> 16u) & 0xFFu);
                buf[byte_idx + 3u] = static_cast<std::byte>((word >> 24u) & 0xFFu);
            }
            // Wait for transfer done
            if constexpr (semantic_traits::kTransferDoneField.valid) {
                bool done = false;
                for (std::size_t i = 0u; i < 0xFFFFu && !done; ++i) {
                    const auto r =
                        detail::runtime::read_field(semantic_traits::kTransferDoneField);
                    done = r.is_ok() && r.unwrap() != 0u;
                }
                if (!done) {
                    return core::Err(core::ErrorCode::Timeout);
                }
            }
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Blocking multi-word write via the transmit data register (HSMCI TDR).
    /// Polls kTxReadyField before each 32-bit word.
    /// Caller must have already sent a WRITE command with transfer enabled.
    [[nodiscard]] auto write_blocks(std::span<const std::byte> buf) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kWriteDataRegister.valid &&
                      semantic_traits::kTxReadyField.valid) {
            const std::size_t words = buf.size() / sizeof(std::uint32_t);
            for (std::size_t w = 0u; w < words; ++w) {
                bool ready = false;
                for (std::size_t i = 0u; i < 0xFFFFu && !ready; ++i) {
                    const auto r =
                        detail::runtime::read_field(semantic_traits::kTxReadyField);
                    ready = r.is_ok() && r.unwrap() != 0u;
                }
                if (!ready) {
                    return core::Err(core::ErrorCode::Timeout);
                }
                const std::size_t byte_idx = w * sizeof(std::uint32_t);
                const std::uint32_t word =
                    (static_cast<std::uint32_t>(buf[byte_idx + 0u])) |
                    (static_cast<std::uint32_t>(buf[byte_idx + 1u]) <<  8u) |
                    (static_cast<std::uint32_t>(buf[byte_idx + 2u]) << 16u) |
                    (static_cast<std::uint32_t>(buf[byte_idx + 3u]) << 24u);
                const auto r = detail::runtime::write_register(
                    semantic_traits::kWriteDataRegister, word);
                if (!r.is_ok()) {
                    return r;
                }
            }
            // Wait for transfer done
            if constexpr (semantic_traits::kTransferDoneField.valid) {
                bool done = false;
                for (std::size_t i = 0u; i < 0xFFFFu && !done; ++i) {
                    const auto r =
                        detail::runtime::read_field(semantic_traits::kTransferDoneField);
                    done = r.is_ok() && r.unwrap() != 0u;
                }
                if (!done) {
                    return core::Err(core::ErrorCode::Timeout);
                }
            }
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Enable DMA for data transfers. Gated on kHasDma / kDmaEnableField.
    [[nodiscard]] auto enable_dma(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasDma && semantic_traits::kDmaEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kDmaEnableField,
                                                 enable ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Wire up a DMA channel. Gated on kHasDma.
    template <typename DmaChannel>
    [[nodiscard]] auto configure_dma(const DmaChannel& channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasDma) {
            static_assert(DmaChannel::valid);
            static_assert(DmaChannel::peripheral_id == peripheral_id);
            return channel.configure();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the data timeout (HSMCI DTOR register raw value).
    [[nodiscard]] auto set_data_timeout(std::uint32_t cycles) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kDataTimeoutRegister.valid) {
            return detail::runtime::write_register(
                semantic_traits::kDataTimeoutRegister, cycles);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the completion timeout (HSMCI CSTOR register raw value).
    [[nodiscard]] auto set_completion_timeout(std::uint32_t cycles) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kCompletionTimeoutRegister.valid) {
            return detail::runtime::write_register(
                semantic_traits::kCompletionTimeoutRegister, cycles);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Phase 3: Interrupts + IRQ vector ──────────────────────────────────────

    /// Enable a specific interrupt kind (writes to IER on HSMCI).
    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kInterruptEnableRegister.valid) {
            const std::uint32_t bit = interrupt_kind_to_bit(kind);
            if (bit == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return detail::runtime::write_register(
                semantic_traits::kInterruptEnableRegister, bit);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Disable a specific interrupt kind (writes to IDR on HSMCI).
    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "SDMMC peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kInterruptDisableRegister.valid) {
            const std::uint32_t bit = interrupt_kind_to_bit(kind);
            if (bit == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return detail::runtime::write_register(
                semantic_traits::kInterruptDisableRegister, bit);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns the device-level IRQ numbers for this peripheral.
    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
    }

    // ── Data register addresses (for DMA descriptors) ─────────────────────────

    [[nodiscard]] static constexpr auto rx_data_register_address() -> std::uintptr_t {
        if constexpr (semantic_traits::kReadDataRegister.valid) {
            return semantic_traits::kReadDataRegister.base_address +
                   semantic_traits::kReadDataRegister.offset_bytes;
        }
        return 0u;
    }

    [[nodiscard]] static constexpr auto tx_data_register_address() -> std::uintptr_t {
        if constexpr (semantic_traits::kWriteDataRegister.valid) {
            return semantic_traits::kWriteDataRegister.base_address +
                   semantic_traits::kWriteDataRegister.offset_bytes;
        }
        return 0u;
    }

   private:
    Config config_{};

    // ── Interrupt bit mapping ─────────────────────────────────────────────────
    // IER and IDR bit positions mirror the SR register layout (HSMCI-specific).
    // Where a status FieldRef exists, we derive the bit offset from it;
    // otherwise use the known HSMCI SR bit positions (see SAME70 datasheet §40).

    [[nodiscard]] static constexpr auto interrupt_kind_to_bit(InterruptKind kind)
        -> std::uint32_t {
        switch (kind) {
            case InterruptKind::CommandComplete:
                return semantic_traits::kCommandReadyField.valid
                       ? (1u << semantic_traits::kCommandReadyField.bit_offset) : 0u;
            case InterruptKind::DataComplete:
                return semantic_traits::kTransferDoneField.valid
                       ? (1u << semantic_traits::kTransferDoneField.bit_offset) : 0u;
            case InterruptKind::CardBusy:
                return semantic_traits::kNotBusyField.valid
                       ? (1u << semantic_traits::kNotBusyField.bit_offset) : 0u;
            case InterruptKind::DataCrc:       return 1u << 21u;  // DCRCE
            case InterruptKind::DataTimeout:   return 1u << 22u;  // DTOE
            case InterruptKind::CommandCrc:    return 1u << 18u;  // RCRCE
            case InterruptKind::CommandTimeout: return 1u << 20u; // RTOE
            case InterruptKind::RxFifoFull:    return 1u << 14u;  // RXBUFF
            case InterruptKind::TxFifoEmpty:   return 1u << 15u;  // TXBUFE
            case InterruptKind::CardDetect:
            default:                           return 0u;
        }
    }
};

// ── Factory ───────────────────────────────────────────────────────────────────

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "SDMMC peripheral is not present on the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::sdmmc
