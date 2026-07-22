// SPI master driver for the Atmel/Microchip SPI (SAM E70 lineage).
//
// Quirks honored: NCPHA is INVERTED Motorola CPHA (mode0 = CPOL=0+NCPHA=1 —
// the old driver had this backwards, mining disagreement #6); SCBR is a
// LINEAR divider with 0 forbidden (clamped 1..255); MODFDIS required for
// GPIO chip-selects; and the silicon-found one: in Variable PS mode a TDR
// write with PCS=0xF ("none") SUPPRESSES SCK entirely — every TDR write
// selects NPCS0 (one-cold 0xE) even though the real CS is a caller GPIO.
// CR is command-style write-only (composed masks, never RMW); SWRST drops
// the block to slave mode, so MR/CSR0 are re-written after it.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/spi/spi_impl.hpp"
#include "alloy/ip/microchip/spi_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::spi_v1>
struct spi_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t clock_hz, std::uint8_t mode) {
        alloy::gate_on(Inst::gate);
        // Unlock MR/CSR in case a bootloader left write protection on.
        r().WPMR = 0x535049u << IP::wpkey.pos;  // contract-ok: WPKEY ASCII 'SPI', IP-semantic constant
        r().CR = IP::swrst.mask | IP::spidis.mask;
        r().MR = IP::mstr.mask | IP::ps.mask | IP::modfdis.mask;
        // SPCK = kernel / SCBR, linear, clamped 1..255. Floor is kernel/255 —
        // slower requests run at the floor, faster than asked.
        if (clock_hz == 0u) {
            clock_hz = 1u;
        }
        std::uint32_t scbr = (kernel_hz + clock_hz - 1u) / clock_hz;
        if (scbr < 1u) {
            scbr = 1u;
        }
        if (scbr > 255u) {
            scbr = 255u;
        }
        auto& csr0 = alloy::reg_at(Inst::base, IP::CSR_offset, IP::CSR_stride, 0);
        csr0 = ((mode & 0x2u) ? IP::cpol.mask() : 0u) |
               ((mode & 0x1u) ? 0u : IP::ncpha.mask()) |
               (scbr << IP::scbr.pos);  // BITS=0 = 8-bit frames
        r().CR = IP::spien.mask;
    }

    [[nodiscard]] static std::uint8_t xfer(std::uint8_t byte) {
        while (!(r().SR & IP::tdre.mask)) {
        }
        // One composed write; PCS one-cold NPCS0 (0xE) — 0xF kills SCK.
        r().TDR = byte | (0xEu << IP::td_pcs.pos);  // contract-ok: one-cold PCS encoding, IP-semantic constant
        while (!(r().SR & IP::rdrf.mask)) {
        }
        return static_cast<std::uint8_t>(r().RDR);
    }
};

}  // namespace alloy::hal
