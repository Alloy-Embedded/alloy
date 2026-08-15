// User-facing SPI: typed pin binding, move-only handle satisfying SpiBus.
// Chip-selects are plain gpio outputs owned by the caller (the shared
// SpiDevice layer arrives later, embedded-hal style).

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/dma.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/hal/spi/spi_impl.hpp"

namespace alloy::spi {

struct config {
    std::uint32_t clock_hz = 1'000'000;
    std::uint8_t mode = 0;  // CPOL<<1 | CPHA
};

namespace detail {
// Layer 1's VALUE admission — see alloy/core/admit.hpp.
inline void admit_clock(std::uint32_t clock_hz, std::uint32_t kernel) {
    const bool ok = clock_hz != 0u && kernel != 0u && clock_hz <= kernel;
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::spi_clock();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}
}  // namespace detail

template <class Pin>
struct sck {
    using pin = Pin;
};
template <class Pin>
struct miso {
    using pin = Pin;
};
template <class Pin>
struct mosi {
    using pin = Pin;
};

// The board's DMA assignments for this role ("spi.rx" / "spi.tx" in
// board.json), attached to the binder as tags by the generator — the same
// mechanism the UART's rx_dma/tx_dma already uses, and one emitter helper
// spells both so a binder tag and its `board::dma` twin cannot drift.
//
// A pair is BOTH of these or neither. The generator will not invent the
// missing half (picking a channel is exactly what design §1 forbids it), so a
// board that states one direction gets one tag and `transfer_dma` is
// constrained away with the missing fact named. PORTABLE code probes through
// the binder's dependent alias (`Role::rx_route`), never the namespace-scope
// `board::dma::spi_rx` constant — a namespace-scope constant does not fold in
// a requires-clause, which is the phase-2 lesson this project already paid
// for once.
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
inline constexpr bool is_rx_dma_tag<spi::rx_dma<Route>> = true;

template <class... Extra>
struct rx_route_of {
    using type = void;
};
template <class Route, class... Rest>
struct rx_route_of<spi::rx_dma<Route>, Rest...> {
    using type = Route;
};
template <class First, class... Rest>
    requires (!is_rx_dma_tag<First>)
struct rx_route_of<First, Rest...> : rx_route_of<Rest...> {};

template <class T>
inline constexpr bool is_tx_dma_tag = false;
template <class Route>
inline constexpr bool is_tx_dma_tag<spi::tx_dma<Route>> = true;

template <class... Extra>
struct tx_route_of {
    using type = void;
};
template <class Route, class... Rest>
struct tx_route_of<spi::tx_dma<Route>, Rest...> {
    using type = Route;
};
template <class First, class... Rest>
    requires (!is_tx_dma_tag<First>)
struct tx_route_of<First, Rest...> : tx_route_of<Rest...> {};

}  // namespace detail

// What the port and the two routes must offer before `spi.transfer_dma()`
// exists: the driver's DMA endpoint hooks (one data-register address, a
// request enable per direction, the drain-and-stop that keeps the last frame
// from being truncated) and two routes that are actually assigned. One
// concept, so the facade's requires-gate names the whole contract in one
// place and a portable program can spell the predicate itself — the same
// reason `uart::rx_stream_capable` is public.
template <class Inst, class RxRoute, class TxRoute>
concept pair_capable =
    (!std::is_void_v<RxRoute>) && (!std::is_void_v<TxRoute>) && requires {
        hal::spi_impl<Inst>::dr_addr();
        hal::spi_impl<Inst>::dma_rx_begin();
        hal::spi_impl<Inst>::dma_tx_begin();
        hal::spi_impl<Inst>::dma_end();
    };

// RxRoute/TxRoute are the board's `spi.rx` / `spi.tx` assignments (rx_dma<> /
// tx_dma<> tags on the binder), or void where the board assigned none — which
// constrains transfer_dma() away, so a portable program's `requires` probe
// reports the missing fact at compile time instead of at link time.
template <class Inst, class RxRoute = void, class TxRoute = void>
class handle {
public:
    //: The board's `spi.rx` / `spi.tx` assignments, or void when the board
    //: assigns none — the dependent names portable code probes, mirroring
    //: i2c::handle. The binder carries the same two aliases; the HANDLE is
    //: what a portable program has in hand (`board::spi::open(...)` returns
    //: it), and a namespace-scope `board::dma::spi_rx` does not SFINAE-fold
    //: in a requires-clause, so this is the only spelling that folds.
    using rx_route = RxRoute;
    using tx_route = TxRoute;

    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    [[nodiscard]] std::uint8_t xfer(std::uint8_t byte) const {
        return hal::spi_impl<Inst>::xfer(byte);
    }
    void write(std::span<const std::uint8_t> data) const {
        for (auto b : data) {
            (void)hal::spi_impl<Inst>::xfer(b);
        }
    }
    void transfer(std::span<std::uint8_t> data) const {  // in-place
        for (auto& b : data) {
            b = hal::spi_impl<Inst>::xfer(b);
        }
    }

    // --- Interrupt-driven transfer ---------------------------------------
    //
    // xfer/write/transfer above are byte-at-a-time spins: an N-byte exchange
    // burns the CPU for the whole of it. transfer_async starts the exchange and
    // RETURNS; the driver's ISR clocks every remaining byte and calls fn(ctx)
    // when the last one lands. `fn` runs in interrupt context — set a flag,
    // wake a task, or start the next transfer, nothing more.
    //
    // Only exists where the backing driver implements it (ST spi_v1/spi_v2
    // today). On a port without the hook the method is not declared at all,
    // rather than silently degrading to a blocking loop.
    //
    // The two APIs must not overlap on one bus: the ISR consumes the RXNE that
    // the blocking xfer() waits for. Ask busy() first.
    void transfer_async(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx,
                        void (*fn)(void*) = nullptr, void* ctx = nullptr) const
        requires requires { hal::spi_impl<Inst>::start_transfer_irq(nullptr, nullptr, 1u, fn, ctx); }
    {
        if (!tx.empty() && !rx.empty() && tx.size() != rx.size()) {
            __builtin_trap();  // ambiguous full-duplex length: honest runtime guard
        }
        const std::size_t n = tx.empty() ? rx.size() : tx.size();
        hal::spi_impl<Inst>::start_transfer_irq(tx.empty() ? nullptr : tx.data(),
                                                rx.empty() ? nullptr : rx.data(),
                                                static_cast<std::uint16_t>(n), fn, ctx);
    }

    /// True while an interrupt-driven transfer is still in flight.
    [[nodiscard]] bool busy() const
        requires requires { hal::spi_impl<Inst>::transfer_busy(); }
    {
        return hal::spi_impl<Inst>::transfer_busy();
    }

    /// Spin until the in-flight transfer completes. BOUNDED — returns false if
    /// the budget runs out, which is what an interrupt that never fires looks
    /// like. An unbounded wait would turn that into a hang, and a hang is not a
    /// test failure, it is a timeout that looks like anything at all.
    [[nodiscard]] bool wait_transfer() const
        requires requires { hal::spi_impl<Inst>::transfer_busy(); }
    {
        for (std::uint32_t spin = 0; spin < kWaitBudget; ++spin) {
            if (!hal::spi_impl<Inst>::transfer_busy()) {
                return true;
            }
        }
        return false;
    }

    /// Disarm the interrupt and forget any in-flight transfer — the escape
    /// hatch after wait_transfer() returned false, so a wedged bus does not
    /// leave the bus permanently busy().
    void detach_transfer() const
        requires requires { hal::spi_impl<Inst>::disable_transfer_irq(); }
    {
        hal::spi_impl<Inst>::disable_transfer_irq();
    }

    // --- DMA full duplex: the design's anchor 2.4 --------------------------
    //
    // WHERE EACH DIRECTION'S REQUEST ID COMES FROM — the one place, so the two
    // halves cannot disagree. Two silicon shapes (design §1):
    //  * DMAMUX / free router (G0 shape): the id is chip-wide,
    //    `Inst::dmareq_rx` / `Inst::dmareq_tx`, and any channel can serve it.
    //  * stream engine (F4/F7 dma_v2): there is NO chip-wide id — CHSEL is a
    //    per-stream field, so SPI1_RX is request 3 on dma2 stream 0 and also
    //    on stream 2, and only the triple the board MATCHED is the honest
    //    source. That is the route's request.
    // Where both exist they are equal by construction (board.py derives the
    // route's request from the chip's `dma_requests`), and the chip-wide fact
    // wins so the explicit-route escape hatch below keeps working on a board
    // that assigned nothing.
    //
    // Parameterised on the route rather than reading the handle's own, so the
    // explicit-route overload below derives its ids by the SAME rule as the
    // board-assigned one — two spellings of one call, not two rules.
    template <class Route>
    static constexpr std::uint8_t detail_rx_request()
        requires requires { Inst::dmareq_rx; } || (!std::is_void_v<Route>)
    {
        if constexpr (requires { Inst::dmareq_rx; }) {
            return Inst::dmareq_rx;
        } else {
            return Route::request;
        }
    }
    template <class Route>
    static constexpr std::uint8_t detail_tx_request()
        requires requires { Inst::dmareq_tx; } || (!std::is_void_v<Route>)
    {
        if constexpr (requires { Inst::dmareq_tx; }) {
            return Inst::dmareq_tx;
        } else {
            return Route::request;
        }
    }

    /// Exchange `tx` for `rx` with the CPU out of the data path: claims BOTH
    /// board-assigned channels as one unit (RX before TX — the order that
    /// cannot lose the first byte back), runs both, waits both, releases both.
    /// One call, because half a duplex is not a thing.
    ///
    /// THE BOOL MEANS SOMETHING. It is false when either channel reported a
    /// transfer error, when either never moved, or when the port failed to
    /// drain — a half-failed duplex did not succeed, even though half of it
    /// did. (The driver's error latch is what keeps a failure readable after
    /// the completion interrupt consumed its flag.)
    ///
    /// `tx` must live in DMA-visible RAM: on SAME70 the XDMAC cannot read
    /// embedded flash and a string literal traps as a bus error (design §4).
    /// ST families read flash fine, so this is a portability rule, not an
    /// ST one.
    ///
    /// TEARDOWN ORDER (design §4): the peripheral's requests come down first
    /// (`dma_end`), and the channels stop after, when the pair goes out of
    /// scope — a request that lands mid-teardown on a disabled channel is how
    /// overrun flags get stuck.
    [[nodiscard]] bool transfer_dma(std::span<const std::uint8_t> tx,
                                    std::span<std::uint8_t> rx) const
        requires pair_capable<Inst, RxRoute, TxRoute> && requires {
            detail_rx_request<RxRoute>();
            detail_tx_request<TxRoute>();
        }
    {
        return transfer_dma(RxRoute{}, TxRoute{}, tx, rx);
    }

    /// The explicit-route overload — the documented escape hatch for
    /// hand-wired projects (design §1): the same exchange, on routes the
    /// caller spelled out instead of the board's assignment. It exists on a
    /// route-less handle too, which is what makes the escape hatch an escape
    /// hatch and not a second board mechanism.
    template <class Rx, class Tx>
    [[nodiscard]] bool transfer_dma(Rx, Tx, std::span<const std::uint8_t> tx,
                                    std::span<std::uint8_t> rx) const
        requires pair_capable<Inst, Rx, Tx> && requires {
            detail_rx_request<Rx>();
            detail_tx_request<Tx>();
        }
    {
        if (tx.empty() || tx.size() != rx.size()) {
            // A duplex has ONE length, and it is not zero: every byte clocked
            // out brings a byte back. An honest runtime guard, the same shape
            // transfer_async's length check already has.
            __builtin_trap();
        }
        const std::uintptr_t dr = hal::spi_impl<Inst>::dr_addr();
        // Claims RX then TX in one interrupts-masked section (dma::pair).
        dma::pair<Rx, Tx> both{};
        // The RM's full-duplex order, which ST's own HAL follows: arm the
        // receive channel, raise RXDMAEN, arm the transmit channel, raise
        // TXDMAEN — and that last write is what starts the traffic.
        both.start_rx_p2m_u8(dr, rx, detail_rx_request<Rx>());
        hal::spi_impl<Inst>::dma_rx_begin();
        both.start_tx_m2p_u8(tx, dr, detail_tx_request<Tx>());
        hal::spi_impl<Inst>::dma_tx_begin();
        // RX completion is the honest completion of a duplex — every byte
        // received implies every byte was clocked — but the pair waits for
        // both, so a transmit-side error is not silently forgiven.
        const bool moved = both.wait();
        // ...and the port drains before anything drops chip-select: the DMA
        // finishing means the last byte reached the FIFO, not the wire.
        const bool drained = hal::spi_impl<Inst>::dma_end();
        return moved && drained;
    }

private:
    // Iteration cap for wait_transfer(), the same idiom (and reasoning) as the
    // ST I2C driver's kPollBudget: far above any legal transfer's latency, so
    // it only ever fires on a genuine fault.
    static constexpr std::uint32_t kWaitBudget = 4'000'000u;

    template <class, class, class, class, class, class...>
    friend struct bind;
    handle() = default;
};

// `Extra...` is a TAG PACK, not more pins: the board's DMA assignments ride in
// as `rx_dma<Route>` / `tx_dma<Route>`, the way the UART's already do. A pack
// rather than positional route parameters (the ADC's shape) because SPI needs
// TWO routes and positional parameters after `Clock` would change the spelling
// of every binder that has none.
template <class Inst, class Sck, class Miso, class Mosi, class Clock, class... Extra>
struct bind {
    //: The board's `spi.rx` DMA assignment (a generated alloy::dma::route,
    //: attached via the rx_dma<> tag), or void when the board assigns none.
    //: PORTABLE code gates on THIS — `requires { s.transfer_dma(t, r); }`
    //: folds on the binder's dependent alias and cannot fold on the
    //: namespace-scope `board::dma::spi_rx` constant.
    using rx_route = typename detail::rx_route_of<Extra...>::type;
    //: The `spi.tx` twin (tx_dma<> tag), or void. A pair is both or neither:
    //: a board that states one direction leaves transfer_dma constrained
    //: away, because inventing the missing channel is what design §1 forbids
    //: a generator to do.
    using tx_route = typename detail::tx_route_of<Extra...>::type;

    static_assert(routes::routable<typename Sck::pin, Inst, signal::sck>,
                  "SCK pin has no route to this SPI on the selected chip");
    static_assert(routes::routable<typename Miso::pin, Inst, signal::miso>,
                  "MISO pin has no route to this SPI on the selected chip");
    static_assert(routes::routable<typename Mosi::pin, Inst, signal::mosi>,
                  "MOSI pin has no route to this SPI on the selected chip");

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::apb2: return Clock::apb2_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    template <class Pin, alloy::signal S>
    static void route_pin() {
        using R = routes::route<Pin, Inst, S>;
        hal::pin_impl<Pin>::make_af(routes::mux_value<R>());
    }

    static handle<Inst, rx_route, tx_route> open(config c = {}) {
        // Per INSTANCE, cross-TU (alloy/core/claim.hpp), not per binder type.
        alloy::claim::exclusive<Inst, alloy::claim::personality::spi>();
        detail::admit_clock(c.clock_hz, kernel_hz());
        route_pin<typename Sck::pin, signal::sck>();
        route_pin<typename Miso::pin, signal::miso>();
        route_pin<typename Mosi::pin, signal::mosi>();
        hal::spi_impl<Inst>::enable(kernel_hz(), c.clock_hz, c.mode);
        return handle<Inst, rx_route, tx_route>{};
    }
};

}  // namespace alloy::spi
