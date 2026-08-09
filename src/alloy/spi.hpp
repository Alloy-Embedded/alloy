// User-facing SPI: typed pin binding, move-only handle satisfying SpiBus.
// Chip-selects are plain gpio outputs owned by the caller (the shared
// SpiDevice layer arrives later, embedded-hal style).

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
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

private:
    // Iteration cap for wait_transfer(), the same idiom (and reasoning) as the
    // ST I2C driver's kPollBudget: far above any legal transfer's latency, so
    // it only ever fires on a genuine fault.
    static constexpr std::uint32_t kWaitBudget = 4'000'000u;

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

    static handle<Inst> open(config c = {}) {
        // Per INSTANCE, cross-TU (alloy/core/claim.hpp), not per binder type.
        alloy::claim::exclusive<Inst, alloy::claim::personality::spi>();
        detail::admit_clock(c.clock_hz, kernel_hz());
        route_pin<typename Sck::pin, signal::sck>();
        route_pin<typename Miso::pin, signal::miso>();
        route_pin<typename Mosi::pin, signal::mosi>();
        hal::spi_impl<Inst>::enable(kernel_hz(), c.clock_hz, c.mode);
        return handle<Inst>{};
    }
};

}  // namespace alloy::spi
