// Generic 25-series SPI NOR flash driver (W25Q, MX25, IS25, N25Q/MT25Q;
// SST26 needs a block-protection unlock first — see note below).
//
// Constrained ONLY on SpiBus + OutputPin (software CS) — no chip, family or
// board. Universal command subset, mode 0, MSB-first, 3-byte addresses;
// addresses >= 16 MiB are REJECTED (false) — the 4-byte command set is out
// of v1 scope and byte-wise truncation would silently alias low sectors.
// CS discipline per datasheet: WREN is its OWN CS transaction (WEL only
// latches on CS rise), and program/erase launch on the CS rising edge of
// their frame. Dummy TX during reads is 0xFF (idle-high convention).
//
// SST26VF (manufacturer 0xBF, type 0x26) powers up fully write-protected
// and would need WREN + ULBPR (0x98); v1 reports it via jedec_id and leaves
// unlocking to the caller. Legacy SST25 (type 0x25) has single-byte 0x02 —
// not supported.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::drivers {

struct jedec_id_t {
    std::uint8_t manufacturer;  // JEP106: 0xEF Winbond, 0xC2 Macronix, ...
    std::uint8_t memory_type;
    std::uint8_t capacity_code;

    // No device / floating MISO reads back all-0/all-1; a JEP106 code is
    // never 0x00 or 0xFF, so those manufacturer bytes always mean noise.
    [[nodiscard]] constexpr bool present() const {
        return manufacturer != 0x00 && manufacturer != 0xFF;
    }
    // log2 rule holds across vendors inside the sane window (128 KiB..64
    // MiB); outside it (e.g. SST26 0x42/0x43) auto-sizing is refused.
    [[nodiscard]] constexpr std::uint32_t capacity_bytes() const {
        if (capacity_code < 0x11 || capacity_code > 0x1A) {
            return 0;
        }
        return 1u << capacity_code;
    }
};

template <class Bus, class Cs>
    requires alloy::SpiBus<Bus> && alloy::OutputPin<Cs>
class spi_flash {
public:
    static constexpr std::uint32_t kPageSize = 256;
    static constexpr std::uint32_t kSectorSize = 4096;

    constexpr spi_flash(const Bus& bus, const Cs& cs) : bus_(bus), cs_(cs) {}

    [[nodiscard]] jedec_id_t jedec_id() const {
        select();
        (void)bus_.xfer(0x9F);
        jedec_id_t id{bus_.xfer(0xFF), bus_.xfer(0xFF), bus_.xfer(0xFF)};
        deselect();
        return id;
    }

    [[nodiscard]] bool read(std::uint32_t addr, std::span<std::uint8_t> out) const {
        if (addr >= kAddrLimit || out.size() > kAddrLimit - addr) {
            return false;
        }
        select();
        command_addr(0x03, addr);
        for (auto& b : out) {
            b = bus_.xfer(0xFF);
        }
        deselect();
        return true;
    }

    // One page max, must not cross the 256-byte page boundary (the device
    // wraps within the page). False on bad bounds or busy-timeout.
    [[nodiscard]] bool page_program(std::uint32_t addr, std::span<const std::uint8_t> data) const {
        if (addr >= kAddrLimit || data.empty() || data.size() > kPageSize ||
            (addr % kPageSize) + data.size() > kPageSize) {
            return false;
        }
        if (status() & kWip) {
            return false;  // still busy from a prior op — WREN would be ignored
        }
        write_enable();
        select();
        command_addr(0x02, addr);
        for (auto b : data) {
            (void)bus_.xfer(b);
        }
        deselect();  // program launches on CS rise
        return wait_idle(kProgramSpins);
    }

    [[nodiscard]] bool sector_erase_4k(std::uint32_t addr) const {
        if (addr >= kAddrLimit) {
            return false;
        }
        if (status() & kWip) {
            return false;
        }
        write_enable();
        select();
        command_addr(0x20, addr);
        deselect();  // erase launches on CS rise
        return wait_idle(kEraseSpins);
    }

    [[nodiscard]] std::uint8_t status() const {
        select();
        (void)bus_.xfer(0x05);
        const std::uint8_t s = bus_.xfer(0xFF);
        deselect();
        return s;
    }

private:
    // Spin bounds sized for the worst datasheet times (t_PP <= 5 ms, t_SE
    // 4K <= 500 ms) even at a pessimistic ~1 us per status poll (fast core
    // + fast SPI clock); at slow clocks they just mean a longer timeout.
    static constexpr std::uint32_t kProgramSpins = 20'000;
    static constexpr std::uint32_t kEraseSpins = 2'000'000;
    static constexpr std::uint32_t kAddrLimit = 1u << 24;  // 3-byte reach
    static constexpr std::uint8_t kWip = 0x01;  // status bit 0

    void select() const { cs_.set_low(); }
    void deselect() const { cs_.set_high(); }

    void command_addr(std::uint8_t op, std::uint32_t addr) const {
        (void)bus_.xfer(op);
        (void)bus_.xfer(static_cast<std::uint8_t>(addr >> 16));
        (void)bus_.xfer(static_cast<std::uint8_t>(addr >> 8));
        (void)bus_.xfer(static_cast<std::uint8_t>(addr));
    }

    void write_enable() const {
        select();
        (void)bus_.xfer(0x06);
        deselect();  // WEL latches on CS rise — own transaction, always
    }

    [[nodiscard]] bool wait_idle(std::uint32_t spins) const {
        for (std::uint32_t i = 0; i < spins; ++i) {
            if (!(status() & kWip)) {
                return true;
            }
        }
        return false;
    }

    const Bus& bus_;
    const Cs& cs_;
};

}  // namespace alloy::drivers
