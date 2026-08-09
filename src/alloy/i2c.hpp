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

private:
    // Iteration cap for wait_transfer(), the same idiom (and reasoning) as the
    // driver's kPollBudget: far above any legal transfer's latency, so it only
    // ever fires on a genuine fault.
    static constexpr std::uint32_t kWaitBudget = 4'000'000u;

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

    static handle<Inst> open(config c = {}) {
        // Per INSTANCE, cross-TU (alloy/core/claim.hpp), not per binder type.
        alloy::claim::exclusive<Inst, alloy::claim::personality::i2c>();
        detail::admit_speed(c.speed_hz, kernel_hz());
        using scl_route = routes::route<scl_pin, Inst, signal::scl>;
        using sda_route = routes::route<sda_pin, Inst, signal::sda>;
        route_pin<scl_route, scl_pin>();
        route_pin<sda_route, sda_pin>();
        hal::i2c_impl<Inst>::enable(kernel_hz(), c.speed_hz);
        return handle<Inst>{};
    }
};

}  // namespace alloy::i2c
