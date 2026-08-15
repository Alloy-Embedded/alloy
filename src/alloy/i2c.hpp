// User-facing I2C: typed pin binding with compile-time route checking,
// move-only handle satisfying the I2cBus concept.
//
//   auto bus = board::i2c::open({.speed_hz = 400'000});
//   if (!bus.probe(0x48)) { /* nothing ACKed */ }
//
// Errors are bool (false = NACK / bus error) — the honest v1 contract.
//
// THREE transfer paths, one bus, and they must not overlap: the polled
// write/read/write_read, the interrupt-driven write_async/read_async, and the
// one-shot DMA read_dma/write_dma (docs/design/dma-streams.md §6 phase 4).
// Every one of them consumes the same TXIS/RXNE/STOPF/NACKF flags; ask busy()
// before starting one while another may still be in flight.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/hal/i2c/i2c_impl.hpp"

namespace alloy::i2c {

struct config {
    std::uint32_t speed_hz = 100'000;
};

namespace detail {
// Layer 1's VALUE admission — see alloy/core/admit.hpp.
inline void admit_speed(std::uint32_t speed_hz, std::uint32_t kernel) {
    const bool ok = speed_hz != 0u && kernel != 0u && speed_hz <= kernel;
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::i2c_speed();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}
}  // namespace detail

template <class Pin>
struct scl {
    using pin = Pin;
};
template <class Pin>
struct sda {
    using pin = Pin;
};

// The board's `i2c.rx` / `i2c.tx` DMA assignments, attached to the binder by
// the GENERATOR as tags in the binder's Extra... list — the uart::rx_dma<> /
// uart::tx_dma<> pattern, for the same reason (docs/design/dma-streams.md §1):
// a board that assigns "i2c.rx": {controller, channel} gets one of these
// appended to its generated bind<>, a board that assigns nothing simply has no
// tag, and the DMA methods are constrained away with the missing fact named
// rather than failing deep in a body.
//
// The binder and the handle both expose them as `rx_route` / `tx_route`, so
// PORTABLE code has a DEPENDENT name to gate on and to claim
// (`alloy::dma::claim(board::i2c::rx_route{})`). Probe THAT, never the
// `board::dma::i2c_rx` constant: a missing namespace member is a hard error,
// not a substitution failure, so it does not fold in a requires-clause.
template <class Route>
struct rx_dma {
    using route = Route;
};
template <class Route>
struct tx_dma {
    using route = Route;
};

namespace detail {
template <class T>
inline constexpr bool is_rx_dma_tag = false;
template <class Route>
inline constexpr bool is_rx_dma_tag<i2c::rx_dma<Route>> = true;

template <class... Extra>
struct rx_route_of {
    using type = void;
};
template <class Route, class... Rest>
struct rx_route_of<i2c::rx_dma<Route>, Rest...> {
    using type = Route;
};
template <class First, class... Rest>
    requires(!is_rx_dma_tag<First>)
struct rx_route_of<First, Rest...> : rx_route_of<Rest...> {};

template <class T>
inline constexpr bool is_tx_dma_tag = false;
template <class Route>
inline constexpr bool is_tx_dma_tag<i2c::tx_dma<Route>> = true;

template <class... Extra>
struct tx_route_of {
    using type = void;
};
template <class Route, class... Rest>
struct tx_route_of<i2c::tx_dma<Route>, Rest...> {
    using type = Route;
};
template <class First, class... Rest>
    requires(!is_tx_dma_tag<First>)
struct tx_route_of<First, Rest...> : tx_route_of<Rest...> {};
}  // namespace detail

// What a claimed DMA channel token must offer before the one-shot DMA methods
// exist on a handle. Spelled as concepts so the requires-gate names the whole
// contract in one place; `alloy::dma::channel` satisfies them, and so does any
// hand-rolled token with the same four operations.
//
// Direction-specific on purpose: a board may assign `i2c.rx` and not `i2c.tx`
// (nucleo_g071rb does exactly that — its die offers nine assignable DMA
// signals for seven channels), and half a one-shot IS a thing, unlike half a
// duplex.
template <class Chan>
concept rx_channel = requires(const Chan& c, std::span<std::uint8_t> dst) {
    c.start_p2m_u8(std::uintptr_t{}, dst, std::uint8_t{});
    { c.done() } -> std::convertible_to<bool>;
    { c.error() } -> std::convertible_to<bool>;
    c.stop();
};

template <class Chan>
concept tx_channel = requires(const Chan& c, std::span<const std::uint8_t> src) {
    c.start_m2p_u8(src, std::uintptr_t{}, std::uint8_t{});
    { c.done() } -> std::convertible_to<bool>;
    { c.error() } -> std::convertible_to<bool>;
    c.stop();
};

template <class Inst, class RxRoute = void, class TxRoute = void>
class handle {
public:
    //: The board's `i2c.rx` / `i2c.tx` assignments (generated alloy::dma
    //: routes, attached via the rx_dma<>/tx_dma<> tags), or void when the
    //: board assigns none — the dependent names portable code probes.
    using rx_route = RxRoute;
    using tx_route = TxRoute;

    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    [[nodiscard]] bool write(std::uint8_t addr, std::span<const std::uint8_t> data) const {
        return hal::i2c_impl<Inst>::write(addr, data);
    }
    [[nodiscard]] bool read(std::uint8_t addr, std::span<std::uint8_t> data) const {
        return hal::i2c_impl<Inst>::read(addr, data);
    }
    [[nodiscard]] bool write_read(std::uint8_t addr, std::span<const std::uint8_t> wr,
                                  std::span<std::uint8_t> rd) const {
        return hal::i2c_impl<Inst>::write_read(addr, wr, rd);
    }
    // Address probe: true when a device ACKs (a zero-length write).
    [[nodiscard]] bool probe(std::uint8_t addr) const {
        return hal::i2c_impl<Inst>::write(addr, {});
    }

    // --- Interrupt-driven transfers ---------------------------------------
    //
    // write/read/write_read above spin once per byte: an N-byte transfer burns
    // the CPU for all of it, ~90 µs a byte at 100 kHz. write_async/read_async
    // program the transfer and RETURN; the driver's ISR moves every byte and
    // calls fn(ctx) when the STOP lands. `fn` runs in interrupt context — set a
    // flag, wake a task, or start the next transfer, nothing more.
    //
    // Whether the transfer SUCCEEDED is a separate question from whether it
    // finished: ask transfer_ok() after it completes, the async counterpart of
    // the bool that write()/read() return.
    //
    // Only exists where the backing driver implements it (ST i2c_v2 today). On
    // a port without the hook the method is not declared at all, rather than
    // silently degrading to a blocking loop.
    //
    // The two APIs must not overlap on one bus: the ISR consumes the very flags
    // the blocking path waits for. Ask busy() first.
    void write_async(std::uint8_t addr, std::span<const std::uint8_t> data,
                     void (*fn)(void*) = nullptr, void* ctx = nullptr) const
        requires requires { hal::i2c_impl<Inst>::start_transfer_irq(0u, nullptr, nullptr, 1u, fn, ctx); }
    {
        hal::i2c_impl<Inst>::start_transfer_irq(addr, data.data(), nullptr,
                                                static_cast<std::uint16_t>(data.size()), fn, ctx);
    }

    void read_async(std::uint8_t addr, std::span<std::uint8_t> data,
                    void (*fn)(void*) = nullptr, void* ctx = nullptr) const
        requires requires { hal::i2c_impl<Inst>::start_transfer_irq(0u, nullptr, nullptr, 1u, fn, ctx); }
    {
        hal::i2c_impl<Inst>::start_transfer_irq(addr, nullptr, data.data(),
                                                static_cast<std::uint16_t>(data.size()), fn, ctx);
    }

    /// True while an interrupt-driven transfer is still in flight.
    [[nodiscard]] bool busy() const
        requires requires { hal::i2c_impl<Inst>::transfer_busy(); }
    {
        return hal::i2c_impl<Inst>::transfer_busy();
    }

    /// False if the last interrupt-driven transfer NACKed, hit a bus error, or
    /// was abandoned — the same contract as write()/read() returning false.
    [[nodiscard]] bool transfer_ok() const
        requires requires { hal::i2c_impl<Inst>::transfer_ok(); }
    {
        return hal::i2c_impl<Inst>::transfer_ok();
    }

    /// Spin until the in-flight transfer completes. BOUNDED — returns false if
    /// the budget runs out, which is what an interrupt that never fires looks
    /// like. An unbounded wait would turn that into a hang, and a hang is not a
    /// test failure, it is a timeout that looks like anything at all.
    [[nodiscard]] bool wait_transfer() const
        requires requires { hal::i2c_impl<Inst>::transfer_busy(); }
    {
        for (std::uint32_t spin = 0; spin < kWaitBudget; ++spin) {
            if (!hal::i2c_impl<Inst>::transfer_busy()) {
                return true;
            }
        }
        return false;
    }

    /// Disarm the interrupt and forget any in-flight transfer — the escape
    /// hatch after wait_transfer() returned false, so a bus error that produced
    /// no STOP does not leave the bus permanently busy().
    void detach_transfer() const
        requires requires { hal::i2c_impl<Inst>::disable_transfer_irq(); }
    {
        hal::i2c_impl<Inst>::disable_transfer_irq();
    }

    // WHERE THE REQUEST ID COMES FROM — one place per direction, so the two
    // DMA methods cannot disagree with each other or with the board. Two
    // silicon shapes (design §1):
    //  * DMAMUX / free-router (G0 shape): the id is a chip-wide fact,
    //    `Inst::dmareq_rx` / `Inst::dmareq_tx` — any channel may serve it.
    //  * Stream engine (F4/F7 dma_v2): there is no chip-wide id, because CHSEL
    //    is a per-stream fact of the MATCHED dma_routes triple; the board's
    //    route is then the only honest source, and the channel passed in must
    //    be the one that route names.
    // When both exist they are equal by construction — the emitter derives the
    // route's request from the chip's `dma_requests` — and the chip-wide fact
    // is preferred, which keeps the escape hatch (an explicitly claimed
    // channel on a board that assigned no route) working. Same rule as
    // uart::handle::detail_tx_request(), deliberately.
    static constexpr std::uint8_t detail_rx_request()
        requires requires { Inst::dmareq_rx; } || (!std::is_void_v<RxRoute>)
    {
        if constexpr (requires { Inst::dmareq_rx; }) {
            return Inst::dmareq_rx;
        } else {
            return RxRoute::request;
        }
    }

    static constexpr std::uint8_t detail_tx_request()
        requires requires { Inst::dmareq_tx; } || (!std::is_void_v<TxRoute>)
    {
        if constexpr (requires { Inst::dmareq_tx; }) {
            return Inst::dmareq_tx;
        } else {
            return TxRoute::request;
        }
    }

    // --- One-shot DMA transfers -------------------------------------------
    //
    // read_dma()/write_dma() are the write_async/read_async story with the
    // CPU removed from the DATA phase as well as from the waiting: the driver
    // programs the address phase, the engine moves every payload byte, and the
    // CPU's only remaining job is to wait for the AUTOEND STOP.
    //
    // WHAT DMA MOVES AND WHAT IT DOES NOT — the boundary, stated here because
    // it is what keeps this path small and keeps the existing refusals intact.
    // The DEVICE ADDRESS, the direction bit, NBYTES and AUTOEND are written to
    // CR2 by the CPU, by the same driver helper the polled and interrupt paths
    // use. The engine never sees an address and never ends a transaction.
    // Consequences, all deliberate: the 255-byte NBYTES refusal is inherited
    // rather than re-implemented (a 256-byte read_dma() returns false having
    // armed nothing, exactly as read() returns false); AUTOEND still makes the
    // STOP, so completion is still STOPF and a NACK is still NACKF and both
    // still report through the shipped `false`; and the RELOAD/TCR chunking
    // that a >255-byte transfer would need stays exactly as unimplemented as
    // it is on the polled path, in one place, for all three paths at once.
    //
    // THE CHANNEL IS THE CALLER'S. These take a claimed token, the shipped
    // `uart::write_dma` spelling — claim once at startup and reuse it:
    //
    //     auto rx = alloy::dma::claim(board::i2c::rx_route{});
    //     bus.read_dma(rx, 0x48, buf);
    //
    // A route-only overload that claimed and released per call was considered
    // and rejected: release-on-destruction is the STREAM's departure from the
    // one-token-per-firmware rule (design §1), granted to rings because a ring
    // owns hardware for its whole lifetime. A one-shot has no lifetime to hang
    // it on, and a second ownership mechanism whose trap looks like the first
    // one's is exactly what `claim::sub_exclusive` exists to avoid.
    //
    // ORDER, both directions the same, and asserted by tests/test_i2c_dma.cpp:
    //   arm the CHANNEL, then the peripheral's request enable, then the
    //   address phase that starts traffic. Teardown is the exact reverse — the
    //   request enable goes off before the channel stops, so a late request
    //   cannot reach a channel that is still armed. Teardown runs on EVERY
    //   exit path, including the refusals and the timeout, so a failed
    //   transfer never leaves DMAEN set on the bus.
    template <class Chan>
    [[nodiscard]] bool read_dma(const Chan& dma, std::uint8_t addr,
                                std::span<std::uint8_t> data) const
        requires rx_channel<Chan> && requires {
            hal::i2c_impl<Inst>::dma_rx_begin();
            hal::i2c_impl<Inst>::kMaxNbytes;
            detail_rx_request();
        }
    {
        if (data.empty() || data.size() > hal::i2c_impl<Inst>::kMaxNbytes) {
            return false;  // the NBYTES boundary, before anything is armed
        }
        dma.start_p2m_u8(hal::i2c_impl<Inst>::rxdr_addr(), data, detail_rx_request());
        hal::i2c_impl<Inst>::dma_rx_begin();
        bool ok = hal::i2c_impl<Inst>::dma_start_transfer(addr, data.size(),
                                                          /*is_read=*/true);
        ok = ok && wait_dma(dma);
        ok = ok && hal::i2c_impl<Inst>::dma_wait_stop();
        hal::i2c_impl<Inst>::dma_rx_end();
        dma.stop();
        return ok;
    }

    template <class Chan>
    [[nodiscard]] bool write_dma(const Chan& dma, std::uint8_t addr,
                                 std::span<const std::uint8_t> data) const
        requires tx_channel<Chan> && requires {
            hal::i2c_impl<Inst>::dma_tx_begin();
            hal::i2c_impl<Inst>::kMaxNbytes;
            detail_tx_request();
        }
    {
        if (data.empty() || data.size() > hal::i2c_impl<Inst>::kMaxNbytes) {
            return false;
        }
        // The source must be DMA-visible RAM: on SAME70 the XDMAC cannot read
        // embedded flash (design §4) — copy .rodata payloads to RAM first.
        dma.start_m2p_u8(data, hal::i2c_impl<Inst>::txdr_addr(), detail_tx_request());
        hal::i2c_impl<Inst>::dma_tx_begin();
        bool ok = hal::i2c_impl<Inst>::dma_start_transfer(addr, data.size(),
                                                          /*is_read=*/false);
        ok = ok && wait_dma(dma);
        ok = ok && hal::i2c_impl<Inst>::dma_wait_stop();
        hal::i2c_impl<Inst>::dma_tx_end();
        dma.stop();
        return ok;
    }

    // REPEATED-START UNDER DMA IS DEFERRED, and this deleted declaration is
    // the deferral said out loud: `bus.write_read_dma(...)` is a compile error
    // naming the function, and a portable program's
    // `requires { b.write_read_dma(...) }` probe is false, so the gap behaves
    // like every other missing capability in this framework instead of being
    // a method nobody thought to write.
    //
    // WHY. The polled write_read() (st_i2c_v2_body.hpp) runs the write phase
    // with AUTOEND=0 and hands off to the read phase on TC — a repeated START
    // with the bus held between the two. Under DMA that handoff has no
    // reachable event: CR1.TCIE is deliberately uncurated in alloy-devices'
    // st/i2c_v2 register file, so there is no interrupt for transfer-complete,
    // and the only alternative is a CPU spin on TC BETWEEN two DMA transfers —
    // which puts the CPU back in the middle of the transaction that DMA exists
    // to take it out of, and adds a second arm/teardown cycle while the bus is
    // held. Doing it properly means curating TCIE (an alloy-devices task,
    // question 0) and giving this driver a two-phase completion topology.
    //
    // NOTE ON PROVENANCE: docs/design/dma-streams.md does NOT say this. The
    // whole design mentions I2C exactly twice (the status note's peripheral
    // list and the §6 phase-4 row "i2c one-shot DMA read/write") and says
    // nothing about repeated start, so this is a call made HERE, not doctrine
    // quoted from there. The phase-4 row's "one-shot" is what is shipped.
    template <class Chan>
    bool write_read_dma(const Chan&, std::uint8_t, std::span<const std::uint8_t>,
                        std::span<std::uint8_t>) const = delete;

private:
    // The DMA half of the wait, BOUNDED for the same reason wait_transfer()
    // is: an engine whose completion never arrives (a wedged bus that stalls
    // the request stream, a channel the board pointed somewhere else) must
    // become an honest false, not a hang. `dma::channel::wait()` spins
    // forever by design — right for a transfer the CPU started and the CPU
    // alone finishes, wrong here, where the DMA only moves when a bus that can
    // wedge asks it to. Same budget and same reasoning as kWaitBudget above.
    template <class Chan>
    [[nodiscard]] static bool wait_dma(const Chan& dma) {
        for (std::uint32_t spin = 0; spin < kWaitBudget; ++spin) {
            if (dma.error()) {
                return false;
            }
            if (dma.done()) {
                return !dma.error();  // an error on the last beat coincides
            }
        }
        return false;
    }

    // Iteration cap for wait_transfer(), the same idiom (and reasoning) as the
    // driver's kPollBudget: far above any legal transfer's latency, so it only
    // ever fires on a genuine fault.
    static constexpr std::uint32_t kWaitBudget = 4'000'000u;

    template <class, class, class, class, class...>
    friend struct bind;
    handle() = default;
};

template <class Inst, class Scl, class Sda, class Clock, class... Extra>
struct bind {
    using scl_pin = typename Scl::pin;
    using sda_pin = typename Sda::pin;

    //: The instance this role is bound to.
    using inst = Inst;

    //: The board's `i2c.rx` DMA assignment (a generated alloy::dma::route,
    //: attached via the rx_dma<> tag), or void when the board assigns none —
    //: what gates handle::read_dma() at compile time on a stream engine, and
    //: what portable code claims: `alloy::dma::claim(Role::rx_route{})`.
    using rx_route = typename detail::rx_route_of<Extra...>::type;
    //: The `i2c.tx` twin (tx_dma<> tag), or void.
    using tx_route = typename detail::tx_route_of<Extra...>::type;

    static_assert(routes::routable<scl_pin, Inst, signal::scl>,
                  "SCL pin has no route to this I2C on the selected chip "
                  "(check the chip's route table in alloy-devices)");
    static_assert(routes::routable<sda_pin, Inst, signal::sda>,
                  "SDA pin has no route to this I2C on the selected chip");

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::apb2: return Clock::apb2_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    template <class Route, class Pin>
    static void route_pin() {
        constexpr std::uint8_t mux = routes::mux_value<Route>();
        // I2C pads must be open-drain where the mux doesn't imply it (ST);
        // drivers that need it expose make_af_od, others fall back.
        if constexpr (requires { hal::pin_impl<Pin>::make_af_od(mux); }) {
            hal::pin_impl<Pin>::make_af_od(mux);
        } else {
            hal::pin_impl<Pin>::make_af(mux);
        }
    }

    static handle<Inst, rx_route, tx_route> open(config c = {}) {
        // Per INSTANCE, cross-TU (alloy/core/claim.hpp), not per binder type.
        alloy::claim::exclusive<Inst, alloy::claim::personality::i2c>();
        detail::admit_speed(c.speed_hz, kernel_hz());
        using scl_route = routes::route<scl_pin, Inst, signal::scl>;
        using sda_route = routes::route<sda_pin, Inst, signal::sda>;
        route_pin<scl_route, scl_pin>();
        route_pin<sda_route, sda_pin>();
        hal::i2c_impl<Inst>::enable(kernel_hz(), c.speed_hz);
        return handle<Inst, rx_route, tx_route>{};
    }
};

}  // namespace alloy::i2c
