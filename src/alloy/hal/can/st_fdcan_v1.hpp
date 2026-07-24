// Classic-CAN driver for the Bosch M_CAN (FDCAN) on STM32G0/G4 (can_fdcan_v1).
//
// BEHAVIOR only: registers + gate + the message-RAM base (companion instance)
// come from generated data. This variant has a FIXED message-RAM layout, so the
// section offsets (RX FIFO 0 at +0xB0, TX buffers at +0x278, 18-word/72-byte
// elements) are IP constants, and the M_CAN element headers (T0/T1 for TX,
// R0/R1 for RX) are hand-encoded per the Bosch manual.
//
// Bring-up is internal loopback (CCCR.TEST + MON + TEST.LBCK): a transmitted
// frame is delivered straight back into the RX FIFO with no transceiver or bus,
// which is exactly what makes it self-testable. Standard 11-bit IDs, classic
// (non-FD) frames, all frames accepted into RX FIFO 0 (RXGFC.ANFS=0).

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/concepts.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/can/can_impl.hpp"
#include "alloy/ip/st/fdcan_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::fdcan_v1>
struct can_impl<Inst> {
    using IP = typename Inst::ip;

    // Fixed M_CAN message-RAM layout (word indices + element size in words).
    static constexpr std::uint32_t rxf0_word = 0xB0u / 4u;    // RX FIFO 0 base
    static constexpr std::uint32_t txbuf_word = 0x278u / 4u;  // TX buffers base
    static constexpr std::uint32_t elem_words = 18u;          // 72-byte element

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    static volatile std::uint32_t* ram() {
        return reinterpret_cast<volatile std::uint32_t*>(Inst::ram_t::base);
    }

    static void enable() {
        alloy::gate_on(Inst::gate);
        IP::init.set(r());  // request config mode
        while (IP::init.read(r()) == 0u) {
        }
        IP::cce.set(r());   // config change enable
        // Internal loopback: test mode + bus monitoring + loopback.
        IP::tstmode.set(r());
        IP::mon.set(r());
        IP::lbck.set(r());
        // 500 kbit/s from the 64 MHz PCLK kernel: BRP=4 (16 MHz tq), 32 tq/bit
        // = sync(1) + tseg1(25) + tseg2(6). Register fields are value-1.
        r().NBTP = (4u << IP::nsjw.pos) | (3u << IP::nbrp.pos) |
                   (24u << IP::ntseg1.pos) | (5u << IP::ntseg2.pos);
        r().RXGFC = 0u;     // no filters (LSS=0); accept all standard -> FIFO 0
        IP::init.clear(r());  // leave init -> operating (loopback)
    }

    [[nodiscard]] static bool send(const alloy::can_frame& f) {
        volatile std::uint32_t* tb = ram() + txbuf_word;
        tb[0] = (f.id & 0x7FFu) << 18u;  // T0: standard ID in bits [28:18]
        tb[1] = static_cast<std::uint32_t>(f.len & 0xFu) << 16u;  // T1: DLC, classic
        std::uint32_t w0 = 0u;
        std::uint32_t w1 = 0u;
        for (std::uint8_t i = 0u; i < f.len && i < 8u; ++i) {
            const std::uint32_t b = f.data[i];
            if (i < 4u) {
                w0 |= b << (8u * i);
            } else {
                w1 |= b << (8u * (i - 4u));
            }
        }
        tb[2] = w0;
        tb[3] = w1;
        r().TXBAR = 1u;  // add transmission request for TX buffer 0
        return true;
    }

    [[nodiscard]] static bool receive(alloy::can_frame& f) {
        if (IP::ffl.read(r()) == 0u) {
            return false;  // RX FIFO 0 empty
        }
        const std::uint32_t gi = IP::fgi.read(r());  // get index
        volatile std::uint32_t* rx = ram() + rxf0_word + gi * elem_words;
        const std::uint32_t r0 = rx[0];
        const std::uint32_t r1 = rx[1];
        f.id = (r0 >> 18u) & 0x7FFu;
        f.len = static_cast<std::uint8_t>((r1 >> 16u) & 0xFu);
        const std::uint32_t w0 = rx[2];
        const std::uint32_t w1 = rx[3];
        for (std::uint8_t i = 0u; i < f.len && i < 8u; ++i) {
            const std::uint32_t word = (i < 4u) ? w0 : w1;
            const std::uint32_t shift = 8u * (i < 4u ? i : (i - 4u));
            f.data[i] = static_cast<std::uint8_t>((word >> shift) & 0xFFu);
        }
        r().RXFA = gi;  // acknowledge: release this FIFO element
        return true;
    }
};

}  // namespace alloy::hal
