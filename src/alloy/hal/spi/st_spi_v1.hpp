// SPI master driver for the ST spi_v1 IP — the NON-FIFO variant (F1/F2/F4).
//
// The load-bearing difference from st_spi_v2 (FIFO variant): frame size is
// CR1.DFF (bit 11), 0 = 8-bit — there is NO CR2.DS, NO FRXTH, NO FIFO. DR is
// a 16-bit register and one access moves exactly ONE frame (no byte packing
// like the FIFO window). RXNE means one frame received; TXE means the single
// TX buffer is empty. Config bits (DFF/BR/CPOL/CPHA/MSTR/SSM/SSI) may only be
// written with SPE=0, so they all land in one CR1 write before SPE.
// SSM=1 needs SSI=1 in master mode or MODF self-clears SPE. CS is caller GPIO.
//
// Not silicon-validated (no F4 board on hand) — tier-2, compile-checked.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/spi/spi_impl.hpp"
#include "alloy/ip/st/spi_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::spi_v1>
struct spi_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t clock_hz, std::uint8_t mode) {
        alloy::gate_on(Inst::gate);
        // SCK = kernel / 2^(BR+1); smallest divisor whose SCK <= the request.
        std::uint32_t br = 0;
        while (br < 7 && (kernel_hz >> (br + 1)) > clock_hz) {
            ++br;
        }
        // DFF=0 (8-bit), software NSS held high, master, then SPE — all in
        // one write since config requires SPE=0.
        r().CR1 = IP::mstr.mask | IP::ssm.mask | IP::ssi.mask |
                  (br << IP::br.pos) |
                  ((mode & 0x2u) ? IP::cpol.mask : 0u) |
                  ((mode & 0x1u) ? IP::cpha.mask : 0u) |
                  IP::spe.mask;
    }

    [[nodiscard]] static std::uint8_t xfer(std::uint8_t byte) {
        while (!(r().SR & IP::txe.mask)) {
        }
        r().DR = byte;  // one halfword write = one 8-bit frame (DFF=0)
        while (!(r().SR & IP::rxne.mask)) {
        }
        return static_cast<std::uint8_t>(r().DR);
    }
};

}  // namespace alloy::hal
