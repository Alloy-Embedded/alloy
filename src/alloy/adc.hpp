// User-facing ADC: blocking single conversions, raw 12-bit results.
//
//   auto adc = board::adc::open();
//   std::uint16_t raw = adc.read(board::adc_vref_channel);
//
// Internal channels (VREFINT, temperature) come from chip data via
// generated board constants, so probes stay portable with zero wiring.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/core/types.hpp"
#include "alloy/hal/adc/adc_impl.hpp"

namespace alloy::adc {

struct config {};

template <class Inst>
class handle {
public:
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    // Blocking single conversion of the given channel (raw counts).
    [[nodiscard]] std::uint16_t read(std::uint8_t channel) const {
        return hal::adc_impl<Inst>::read(channel);
    }

    // DMA burst: N continuous conversions of one channel straight to RAM.
    // Blocking (returns when the buffer is full); only exists where the
    // driver has DMA hooks AND the chip data carries the request ID.
    template <class Chan>
    [[nodiscard]] bool read_burst(const Chan& dma, std::uint8_t channel,
                                  std::span<std::uint16_t> out) const
        requires requires {
            hal::adc_impl<Inst>::dma_burst_begin(channel);
            Inst::dmareq_conv;
        }
    {
        hal::adc_impl<Inst>::dma_burst_begin(channel);
        dma.start_p2m_u16(hal::adc_impl<Inst>::dr_addr(), out, Inst::dmareq_conv);
        hal::adc_impl<Inst>::dma_burst_kick();
        const bool ok = dma.wait();
        dma.stop();
        hal::adc_impl<Inst>::dma_burst_end();
        return ok;
    }

private:
    template <class, class>
    friend struct bind;
    handle() = default;
};

template <class Inst, class Clock>
struct bind {
    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::apb2: return Clock::apb2_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    static handle<Inst> open(config = {}) {
        hal::adc_impl<Inst>::enable(kernel_hz());
        return handle<Inst>{};
    }
};

}  // namespace alloy::adc
