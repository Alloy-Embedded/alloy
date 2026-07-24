// User-facing GPIO wrappers. Stateless, zero-size, satisfy the OutputPin /
// InputPin concepts. Generated boards expose roles as inline constexpr
// instances of these types.

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include "alloy/hal/gpio/pin_impl.hpp"

namespace alloy::gpio {

struct active_high_t {};
struct active_low_t {};

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
            m.set |= 1u << bits[i];
        } else {
            m.clear |= 1u << bits[i];
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
