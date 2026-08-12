// User-facing GPIO wrappers. Stateless, zero-size, satisfy the OutputPin /
// InputPin concepts. Generated boards expose roles as inline constexpr
// instances of these types.
//
// NO CLAIM ON THE PIN, and this one is a limit rather than a rule — the honest
// statement of a THIRD ownership scope that alloy::claim does not model. A pin
// is not a
// peripheral instance and not a numbered part of one; it is a resource that
// several peripherals compete for through the mux. The shipped boards prove
// the contention is real and often INTENTIONAL: on the nucleo_g0b1re PA5 is
// both the `led` role (GPIO output) and the `led_pwm` role (TIM2_CH1 via AF),
// and PB3/PB4/PB5 are both `gpio_bus` and `spi` — the board offers the
// alternatives and the application picks one. A pin claim that fired on those
// would refuse a board alloy ships on purpose, and putting it in `set()` or
// `read()` would tax the hottest path in the framework. What the type system
// DOES enforce here is that a pin reaches a peripheral at all: `routes::route`
// is a compile error for a pin that has no route (guard #7). Who wins when two
// routes are both legal is, today, the programmer's problem.
//
// ONE CLAIM BEHIND on_edge(), though, and it is not on the pin — it is on the
// EXTI LINE the pin is wired to, which IS a numbered part of a peripheral. PA5
// and PB5 are one line with one port-select field, one trigger pair and one
// callback slot, and arming both used to compile clean and leave the FIRST
// pin's handle reporting the SECOND pin's edges as its own. The claim lives in
// the exti driver, where the line number is (alloy/hal/exti/exti_impl.hpp);
// clear_on_edge() releases it, so handing a line from one pin to another is
// still allowed as long as the program says so out loud.

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include "alloy/hal/exti/exti_impl.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"

namespace alloy::gpio {

struct active_high_t {};
struct active_low_t {};

// Which transition on an input raises an interrupt. Aliased rather than
// redeclared: one enum means the user layer and the driver layer cannot drift
// into disagreeing about what `rising` is.
using edge = hal::pin_edge;

template <class Pin, class Polarity = active_high_t>
class output {
    static constexpr bool inverted = std::same_as<Polarity, active_low_t>;
    using impl = hal::pin_impl<Pin>;

public:
    static void init() { impl::make_output(); }

    void set_high() const { impl::set_high(); }
    void set_low() const { impl::set_low(); }
    void toggle() const { impl::toggle(); }

    // Polarity-aware semantic helpers (LEDs, enables).
    void on() const {
        if constexpr (inverted) { impl::set_low(); } else { impl::set_high(); }
    }
    void off() const {
        if constexpr (inverted) { impl::set_high(); } else { impl::set_low(); }
    }
};

// No-op stand-in for OPTIONAL role pins (spi_cs, eeprom_wp) on boards that
// don't wire them — keeps `if constexpr (caps)` discarded branches valid.
struct null_output {
    static void init() {}
    void set_high() const {}
    void set_low() const {}
    void toggle() const {}
    void on() const {}
    void off() const {}
};

template <class Pin, class Polarity = active_high_t>
class input {
    static constexpr bool inverted = std::same_as<Polarity, active_low_t>;
    using impl = hal::pin_impl<Pin>;

public:
    static void init() { impl::make_input(); }
    static void init_pullup() { impl::make_input_pullup(); }

    [[nodiscard]] bool is_high() const { return impl::read(); }

    // Polarity-aware: true when the button/signal is logically active.
    [[nodiscard]] bool is_active() const { return impl::read() != inverted; }

    // ── Edge interrupts ──────────────────────────────────────────────────
    //
    // The alternative to polling: the pin's own edge wakes the CPU. This is
    // what a sensor DRDY/INT line, a button that must not be sampled in a
    // loop, and a wake-on-pin all need.
    //
    //     board::user_button().on_active(+[](void*) { g_pressed = true; });
    //
    // `fn` runs in INTERRUPT context — set a flag, wake a task, start the next
    // transfer; nothing that blocks.
    //
    // Every method is `requires`-gated on the backing pin driver actually
    // having the hook, exactly like spi::handle::transfer_async and
    // dma::channel::on_complete. That gate is the portability contract: on a
    // pin whose chip has no curated interrupt controller these methods do not
    // exist, `null_input` never had them at all, and portable code detects the
    // gap with `if constexpr (requires { b.on_edge(...); })` instead of failing
    // to compile. There is deliberately NO `board::caps::pin_irq` — a cap can
    // disagree with the gate, and that is a new way to lie.
    void on_edge(edge e, void (*fn)(void*), void* ctx = nullptr) const
        requires requires { impl::enable_edge_irq(edge::rising, nullptr, nullptr); }
    {
        impl::enable_edge_irq(e, fn, ctx);
    }

    // Portable form: the edge comes from the BOARD's declared polarity, which
    // is the entire reason boards record `active`. A button wired active-low
    // becomes a falling edge here and a rising edge on a board that wires it
    // the other way, without the app knowing either.
    void on_active(void (*fn)(void*), void* ctx = nullptr) const
        requires requires { impl::enable_edge_irq(edge::rising, nullptr, nullptr); }
    {
        impl::enable_edge_irq(inverted ? edge::falling : edge::rising, fn, ctx);
    }

    /// Stop reporting edges from this pin. The pin itself is untouched and
    /// is_high()/is_active() keep working.
    void clear_on_edge() const
        requires requires { impl::disable_edge_irq(); }
    {
        impl::disable_edge_irq();
    }

    /// Edges counted since on_edge/on_active armed the pin. A SOFTWARE counter
    /// kept by the handler, never the hardware pending bit — so reading it can
    /// never eat an interrupt, and it is monotonic, so a caller that was busy
    /// can see that it MISSED edges rather than being told there was one.
    [[nodiscard]] std::uint32_t edges() const
        requires requires { impl::edge_count(); }
    {
        return impl::edge_count();
    }

    /// Consume one counted edge; false once the caller has caught up. The
    /// superloop form: `while (btn.take_edge()) { ... }` handles a burst
    /// exactly as many times as it happened.
    [[nodiscard]] bool take_edge() const
        requires requires { impl::take_edge(); }
    {
        return impl::take_edge();
    }
};

// No-op stand-in for an OPTIONAL input role (a button) on boards that don't
// wire one — mirrors null_output so a generated board::user_button() accessor
// compiles everywhere and reads as "never pressed".
struct null_input {
    static void init() {}
    static void init_pullup() {}
    [[nodiscard]] bool is_high() const { return false; }
    [[nodiscard]] bool is_active() const { return false; }
};

// ── Multi-pin bus (GpioSet): drive/read several pins on ONE port together ─────
//
// The value's bit i maps to the i-th pin. write() is a SINGLE atomic port store
// (one BSRR on ST), so every pin changes on the same clock edge — no glitch
// where a parallel bus, LED bar, or stepper phase is briefly half-updated.
//
//   using Bus = alloy::gpio::bus<dev::pb3_t, dev::pb4_t, dev::pb5_t>;
//   Bus::init();
//   bus.write(0b101);          // PB3=1, PB4=0, PB5=1 in one store
//   auto v = bus.read();       // gather the same three pins back into bits 0..2

namespace detail {
struct bus_masks_t {
    std::uint32_t set;
    std::uint32_t clear;
};

// Value bit i -> port pin `bits[i]`. Returns the set/clear halves for one write.
template <std::size_t N>
[[nodiscard]] constexpr bus_masks_t bus_masks(std::uint32_t value,
                                              const std::array<std::uint8_t, N>& bits) {
    bus_masks_t m{0u, 0u};
    for (std::size_t i = 0; i < N; ++i) {
        if (((value >> i) & 1u) != 0u) {
            m.set |= std::uint32_t{1} << bits[i];
        } else {
            m.clear |= std::uint32_t{1} << bits[i];
        }
    }
    return m;
}

// Inverse: gather port pins `bits[i]` back into value bit i.
template <std::size_t N>
[[nodiscard]] constexpr std::uint32_t bus_gather(std::uint32_t port,
                                                 const std::array<std::uint8_t, N>& bits) {
    std::uint32_t value = 0u;
    for (std::size_t i = 0; i < N; ++i) {
        value |= ((port >> bits[i]) & 1u) << i;
    }
    return value;
}
}  // namespace detail

template <class... Pins>
class bus {
    static_assert(sizeof...(Pins) >= 1, "gpio::bus needs at least one pin");
    using first = std::tuple_element_t<0, std::tuple<Pins...>>;
    static_assert((std::same_as<typename Pins::port_t, typename first::port_t> && ...),
                  "gpio::bus pins must all be on the same GPIO port (one atomic store)");

    static constexpr std::array<std::uint8_t, sizeof...(Pins)> bits_{
        static_cast<std::uint8_t>(Pins::index)...};

public:
    static constexpr std::size_t width = sizeof...(Pins);

    static void init() { (hal::pin_impl<Pins>::make_output(), ...); }

    void write(std::uint32_t value) const {
        const auto m = detail::bus_masks(value, bits_);
        hal::pin_impl<first>::write_masked(m.set, m.clear);
    }
    [[nodiscard]] std::uint32_t read() const {
        return detail::bus_gather(hal::pin_impl<first>::read_port(), bits_);
    }
};

// No-op stand-in for boards without a gpio_bus role.
struct null_bus {
    static constexpr std::size_t width = 0;
    static void init() {}
    void write(std::uint32_t) const {}
    [[nodiscard]] std::uint32_t read() const { return 0u; }
};

}  // namespace alloy::gpio
