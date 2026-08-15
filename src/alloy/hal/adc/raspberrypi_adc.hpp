// ADC driver for the RP2040's 12-bit SAR converter.
//
// The simplest ADC in the tree, and the one least like the others. There is no
// sequencer, no channel bitmap, no per-channel sampling time, no resolution
// selector, no oversampler and no analog watchdog — so this driver's Layer 2
// is EMPTY (it inherits the primary `adc_opts`), and that absence is the
// capability answer rather than an omission. A program that probes
// `requires { Opts{}.oversample_ratio; }` gets false here and true on the G0,
// which is exactly what the Layer-2 naming rule exists to make possible.
//
// A conversion is three steps: point the single sample-and-hold at an input
// (CS.AINSEL), set CS.START_ONCE, wait for CS.READY. Five inputs — 0..3 on
// GPIO26..29, and 4 on the on-die temperature sensor.
//
// TWO THINGS THIS DRIVER DELIBERATELY DOES NOT DO:
//
//  1. IT DOES NOT MUX THE PINS. GPIO26..29 reach the converter only with their
//     digital pad disabled (pads_bank0: IE low, OD high), which is not an
//     alternate function and not something a route table can express. Sampling
//     a real pin therefore needs the caller to prepare the pad; the
//     temperature input (4) needs no pin at all and is what a bring-up example
//     can use unaided. Doing the pad write here would be a driver reaching
//     into a peripheral it does not own, and it would do it for channels the
//     caller may not have meant.
//
//  2. IT DOES NOT CONVERT COUNTS TO A TEMPERATURE. The transfer function is a
//     datasheet fact this curation had no served copy of, so `read()` returns
//     raw counts for every input including input 4. A plausible formula here
//     would be a number nobody checked, silently scaling a reading the caller
//     trusts.
//
// The FIFO, its DMA request and the round-robin free-run mode (CS.RROBIN) are
// curated in the register data and unused here — they are what a future
// `ring()` on this IP would be built from, and the facade already constrains
// that method away for a driver with no burst hooks.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/adc/adc_impl.hpp"
#include "alloy/ip/raspberrypi/adc.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::raspberrypi::adc>
struct adc_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // The temperature sensor's input index. Named because 4 appearing bare in
    // a caller's code is the kind of literal that outlives the reason for it.
    static constexpr std::uint8_t temp_sensor_channel = 4u;

    static void enable(std::uint32_t /*kernel_hz*/) {
        // RELEASE FROM RESET FIRST, and this is not boilerplate on this part:
        // the RP2040 boots every peripheral HELD in reset, so a converter
        // whose gate never ran is not slow, it is a block that never answers.
        // The gate's style is reset_release (clear the bit, poll RESET_DONE),
        // which alloy::gate_on reads from the chip data.
        alloy::gate_on(Inst::gate);

        // Power the converter and the temperature sensor. TS_EN is raised at
        // bring-up rather than per read because it has a settling time of its
        // own and the cost of leaving it on is static current, not correctness
        // — the same trade st_adc_v2 makes for the G0's internal sources.
        IP::en.set(r());
        IP::ts_en.set(r());

        // Wait for the converter to report itself idle before the first
        // conversion. READY is low while converting AND while powering up, so
        // this is the startup delay, read from the block rather than spun for.
        while (IP::ready.read(r()) == 0u) {
        }
    }

    [[nodiscard]] static std::uint16_t read(std::uint8_t channel) {
        IP::ainsel.write(r(), channel);
        // START_ONCE is self-clearing; READY drops for the duration.
        IP::start_once.set(r());
        while (IP::ready.read(r()) == 0u) {
        }
        return static_cast<std::uint16_t>(IP::result.read(r()));
    }
};

}  // namespace alloy::hal
