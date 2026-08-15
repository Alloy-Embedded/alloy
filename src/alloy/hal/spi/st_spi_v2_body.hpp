// The programming sequence of the ST FIFO SPI (spi2s1 v3.x: G0/F7/L4/H7-lite)
// — the ENTIRE driver body, split out of st_spi_v2.hpp so a host test can
// reach it (the driver proper is constrained on the generated
// alloy::ip::st::spi_v2 tag, which exists only inside a built project; tests/
// deliberately never reads generated headers). Same seam as
// st_dma_v1_body.hpp and st_usart_v4_body.hpp: the logic lives once, here;
// the constrained spi_impl<Inst> specialization in st_spi_v2.hpp inherits it
// unchanged.
//
// Everything below reaches the register layout through `typename Inst::ip`,
// so a host test supplies a hand-written IP double over a memory-backed
// register file and exercises the REAL sequences — which is what makes the
// DMA shutdown dance (the RM's FTLVL/BSY drain, the trap that truncates the
// last frame on the wire) witnessable off-target:
// tests/test_st_spi_v2_dma.cpp fails if any step of it is reverted.
//
// Quirks honored (all hardware-proven by the old G0 driver on PB3/PB4/PB5):
// DR must be BYTE-accessed for 8-bit frames — a 16/32-bit store data-packs
// two bytes into the TX FIFO; FRXTH=1 or RXNE only fires at 2 bytes and a
// single-byte poll hangs; SSM=1 requires SSI=1 in master mode or hardware
// raises MODF and silently drops to slave; CR2 (DS|FRXTH) is written BEFORE
// CR1, and SPE goes in the same CR1 write as the configuration. Lockstep
// discipline (one byte in flight, RX always drained) keeps the FIFOs empty
// so the RM's FTLVL/FRLVL shutdown dance is unnecessary FOR THE POLLED AND
// INTERRUPT PATHS. The DMA path gives that discipline up on purpose — the
// whole point is that the CPU is not in the loop — so it pays the dance in
// full; see dma_end(). CS is caller GPIO.

#pragma once

#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal::detail {

template <class Inst>
struct st_spi_v2_engine {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    // Byte window into the FIFO (the data-packing quirk above).
    static volatile std::uint8_t& dr8() {
        return *reinterpret_cast<volatile std::uint8_t*>(&r().DR);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t clock_hz, std::uint8_t mode) {
        using cr2 = typename IP::cr2;
        alloy::gate_on(Inst::gate);
        // SCK = kernel / 2^(BR+1); smallest divisor whose SCK <= the request.
        // Floor is kernel/256 (BR=7) — a slower request than that still runs
        // at the floor, faster than asked; there is no deeper divider.
        std::uint32_t br = 0;
        while (br < 7 && (kernel_hz >> (br + 1)) > clock_hz) {
            ++br;
        }
        r().CR1 = 0;  // config with SPE=0 (CR2.DS change under SPE is forbidden)
        r().CR2 = cr2::ds_eight | cr2::frxth;  // contract-ok: 0x7 = 8-bit frame size (DS+1), IP-semantic constant
        r().CR1 = IP::mstr.mask | IP::ssm.mask | IP::ssi.mask |
                  (br << IP::br.pos) |
                  ((mode & 0x2u) ? IP::cpol.mask : 0u) |
                  ((mode & 0x1u) ? IP::cpha.mask : 0u) |
                  IP::spe.mask;
    }

    [[nodiscard]] static std::uint8_t xfer(std::uint8_t byte) {
        using sr = typename IP::sr;
        while (!(r().SR & sr::txe)) {
        }
        dr8() = byte;
        while (!(r().SR & sr::rxne)) {
        }
        return dr8();
    }

    // --- Interrupt-driven transfer: the same lockstep, driven by RXNE ---
    //
    // The whole transfer runs off RXNE and nothing else. TXEIE is NEVER
    // enabled: TXE is set whenever the TX FIFO has room, which on an idle bus
    // is always, so a level-triggered TXEIE asserts the NVIC line forever and
    // the CPU never leaves the handler. (Observed under emulation, not merely
    // read out of the RM.) ERRIE is not enabled either — OVR needs the RM's
    // read-DR-then-read-SR clear sequence, and arming an error interrupt
    // without implementing its clear is the identical failure. Lockstep with
    // one byte in flight cannot overrun, so nothing is given up by that.
    //
    // Note what is absent: the software latch st_dma_v1 needs on TCIF. RXNE is
    // cleared by READING DR, which is the ISR doing its actual job — there is
    // no separate flag for the ISR to eat out from under a poller. `busy_`
    // exists for two other reasons: the hardware has no transfer-complete flag
    // of its own to poll, and a second concurrent transfer must be rejected.
    //
    // What busy_ does NOT cover: calling the blocking xfer() while an async
    // transfer is in flight. The ISR consumes the RXNE that xfer()'s loop is
    // waiting for, so the two APIs must not overlap on one bus — ask busy()
    // first. xfer() is deliberately left byte-for-byte as it was so the polled
    // path emits exactly the code it always did.
    inline static const std::uint8_t* tx_ = nullptr;
    inline static std::uint8_t* rx_ = nullptr;
    inline static std::uint16_t len_ = 0;
    inline static std::uint16_t idx_ = 0;
    inline static void (*done_fn_)(void*) = nullptr;
    inline static void* done_ctx_ = nullptr;
    inline static volatile bool busy_ = false;
    inline static bool attached_ = false;

    static void transfer_isr(void*) {
        // Shared-vector contract (alloy::irq): a no-op unless THIS instance has
        // an interrupt we armed. RXNEIE is checked as well as RXNE because a
        // co-sharing instance's polled xfer() legitimately leaves RXNE set on
        // itself between the store and the load.
        if (IP::rxneie.read(r()) == 0u || IP::rxne.read(r()) == 0u) {
            return;
        }
        if (!busy_) {
            IP::rxneie.clear(r());  // armed with no transfer: disarm, never storm
            return;
        }
        while (IP::rxne.read(r()) != 0u) {
            const std::uint8_t got = dr8();  // reading DR IS the flag clear
            if (rx_ != nullptr) {
                rx_[idx_] = got;
            }
            ++idx_;
            if (idx_ < len_) {
                // Lockstep: a byte just arrived, so the one in flight has left
                // and TXE is set — this store cannot stall inside the handler.
                dr8() = (tx_ != nullptr) ? tx_[idx_] : 0u;
                continue;
            }
            IP::rxneie.clear(r());  // idle with no enable armed
            busy_ = false;
            if (done_fn_ != nullptr) {
                // The callback sees a finished, disarmed transfer, so it may
                // start the next one without racing its own completion.
                done_fn_(done_ctx_);
            }
            break;
        }
    }

    // tx == nullptr clocks out zeros; rx == nullptr discards what comes back.
    static void start_transfer_irq(const std::uint8_t* tx, std::uint8_t* rx, std::uint16_t len,
                                   void (*fn)(void*), void* ctx) {
        using sr = typename IP::sr;
        if (busy_ || len == 0u) {
            __builtin_trap();  // concurrent or empty transfer: honest runtime guard
        }
        while (r().SR & sr::rxne) {
            // Drop anything a previous polled xfer left behind. Bound to a
            // local: `(void)dr8()` is NOT a volatile access and would not clear
            // RXNE — gcc warns about exactly that, and the loop would spin.
            const std::uint8_t stale = dr8();
            (void)stale;
        }
        tx_ = tx;
        rx_ = rx;
        len_ = len;
        idx_ = 0;
        done_fn_ = fn;
        done_ctx_ = ctx;
        if (!attached_) {
            alloy::irq::attach(Inst::irq, &transfer_isr);
            alloy::irq::enable(Inst::irq);
            attached_ = true;
        }
        while (!(r().SR & sr::txe)) {
        }
        busy_ = true;
        IP::rxneie.set(r());  // armed before the first byte: RXNE is not yet set
        dr8() = (tx != nullptr) ? tx[0] : 0u;
    }

    [[nodiscard]] static bool transfer_busy() { return busy_; }

    // --- DMA hooks: the endpoint half of the full-duplex pair -------------
    //
    // What a DMA'd exchange needs from the driver, and nothing more: WHERE the
    // data register is, HOW to raise each direction's request, and how to end
    // a transfer without truncating its last frame. The channels, their claim
    // and their ordering belong to alloy::dma::pair; the facade above stitches
    // the two together (design §2.4).
    //
    // ONE ADDRESS FOR BOTH DIRECTIONS: this IP has a single DR, and it must be
    // BYTE-accessed for 8-bit frames (the data-packing quirk this file opens
    // with). Both channels therefore run psize=b8/msize=b8, which is what
    // dma::pair's byte-wide starters give — a halfword access here would push
    // two frames per transfer on TX and pull garbage on RX.
    [[nodiscard]] static std::uintptr_t dr_addr() {
        return reinterpret_cast<std::uintptr_t>(&dr8());
    }

    // Raise the RECEIVE request. Called AFTER the receive channel is armed and
    // BEFORE the transmit side starts — the order the RM gives for full duplex
    // and the order ST's own HAL follows (arm RX, RXDMAEN, arm TX, TXDMAEN;
    // writing TXDMAEN is what starts the traffic).
    //
    // The FIFO is drained first, for the same reason start_transfer_irq drains
    // it: a previous polled xfer() can leave a byte behind, and with RXDMAEN
    // raised that stale byte would be the first thing the channel wrote into
    // the caller's receive buffer, shifting every frame by one. Bound to a
    // local — `(void)dr8()` is not a volatile access and would not clear RXNE.
    static void dma_rx_begin() {
        for (std::uint32_t spin = 0; spin < kDrainBudget; ++spin) {
            if (IP::rxne.read(r()) == 0u) {
                break;
            }
            const std::uint8_t stale = dr8();
            (void)stale;
        }
        IP::rxdmaen.set(r());
    }

    // Raise the TRANSMIT request — the write that starts the exchange.
    static void dma_tx_begin() { IP::txdmaen.set(r()); }

    // End the exchange without truncating its last frame. THE CLASSIC TRAP:
    // the DMA's completion says the last byte reached the FIFO, not that it
    // reached the wire. The RM's shutdown sequence (ST's HAL spells it
    // SPI_EndRxTxTransaction) is: wait for the transmit FIFO to empty, then
    // for BSY to fall — only then is the last frame actually clocked out, and
    // only then may a caller drop chip-select or reconfigure the port.
    //
    // BOUNDED, the same idiom and reasoning as the ST I2C driver's poll
    // budget: far above any legal frame's latency, so it only ever fires on a
    // genuine fault, and a fault reports false instead of hanging. The
    // enables are cleared either way — leaving RXDMAEN raised on a channel
    // that is about to be stopped is how a request lands on a disabled
    // channel and an overrun flag gets stuck (design §4's teardown order).
    //
    // The receive FIFO is DRAINED rather than waited on: anything still in it
    // is a frame the receive channel did not take, so there is no other reader
    // coming and a wait for FRLVL==0 would be a wait for nobody.
    [[nodiscard]] static bool dma_end() {
        bool ok = true;
        std::uint32_t spin = 0;
        for (; spin < kPollBudget && IP::ftlvl.read(r()) != 0u; ++spin) {
        }
        ok = ok && spin < kPollBudget;
        for (spin = 0; spin < kPollBudget && IP::bsy.read(r()) != 0u; ++spin) {
        }
        ok = ok && spin < kPollBudget;
        IP::txdmaen.clear(r());
        IP::rxdmaen.clear(r());
        for (spin = 0; spin < kDrainBudget && IP::frlvl.read(r()) != 0u; ++spin) {
            const std::uint8_t stale = dr8();
            (void)stale;
        }
        return ok;
    }

    // Disarm and forget any in-flight transfer. The escape hatch when a
    // transfer stalls (MODF drops SPE and RXNE never comes again), so a wedged
    // bus does not leave the driver permanently busy_.
    static void disable_transfer_irq() {
        IP::rxneie.clear(r());
        if (attached_) {
            alloy::irq::detach(Inst::irq, &transfer_isr);
            attached_ = false;
        }
        busy_ = false;
        done_fn_ = nullptr;
        done_ctx_ = nullptr;
    }

private:
    // Iteration caps for the two DMA spins above. kPollBudget is the ST I2C
    // driver's constant and its reasoning; kDrainBudget is small on purpose —
    // the FIFO this IP has holds four frames, so a drain that has not finished
    // in a few dozen reads is a port that is not answering, not a deep queue.
    static constexpr std::uint32_t kPollBudget = 1'000'000u;
    static constexpr std::uint32_t kDrainBudget = 64u;
};

}  // namespace alloy::hal::detail
