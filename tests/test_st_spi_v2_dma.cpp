// The DMA endpoint contract of the ST FIFO SPI engine, witnessed on the host
// — the driver-level half of anchor 2.4 (docs/design/dma-streams.md §2.4).
// tests/test_spi_dma.cpp exercises the FACADE against a recording fake
// driver, which by construction cannot see whether the driver's own register
// sequence is right; this file runs the REAL engine
// (detail::st_spi_v2_engine, split out of st_spi_v2.hpp for exactly this)
// over a hand-written IP double whose registers are plain host memory.
//
// WHAT IS ACTUALLY AT STAKE HERE: the DMA path gives up the lockstep
// discipline the polled and interrupt paths rely on — that is the whole point
// of it — so it has to pay the RM's shutdown dance in full. The DMA's
// completion means the last byte reached the transmit FIFO, not that it
// reached the wire. Dropping chip-select, reconfiguring the port or stopping
// the channels before FTLVL empties and BSY falls truncates the last frame,
// and a truncated frame is not an error anywhere — it is a device that
// answers slightly wrong, forever.
//
// WHAT THIS FILE CANNOT WITNESS, stated rather than implied: the receive-FIFO
// DRAIN. Draining is a READ side effect — on silicon each read of DR pops one
// frame and drops FRLVL — and alloy's `rw32` is a plain `volatile uint32_t`,
// so a memory-backed double has no way to model a read that changes state.
// What is witnessed of the drain is that it is BOUNDED (a port that never
// clears RXNE does not hang the caller); that it pops anything is a claim
// resting on the RM and on the identical, hardware-proven loop in
// start_transfer_irq. Nor can this file witness real FTLVL/BSY timing: no
// Renode SPI model raises either, so silicon is the only witness that exists
// for the dance itself.

#include <cstddef>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/spi/st_spi_v2_body.hpp"
#include "alloy_test.hpp"

namespace {

// The spi_v2 register shape, hand-written (tests never read generated
// headers): same offsets and bit positions as registers/st/spi_v2.yaml, so
// the sequences exercised here are the sequences the silicon sees.
template <class Tag>
struct fake_spi_ip {
    struct regs {
        alloy::rw32 CR1;
        alloy::rw32 CR2;
        alloy::rw32 SR;
        alloy::rw32 DR;
    };

    static_assert(offsetof(regs, CR2) == 0x04);
    static_assert(offsetof(regs, SR) == 0x08);
    static_assert(offsetof(regs, DR) == 0x0C);

    static constexpr auto rxdmaen = alloy::field<&regs::CR2, 0u, 1>;
    static constexpr auto txdmaen = alloy::field<&regs::CR2, 1u, 1>;
    static constexpr auto rxne = alloy::field<&regs::SR, 0u, 1>;
    static constexpr auto bsy = alloy::field<&regs::SR, 7u, 1>;
    static constexpr auto frlvl = alloy::field<&regs::SR, 9u, 2>;
    static constexpr auto ftlvl = alloy::field<&regs::SR, 11u, 2>;
};

// One fake port per test tag (the engine's transfer statics are keyed by
// Inst), each over its own memory-backed register file.
template <class Tag>
struct fake_spi_inst {
    using ip = fake_spi_ip<Tag>;
    static inline std::uint32_t mem[8]{};
    static inline const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(&mem[0]);
};

template <class Tag>
using engine = alloy::hal::detail::st_spi_v2_engine<fake_spi_inst<Tag>>;

// The hardware's side, spelled once: raise the requests as a live transfer
// would leave them, and put the port in a chosen shutdown state.
template <class Tag>
void arm_requests() {
    auto& r = engine<Tag>::r();
    fake_spi_ip<Tag>::rxdmaen.set(r);
    fake_spi_ip<Tag>::txdmaen.set(r);
}

}  // namespace

// ── the endpoint address ──────────────────────────────────────────────────

ALLOY_TEST(st_spi_v2_dma_endpoint_is_the_byte_window_of_dr) {
    // ONE address serves both directions (this IP has a single DR), and it is
    // the BYTE window: a 16- or 32-bit access data-packs two frames into the
    // transmit FIFO and pulls two out of the receive one, which is why both
    // channels of the pair run psize=b8/msize=b8.
    struct tag {};
    using Inst = fake_spi_inst<tag>;
    ALLOY_CHECK_EQ(engine<tag>::dr_addr(), Inst::base + 0x0Cu);
    // ...and it is a byte pointer into the same register the driver's own
    // FIFO window uses, not a separate one.
    ALLOY_CHECK_EQ(engine<tag>::dr_addr(),
                   reinterpret_cast<std::uintptr_t>(&engine<tag>::dr8()));
}

// ── raising each direction's request ──────────────────────────────────────

ALLOY_TEST(st_spi_v2_dma_begin_raises_one_direction_each_and_leaves_cr1_alone) {
    struct tag {};
    using IP = fake_spi_ip<tag>;
    auto& r = engine<tag>::r();
    r.CR1 = 0x0344u;  // a configured, enabled port (SPE and friends)
    r.CR2 = 0u;
    r.SR = 0u;

    engine<tag>::dma_rx_begin();
    ALLOY_CHECK_EQ(IP::rxdmaen.read(r), 1u);
    ALLOY_CHECK_EQ(IP::txdmaen.read(r), 0u);  // TXDMAEN is the LAST write, and
                                              // it is what starts the traffic
    engine<tag>::dma_tx_begin();
    ALLOY_CHECK_EQ(IP::txdmaen.read(r), 1u);
    // Neither touches CR1: dropping SPE mid-exchange is a truncated frame.
    ALLOY_CHECK_EQ(r.CR1, 0x0344u);
}

ALLOY_TEST(st_spi_v2_dma_rx_begin_is_bounded_when_rxne_never_clears) {
    // The stale-byte drain has to terminate on a port that is not answering.
    // (That it POPS a byte is not witnessable here — see the header note — but
    // that it cannot hang the caller is.)
    struct tag {};
    using IP = fake_spi_ip<tag>;
    auto& r = engine<tag>::r();
    r.CR2 = 0u;
    r.SR = 0u;
    IP::rxne.set(r);  // a byte a previous polled xfer() left behind, forever

    engine<tag>::dma_rx_begin();
    ALLOY_CHECK_EQ(IP::rxdmaen.read(r), 1u);
}

// ── the shutdown dance ────────────────────────────────────────────────────

ALLOY_TEST(st_spi_v2_dma_end_succeeds_on_a_drained_port) {
    // The negative control for the two refusals below: a port whose transmit
    // FIFO is empty and whose shift register is idle ends cleanly, and both
    // request enables come down.
    struct tag {};
    using IP = fake_spi_ip<tag>;
    auto& r = engine<tag>::r();
    r.CR1 = 0x0344u;
    r.SR = 0u;
    arm_requests<tag>();

    ALLOY_CHECK(engine<tag>::dma_end());
    ALLOY_CHECK_EQ(IP::rxdmaen.read(r), 0u);
    ALLOY_CHECK_EQ(IP::txdmaen.read(r), 0u);
    ALLOY_CHECK_EQ(r.CR1, 0x0344u);  // SPE untouched
}

ALLOY_TEST(st_spi_v2_dma_end_refuses_a_port_whose_shift_register_is_still_busy) {
    // THE LAST-BYTE TRAP. Every byte left the DMA and the FIFO, and the frame
    // is STILL on the wire — BSY says so. Returning success here is what lets
    // a caller drop chip-select on a half-clocked frame. Delete the BSY wait
    // from dma_end() and this test goes green, which is the whole reason it
    // exists: the facade's own tests cannot see it, because they run against a
    // fake driver.
    struct tag {};
    using IP = fake_spi_ip<tag>;
    auto& r = engine<tag>::r();
    r.SR = 0u;
    IP::bsy.set(r);
    arm_requests<tag>();

    ALLOY_CHECK(!engine<tag>::dma_end());
    // ...and the requests came down ANYWAY. A port that failed to drain is
    // exactly the one that must not be left with its DMA enables raised: the
    // channels are about to stop, and a request landing on a disabled channel
    // is how an overrun flag gets stuck (design §4's teardown order).
    ALLOY_CHECK_EQ(IP::rxdmaen.read(r), 0u);
    ALLOY_CHECK_EQ(IP::txdmaen.read(r), 0u);
}

ALLOY_TEST(st_spi_v2_dma_end_refuses_a_port_whose_transmit_fifo_never_emptied) {
    // The step BEFORE the BSY wait, and it is a separate fact: BSY can be
    // clear between frames while the FIFO still holds bytes the shift register
    // has not picked up yet. Waiting only on BSY would end the transfer with
    // frames still queued.
    struct tag {};
    using IP = fake_spi_ip<tag>;
    auto& r = engine<tag>::r();
    r.SR = 0u;
    IP::ftlvl.write(r, 2u);  // two frames still queued, shift register idle
    arm_requests<tag>();

    ALLOY_CHECK(!engine<tag>::dma_end());
    ALLOY_CHECK_EQ(IP::rxdmaen.read(r), 0u);
    ALLOY_CHECK_EQ(IP::txdmaen.read(r), 0u);
}

ALLOY_TEST(st_spi_v2_dma_end_is_bounded_when_the_receive_fifo_never_empties) {
    // Whatever the receive channel did not take has no other reader coming, so
    // dma_end drains rather than waits — and the drain is bounded, so a port
    // whose FRLVL never falls reports a clean end rather than hanging. (The
    // transfer itself already succeeded here: the receive FIFO's leftovers are
    // a diagnosis, not a failure of the exchange.)
    struct tag {};
    using IP = fake_spi_ip<tag>;
    auto& r = engine<tag>::r();
    r.SR = 0u;
    IP::frlvl.write(r, 3u);
    arm_requests<tag>();

    ALLOY_CHECK(engine<tag>::dma_end());
    ALLOY_CHECK_EQ(IP::rxdmaen.read(r), 0u);
    ALLOY_CHECK_EQ(IP::txdmaen.read(r), 0u);
}
