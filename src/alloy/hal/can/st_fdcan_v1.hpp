// Classic-CAN driver for the Bosch M_CAN (FDCAN) on STM32G0/G4 (can_fdcan_v1).
//
// BEHAVIOR only: registers + gate + the message-RAM base (companion instance)
// come from generated data. This variant has a FIXED message-RAM layout (no
// start-address registers), so the section offsets are IP constants — read
// from the COMPANION's generated header rather than typed here — and the M_CAN
// element headers (T0/T1 for TX, R0/R1 for RX) are hand-encoded per the Bosch
// manual because no register map describes a RAM word.
//
// Bring-up is internal loopback (CCCR.TEST + MON + TEST.LBCK): a transmitted
// frame is delivered straight back into the RX FIFO with no transceiver or bus,
// which is exactly what makes it self-testable. Standard 11-bit IDs, classic
// (non-FD) frames, all frames accepted into RX FIFO 0 until accept_only() says
// otherwise.
//
// ── THE SHAPE OF THIS PERIPHERAL, because it is the reason the code below is
//    arranged the way it is ─────────────────────────────────────────────────
//
// FDCAN acceptance filtering is ONE feature living in TWO peripherals with
// different rules:
//
//   the ELEMENTS — what to match — are words in the companion `fdcanram`.
//   Plain SRAM. Writable at any time, in any state, with no window and no
//   handshake.
//
//   the LIST SIZE and the unmatched-frame policy — RXGFC in the CONTROLLER.
//   Writable ONLY while CCCR.INIT and CCCR.CCE are set, which means taking
//   the node off the bus for the duration.
//
// Two consequences the code has to honour and no portable rule states:
//
//   ORDER. Elements first, list size second, always. The core scans LSS
//   elements from the start of the message RAM the instant LSS is nonzero; a
//   size published before its elements exist is a core scanning garbage.
//
//   CAPACITY. `RXGFC.LSS` is five bits wide, so `field_t::raw_mask` says 31.
//   The real number is 28, and it is stated by the companion — a standard
//   filter element is exactly one word, so the companion's `FLSSA_count` IS
//   the filter count. The controller's register that names the quantity is
//   the wrong place to read it; the peripheral that stores the things is the
//   right one. The static_assert below pins the relationship between the two
//   so a future die that widens one and not the other fails to build.

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
    //: The COMPANION's IP — the message RAM's own register map. Everything
    //: about where things live in that RAM is read from here.
    using RAM = typename Inst::ram_t::ip;

    // M_CAN element size in words. Not in the data because the data describes
    // the RAM as words: RX FIFO 0 is 54 words, and only the Bosch manual says
    // that is 3 elements of 18. A standard FILTER element, by contrast, is one
    // word, which is why the filter count needs no such constant.
    static constexpr std::uint32_t elem_words = 18u;  // 72-byte element

    // DEGREE. The companion's word count is the filter count, one word each.
    static constexpr unsigned filter_capacity = RAM::FLSSA_count;

    // The two cross-peripheral facts, asserted rather than assumed.
    static_assert(filter_capacity <= IP::lss.raw_mask,
                  "this instance's message RAM holds more standard filters than "
                  "the controller's RXGFC.LSS field can count — the chip data "
                  "has the controller and its companion from different dies");
    static_assert(RAM::sfid1.raw_mask() == alloy::hal::can_standard_id_mask,
                  "the filter element's identifier field is not eleven bits wide, "
                  "so either the curated element format or ISO 11898-1 is wrong");

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    static volatile std::uint32_t* ram() {
        return reinterpret_cast<volatile std::uint32_t*>(Inst::ram_t::base);
    }

    // The controller's config window. Everything in RXGFC/NBTP/CCCR's protected
    // bits has to be written between these two, and nothing in message RAM does.
    static void config_enter() {
        IP::init.set(r());  // request config mode
        while (IP::init.read(r()) == 0u) {
        }
        IP::cce.set(r());  // config change enable
    }
    static void config_leave() {
        IP::init.clear(r());  // leave init -> operating
    }

    static void enable() {
        alloy::gate_on(Inst::gate);
        config_enter();
        // Internal loopback: test mode + bus monitoring + loopback.
        IP::tstmode.set(r());
        IP::mon.set(r());
        IP::lbck.set(r());
        // 500 kbit/s from the 64 MHz PCLK kernel: BRP=4 (16 MHz tq), 32 tq/bit
        // = sync(1) + tseg1(25) + tseg2(6). Register fields are value-1.
        r().NBTP = (4u << IP::nsjw.pos) | (3u << IP::nbrp.pos) |
                   (24u << IP::ntseg1.pos) | (5u << IP::ntseg2.pos);
        r().RXGFC = accept_all_word();  // no filters (LSS=0), accept all -> FIFO 0
        config_leave();
    }

    // ── acceptance filters ────────────────────────────────────────────────

    //: RXGFC with an empty filter list and both non-matching classes admitted
    //: to FIFO 0. Zero, as it happens — spelled from the curated encodings so
    //: it says which zero it is.
    static constexpr std::uint32_t accept_all_word() {
        return IP::rxgfc::anfs_fifo0 | IP::rxgfc::anfe_fifo0;
    }

    static void accept_all() {
        alloy::gate_on(Inst::gate);
        config_enter();
        r().RXGFC = accept_all_word();
        config_leave();
    }

    static void accept_only(const alloy::hal::can_filter* list, unsigned n) {
        alloy::gate_on(Inst::gate);
        // 1. The ELEMENTS, in the companion. No window, no handshake — but
        //    they must be valid BEFORE step 2 publishes the size.
        for (unsigned i = 0u; i < n; ++i) {
            alloy::reg_at(Inst::ram_t::base, RAM::FLSSA_offset, RAM::FLSSA_stride, i) =
                (RAM::sft_classic << RAM::sft.pos) |
                (RAM::sfec_fifo0 << RAM::sfec.pos) |
                ((list[i].id & RAM::sfid1.raw_mask()) << RAM::sfid1.pos) |
                ((list[i].mask & RAM::sfid1.raw_mask()) << RAM::sfid2.pos);
        }
        // 2. The SIZE and the policy, in the controller, inside its window.
        //    Elements past `n` are never scanned, so a shorter list needs no
        //    erase pass — the size is the erase.
        config_enter();
        r().RXGFC = (n << IP::lss.pos) |
                    static_cast<std::uint32_t>(IP::rxgfc::anfs_reject) |
                    static_cast<std::uint32_t>(IP::rxgfc::anfe_reject);
        config_leave();
    }

    [[nodiscard]] static bool send(const alloy::can_frame& f) {
        volatile std::uint32_t* tb = ram() + RAM::TXBUF_offset / 4u;
        tb[0] = (f.id & alloy::hal::can_standard_id_mask) << 18u;  // T0: standard ID in [28:18]
        tb[1] = static_cast<std::uint32_t>(f.len & 0xFu) << 16u;   // T1: DLC, classic
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
        volatile std::uint32_t* rx = ram() + RAM::RXFIFO0_offset / 4u + gi * elem_words;
        const std::uint32_t r0 = rx[0];
        const std::uint32_t r1 = rx[1];
        f.id = (r0 >> 18u) & alloy::hal::can_standard_id_mask;
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
