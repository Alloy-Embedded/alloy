// SPI master driver for the ST FIFO variant (spi2s1 v3.x: G0/F7/L4/H7-lite).
//
// Quirks honored (all hardware-proven by the old G0 driver on PB3/PB4/PB5):
// DR must be BYTE-accessed for 8-bit frames — a 16/32-bit store data-packs
// two bytes into the TX FIFO; FRXTH=1 or RXNE only fires at 2 bytes and a
// single-byte poll hangs; SSM=1 requires SSI=1 in master mode or hardware
// raises MODF and silently drops to slave; CR2 (DS|FRXTH) is written BEFORE
// CR1, and SPE goes in the same CR1 write as the configuration. Lockstep
// discipline (one byte in flight, RX always drained) keeps the FIFOs empty
// so the RM's FTLVL/FRLVL shutdown dance is unnecessary. CS is caller GPIO.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/spi/spi_impl.hpp"
#include "alloy/ip/st/spi_v2.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::spi_v2>
struct spi_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    // Byte window into the FIFO (the data-packing quirk above).
    static volatile std::uint8_t& dr8() {
        return *reinterpret_cast<volatile std::uint8_t*>(&r().DR);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t clock_hz, std::uint8_t mode) {
        alloy::gate_on(Inst::gate);
        // SCK = kernel / 2^(BR+1); smallest divisor whose SCK <= the request.
        // Floor is kernel/256 (BR=7) — a slower request than that still runs
        // at the floor, faster than asked; there is no deeper divider.
        std::uint32_t br = 0;
        while (br < 7 && (kernel_hz >> (br + 1)) > clock_hz) {
            ++br;
        }
        r().CR1 = 0;  // config with SPE=0 (CR2.DS change under SPE is forbidden)
        r().CR2 = (0x7u << IP::ds.pos) | IP::frxth.mask;  // contract-ok: 0x7 = 8-bit frame size (DS+1), IP-semantic constant
        r().CR1 = IP::mstr.mask | IP::ssm.mask | IP::ssi.mask |
                  (br << IP::br.pos) |
                  ((mode & 0x2u) ? IP::cpol.mask : 0u) |
                  ((mode & 0x1u) ? IP::cpha.mask : 0u) |
                  IP::spe.mask;
    }

    [[nodiscard]] static std::uint8_t xfer(std::uint8_t byte) {
        while (!(r().SR & IP::txe.mask)) {
        }
        dr8() = byte;
        while (!(r().SR & IP::rxne.mask)) {
        }
        return dr8();
    }
};

}  // namespace alloy::hal
