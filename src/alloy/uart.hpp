// User-facing UART: typed pin binding with compile-time route checking,
// move-only handle satisfying ByteStream.
//
//   using Dbg = alloy::uart::bind<dev::usart2_t,
//                                 alloy::uart::tx<dev::pa2_t>,
//                                 alloy::uart::rx<dev::pa3_t>, Clock>;
//   auto u = Dbg::open({.baud = 115'200});
//
// A pin with no route to the peripheral fails the static_assert below with
// the pin and signal named in the message.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/core/units.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/hal/uart/uart_impl.hpp"

namespace alloy::uart {

// LAYER 1 — the portable line shape. Runtime, because baud and parity
// legitimately change on a live port. Every field is honoured by every UART
// driver alloy ships; see hal::uart_config for the admission test and for
// what is deliberately NOT here.
using config = hal::uart_config;
using parity = hal::parity;
using stop_bits = hal::stop_bits;

// Layer 1 MINUS the rate — the argument shape for the calls that state the
// baud some other way. `open_checked<115'200_baud>({.baud = 9'600})` used to
// compile clean and run at 115 200, because the template argument overwrote
// the field and the loser was never mentioned. Two spellings of one fact
// cannot disagree if only one of them is typeable, so open_checked() takes
// this and `.baud` there is a compile error naming the member.
using frame = hal::uart_frame;

// LAYER 2 — the vendor knobs, keyed by INSTANCE, so a knob the silicon lacks
// is not a member and cannot be asked for. Surfaced on every binder as
// `Role::opts`, which is the spelling to use: naming the type at the call
// site is what buys the good diagnostic.
//
//     auto bus = board::rs485::open<board::rs485::opts{.de_assert_16ths = 12}>(
//                    {.baud = 19'200, .parity = alloy::uart::parity::even});
template <class Inst>
using opts = hal::uart_opts<Inst>;

// DEPRECATED — the nine-field struct that one driver honoured and five
// ignored. Layer 1 (`config`) plus Layer 2 (`opts<Inst>`) replace it, and
// unlike it they cannot promise a field the silicon does not have. Kept for
// its stability window; there are no in-tree callers.
using serial_config [[deprecated(
    "use alloy::uart::config for portable fields and Role::opts for vendor "
    "knobs — serial_config promised nine fields on behalf of six drivers")]] =
    hal::serial_config;

template <class Pin>
struct tx {
    using pin = Pin;
};
template <class Pin>
struct rx {
    using pin = Pin;
};

// PIN-BEARING OPTIONS ARE BINDER TAGS, NOT CONFIG FIELDS. Hardware RS-485
// driver-enable needs a muxed pin, so the thing that turns it on is the tag
// that names the pin — one decision, checked by routes::routable<> like any
// other pin. A `.de_enable = true` bool that silently does nothing unless
// some other code happened to mux the right pin is the exact failure mode
// this framework exists to remove.
//
//     using Bus = alloy::uart::bind<dev::usart3_t,
//                                   alloy::uart::tx<dev::pd8_t>,
//                                   alloy::uart::rx<dev::pd9_t>, Clock,
//                                   alloy::uart::de<dev::pd12_t>>;
//
// The DE pin routes on the UART's RTS signal, which is what the silicon
// repurposes. A driver whose IP has no DEM bit rejects the tag with a
// static_assert naming the IP; see the five uart_impl<>s that do.
template <class Pin>
struct de {
    using pin = Pin;
};

namespace detail {
// Layer 1's VALUE admission. Compile error when the baud is a constant (which
// every literal call site is), named trap when it is not — see
// alloy/core/admit.hpp for why the two lines are spelled here rather than
// wrapped. `kernel` of zero is the same class of impossible: a port whose
// clock tree never started cannot divide anything.
inline void admit_baud(std::uint32_t baud, std::uint32_t kernel) {
    const bool ok = baud != 0u && kernel != 0u && baud <= kernel;
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::uart_baud();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}

// A unique address per Layer-2 VALUE, so a binder can remember which `opts`
// it was opened with and refuse a reconfigure that quietly asks for others.
template <auto Opts>
inline constexpr char opts_tag = 0;

template <class T>
inline constexpr bool is_de_tag = false;
template <class Pin>
inline constexpr bool is_de_tag<uart::de<Pin>> = true;

template <class... Extra>
inline constexpr bool has_de = (is_de_tag<Extra> || ...);

template <class... Extra>
struct de_pin_of {
    using type = void;
};
template <class Pin, class... Rest>
struct de_pin_of<uart::de<Pin>, Rest...> {
    using type = Pin;
};
template <class First, class... Rest>
    requires (!is_de_tag<First>)
struct de_pin_of<First, Rest...> : de_pin_of<Rest...> {};
}  // namespace detail

// Move-only handle: opening twice is a runtime trap (C++ cannot make a
// cross-TU double-open a compile error — see NORTH_STAR guard #7).
template <class Inst>
class handle {
public:
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    void write(std::uint8_t byte) const { hal::uart_impl<Inst>::write(byte); }
    void write(const char* zstr) const {
        while (*zstr != '\0') {
            hal::uart_impl<Inst>::write(static_cast<std::uint8_t>(*zstr++));
        }
    }
    [[nodiscard]] bool read(std::uint8_t& byte) const { return hal::uart_impl<Inst>::read(byte); }
    void flush() const { hal::uart_impl<Inst>::flush(); }

    // Register a function to run in the RX interrupt: the driver's ISR
    // drains the hardware and calls fn(ctx, byte) per byte received. Only
    // exists where the backing driver implements interrupts (Cortex-M v1).
    void on_receive(void (*fn)(void*, std::uint8_t), void* ctx = nullptr) const
        requires requires { hal::uart_impl<Inst>::enable_rx_irq(fn, ctx); }
    {
        hal::uart_impl<Inst>::enable_rx_irq(fn, ctx);
    }
    void detach_receive() const
        requires requires { hal::uart_impl<Inst>::disable_rx_irq(); }
    {
        hal::uart_impl<Inst>::disable_rx_irq();
    }

    // Blocking TX of a buffer via a claimed DMA channel: returns once the
    // DMA finished AND the transmitter drained (honest completion). Only
    // exists where the driver has TX-DMA hooks and the chip data carries
    // the request ID.
    template <class Chan>
    [[nodiscard]] bool write_dma(const Chan& dma, std::span<const std::uint8_t> data) const
        requires requires {
            hal::uart_impl<Inst>::dma_tx_begin();
            Inst::dmareq_tx;
        }
    {
        write_dma_begin(dma, data);
        const bool ok = dma.wait();
        write_dma_end(dma);
        return ok;
    }

    // The two halves of write_dma(), split so the wait in the middle can be a
    // `co_await` instead of a spin (alloy::async::dma_waiter). Call them in
    // order and wait for the channel between them:
    //
    //     co_await w.run([&] { uart.write_dma_begin(chan, msg); });
    //     uart.write_dma_end(chan);
    //
    // write_dma() itself is now written in terms of these, so there is one
    // sequence, not two that can drift.
    //
    // HONEST NOTE on write_dma_end(): it still SPINS, on the transmitter's
    // TC flag — the DMA finishing means the last byte reached TDR, not that it
    // reached the wire. That is one character time (~87 µs at 115200 baud) and
    // this change does not remove it; TC has no interrupt path in the driver.
    template <class Chan>
    void write_dma_begin(const Chan& dma, std::span<const std::uint8_t> data) const
        requires requires {
            hal::uart_impl<Inst>::dma_tx_begin();
            Inst::dmareq_tx;
        }
    {
        hal::uart_impl<Inst>::dma_tx_begin();
        dma.start_m2p_u8(data, hal::uart_impl<Inst>::tdr_addr(), Inst::dmareq_tx);
    }

    template <class Chan>
    void write_dma_end(const Chan& dma) const
        requires requires { hal::uart_impl<Inst>::dma_tx_end(); }
    {
        dma.stop();
        hal::uart_impl<Inst>::dma_tx_end();
    }

private:
    template <class, class, class, class, class...>
    friend struct bind;
    template <class>
    friend struct rom_bind;
    handle() = default;
};

// For UARTs fully configured by a boot ROM/bootloader (classic ESP32 UART0):
// no pin routing, no clock math — open() defers entirely to the driver.
template <class Inst>
struct rom_bind {
    using inst = Inst;
    using opts = hal::uart_opts<Inst>;

    template <opts Opts = {}>
    static handle<Inst> open(config c = {}) {
        // The ROM port has no clock math and no baud divisor of alloy's, so
        // there is no rate to admit — but it is still one instance with one
        // owner, and it used to have no double-open guard at all.
        alloy::claim::exclusive<Inst, alloy::claim::personality::uart>();
        hal::uart_impl<Inst>::template enable<Opts>(0u, c);
        return handle<Inst>{};
    }
};

template <class Inst, class Tx, class Rx, class Clock, class... Extra>
struct bind {
    using tx_pin = typename Tx::pin;
    using rx_pin = typename Rx::pin;

    //: The instance this role is bound to. The door to generated DEGREE
    //: numbers from portable code: `Role::inst::feat::rx_fifo_depth`.
    using inst = Inst;
    //: LAYER 2 for this role's IP. Spell it at the call site —
    //: `Role::open<Role::opts{...}>(...)` — because naming the type is what
    //: turns a substitution failure into a diagnostic that names the field.
    using opts = hal::uart_opts<Inst>;

    static constexpr bool has_de = detail::has_de<Extra...>;
    using de_pin = typename detail::de_pin_of<Extra...>::type;

    static_assert(routes::routable<tx_pin, Inst, signal::tx>,
                  "TX pin has no route to this UART on the selected chip "
                  "(check the chip's route table in alloy-devices)");
    static_assert(routes::routable<rx_pin, Inst, signal::rx>,
                  "RX pin has no route to this UART on the selected chip "
                  "(check the chip's route table in alloy-devices)");
    static_assert(!has_de || routes::routable<de_pin, Inst, signal::rts>,
                  "DE pin has no RTS route to this UART on the selected chip "
                  "(hardware driver-enable repurposes the RTS output)");

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::apb2: return Clock::apb2_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    // Route payload -> mux value per kind: af_fixed carries an AF number,
    // funcsel a FUNCSEL value; both reach the pin driver's make_af().
    template <class Route>
    static constexpr std::uint8_t mux_value() {
        static_assert(Route::k == routes::kind::af_fixed ||
                          Route::k == routes::kind::funcsel,
                      "route kind not implemented yet (full_matrix/psel pending)");
        if constexpr (Route::k == routes::kind::af_fixed) {
            return Route::af;
        } else {
            return Route::funcsel;
        }
    }

    // Layer 1 is the runtime argument, Layer 2 the template argument, and
    // both default — so `open({.baud = ...})` is unchanged, which is why this
    // is an additive MINOR and not a migration.
    template <opts Opts = {}>
    static handle<Inst> open(config c = {}) {
        using tx_route = routes::route<tx_pin, Inst, signal::tx>;
        using rx_route = routes::route<rx_pin, Inst, signal::rx>;

        // WHO OWNS THE INSTANCE — per instance and cross-TU, where the old
        // `detail_opened` was per BINDER TYPE. Two bind<>s naming one
        // usart2_t (a second legitimately routed pin pair) used to open both
        // and silently reprogram the port under the first handle.
        alloy::claim::exclusive<Inst, alloy::claim::personality::uart>();
        // And WHAT WAS ASKED FOR, before a register is touched.
        detail::admit_baud(c.baud, kernel_hz());
        if constexpr (requires {
                          hal::uart_impl<Inst>::template configure_running<Opts, has_de>(0u, c);
                      }) {
            detail_opts = &detail::opts_tag<Opts>;
        }

        hal::pin_impl<tx_pin>::make_af(mux_value<tx_route>());
        hal::pin_impl<rx_pin>::make_af(mux_value<rx_route>());
        if constexpr (has_de) {
            using de_route = routes::route<de_pin, Inst, signal::rts>;
            hal::pin_impl<de_pin>::make_af(mux_value<de_route>());
        }
        hal::uart_impl<Inst>::template enable<Opts, has_de>(kernel_hz(), c);
        return handle<Inst>{};
    }

    // Reprogram the full line shape on a RUNNING port — the RS-485/multidrop
    // path: a protocol write changes baud/parity, the ACK goes out at the old
    // settings, then this applies the new ones (UE-low rewrite + TEACK/REACK
    // waits live in the driver). Only exists where the driver implements
    // configure_running().
    //
    // Three things this used to accept and no longer does: a port nobody
    // opened, an impossible baud, and a set of Layer-2 opts different from the
    // ones open() programmed. The last one mattered because Layer 2 belonged
    // to the CALL and not to the handle, so `reconfigure<opts{...}>` could
    // quietly install a different frame than the port was opened with. It
    // belongs to the port now.
    template <opts Opts = {}>
    static void reconfigure(config c)
        requires requires {
            hal::uart_impl<Inst>::template configure_running<Opts, has_de>(0u, c);
        }
    {
        if (!alloy::claim::held<Inst, alloy::claim::personality::uart>()) {
            alloy::trap<alloy::trap_code::not_open>();
        }
        if (detail_opts != &detail::opts_tag<Opts>) {
            alloy::trap<alloy::trap_code::opts_mismatch>();
        }
        detail::admit_baud(c.baud, kernel_hz());
        hal::uart_impl<Inst>::template configure_running<Opts, has_de>(kernel_hz(), c);
    }

    // DEPRECATED — the nine-field path, kept for its stability window. It
    // never checked that the driver HONOURED the fields, only that a method
    // existed, which is the defect the layered surface replaces.
    [[deprecated("use reconfigure<Opts>(config) — serial_config's nine fields "
                 "were honoured by one driver in six")]]
    static void reconfigure_legacy(hal::serial_config c)
        requires requires { hal::uart_impl<Inst>::configure(0u, c); }
    {
        hal::uart_impl<Inst>::configure(kernel_hz(), c);
    }

    // Compile-time-checked open: the requested baud is REJECTED AT COMPILE TIME
    // if the driver's own divisor cannot reach it within TolPermille (parts per
    // thousand of the target; default 20 = 2%) on this board's kernel clock.
    // The achieved rate is computed by the SAME formula the driver programs, so
    // the check can never disagree with the hardware, and the failing
    // rate_check<requested, achieved, tol> names the numbers. Only offered where
    // the backing driver computes baud — ROM/bootloader-baud UARTs don't.
    //
    //   auto u = Dbg::open_checked<115'200_baud>();        // 2% default
    //   auto u = Dbg::open_checked<3'000'000_baud, 5>();   // tighten to 0.5%
    //
    // It takes a `frame`, not a `config`: the baud is already stated in the
    // template argument, and a second, disagreeing spelling of it used to be
    // accepted and silently discarded. `.baud` is not a member of `frame`, so
    // that conflict is now a compile error naming the member.
    template <alloy::frequency Baud, std::uint32_t TolPermille = 20,
              opts Opts = {}>
    static handle<Inst> open_checked(frame f = {})
        requires requires { hal::uart_impl<Inst>::achieved_baud(kernel_hz(), Baud.hz()); }
    {
        static_assert(Baud.hz() != 0u,
                      "open_checked<Baud>: a baud rate of zero has no divisor");
        constexpr alloy::frequency achieved =
            hal::uart_impl<Inst>::achieved_baud(kernel_hz(), Baud.hz());
        (void)alloy::rate_check<Baud.hz(), achieved.hz(), TolPermille>{};
        return open<Opts>(config{.baud = Baud.hz(), .parity = f.parity, .stop = f.stop});
    }

private:
    //: Which Layer-2 value open() programmed, as a unique address per `opts`
    //: value. Only written where the driver can reconfigure a running port,
    //: so a port that cannot be reconfigured pays nothing for the check.
    inline static const void* detail_opts = nullptr;
};

}  // namespace alloy::uart
