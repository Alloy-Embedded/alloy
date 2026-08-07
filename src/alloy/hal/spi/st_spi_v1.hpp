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
#include "alloy/irq.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::spi_v1>
struct spi_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t clock_hz, std::uint8_t mode) {
        using cr1 = typename IP::cr1;
        alloy::gate_on(Inst::gate);
        // SCK = kernel / 2^(BR+1); smallest divisor whose SCK <= the request.
        std::uint32_t br = 0;
        while (br < 7 && (kernel_hz >> (br + 1)) > clock_hz) {
            ++br;
        }
        // DFF=0 (8-bit), software NSS held high, master, then SPE — all in
        // one write since config requires SPE=0. The named CR1 flags read like
        // the register table; BR stays a field_t shift because it is a
        // computed divisor, not a named constant.
        r().CR1 = (cr1::mstr | cr1::ssm | cr1::ssi | cr1::spe |
                   ((mode & 0x2u) ? cr1::cpol : cr1{}) |
                   ((mode & 0x1u) ? cr1::cpha : cr1{})) |
                  (br << IP::br.pos);
    }

    [[nodiscard]] static std::uint8_t xfer(std::uint8_t byte) {
        using sr = typename IP::sr;
        while (!(r().SR & sr::txe)) {
        }
        r().DR = byte;  // one halfword write = one 8-bit frame (DFF=0)
        while (!(r().SR & sr::rxne)) {
        }
        return static_cast<std::uint8_t>(r().DR);
    }

    // --- Interrupt-driven transfer, same RXNE-only lockstep as st_spi_v2 ---
    //
    // The reasoning is identical (see that file's long note): TXEIE would
    // storm on an idle bus, ERRIE without an OVR clear sequence would too, and
    // RXNE clears by reading DR so no software latch is needed on the flag
    // itself. The ONE difference that must not be copied across: DR here is a
    // plain 16-bit frame register — one access moves exactly one frame — so
    // this ISR uses r().DR, not st_spi_v2's dr8() FIFO byte window.
    //
    // Not silicon-validated and not emulated: no board in this tree binds
    // st/spi_v1, so this path is compile-checked only, like the rest of the
    // file.
    inline static const std::uint8_t* tx_ = nullptr;
    inline static std::uint8_t* rx_ = nullptr;
    inline static std::uint16_t len_ = 0;
    inline static std::uint16_t idx_ = 0;
    inline static void (*done_fn_)(void*) = nullptr;
    inline static void* done_ctx_ = nullptr;
    inline static volatile bool busy_ = false;
    inline static bool attached_ = false;

    static void transfer_isr(void*) {
        // Shared-vector contract (alloy::irq): a no-op unless this instance has
        // an interrupt we armed.
        if (IP::rxneie.read(r()) == 0u || IP::rxne.read(r()) == 0u) {
            return;
        }
        if (!busy_) {
            IP::rxneie.clear(r());  // armed with no transfer: disarm, never storm
            return;
        }
        while (IP::rxne.read(r()) != 0u) {
            const auto got = static_cast<std::uint8_t>(r().DR);  // the read IS the clear
            if (rx_ != nullptr) {
                rx_[idx_] = got;
            }
            ++idx_;
            if (idx_ < len_) {
                r().DR = (tx_ != nullptr) ? tx_[idx_] : 0u;
                continue;
            }
            IP::rxneie.clear(r());
            busy_ = false;
            if (done_fn_ != nullptr) {
                done_fn_(done_ctx_);
            }
            break;
        }
    }

    static void start_transfer_irq(const std::uint8_t* tx, std::uint8_t* rx, std::uint16_t len,
                                   void (*fn)(void*), void* ctx) {
        using sr = typename IP::sr;
        if (busy_ || len == 0u) {
            __builtin_trap();  // concurrent or empty transfer: honest runtime guard
        }
        while (r().SR & sr::rxne) {
            // Drop anything a previous polled xfer left behind. Bound to a
            // local: `(void)r().DR` is NOT a volatile access, so it would not
            // clear RXNE and the loop would spin.
            const auto stale = static_cast<std::uint8_t>(r().DR);
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
        IP::rxneie.set(r());
        r().DR = (tx != nullptr) ? tx[0] : 0u;
    }

    [[nodiscard]] static bool transfer_busy() { return busy_; }

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
};

}  // namespace alloy::hal
