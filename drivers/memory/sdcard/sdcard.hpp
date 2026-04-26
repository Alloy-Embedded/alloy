#pragma once

// SD card driver — SPI mode
//
// Implements the BlockDevice concept for SD/SDHC/SDXC cards over SPI.
// Only SDHC/SDXC (block-addressed, CCS=1) cards are supported; SDSC
// (byte-addressed, legacy) is rejected with ErrorCode::NotSupported.
//
// Template parameters:
//   Spi       — bus handle: must provide transmit(span<const uint8_t>) and
//               receive(span<uint8_t>), each returning Result<void, ErrorCode>.
//   CsPolicy  — chip-select policy (default: NoOpCsPolicy).
//               Must provide: assert_cs() and deassert_cs() member functions.
//               Use GpioCsPolicy<Pin> for software GPIO CS (recommended).
//
// Init sequence (SD Physical Layer Simplified Spec v8.00 §7.2.1):
//   1. CMD0  — GO_IDLE_STATE  (enter SPI mode)
//   2. CMD8  — SEND_IF_COND  (verify 2.7–3.6 V support + echo pattern)
//   3. ACMD41 — SD_SEND_OP_COND (HCS=1, wait for card ready)
//   4. CMD58 — READ_OCR (confirm CCS=1 → SDHC/SDXC block addressing)
//
// Block read/write use CMD17 / CMD24 respectively (512-byte blocks).
//
// CS management:
//   The SdCard driver asserts CS at the beginning of each command/data
//   exchange and deasserts it after the complete response (including data
//   token + block + CRC for reads). This satisfies SD SPI mode timing
//   requirements even when the SPI hardware deasserts CS between calls.
//
//   GpioCsPolicy<Pin>  — use with a software GPIO CS pin (recommended).
//   NoOpCsPolicy       — use when the SPI hardware manages CS permanently.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/filesystem/block_device.hpp"

namespace alloy::drivers::memory::sdcard {

namespace detail {
inline constexpr std::uint8_t kCmdMask      = 0x40;
inline constexpr std::uint8_t kR1_Idle      = 0x01;
inline constexpr std::uint8_t kR1_Ready     = 0x00;
inline constexpr std::uint8_t kDataToken    = 0xFE;
inline constexpr std::uint8_t kDataAccepted = 0x05;
inline constexpr std::uint32_t kAcmd41Retries = 4000;
inline constexpr std::uint32_t kCmdRetries    = 8;
inline constexpr std::size_t   kBlockSize     = 512;
}  // namespace detail

// ── CS policies ──────────────────────────────────────────────────────────────

// Use when the SPI hardware permanently holds CS asserted (e.g. dedicated line).
struct NoOpCsPolicy {
    void assert_cs()   const noexcept {}
    void deassert_cs() const noexcept {}
};

// Software GPIO CS. Pin must provide set() and clear() returning any Result.
// Typical: alloy::hal::gpio pin open with output configured.
template <typename GpioPin>
struct GpioCsPolicy {
    explicit GpioCsPolicy(GpioPin& pin) : pin_(&pin) {
        (void)pin_->set_high();  // deasserted on construction
    }
    void assert_cs()   const noexcept { (void)pin_->set_low(); }
    void deassert_cs() const noexcept { (void)pin_->set_high(); }
private:
    GpioPin* pin_;
};

// ── SdCard<Spi, CsPolicy> ────────────────────────────────────────────────────

template <typename Spi, typename CsPolicy = NoOpCsPolicy>
class SdCard {
public:
    using Result    = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultSz  = alloy::core::Result<std::size_t, alloy::core::ErrorCode>;

    explicit SdCard(Spi& spi, CsPolicy cs = {}) : spi_(&spi), cs_(cs) {}

    // Runs the full SD SPI-mode initialization sequence.
    // Must be called before read/write. Rejects SDSC cards.
    [[nodiscard]] auto init() -> Result {
        // Power-up: 74+ clocks with CS deasserted.
        cs_.deassert_cs();
        std::array<std::uint8_t, 10> ff{};
        ff.fill(0xFF);
        if (auto r = spi_->transmit(ff); r.is_err()) return r;

        // CMD0: GO_IDLE_STATE → R1 = 0x01 Idle.
        {
            ScopedCs guard{cs_};
            if (auto r = cmd(0, 0, 0x95); r.is_err()) return r;
            if (auto r = wait_r1(); r.is_err()) return r;
            if (r1_ != detail::kR1_Idle) {
                return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
            }
        }

        // CMD8: SEND_IF_COND — VHS=0x01, pattern=0xAA.
        {
            ScopedCs guard{cs_};
            if (auto r = cmd(8, 0x000001AAu, 0x87); r.is_err()) return r;
            if (auto r = wait_r1(); r.is_err()) return r;
            if (r1_ != detail::kR1_Idle) {
                return alloy::core::Err(alloy::core::ErrorCode::NotSupported);
            }
            std::array<std::uint8_t, 4> r7{};
            if (auto r = spi_->receive(r7); r.is_err()) return r;
            if (r7[3] != 0xAA) {
                return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
            }
        }

        // ACMD41 (CMD55 + CMD41, HCS=1): wait until ready.
        bool ready = false;
        for (std::uint32_t i = 0; i < detail::kAcmd41Retries && !ready; ++i) {
            {
                ScopedCs guard{cs_};
                if (auto r = cmd(55, 0, 0x65); r.is_err()) return r;
                if (auto r = wait_r1(); r.is_err()) return r;
            }
            {
                ScopedCs guard{cs_};
                if (auto r = cmd(41, 0x40000000u, 0x77); r.is_err()) return r;
                if (auto r = wait_r1(); r.is_err()) return r;
                ready = (r1_ == detail::kR1_Ready);
            }
        }
        if (!ready) {
            return alloy::core::Err(alloy::core::ErrorCode::Timeout);
        }

        // CMD58: READ_OCR — CCS bit (bit 30) must be 1 for SDHC/SDXC.
        {
            ScopedCs guard{cs_};
            if (auto r = cmd(58, 0, 0xFD); r.is_err()) return r;
            if (auto r = wait_r1(); r.is_err()) return r;
            std::array<std::uint8_t, 4> ocr{};
            if (auto r = spi_->receive(ocr); r.is_err()) return r;
            if ((ocr[0] & 0x40) == 0) {
                return alloy::core::Err(alloy::core::ErrorCode::NotSupported);
            }
        }

        initialized_ = true;
        return alloy::core::Ok();
    }

    // Set total block count (call after init if known; required by FatFS/LittleFS).
    void set_block_count(std::size_t count) { block_count_ = count; }

    // ── BlockDevice concept methods ──────────────────────────────────────────

    [[nodiscard]] auto read(std::size_t block, std::span<std::byte> buf)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (!initialized_ || buf.size() != detail::kBlockSize) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        ScopedCs guard{cs_};
        if (auto r = cmd(17, static_cast<std::uint32_t>(block), 0xFF); r.is_err()) return r;
        if (auto r = wait_r1(); r.is_err()) return r;
        if (r1_ != detail::kR1_Ready) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        if (auto r = wait_data_token(); r.is_err()) return r;
        auto u8_buf = std::span<std::uint8_t>{
            reinterpret_cast<std::uint8_t*>(buf.data()), buf.size()};
        if (auto r = spi_->receive(u8_buf); r.is_err()) return r;
        std::array<std::uint8_t, 2> crc{};
        return spi_->receive(crc);
    }

    [[nodiscard]] auto write(std::size_t block, std::span<const std::byte> data)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (!initialized_ || data.size() != detail::kBlockSize) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        ScopedCs guard{cs_};
        if (auto r = cmd(24, static_cast<std::uint32_t>(block), 0xFF); r.is_err()) return r;
        if (auto r = wait_r1(); r.is_err()) return r;
        if (r1_ != detail::kR1_Ready) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        const std::array<std::uint8_t, 2> pre_token{0xFF, detail::kDataToken};
        if (auto r = spi_->transmit(pre_token); r.is_err()) return r;
        auto u8_data = std::span<const std::uint8_t>{
            reinterpret_cast<const std::uint8_t*>(data.data()), data.size()};
        if (auto r = spi_->transmit(u8_data); r.is_err()) return r;
        const std::array<std::uint8_t, 2> dummy_crc{0xFF, 0xFF};
        if (auto r = spi_->transmit(dummy_crc); r.is_err()) return r;
        std::array<std::uint8_t, 1> resp{};
        if (auto r = spi_->receive(resp); r.is_err()) return r;
        if ((resp[0] & 0x1F) != 0x05) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        return wait_not_busy();
    }

    // SD SPI mode erase is a no-op at the BlockDevice level.
    // LittleFS calls erase before write; for SD cards this is safe to skip.
    [[nodiscard]] auto erase(std::size_t, std::size_t)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }

    [[nodiscard]] static constexpr auto block_size() -> std::size_t {
        return detail::kBlockSize;
    }
    [[nodiscard]] auto block_count() const -> std::size_t { return block_count_; }

private:
    // RAII scoped CS assertion.
    struct ScopedCs {
        const CsPolicy& cs;
        explicit ScopedCs(const CsPolicy& c) : cs(c) { cs.assert_cs(); }
        ~ScopedCs() { cs.deassert_cs(); }
        ScopedCs(const ScopedCs&) = delete;
        ScopedCs& operator=(const ScopedCs&) = delete;
    };

    [[nodiscard]] auto cmd(std::uint8_t index, std::uint32_t arg, std::uint8_t crc)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        const std::array<std::uint8_t, 6> pkt{
            static_cast<std::uint8_t>(detail::kCmdMask | index),
            static_cast<std::uint8_t>((arg >> 24) & 0xFF),
            static_cast<std::uint8_t>((arg >> 16) & 0xFF),
            static_cast<std::uint8_t>((arg >> 8) & 0xFF),
            static_cast<std::uint8_t>(arg & 0xFF),
            crc,
        };
        return spi_->transmit(pkt);
    }

    [[nodiscard]] auto wait_r1() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (std::uint32_t i = 0; i < detail::kCmdRetries; ++i) {
            std::array<std::uint8_t, 1> b{};
            if (auto r = spi_->receive(b); r.is_err()) return r;
            if ((b[0] & 0x80) == 0) { r1_ = b[0]; return alloy::core::Ok(); }
        }
        return alloy::core::Err(alloy::core::ErrorCode::Timeout);
    }

    [[nodiscard]] auto wait_data_token()
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (std::uint32_t i = 0; i < 512; ++i) {
            std::array<std::uint8_t, 1> b{};
            if (auto r = spi_->receive(b); r.is_err()) return r;
            if (b[0] == detail::kDataToken) return alloy::core::Ok();
        }
        return alloy::core::Err(alloy::core::ErrorCode::Timeout);
    }

    [[nodiscard]] auto wait_not_busy()
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (std::uint32_t i = 0; i < 65536; ++i) {
            std::array<std::uint8_t, 1> b{};
            if (auto r = spi_->receive(b); r.is_err()) return r;
            if (b[0] == 0xFF) return alloy::core::Ok();
        }
        return alloy::core::Err(alloy::core::ErrorCode::Timeout);
    }

    Spi*         spi_;
    CsPolicy     cs_;
    std::size_t  block_count_{0};
    std::uint8_t r1_{0xFF};
    bool         initialized_{false};
};

}  // namespace alloy::drivers::memory::sdcard

// Concept gate.
namespace {
struct _MockSpiForSdGate {
    [[nodiscard]] auto transmit(std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
    [[nodiscard]] auto receive(std::span<std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
};
static_assert(
    alloy::hal::filesystem::BlockDevice<
        alloy::drivers::memory::sdcard::SdCard<_MockSpiForSdGate>>,
    "SdCard must satisfy alloy::hal::filesystem::BlockDevice");
}  // namespace
