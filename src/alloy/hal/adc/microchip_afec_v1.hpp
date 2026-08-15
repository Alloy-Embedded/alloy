// ADC driver for the SAM AFEC (E70-era), blocking single conversions.
//
// Silicon quirks honored: MR.ONE must be written 1; per-channel COCR
// offset DAC must sit mid-scale (0x200) or readings are shifted; CSELR
// selects the channel for both COCR and CDR. AFEC clock kept <= 40 MHz
// via MR.PRESCAL.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/adc/adc_impl.hpp"
#include "alloy/ip/microchip/afec_v1.hpp"

namespace alloy::hal {

// ── Layer 2 for microchip/afec_v1 ───────────────────────────────────────
//
// ONE MEMBER, AND THE ABSENCES ARE THE INTERESTING PART.
//
// On this IP resolution and oversampling are THE SAME FIELD: EMR.RES selects
// 12/13/14/15/16 bits, and each extra bit IS averaging — 13-bit is the sample
// rate divided by 4, 16-bit is divided by 256. There is no mode that produces
// 16 bits without averaging, and no way to ask for a ratio independently of a
// width.
//
// So there is NO `oversample_ratio` member here, and that absence means
// something different from the same absence on st/adc_f4. On the F4 the
// silicon has no oversampler at all; here it has one that cannot be steered
// separately. Both answer `false` to a `requires { Opts{}.oversample_ratio; }`
// probe, which is correct in both cases — a caller cannot set that value on
// either part — and the REASON lives in each header rather than in a field
// that would have to lie about one of them.
//
// What a portable caller CAN do is ask for width and read `result_bits`, which
// is the shared vocabulary's honest intersection: on the ST parts the width
// follows from resolution and shift, and here it follows from one enum. Same
// question, same units, same answer.
//
// `oversample_shift` is absent for the same reason: the averaged result is
// already presented at the wider width, so there is no post-shift to choose.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::afec_v1>
struct adc_opts<Inst> {
    // 12, 13, 14, 15 or 16. Above 12 the converter averages, at a
    // proportionally lower sample rate — that is not a side effect, it is what
    // the extra bits are made of.
    std::uint8_t resolution_bits = 12;
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::afec_v1>
struct adc_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    template <adc_opts<Inst> Opts = {}>
    static void enable(std::uint32_t kernel_hz) {
        alloy::gate_on(Inst::gate);
        r().CR = IP::swrst.mask;

        // RESOLUTION, from the curated encoding rather than from arithmetic.
        // The encoding is NOT contiguous — 12-bit is 0 and 13-bit is 2, with 1
        // absent — so `bits - 12` would write a reserved value for a 13-bit
        // request. This is the whole reason the values were curated instead of
        // the field being left as a width.
        static_assert(Opts.resolution_bits >= 12u && Opts.resolution_bits <= 16u,
                      "microchip/afec_v1 converts at 12, 13, 14, 15 or 16 bits — and above 12 "
                      "the extra bits ARE averaging, at a proportionally lower sample rate");
        if constexpr (Opts.resolution_bits != 12u) {
            // The encoded values live in the REGISTER's own flags enum, already
            // shifted into place — so this is a named constant, never a
            // hand-placed literal.
            using emr = typename IP::emr;
            constexpr auto code =
                Opts.resolution_bits == 13u   ? emr::res_osr4
                : Opts.resolution_bits == 14u ? emr::res_osr16
                : Opts.resolution_bits == 15u ? emr::res_osr64
                                              : emr::res_osr256;
            r().EMR = (r().EMR & ~IP::res.mask) | static_cast<std::uint32_t>(code);
        }
        // fAFE = kernel/(PRESCAL+1), capped at 40 MHz.
        std::uint32_t prescal = kernel_hz / 40'000'000u;
        r().MR = (prescal << IP::prescal.pos) | (4u << IP::startup.pos) |
                 IP::one.mask | (2u << IP::tracktim.pos) | (2u << IP::transfer.pos);
        r().EMR = IP::tag.mask;
        r().ACR = std::uint32_t{1} << IP::ibctl.pos;
        r().CGR = 0;      // gain 1 on all channels
        r().DIFFR = 0;    // single-ended
        r().TEMPMR = 0;   // temp sensor sampled like any channel
    }

    [[nodiscard]] static std::uint16_t read(std::uint8_t channel) {
        // Mid-scale offset compensation for this channel (required).
        r().CSELR = channel;
        r().COCR = 0x200u;
        r().CHER = std::uint32_t{1} << channel;
        r().CR = IP::start.mask;
        while ((r().ISR & (std::uint32_t{1} << channel)) == 0u) {
        }
        r().CSELR = channel;
        const auto data = static_cast<std::uint16_t>(r().CDR & 0xFFFu);
        r().CHDR = std::uint32_t{1} << channel;
        return data;
    }

    // --- DMA burst hooks. The XDMAC request line (PERID) is hardwired
    // from DRDY (data in LCDR; reading LCDR acks it) — no peripheral-side
    // DMA-enable exists. begin() configures ONE channel but does not
    // start; kick() flips MR.FREERUN (RMW — enable() wrote MR wholesale
    // and ONE must stay set), which retriggers conversions continuously;
    // end() stops free-run and drains a stale LCDR/DRDY. ---
    static void dma_burst_begin(std::uint8_t channel) {
        r().CSELR = channel;
        r().COCR = 0x200u;
        r().CHER = std::uint32_t{1} << channel;
        (void)r().LCDR;  // clear stale DRDY so the first request is fresh
    }

    static void dma_burst_kick() {
        IP::freerun.set(r());
        r().CR = IP::start.mask;  // defensive first trigger (harmless)
    }

    static void dma_burst_end() {
        IP::freerun.clear(r());
        // CSELR still selects the burst channel (set in begin): reading CDR
        // clears the latched EOC so the next blocking read() cannot satisfy
        // its poll from a stale conversion.
        (void)r().CDR;
        r().CHDR = 0xFFFFu;  // contract-ok: disable all channels, write-only clear mask
        (void)r().LCDR;
    }

    [[nodiscard]] static std::uintptr_t dr_addr() {
        return reinterpret_cast<std::uintptr_t>(&r().LCDR);
    }

    // What a result is worth: exactly the configured width, because the
    // averaging that produces the extra bits is what the width MEANS here.
    template <adc_opts<Inst> Opts>
    [[nodiscard]] static consteval unsigned result_bits() {
        return Opts.resolution_bits;
    }
};

}  // namespace alloy::hal
