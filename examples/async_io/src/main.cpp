// Driver awaitables — the emulation oracle for `co_await` over a peripheral
// INTERRUPT. async_heartbeat proves the executor and the timer wheel run on the
// real ISA; nothing proved that a coroutine parked on a DRIVER ever resumes.
// This does, for all three interrupt-driven buses at once:
//
//     co_await m.transfer(out, in);                       // SPI
//     co_await m.write(addr, out);  co_await m.read(...);  // I2C
//     co_await w.run([&] { uart.write_dma_begin(ch, msg); });  // DMA
//
// THE OBSERVABLE IS THE RESUME COUNT, and that is the point of the design.
// `executor::run_once()` returns how many coroutine resumes it performed, and
// each leg pumps it until its task retires and prints the total:
//
//     spi await: resumes=2      1 (start, then park) + 1 (woken by the ISR)
//     i2c await: resumes=3      1 + one wake per transfer, write then read
//     dma await: resumes=2
//
// A firmware that did the same I/O WITHOUT suspending — spin on busy() and
// carry on — retires its task in a single resume and prints `resumes=1`. So the
// number is not decoration: it separates "a coroutine suspended and an
// interrupt resumed it" from "the bytes moved". The awaitables suspend
// unconditionally (the transfer is started inside await_suspend, after the park
// — see src/alloy/async/*.hpp), so the counts above are exact, not typical.
//
// Every pump is BOUNDED. An interrupt that never fires leaves the task parked
// forever, and a hung firmware is not a test failure, it is a timeout that
// looks like anything at all — so each leg gives up after a fixed number of
// supersteps and prints "STUCK", which the Renode assertion then fails on.
//
// Legs are removed at compile time on boards whose drivers have no interrupt
// hook (the `requires` inside each templated lambda), so this same main.cpp
// builds everywhere with no #ifdef.
#include <alloy/async/dma.hpp>
#include <alloy/async/executor.hpp>
#include <alloy/async/i2c.hpp>
#include <alloy/async/spi.hpp>
#include <alloy/async/task.hpp>
#include <alloy/board.hpp>
#include <alloy/dma.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

using namespace alloy::async;
using namespace alloy::literals;

namespace {

executor<4> sched;
// `alloy frame-audit` on this toolchain (GCC 14.2, -Os, nucleo_g071rb) measures
// 108 B / 144 B / 72 B — the awaiter itself (a reference plus the spans) lives in
// the frame, which is why the bus legs are bigger than a bare delay task. 256 is
// the default headroom; frame size varies by compiler and flags.
task_storage<256> spi_frame;
task_storage<256> i2c_frame;
task_storage<256> dma_frame;

// Supersteps a leg is allowed before it declares the interrupt dead. An empty
// ready queue makes run_once() cheap, so this is far above any legal transfer's
// latency and only ever runs out on a genuine fault — the same reasoning as the
// drivers' kPollBudget, and the reason a missing interrupt FAILS the test
// instead of hanging it.
constexpr std::uint32_t kSupersteps = 200'000u;

template <class Uart>
void write_hex_byte(const Uart& uart, std::uint8_t value) {
    constexpr char digits[] = "0123456789abcdef";
    uart.write(static_cast<std::uint8_t>(digits[value >> 4]));
    uart.write(static_cast<std::uint8_t>(digits[value & 0xF]));
}

// Decimal, no heap and no printf. A plain function, so it lives on the ordinary
// call stack rather than inside a coroutine frame.
template <class Uart>
void write_u32(const Uart& uart, std::uint32_t v) {
    char buf[10];
    int i = static_cast<int>(sizeof buf);
    do {
        buf[--i] = static_cast<char>('0' + (v % 10));
        v /= 10;
    } while (v != 0);
    while (i < static_cast<int>(sizeof buf)) {
        uart.write(static_cast<std::uint8_t>(buf[i++]));
    }
}

// Run the executor until this leg's task retires, then report how many times it
// was RESUMED — the evidence that it suspended on the driver and something woke
// it. Bounded; see kSupersteps.
template <class Uart, std::size_t N>
void pump(const Uart& uart, task_storage<N>& frame, const char* tag) {
    std::uint32_t resumes = 0;
    for (std::uint32_t i = 0; i < kSupersteps && frame.in_use; ++i) {
        resumes += static_cast<std::uint32_t>(sched.run_once());
    }
    uart.write(tag);
    if (frame.in_use) {
        uart.write(" await: STUCK\r\n");  // parked with no interrupt coming
        return;
    }
    uart.write(" await: resumes=");
    write_u32(uart, resumes);
    uart.write("\r\n");
}

template <class Uart, class Bus>
task spi_leg(task_storage<256>&, const Uart& uart, spi_master<Bus>& m,
             std::span<const std::uint8_t> out, std::span<std::uint8_t> in) {
    co_await m.transfer(out, in);  // one suspend for the WHOLE exchange
    uart.write("spi await: 0x");
    for (std::uint8_t b : in) {
        write_hex_byte(uart, b);
    }
    uart.write("\r\n");
}

template <class Uart, class Bus>
task i2c_leg(task_storage<256>&, const Uart& uart, i2c_master<Bus>& m, std::uint8_t addr,
             std::span<const std::uint8_t> out, std::span<std::uint8_t> in) {
    bool ok = co_await m.write(addr, out);
    if (ok) {
        ok = co_await m.read(addr, in);
    }
    uart.write("i2c await: 0x");
    for (std::uint8_t b : in) {
        write_hex_byte(uart, b);
    }
    uart.write("\r\n");
    if (!ok) {
        uart.write("i2c await: no ack\r\n");
    }
}

template <class Uart, class Chan>
task dma_leg(task_storage<256>&, const Uart& uart, dma_waiter<Chan>& w, Chan& ch,
             std::span<const std::uint8_t> msg) {
    // The line this transfer carries is itself the proof the engine moved the
    // bytes; the CPU never writes it to TDR.
    co_await w.run([&] { uart.write_dma_begin(ch, msg); });
    uart.write_dma_end(ch);
}

// The DMA source must be RAM the engine can read (.data, not flash) and must
// outlive the transfer.
std::uint8_t dma_msg[] = "dma await: sent by DMA\r\n";

template <class Dma>
concept HasDma = requires { alloy::hal::dma_impl<Dma>::enable_controller(); };

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy async_io\r\n");

    // --- SPI: one co_await for a three-byte exchange -----------------------
    if constexpr (board::caps::spi) {
        auto bus = board::spi::open({.clock_hz = 1'000'000, .mode = 0});
        // The templated lambda makes `B` dependent, so the `requires` removes
        // the leg on a board whose SPI driver has no interrupt hook instead of
        // hard-erroring in a concrete main.
        [&uart]<class B>(B& b) {
            if constexpr (requires {
                              b.transfer_async(std::span<const std::uint8_t>{},
                                               std::span<std::uint8_t>{});
                          }) {
                static const std::uint8_t out[3] = {0x11, 0x22, 0x33};
                static std::uint8_t in[3] = {};
                // Outlives the task: pump() returns only once the task retired
                // (or gave up, after which nothing resumes it again).
                spi_master<B> m{b};
                sched.spawn(spi_leg(spi_frame, uart, m, out, in));
                pump(uart, spi_frame, "spi");
            } else {
                uart.write("spi await: not available on this board\r\n");
            }
        }(bus);
    } else {
        uart.write("spi await: not available on this board\r\n");
    }

    // --- I2C: a write and a read, one co_await each ------------------------
    if constexpr (board::caps::i2c) {
        auto bus = board::i2c::open({.speed_hz = 100'000});
        [&uart]<class B>(B& b) {
            if constexpr (requires { b.write_async(0u, std::span<const std::uint8_t>{}); }) {
                static const std::uint8_t out[3] = {0xDE, 0xAD, 0xBE};
                static std::uint8_t in[3] = {};
                i2c_master<B> m{b};
                sched.spawn(i2c_leg(i2c_frame, uart, m, 0x08, out, in));
                pump(uart, i2c_frame, "i2c");
            } else {
                uart.write("i2c await: not available on this board\r\n");
            }
        }(bus);
    } else {
        uart.write("i2c await: not available on this board\r\n");
    }

    // --- DMA: a UART TX buffer moved by the engine -------------------------
    //
    // Anchor 2.3 (docs/design/dma-streams.md §2.3) where the board assigns
    // debug_uart.tx: the channel token comes from the ROUTE — the generated
    // board fact, claimed through `alloy::dma::claim(route)` — the same
    // claim machinery, the same trap on a second claimant, with the channel
    // number nowhere in this file. Portable code reaches the route by its
    // DEPENDENT name, the binder's `tx_route` (a namespace-scope
    // `board::dma::debug_uart_tx` cannot fold inside a requires-probe;
    // board-specific code would claim that constant — the same fact, one
    // emitter helper spells both). The explicit `channel<Dma, 1>::claim()`
    // stays as the escape hatch (design §1) for boards with a DMA driver but
    // no assignment; the marker line pins WHICH spelling ran, because the
    // escape hatch cannot print it.
    [&uart]<class UartBind, class Dma>(UartBind*, Dma*) {
        if constexpr (HasDma<Dma> && requires(alloy::dma::channel<Dma, 1>& c) {
                          uart.write_dma_begin(c, std::span<const std::uint8_t>{});
                          c.on_complete(nullptr, nullptr);
                      }) {
            auto run_leg = [&uart]<class Chan>(Chan& chan) {
                // Constructed BEFORE the first transfer on purpose: the
                // channel's config register may only be written while the
                // channel is disabled, so the driver folds TCIE in at setup()
                // based on whether a completion callback is registered. A
                // waiter that registered itself inside the co_await would arm
                // nothing and park forever.
                dma_waiter<Chan> w{chan};
                sched.spawn(dma_leg(dma_frame, uart, w, chan,
                                    {dma_msg, sizeof dma_msg - 1}));
                pump(uart, dma_frame, "dma");
            };
            if constexpr (requires {
                              requires !std::is_void_v<typename UartBind::tx_route>;
                          }) {
                auto chan = alloy::dma::claim(typename UartBind::tx_route{});
                uart.write("dma route: claimed board debug_uart.tx\r\n");
                run_leg(chan);
            } else {
                auto chan = alloy::dma::channel<Dma, 1>::claim();
                run_leg(chan);
            }
        } else {
            uart.write("dma await: not available on this board\r\n");
        }
    }(static_cast<board::debug_uart*>(nullptr), static_cast<board::dma_t*>(nullptr));

    uart.write("async io: done\r\n");
    for (;;) {
        alloy::sleep_for(1s);
    }
}
