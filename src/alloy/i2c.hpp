// User-facing I2C: typed pin binding with compile-time route checking,
// move-only handle satisfying the I2cBus concept.
//
//   auto bus = board::i2c::open({.speed_hz = 400'000});
//   if (!bus.probe(0x48)) { /* nothing ACKed */ }
//
// Errors are bool (false = NACK / bus error) — the honest v1 contract.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/hal/i2c/i2c_impl.hpp"

namespace alloy::i2c {

struct config {
    std::uint32_t speed_hz = 100'000;
};

template <class Pin>
struct scl {
    using pin = Pin;
};
template <class Pin>
struct sda {
    using pin = Pin;
};

template <class Inst>
class handle {
public:
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

private:
    template <class, class, class, class>
    friend struct bind;
    handle() = default;
};

template <class Inst, class Scl, class Sda, class Clock>
struct bind {
    using scl_pin = typename Scl::pin;
    using sda_pin = typename Sda::pin;

    static_assert(routes::routable<scl_pin, Inst, signal::scl>,
                  "SCL pin has no route to this I2C on the selected chip "
                  "(check the chip's route table in alloy-devices)");
    static_assert(routes::routable<sda_pin, Inst, signal::sda>,
                  "SDA pin has no route to this I2C on the selected chip");

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    template <class Route, class Pin>
    static void route_pin() {
        constexpr std::uint8_t mux =
            (Route::k == routes::kind::af_fixed) ? Route::af : Route::funcsel;
        // I2C pads must be open-drain where the mux doesn't imply it (ST);
        // drivers that need it expose make_af_od, others fall back.
        if constexpr (requires { hal::pin_impl<Pin>::make_af_od(mux); }) {
            hal::pin_impl<Pin>::make_af_od(mux);
        } else {
            hal::pin_impl<Pin>::make_af(mux);
        }
    }

    static handle<Inst> open(config c = {}) {
        using scl_route = routes::route<scl_pin, Inst, signal::scl>;
        using sda_route = routes::route<sda_pin, Inst, signal::sda>;
        route_pin<scl_route, scl_pin>();
        route_pin<sda_route, sda_pin>();
        hal::i2c_impl<Inst>::enable(kernel_hz(), c.speed_hz);
        return handle<Inst>{};
    }
};

}  // namespace alloy::i2c
