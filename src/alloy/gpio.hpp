// User-facing GPIO wrappers. Stateless, zero-size, satisfy the OutputPin /
// InputPin concepts. Generated boards expose roles as inline constexpr
// instances of these types.

#pragma once

#include <concepts>

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

}  // namespace alloy::gpio
