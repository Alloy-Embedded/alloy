// User-facing SPI: typed pin binding, move-only handle satisfying SpiBus.
// Chip-selects are plain gpio outputs owned by the caller (the shared
// SpiDevice layer arrives later, embedded-hal style).

#pragma once

#include <cstdint>
#include <span>

#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/hal/spi/spi_impl.hpp"

namespace alloy::spi {

struct config {
    std::uint32_t clock_hz = 1'000'000;
    std::uint8_t mode = 0;  // CPOL<<1 | CPHA
};

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

template <class Inst>
class handle {
public:
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

private:
    template <class, class, class, class, class>
    friend struct bind;
    handle() = default;
};

template <class Inst, class Sck, class Miso, class Mosi, class Clock>
struct bind {
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
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    template <class Pin, alloy::signal S>
    static void route_pin() {
        using R = routes::route<Pin, Inst, S>;
        hal::pin_impl<Pin>::make_af(routes::mux_value<R>());
    }

    static handle<Inst> open(config c = {}) {
        route_pin<typename Sck::pin, signal::sck>();
        route_pin<typename Miso::pin, signal::miso>();
        route_pin<typename Mosi::pin, signal::mosi>();
        hal::spi_impl<Inst>::enable(kernel_hz(), c.clock_hz, c.mode);
        return handle<Inst>{};
    }
};

}  // namespace alloy::spi
