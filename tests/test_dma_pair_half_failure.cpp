// THE HALF-FAILURE QUESTION, asked of the REAL dma_v1 engine.
//
// `spi.transfer_dma()` returns one bool for two channels, and the case that
// matters is the asymmetric one: a duplex where ONE direction failed. That it
// answers false is already asserted in tests/test_spi_dma.cpp — but there the
// engine is a RECORDING FAKE whose error() is a bool the test sets, so what
// that file witnesses is the FACADE consuming an answer, not the engine
// producing one. This file closes the other half: `alloy::dma::pair` runs over
// the real `st_dma_v1_engine` on a memory-backed register file (the
// test_st_dma_v1_latch.cpp idiom), the failure is a TEIF bit written from the
// hardware's side, and the question is put to `pair::wait()`.
//
// Four shapes, because the wrong ones are the interesting ones:
//   * receive failed / transmit clean, and the mirror;
//   * the failed half ALSO latched completion — which is what the real
//     complete_isr does on the error path — so done() is true for the whole
//     pair and only the error term can tell the truth;
//   * the failure consumed by the channel's OWN interrupt before any poller
//     looks, which is the shape the error latch exists for. (A pair only
//     reaches it when something else already armed a completion callback on
//     that channel: setup() folds TEIE in only when callback::fn is set, so a
//     bare transfer_dma() leaves TEIE clear and error() reads the LIVE flag.)
// Plus the negative control — a clean duplex answers true — so a wait() that
// simply always returned false could not pass.
//
// What this does NOT witness: silicon raising TEIF when we think it does. No
// emulation leg can reach it either (TEIF is an inert tag in both Renode DMA
// models), so the host is the only witness that exists for this path.

#include <cstdint>
#include <span>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/dma.hpp"
#include "alloy/hal/dma/st_dma_v1_body.hpp"
#include "alloy_test.hpp"

namespace {

template <class Tag>
struct dphf_ip {
    struct regs {
        alloy::rw32 ISR;
        alloy::rw32 IFCR;
    };
    static constexpr std::uintptr_t CCR_offset = 0x08;
    static constexpr unsigned CCR_stride = 20u;
    static constexpr std::uintptr_t CNDTR_offset = 0x0C;
    static constexpr unsigned CNDTR_stride = 20u;
    static constexpr std::uintptr_t CPAR_offset = 0x10;
    static constexpr unsigned CPAR_stride = 20u;
    static constexpr std::uintptr_t CMAR_offset = 0x14;
    static constexpr unsigned CMAR_stride = 20u;

    template <unsigned I>
    static constexpr auto gif = alloy::field<&regs::ISR, 0u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto tcif = alloy::field<&regs::ISR, 1u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto htif = alloy::field<&regs::ISR, 2u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto teif = alloy::field<&regs::ISR, 3u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto cgif = alloy::field<&regs::IFCR, 0u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto ctcif = alloy::field<&regs::IFCR, 1u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto chtif = alloy::field<&regs::IFCR, 2u + I * 4u, 1>;
    template <unsigned I>
    static constexpr auto cteif = alloy::field<&regs::IFCR, 3u + I * 4u, 1>;

    static constexpr alloy::raw_field en{0u, 1u};
    static constexpr alloy::raw_field tcie{1u, 1u};
    static constexpr alloy::raw_field htie{2u, 1u};
    static constexpr alloy::raw_field teie{3u, 1u};
    static constexpr alloy::raw_field dir{4u, 1u};
    static constexpr alloy::raw_field circ{5u, 1u};
    static constexpr alloy::raw_field pinc{6u, 1u};
    static constexpr alloy::raw_field minc{7u, 1u};
    static constexpr alloy::raw_field psize{8u, 2u};
    static constexpr alloy::raw_field msize{10u, 2u};
    static constexpr alloy::raw_field pl{12u, 2u};
};

template <class Tag>
struct dphf_mux {
    struct ip {
        static constexpr std::uintptr_t CCR_offset = 0x00;
        static constexpr unsigned CCR_stride = 4u;
        static constexpr alloy::raw_field dmareq_id{0u, 7u};
    };
    static inline std::uint32_t mem[16]{};
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem[0]);
};

template <class Tag>
struct dphf_inst {
    using ip = dphf_ip<Tag>;
    using mux_t = dphf_mux<Tag>;
    static inline std::uint32_t mem[64]{};
    static inline std::uint32_t rcc_reg = 0;
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem[0]);
    static inline const alloy::clock_gate gate{
        reinterpret_cast<std::uintptr_t>(&rcc_reg), 0x1u};
    static constexpr unsigned ch_count = 7;
    static constexpr unsigned ch_mux_offset = 0;
    static constexpr std::uint16_t ch_irqline1 = 5;
    static constexpr std::uint16_t ch_irqline2_3 = 6;
    static constexpr std::uint16_t ch_irqline4_7 = 7;
};

std::uint8_t g_tx[4] = {1, 2, 3, 4};
std::uint8_t g_rx[4] = {};
void dphf_noop(void*) {}

}  // namespace

namespace alloy::hal {
template <class Tag>
struct dma_impl<dphf_inst<Tag>>
    : alloy::hal::detail::st_dma_v1_engine<dphf_inst<Tag>> {};
}  // namespace alloy::hal

// Arm a real pair over the real engine; hand back the raw register file.
#define DPHF_ARM(TAGNAME)                                                    \
    using eng = alloy::hal::detail::st_dma_v1_engine<dphf_inst<TAGNAME>>;    \
    using IP = dphf_ip<TAGNAME>;                                             \
    using Ctrl = dphf_inst<TAGNAME>;                                         \
    alloy::dma::pair<alloy::dma::route<Ctrl, 4, 16>,                        \
                     alloy::dma::route<Ctrl, 5, 17>>                        \
        both{};                                                             \
    both.start_rx_p2m_u8(0x40u, std::span<std::uint8_t>{g_rx}, 16);         \
    both.start_tx_m2p_u8(std::span<const std::uint8_t>{g_tx}, 0x40u, 17);

// Sanity + negative control: a clean duplex answers true.
ALLOY_TEST(dma_pair_wait_is_true_when_both_halves_complete) {
    struct tag {};
    DPHF_ARM(tag)
    // Hardware: both channels finish. dma_v1 channel N uses ISR field index
    // N-1, so ch4 -> tcif<3>, ch5 -> tcif<4>.
    eng::r().ISR = eng::r().ISR | IP::tcif<3>.mask | IP::tcif<4>.mask;
    ALLOY_CHECK(both.wait());
    ALLOY_CHECK(!both.error());
}

// THE QUESTION: receive half fails, transmit half completes.
ALLOY_TEST(dma_pair_wait_is_false_when_only_the_receive_half_failed) {
    struct tag {};
    DPHF_ARM(tag)
    // TX completed; RX raised a transfer error and never completed.
    eng::r().ISR = eng::r().ISR | IP::tcif<4>.mask | IP::teif<3>.mask;
    ALLOY_CHECK(both.error());
    ALLOY_CHECK(!both.wait());
}

// The mirror: transmit half fails, receive half completes.
ALLOY_TEST(dma_pair_wait_is_false_when_only_the_transmit_half_failed) {
    struct tag {};
    DPHF_ARM(tag)
    eng::r().ISR = eng::r().ISR | IP::tcif<3>.mask | IP::teif<4>.mask;
    ALLOY_CHECK(both.error());
    ALLOY_CHECK(!both.wait());
}

// The nastier shape: the failed half ALSO latched completion (which is what
// the real complete_isr does on the error path), so done() is true for the
// whole pair and only the error term can tell the truth.
ALLOY_TEST(dma_pair_wait_is_false_when_the_failed_half_also_completed) {
    struct tag {};
    DPHF_ARM(tag)
    eng::r().ISR = eng::r().ISR | IP::tcif<3>.mask | IP::tcif<4>.mask |
                   IP::teif<3>.mask;
    ALLOY_CHECK(both.done());  // a naive "wait for done" would say success
    ALLOY_CHECK(!both.wait());
}

// And the one the error latch exists for: the failure is consumed by the
// channel's OWN interrupt before any poller looks. TEIE is only folded in
// when a completion callback is registered, so this is the shape a pair
// reaches only when something else already armed one on that channel.
ALLOY_TEST(dma_pair_wait_is_false_when_an_isr_consumed_the_receive_failure) {
    struct tag {};
    using eng = alloy::hal::detail::st_dma_v1_engine<dphf_inst<tag>>;
    using IP = dphf_ip<tag>;
    using Ctrl = dphf_inst<tag>;
    // Something registered a completion callback on the receive channel, so
    // setup() folds TEIE in and the error WILL be consumed by an interrupt.
    eng::template enable_complete_irq<4>(&dphf_noop, nullptr);
    alloy::dma::pair<alloy::dma::route<Ctrl, 4, 16>,
                     alloy::dma::route<Ctrl, 5, 17>>
        both{};
    both.start_rx_p2m_u8(0x40u, std::span<std::uint8_t>{g_rx}, 16);
    both.start_tx_m2p_u8(std::span<const std::uint8_t>{g_tx}, 0x40u, 17);
    ALLOY_CHECK((eng::template ccr<4>() & IP::teie.mask()) != 0u);
    // Hardware raises TEIF on the receive channel; its own ISR runs and
    // clears the flag (write-1-to-clear, committed here).
    eng::r().ISR = eng::r().ISR | IP::teif<3>.mask;
    eng::template complete_isr<4>(nullptr);
    eng::r().ISR = eng::r().ISR & ~eng::r().IFCR;
    eng::r().IFCR = 0u;
    ALLOY_CHECK((eng::r().ISR & IP::teif<3>.mask) == 0u);  // flag really gone
    // Transmit half finishes normally.
    eng::r().ISR = eng::r().ISR | IP::tcif<4>.mask;
    // THE WITNESS: only the latch can answer now.
    ALLOY_CHECK(!both.wait());
}
