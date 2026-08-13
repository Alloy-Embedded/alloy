// CRC-32/ISO-HDLC on the STM32 CRC calculation unit, v3 (G0/G4/L4/WB/H7 —
// the version with programmable POL and CR.POLYSIZE).
//
// WHAT THIS DRIVER PROMISES: the number it returns is bit-for-bit the number
// alloy::ota::crc::crc32_of() returns for the same bytes. Not "a CRC-32" —
// THE one every alloy on-flash format already uses: the OTA image header and
// payload (alloy/ota/image.hpp), the boot state record (ota/boot_store.hpp),
// the UART update frames (ota/uart_transport.hpp) and the provisioning record
// (provision/identity.hpp). A bootloader that verifies a whole slot before
// every boot is the reason that promise is worth silicon.
//
// HOW THE PROMISE IS KEPT — six parameters, five of them in the block:
//
//   polynomial   POL, at its reset value                      hardware
//   seed         INIT, at its reset value (all ones)          hardware
//   width        CR.POLYSIZE = WIDTH_32                       hardware
//   refin=true   CR.REV_IN = BYTE, with big-endian word
//                assembly (see crc_impl.hpp's feed())         hardware
//   refout=true  CR.REV_OUT                                   hardware
//   xorout       *** NOT IN THIS SILICON ***                  software, below
//
// The sixth is the one that matters. There is no XOR register anywhere in the
// block, so value() applies it to what it reads out of DR — by calling
// crc_detail::finalize(), which is where the arithmetic lives so a host test
// can run it. Omitting that step would produce CRC-32/JAMCRC: a stable,
// respectable checksum that agrees with ISO-HDLC on no input at all, and that a
// bootloader would use to reject every image the updater ever wrote. It is one
// instruction and it is the difference between this driver and a silent
// corruption — and for one release it was one instruction that nothing tested,
// because it was written inline in a file the host suite cannot include.
//
// NOT SILICON-WITNESSED. No STM32 was on hand, and Renode 1.16.1 does not model
// this block — its stm32g0 platform carries a bare address Tag for the CRC
// region, which reads back zero, so an emulated run would "verify" every image
// as having checksum 0xFFFF'FFFF. What IS proven is in tests/test_crc.cpp: the
// configuration below and the feed sequence in crc_impl.hpp are driven into a
// model of the block written from its documented semantics, and the result is
// compared against alloy::ota::crc::crc32_of over every length from 0 to 64
// bytes plus the standard check vector. That proves the CONFIGURATION is the
// right one; it cannot prove the silicon behaves as documented.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/crc/crc_impl.hpp"
#include "alloy/ip/st/crc_v3.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::crc_v3>
struct crc_impl<Inst> {
    using IP = typename Inst::ip;
    using cr = typename IP::cr;

    static typename IP::regs& r() { return *reinterpret_cast<typename IP::regs*>(Inst::base); }

    // The control word for CRC-32/ISO-HDLC, spelled entirely in generated
    // names. Nothing here is a bare number, which is the point of curating the
    // encodings: `2u << 5` would have been the same bits and none of the
    // meaning.
    static constexpr std::uint32_t control =
        alloy::flags<cr>(cr::polysize_width_32) | cr::rev_in_byte | cr::rev_out;

    // The algorithm's own numbers, from the shared header the host test also
    // reads — see crc_impl.hpp's iso_hdlc for why the decision lives there and
    // the encoding lives in the generated enumerators above.
    using spec = crc_detail::iso_hdlc;
    static_assert(spec::width_bits == 32u && spec::reverse_input_bits == 8u &&
                      spec::reverse_output,
                  "this specialization writes POLYSIZE_32 / REV_IN_BYTE / REV_OUT "
                  "by name; a spec that wants anything else needs its own control word");

    // Called once, when the facade hands out its handle.
    static void open() {
        alloy::gate_on(Inst::gate);
        // WHOLE-REGISTER WRITE, not read-modify-write, for the reason the
        // encoder build recorded about CCMR1: the bits this driver does not
        // name are somebody else's configuration of the same block, and
        // preserving them carries a foreign polynomial width into a checksum.
        // The block has one personality here, but the habit is the safe one.
        r().CR = control;
        // Both of these already hold the values this driver wants at reset,
        // and both are written anyway: a warm reset that did not clear the
        // peripheral, or a bootloader that used the unit for something else
        // before jumping, leaves them holding whatever it left. Two stores in
        // a function that runs once.
        r().INIT = spec::seed;
        r().POL = spec::polynomial;
        reset();
    }

    // Reload DR from INIT and start a new checksum. The RESET bit is
    // self-clearing, so `control` is re-written with it set rather than
    // read-modify-written.
    static void reset() { r().CR = alloy::flags<cr>(cr::reset) | control; }

    // The sink crc_detail::feed() drives. `word` and `byte` are two ACCESS
    // WIDTHS of one register at one address, which is the whole reason the
    // curated data has a single 32-bit DR: the width is a semantic input to
    // the block (a byte feeds one byte, a word feeds four), upstream models it
    // as three overlapping registers DR8/DR16/DR32, and alloy.registers.v1
    // cannot express two registers at one offset. That limit is survivable
    // HERE and was not survivable for the timer's CCMR1 views, and the
    // difference is exactly fields: DR has none, so narrowing the pointer
    // loses nothing that an accessor would have given.
    struct dr_sink {
        static void word(std::uint32_t w) { r().DR = w; }
        static void byte(std::uint8_t b) {
            *reinterpret_cast<volatile std::uint8_t*>(&r().DR) = b;
        }
    };

    static void update(const std::uint8_t* p, std::size_t n) {
        crc_detail::feed(dr_sink{}, p, n);
    }

    // REV_OUT has already reflected the remainder; finalize() is the xorout the
    // silicon does not have. See the header comment — that step is the
    // difference between ISO-HDLC and JAMCRC.
    //
    // THE XOR IS NOT SPELLED HERE ON PURPOSE. Written inline it was one
    // character of arithmetic in a file no host test can include, and deleting
    // it left the suite green. It now lives in crc_impl.hpp's crc_detail, which
    // tests/test_crc.cpp does include and does drive; the only thing left in
    // this file is the register read and the call. Keep it that way: an
    // arithmetic step re-inlined here is an arithmetic step nothing tests.
    [[nodiscard]] static std::uint32_t value() { return crc_detail::finalize(r().DR); }
};

}  // namespace alloy::hal
