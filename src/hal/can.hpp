#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::can {

#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

struct nominal_timing_config {
    std::uint32_t prescaler      = 1u;
    std::uint32_t time_seg1      = 1u;
    std::uint32_t time_seg2      = 1u;
    std::uint32_t sync_jump_width = 1u;
};

// ── New structs/enums (extend-can-coverage) ───────────────────────────────────

struct data_timing_config {
    std::uint32_t prescaler      = 1u;
    std::uint32_t sync_jump_width = 1u;
    std::uint32_t time_seg1      = 1u;
    std::uint32_t time_seg2      = 1u;
};

/// Maximum CAN-FD data payload is 64 bytes; classic CAN is limited to 8.
struct CanFrame {
    std::uint32_t id           = 0u;
    bool extended_id           = false;
    bool remote_frame          = false;
    bool fd_format             = false;
    bool bit_rate_switch       = false;
    std::uint8_t dlc           = 0u;
    std::array<std::uint8_t, 64> data{};
};

enum class TestMode : std::uint8_t {
    Normal           = 0u,
    BusMonitor,
    LoopbackInternal,
    LoopbackExternal,
    Restricted,
};

enum class FilterTarget : std::uint8_t {
    Rxfifo0         = 0u,
    Rxfifo1,
    RejectMatching,
    AcceptMatching,
};

enum class FifoId : std::uint8_t { Fifo0 = 0u, Fifo1 };

/// Typed interrupt selector (task 3.1).
enum class InterruptKind : std::uint8_t {
    Tx            = 0u,
    RxFifo0,
    RxFifo1,
    Error,
    BusOff,
    ErrorWarning,
    ErrorPassive,
    Wakeup,
};

// ─────────────────────────────────────────────────────────────────────────────

struct Config {
    bool enter_init_mode       = true;
    bool enable_configuration  = true;
    bool enable_fd_operation   = false;
    bool enable_bit_rate_switch = false;
    bool apply_nominal_timing  = false;
    nominal_timing_config nominal_timing{};
};

template <PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::CanSemanticTraits<Peripheral>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        if constexpr (Peripheral != device::PeripheralId::none) {
            if (const auto clk = detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
                clk.is_err()) {
                return clk;
            }
        }
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.enter_init_mode) {
            result = enter_init_mode();
            if (!result.is_ok()) return result;
        }
        if (config_.enable_configuration) {
            result = enable_configuration();
            if (!result.is_ok()) return result;
        }
        if (config_.enable_fd_operation) {
            result = enable_fd_operation(true);
            if (!result.is_ok()) return result;
        }
        if (config_.enable_bit_rate_switch) {
            result = enable_bit_rate_switch(true);
            if (!result.is_ok()) return result;
        }
        if (config_.apply_nominal_timing) {
            result = set_nominal_timing(config_.nominal_timing);
        }
        return result;
    }

    [[nodiscard]] auto enter_init_mode() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kInitField.valid) {
            return detail::runtime::modify_field(semantic_traits::kInitField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto leave_init_mode() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kInitField.valid) {
            return detail::runtime::modify_field(semantic_traits::kInitField, 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto enable_configuration() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kConfigEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kConfigEnableField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto enable_fd_operation(bool enabled = true) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kFdOperationEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kFdOperationEnableField,
                                                 enabled ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto enable_bit_rate_switch(bool enabled = true) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kBitRateSwitchEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kBitRateSwitchEnableField,
                                                 enabled ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto set_nominal_timing(const nominal_timing_config& cfg) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kNominalPrescalerField.valid &&
                      semantic_traits::kNominalTimeSeg1Field.valid &&
                      semantic_traits::kNominalTimeSeg2Field.valid &&
                      semantic_traits::kNominalSyncJumpWidthField.valid) {
            auto r = detail::runtime::modify_field(semantic_traits::kNominalPrescalerField,
                                                   cfg.prescaler);
            if (!r.is_ok()) return r;
            r = detail::runtime::modify_field(semantic_traits::kNominalTimeSeg1Field,
                                              cfg.time_seg1);
            if (!r.is_ok()) return r;
            r = detail::runtime::modify_field(semantic_traits::kNominalTimeSeg2Field,
                                              cfg.time_seg2);
            if (!r.is_ok()) return r;
            return detail::runtime::modify_field(semantic_traits::kNominalSyncJumpWidthField,
                                                 cfg.sync_jump_width);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 1.1: FD data timing ───────────────────────────────────────────────

    /// Set CAN-FD data-phase timing (DBTP register on MCAN).
    /// Gated on kHasFlexibleDataRate and published data timing fields.
    [[nodiscard]] auto set_data_timing(const data_timing_config& cfg) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kHasFlexibleDataRate &&
                      semantic_traits::kDataPrescalerField.valid &&
                      semantic_traits::kDataTimeSeg1Field.valid &&
                      semantic_traits::kDataTimeSeg2Field.valid &&
                      semantic_traits::kDataSyncJumpWidthField.valid) {
            auto r = detail::runtime::modify_field(semantic_traits::kDataPrescalerField,
                                                   cfg.prescaler);
            if (!r.is_ok()) return r;
            r = detail::runtime::modify_field(semantic_traits::kDataTimeSeg1Field,
                                              cfg.time_seg1);
            if (!r.is_ok()) return r;
            r = detail::runtime::modify_field(semantic_traits::kDataTimeSeg2Field,
                                              cfg.time_seg2);
            if (!r.is_ok()) return r;
            return detail::runtime::modify_field(semantic_traits::kDataSyncJumpWidthField,
                                                 cfg.sync_jump_width);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 1.2: test mode ────────────────────────────────────────────────────

    /// Configure the controller operating mode.
    ///
    /// For MCAN:
    ///   Normal           → clear CCCR.MON(5), CCCR.TEST(7), TEST.LBCK(4), CCCR.ASM(2)
    ///   BusMonitor       → set  CCCR.MON(5)
    ///   LoopbackInternal → set  CCCR.TEST(7), TEST.LBCK(4), CCCR.MON(5)
    ///   LoopbackExternal → set  CCCR.TEST(7), TEST.LBCK(4)
    ///   Restricted       → set  CCCR.ASM(2)  (kBusMonitorField in DB naming)
    ///
    /// Note: CCCR.CCE must be set before calling this (done by enable_configuration()).
    [[nodiscard]] auto set_test_mode(TestMode mode) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kControlRegister.valid) {
            // Read-modify-write CCCR.
            // Bit offsets are fixed per MCAN specification.
            static constexpr std::uint32_t kMonBit  = 1u << 5u;  // Bus Monitoring
            static constexpr std::uint32_t kTestBit = 1u << 7u;  // Test Mode Enable
            static constexpr std::uint32_t kAsmBit  = 1u << 2u;  // Restricted Operation

            const auto addr = semantic_traits::kControlRegister.base_address +
                              semantic_traits::kControlRegister.offset_bytes;
            auto cccr = detail::runtime::read_mmio32(addr);
            // Clear mode bits first.
            cccr &= ~(kMonBit | kTestBit | kAsmBit);

            switch (mode) {
                case TestMode::Normal:
                    break;
                case TestMode::BusMonitor:
                    cccr |= kMonBit;
                    break;
                case TestMode::LoopbackInternal:
                    cccr |= kTestBit | kMonBit;
                    break;
                case TestMode::LoopbackExternal:
                    cccr |= kTestBit;
                    break;
                case TestMode::Restricted:
                    cccr |= kAsmBit;
                    break;
            }
            detail::runtime::write_mmio32(addr, cccr);

            // For loopback modes also set TEST.LBCK (bit 4).
            if constexpr (semantic_traits::kTestRegister.valid) {
                if (mode == TestMode::LoopbackInternal || mode == TestMode::LoopbackExternal) {
                    const auto taddr = semantic_traits::kTestRegister.base_address +
                                       semantic_traits::kTestRegister.offset_bytes;
                    auto test_reg = detail::runtime::read_mmio32(taddr);
                    test_reg |= (1u << 4u);  // LBCK
                    detail::runtime::write_mmio32(taddr, test_reg);
                } else if (mode == TestMode::Normal) {
                    const auto taddr = semantic_traits::kTestRegister.base_address +
                                       semantic_traits::kTestRegister.offset_bytes;
                    auto test_reg = detail::runtime::read_mmio32(taddr);
                    test_reg &= ~(1u << 4u);
                    detail::runtime::write_mmio32(taddr, test_reg);
                }
            }
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 1.3: transmit / receive ──────────────────────────────────────────

    /// Enqueue a CAN/CAN-FD frame for transmission.
    /// Requires message-RAM base address configuration — returns NotSupported
    /// until the full TX buffer backend is wired up.
    [[nodiscard]] auto transmit(const CanFrame& /*frame*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Read one frame from the specified RX FIFO.
    /// Returns NotSupported until full RX buffer backend is wired up.
    [[nodiscard]] auto receive(CanFrame& /*out*/, FifoId /*fifo*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 2.1: filter banks ────────────────────────────────────────────────

    /// Configure a standard (11-bit) filter bank.
    /// Returns NotSupported until message-RAM filter bank backend is wired up.
    [[nodiscard]] auto set_filter_standard(std::uint8_t /*bank*/, std::uint32_t /*id*/,
                                            std::uint32_t /*mask*/, FilterTarget /*target*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Configure an extended (29-bit) filter bank.
    [[nodiscard]] auto set_filter_extended(std::uint8_t /*bank*/, std::uint32_t /*id*/,
                                            std::uint32_t /*mask*/, FilterTarget /*target*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 2.2: error counters + protocol status ────────────────────────────

    /// Transmit error counter (MCAN ECR[7:0]).
    [[nodiscard]] auto tx_error_count() const -> std::uint8_t {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kErrorCounterRegister.valid) {
            const auto addr = semantic_traits::kErrorCounterRegister.base_address +
                              semantic_traits::kErrorCounterRegister.offset_bytes;
            return static_cast<std::uint8_t>(detail::runtime::read_mmio32(addr) & 0xFFu);
        }
        return 0u;
    }

    /// Receive error counter (MCAN ECR[15:8]).
    [[nodiscard]] auto rx_error_count() const -> std::uint8_t {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kErrorCounterRegister.valid) {
            const auto addr = semantic_traits::kErrorCounterRegister.base_address +
                              semantic_traits::kErrorCounterRegister.offset_bytes;
            return static_cast<std::uint8_t>((detail::runtime::read_mmio32(addr) >> 8u) & 0xFFu);
        }
        return 0u;
    }

    /// Returns true when the node is in bus-off state (MCAN PSR[6]).
    [[nodiscard]] auto bus_off() const -> bool {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kProtocolStatusRegister.valid) {
            const auto addr = semantic_traits::kProtocolStatusRegister.base_address +
                              semantic_traits::kProtocolStatusRegister.offset_bytes;
            return (detail::runtime::read_mmio32(addr) & (1u << 6u)) != 0u;
        }
        return false;
    }

    /// Returns true when the node is error-passive (MCAN PSR[4]).
    [[nodiscard]] auto error_passive() const -> bool {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kProtocolStatusRegister.valid) {
            const auto addr = semantic_traits::kProtocolStatusRegister.base_address +
                              semantic_traits::kProtocolStatusRegister.offset_bytes;
            return (detail::runtime::read_mmio32(addr) & (1u << 4u)) != 0u;
        }
        return false;
    }

    /// Returns true when the error-warning limit has been reached (MCAN PSR[5]).
    [[nodiscard]] auto error_warning() const -> bool {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kProtocolStatusRegister.valid) {
            const auto addr = semantic_traits::kProtocolStatusRegister.base_address +
                              semantic_traits::kProtocolStatusRegister.offset_bytes;
            return (detail::runtime::read_mmio32(addr) & (1u << 5u)) != 0u;
        }
        return false;
    }

    // ── Task 2.3: bus-off recovery ────────────────────────────────────────────

    /// Initiate bus-off recovery sequence.
    /// For MCAN, clearing INIT triggers the 128+11 recessive-bit recovery.
    [[nodiscard]] auto request_bus_off_recovery() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        return leave_init_mode();
    }

    // ── Task 3.1: typed interrupts ────────────────────────────────────────────

    /// Enable a specific interrupt kind.
    /// Maps Tx → IE.TCE, RxFifo0 → IE.RF0NE; others are NotSupported
    /// until additional IE field refs are published in the DB.
    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        switch (kind) {
            case InterruptKind::Tx:
                if constexpr (semantic_traits::kTxCompleteInterruptEnableField.valid) {
                    return detail::runtime::modify_field(
                        semantic_traits::kTxCompleteInterruptEnableField, 1u);
                }
                break;
            case InterruptKind::RxFifo0:
                if constexpr (semantic_traits::kRxFifo0NewInterruptEnableField.valid) {
                    return detail::runtime::modify_field(
                        semantic_traits::kRxFifo0NewInterruptEnableField, 1u);
                }
                break;
            default:
                break;
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Disable a specific interrupt kind.
    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        switch (kind) {
            case InterruptKind::Tx:
                if constexpr (semantic_traits::kTxCompleteInterruptEnableField.valid) {
                    return detail::runtime::modify_field(
                        semantic_traits::kTxCompleteInterruptEnableField, 0u);
                }
                break;
            case InterruptKind::RxFifo0:
                if constexpr (semantic_traits::kRxFifo0NewInterruptEnableField.valid) {
                    return detail::runtime::modify_field(
                        semantic_traits::kRxFifo0NewInterruptEnableField, 0u);
                }
                break;
            default:
                break;
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 3.2: IRQ vector ──────────────────────────────────────────────────

    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
    }

    // ── Pre-existing methods ──────────────────────────────────────────────────

    [[nodiscard]] auto enable_rx_fifo0_interrupt() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kRxFifo0NewInterruptEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kRxFifo0NewInterruptEnableField,
                                                 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto rx_fifo0_fill_level() const
        -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kRxFifo0FillLevelField.valid) {
            return detail::runtime::read_field(semantic_traits::kRxFifo0FillLevelField);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto request_tx(std::size_t buffer_index) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");
        if constexpr (semantic_traits::kTxBufferAddRequestPattern.valid) {
            return detail::runtime::modify_indexed_field(
                semantic_traits::kTxBufferAddRequestPattern, buffer_index, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested CAN peripheral is not published for the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::can
