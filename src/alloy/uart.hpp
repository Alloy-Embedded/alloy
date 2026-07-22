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

#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/hal/uart/uart_impl.hpp"

namespace alloy::uart {

struct config {
    std::uint32_t baud = 115'200;
};

template <class Pin>
struct tx {
    using pin = Pin;
};
template <class Pin>
struct rx {
    using pin = Pin;
};

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

private:
    template <class, class, class, class>
    friend struct bind;
    handle() = default;
};

template <class Inst, class Tx, class Rx, class Clock>
struct bind {
    using tx_pin = typename Tx::pin;
    using rx_pin = typename Rx::pin;

    static_assert(routes::routable<tx_pin, Inst, signal::tx>,
                  "TX pin has no route to this UART on the selected chip "
                  "(check the chip's route table in alloy-devices)");
    static_assert(routes::routable<rx_pin, Inst, signal::rx>,
                  "RX pin has no route to this UART on the selected chip "
                  "(check the chip's route table in alloy-devices)");

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    static handle<Inst> open(config c) {
        using tx_route = routes::route<tx_pin, Inst, signal::tx>;
        using rx_route = routes::route<rx_pin, Inst, signal::rx>;
        static_assert(tx_route::k == routes::kind::af_fixed &&
                          rx_route::k == routes::kind::af_fixed,
                      "walking skeleton implements af_fixed routing only");

        if (detail_opened) {
            __builtin_trap();  // double-open: honest runtime guard
        }
        detail_opened = true;

        hal::pin_impl<tx_pin>::make_af(tx_route::af);
        hal::pin_impl<rx_pin>::make_af(rx_route::af);
        hal::uart_impl<Inst>::enable(kernel_hz(), c.baud);
        return handle<Inst>{};
    }

private:
    inline static bool detail_opened = false;
};

}  // namespace alloy::uart
