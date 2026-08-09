// Concurrency probe — the firmware that PRODUCES the numbers in
// docs/guide/async.md. Every figure alloy publishes about its concurrency model
// comes from a run of this program. None are estimated, none are read off a
// datasheet, and the ones that CANNOT be measured this way are not printed.
//
// THE RULER. The instrument is the SysTick counter at its native resolution
// (arch/cortex_m/systick.hpp): uptime_us() resolves to 1 µs, which is 16 core
// clocks on this board — coarser than the things being timed. Even so a single
// count is coarse under emulation, so the probe calibrates itself against a
// block of exactly 1000 straight-line `nop`s and reports every software cost in
// INSTRUCTION-EQUIVALENTS (ieq): the measured interval divided by the cost of
// one nop. On silicon one ieq is one instruction issue; under Renode it is the
// fixed slice of virtual time the interpreter charges per instruction. Raw
// counts are printed alongside so the derivation can be checked.
//
// WHAT IT MEASURES (all in-band — the firmware prints its own verdicts, so the
// emulation leg gates on this program's arithmetic, not on the emulator's wall
// clock):
//
//   cal    the ruler: cost of one stamp, and of 1000 straight-line nops.
//   irq    from the store that raises an interrupt to the first instruction of
//          the handler — for a strong vendor-style handler (raw), for one
//          attached through alloy::irq (alloy), and for a second handler
//          sharing the line (chain2). The DIFFERENCES are alloy's dispatch
//          cost; the absolute value also contains the architectural exception
//          entry, which an interpreter does not model (see the guide).
//   mask   what an interrupts-off critical section adds to that latency, empty
//          and 100 iterations long. This is the ONLY thing in alloy that
//          delays an ISR.
//   sched  alloy::scheduler (the non-async cooperative table): one run_once()
//          superstep against 1..16 runnable tasks.
//   exec   alloy::async::executor: an empty superstep, and one wake+resume of a
//          coroutine parked on an event, against 1..8 tasks.
//   parked what a SUSPENDED task costs per superstep — nothing when it waits
//          on an event, a timer-list walk when it waits on delay().
//   jitter the doctrine, measured: how late a `co_await delay(2ms)` actually
//          wakes — alone, and beside a task that runs 5 ms without yielding.
//          Reported in µs of the firmware's OWN timebase, so it is a property
//          of the scheduling model rather than of the emulator's speed.
//
// Cortex-M ONLY (SysTick + NVIC + the arch stamp), which is why it is not in
// the all-boards build matrix.
#include <chrono>
#include <cstddef>
#include <cstdint>

#include <alloy/arch/cortex_m/nvic.hpp>
#include <alloy/arch/cortex_m/systick.hpp>
#include <alloy/arch/irq.hpp>
#include <alloy/async/delay.hpp>
#include <alloy/async/event.hpp>
#include <alloy/async/executor.hpp>
#include <alloy/async/task.hpp>
#include <alloy/board.hpp>
#include <alloy/irq.hpp>
#include <alloy/sched.hpp>
#include <alloy/time.hpp>

using namespace alloy::literals;
using alloy::arch::cortex_m::systick_elapsed;
using alloy::arch::cortex_m::systick_period;
using alloy::arch::cortex_m::systick_since;
namespace nvic = alloy::arch::cortex_m::nvic;
namespace async = alloy::async;

namespace {

// Two IRQ lines with no peripheral behind them on this board, raised in
// software. Pending a line the NVIC owns is the same entry sequence a
// peripheral edge takes AFTER the peripheral has set its own pending bit; what
// it excludes is the peripheral's own detection/synchronisation delay, which is
// a per-IP datasheet number and not alloy's to claim.
constexpr unsigned kLineAlloy = 21;  // TIM16 on STM32G071 — unused here
constexpr unsigned kLineRaw = 22;    // TIM17 — ditto

// Samples per latency figure. Each sample is quantised to a whole SysTick
// count, so the MEAN over many samples is what carries resolution; min and max
// are printed too and are honestly coarse.
constexpr unsigned kLatencySamples = 1024;

volatile std::uint32_t g_isr_stamp = 0;
volatile std::uint32_t g_isr_seq = 0;
volatile std::uint32_t g_chain_hits = 0;
bool g_irq_timeout = false;

// The wait for the handler is BOUNDED. The raw leg needs a strong handler whose
// name is this chip's vector 22 (TIM17 on STM32G071; CAN1_SCE on an STM32F722,
// where the strong symbol would be an orphan and the interrupt would never
// reach a stamp). Without the bound that mistake is an infinite spin, which
// under emulation reads as a hung leg rather than a failure. The guard costs
// the measurement nothing: the stamp is taken INSIDE the handler, so this loop
// runs entirely after the measured interval has closed.
// 1000 is generous: in a working run the handler has already stamped before
// the first check, so this loop normally executes zero times.
constexpr std::uint32_t kIsrWaitBound = 1000u;

// Counts charged for 1000 straight-line nops — the conversion to ieq.
std::uint32_t g_nop1000 = 1;

// Attached through alloy::irq — reached via the generated weak wrapper ->
// alloy_irq_dispatch(n) -> the per-line handler chain.
void alloy_isr(void*) {
    g_isr_stamp = systick_elapsed();
    g_isr_seq = g_isr_seq + 1u;  // volatile: ++ is deprecated in C++20
}

// A second handler on the SAME line, so the chain-walk cost is visible.
void alloy_isr_second(void*) { g_chain_hits = g_chain_hits + 1u; }

// Exactly 1000 instructions, straight line — the calibration block. It has to be
// its own noinline function: 2 KB of nops inlined into main() puts Thumb-1
// literal pools and conditional branches out of range and the ASSEMBLER refuses
// the file. The bl/bx around it are the only untracked instructions, 2 in 1000.
[[gnu::noinline]] void nop1000_block() {
    __asm volatile(".rept 1000\n\tnop\n\t.endr" ::: "memory");
}

// Instruction-equivalents (x100) for an interval given in counts (x100).
std::uint32_t to_ieq(std::uint32_t counts_x100) {
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(counts_x100) * 1000u /
                                      g_nop1000);
}

struct stats {
    std::uint32_t min = 0xFFFF'FFFFu;
    std::uint32_t max = 0;
    std::uint32_t sum = 0;
    std::uint32_t n = 0;

    void add(std::uint32_t v) {
        if (v < min) {
            min = v;
        }
        if (v > max) {
            max = v;
        }
        sum += v;
        ++n;
    }
    // Mean in hundredths of a count. Integer-only: no soft-float on the wire.
    [[nodiscard]] std::uint32_t mean_x100() const { return n == 0 ? 0 : sum * 100u / n; }
};

template <class Uart>
void wu(const Uart& uart, std::uint32_t v) {
    char buf[10];
    unsigned n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10u);
        v /= 10u;
    } while (v != 0u);
    while (n != 0u) {
        uart.write(static_cast<std::uint8_t>(buf[--n]));
    }
}

// A value carried as hundredths, printed as N.MM.
template <class Uart>
void wfix(const Uart& uart, std::uint32_t x100) {
    wu(uart, x100 / 100u);
    uart.write(".");
    const std::uint32_t frac = x100 % 100u;
    if (frac < 10u) {
        uart.write("0");
    }
    wu(uart, frac);
}

template <class Uart>
void report(const Uart& uart, const char* label, const stats& s) {
    uart.write(label);
    uart.write(" n=");
    wu(uart, s.n);
    uart.write(" ieq=");
    wfix(uart, to_ieq(s.mean_x100()));
    uart.write(" counts=");
    wfix(uart, s.mean_x100());
    uart.write(" min=");
    wu(uart, s.n == 0 ? 0u : s.min);  // an empty stats' sentinel min is not a datum
    uart.write(" max=");
    wu(uart, s.max);
    uart.write("\r\n");
}

// A software cost measured as a batch: report it in ieq and in counts.
template <class Uart>
void report_cost(const Uart& uart, const char* label, std::uint32_t counts_x100) {
    uart.write(label);
    uart.write(" ieq=");
    wfix(uart, to_ieq(counts_x100));
    uart.write(" counts=");
    wfix(uart, counts_x100);
    uart.write("\r\n");
}

// --- irq -------------------------------------------------------------------

// One sample: stamp, raise the line, read the handler's own stamp. Both stamps
// and the NVIC store are volatile accesses, so the compiler may not reorder
// them past each other.
std::uint32_t irq_sample(unsigned line) {
    std::uint32_t guard = 0;  // hoisted: nothing but the store may sit in the window
    const std::uint32_t seq = g_isr_seq;
    const std::uint32_t t0 = systick_elapsed();
    nvic::set_pending(line);
    while (g_isr_seq == seq) {
        if (++guard > kIsrWaitBound) {
            g_irq_timeout = true;
            return 0;
        }
    }
    return systick_since(t0, g_isr_stamp);
}

// The same, but the line is raised with interrupts masked: the exception is
// held pending and taken the instant the mask drops, so the measured latency is
// entry PLUS however long the critical section ran. `pad` sets its length.
std::uint32_t irq_sample_masked(unsigned line, unsigned pad) {
    std::uint32_t guard = 0;
    const std::uint32_t seq = g_isr_seq;
    const std::uint32_t t0 = systick_elapsed();
    {
        const alloy::arch::irq_state s = alloy::arch::irq_save();
        nvic::set_pending(line);
        for (unsigned k = 0; k < pad; ++k) {
            __asm volatile("nop");
        }
        alloy::arch::irq_restore(s);
    }
    while (g_isr_seq == seq) {
        if (++guard > kIsrWaitBound) {
            g_irq_timeout = true;
            return 0;
        }
    }
    return systick_since(t0, g_isr_stamp);
}

// --- alloy::scheduler ------------------------------------------------------

void noop_task(void*, std::uint32_t) {}

// Microseconds for `iters` supersteps of an N-task table, with the cost of
// signalling those tasks measured separately and subtracted. Event-driven tasks
// (period 0), so the result is dispatch cost and not timer polling.
template <std::size_t N>
std::uint32_t sched_batch_us(unsigned iters) {
    alloy::scheduler<N> sc;
    for (std::size_t i = 0; i < N; ++i) {
        sc.add(&noop_task, nullptr, std::chrono::milliseconds{0});
    }
    const std::uint32_t b0 = alloy::uptime_us();
    for (unsigned it = 0; it < iters; ++it) {
        for (std::size_t i = 0; i < N; ++i) {
            sc.signal(i, 1);
        }
    }
    const std::uint32_t base = alloy::uptime_us() - b0;

    const std::uint32_t t0 = alloy::uptime_us();
    for (unsigned it = 0; it < iters; ++it) {
        for (std::size_t i = 0; i < N; ++i) {
            sc.signal(i, 1);
        }
        sc.run_once(0);
    }
    const std::uint32_t total = alloy::uptime_us() - t0;
    return total > base ? total - base : 0u;
}

// --- alloy::async::executor ------------------------------------------------

constexpr std::size_t kMaxAsync = 8;

async::executor<16> ex;
async::event ev[kMaxAsync];
async::task_storage<256> park_frame[kMaxAsync];
async::task_storage<256> sleeper_frame[kMaxAsync];
async::task_storage<256> jitter_frame;
async::task_storage<256> hog_frame;

// Parks forever on its event: the shape of every interrupt-driven driver
// awaitable in alloy (uart/spi/i2c/dma all reduce to this).
async::task park(async::task_storage<256>&, async::event& e) {
    for (;;) {
        co_await e;
    }
}

// Parks on a TIMER instead. The two are not the same to the executor: an
// event-parked task is in no list at all, while a sleeping one holds a
// timer_node that run_once() walks on EVERY superstep before it looks at the
// ready queue. Measured separately because that difference is a per-superstep
// cost the doctrine has to state.
async::task sleeper(async::task_storage<256>&) {
    for (;;) {
        co_await async::delay(60000ms);
    }
}

volatile bool g_jitter_done = false;
volatile bool g_hog_stop = false;
std::uint32_t g_late_min = 0xFFFF'FFFFu;
std::uint32_t g_late_max = 0;

// Asks for 2 ms, records what it actually got. `delay` is built on uptime_ms(),
// so even unloaded the answer is quantised to the 1 kHz tick.
async::task periodic(async::task_storage<256>&, unsigned samples) {
    for (unsigned i = 0; i < samples; ++i) {
        const std::uint32_t t0 = alloy::uptime_us();
        co_await async::delay(2ms);
        const std::uint32_t dt = alloy::uptime_us() - t0;
        if (dt < g_late_min) {
            g_late_min = dt;
        }
        if (dt > g_late_max) {
            g_late_max = dt;
        }
    }
    g_jitter_done = true;
}

// A well-behaved-LOOKING task that nevertheless runs 5 ms straight without a
// suspension point. In a cooperative executor that is the whole story.
async::task hog(async::task_storage<256>&, std::uint32_t spin_us) {
    while (!g_hog_stop) {
        const std::uint32_t t0 = alloy::uptime_us();
        while (alloy::uptime_us() - t0 < spin_us) {
        }
        co_await async::delay(1ms);
    }
}

// Drive supersteps until the periodic task finishes, with a wall-clock escape so
// a broken run fails the leg instead of hanging it.
void pump_until_done(std::uint32_t budget_ms) {
    const std::uint32_t start = alloy::uptime_ms();
    while (!g_jitter_done && alloy::uptime_ms() - start < budget_ms) {
        ex.run_once();
    }
}

}  // namespace

// The vendor-style expert path: a STRONG handler overrides alloy's weak wrapper
// entirely, so this measures the exception path with no framework in the way.
// The difference between `irq raw` and `irq alloy` is what alloy's dispatch
// costs — stated rather than hidden.
extern "C" void TIM17_IRQHandler(void) {  // NOLINT: strong override, by design
    g_isr_stamp = systick_elapsed();
    g_isr_seq = g_isr_seq + 1u;
}

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy concurrency_probe\r\n");

    const std::uint32_t period = systick_period();
    uart.write("period=");
    wu(uart, period);
    uart.write(" core_hz=");
    wu(uart, period * 1000u);
    uart.write("\r\n");

    // counts per microsecond, for turning the batch (µs) numbers into counts.
    const std::uint32_t cpu_per_us = period / 1000u;

    // --- cal: the ruler ----------------------------------------------------
    // Exactly 1000 instructions, straight line, no loop overhead to guess at.
    // Best-of-8 so a tick landing mid-block cannot inflate the constant.
    for (unsigned i = 0; i < 8; ++i) {
        const std::uint32_t a = systick_elapsed();
        nop1000_block();
        const std::uint32_t b = systick_elapsed();
        const std::uint32_t d = systick_since(a, b);
        if (i == 0 || d < g_nop1000) {
            g_nop1000 = d;
        }
    }
    if (g_nop1000 == 0u) {
        g_nop1000 = 1u;  // never divide by zero; a 0 here means "unmeasurable"
    }
    uart.write("cal nop1000=");
    wu(uart, g_nop1000);
    uart.write(" counts, so 1 count = ");
    wfix(uart, 100u * 1000u / g_nop1000);
    uart.write(" ieq\r\n");

    stats stamp_pair;
    for (unsigned i = 0; i < 256; ++i) {
        const std::uint32_t a = systick_elapsed();
        const std::uint32_t b = systick_elapsed();
        stamp_pair.add(systick_since(a, b));
    }
    report(uart, "cal stamp", stamp_pair);

    // --- irq ---------------------------------------------------------------
    // Board-pinned: vectors 21/22 must be TIM16/TIM17, which is an STM32G071
    // fact. Printed so a reader of the CI log knows what the numbers describe.
    uart.write("irq lines: 21=TIM16 (alloy) 22=TIM17 (strong) on nucleo_g071rb\r\n");
    alloy::irq::enable(alloy::irq_line{kLineRaw});
    stats raw;
    // Bail on the first un-vectored sample: 1024 timeouts is 1024 wasted guard
    // loops, and the `irq vectored` line already reports the failure.
    for (unsigned i = 0; i < kLatencySamples && !g_irq_timeout; ++i) {
        raw.add(irq_sample(kLineRaw));
    }
    report(uart, "irq raw", raw);

    alloy::irq::attach(alloy::irq_line{kLineAlloy}, &alloy_isr);
    alloy::irq::enable(alloy::irq_line{kLineAlloy});
    stats via_alloy;
    for (unsigned i = 0; i < kLatencySamples && !g_irq_timeout; ++i) {
        via_alloy.add(irq_sample(kLineAlloy));
    }
    report(uart, "irq alloy", via_alloy);

    alloy::irq::attach(alloy::irq_line{kLineAlloy}, &alloy_isr_second);
    stats chained;
    for (unsigned i = 0; i < kLatencySamples && !g_irq_timeout; ++i) {
        chained.add(irq_sample(kLineAlloy));
    }
    report(uart, "irq chain2", chained);
    // `> 0` matters: with no samples taken (the un-vectored case) 0 == 0 would
    // read as a pass from a measurement that never ran.
    uart.write(chained.n > 0 && g_chain_hits == chained.n ? "irq chain2 ran: ok\r\n"
                                                          : "irq chain2 ran: FAIL\r\n");
    alloy::irq::detach(alloy::irq_line{kLineAlloy}, &alloy_isr_second);

    // --- mask --------------------------------------------------------------
    stats masked0;
    stats masked100;
    for (unsigned i = 0; i < kLatencySamples && !g_irq_timeout; ++i) {
        masked0.add(irq_sample_masked(kLineAlloy, 0));
    }
    for (unsigned i = 0; i < kLatencySamples && !g_irq_timeout; ++i) {
        masked100.add(irq_sample_masked(kLineAlloy, 100));
    }
    report(uart, "mask pad0", masked0);
    report(uart, "mask pad100", masked100);
    uart.write(g_irq_timeout ? "irq vectored: FAIL (rebuild for nucleo_g071rb)\r\n"
                             : "irq vectored: ok\r\n");

    // --- sched -------------------------------------------------------------
    constexpr unsigned kSchedIters = 512;
    const std::uint32_t sched_us[5] = {
        sched_batch_us<1>(kSchedIters), sched_batch_us<2>(kSchedIters),
        sched_batch_us<4>(kSchedIters), sched_batch_us<8>(kSchedIters),
        sched_batch_us<16>(kSchedIters),
    };
    const std::uint32_t sched_n[5] = {1, 2, 4, 8, 16};
    for (unsigned i = 0; i < 5; ++i) {
        const std::uint32_t counts_x100 = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(sched_us[i]) * cpu_per_us * 100u / kSchedIters);
        uart.write("sched n=");
        wu(uart, sched_n[i]);
        uart.write(" superstep_ieq=");
        wfix(uart, to_ieq(counts_x100));
        uart.write(" per_task_ieq=");
        wfix(uart, to_ieq(counts_x100 / sched_n[i]));
        uart.write("\r\n");
    }

    // --- exec --------------------------------------------------------------
    for (std::size_t i = 0; i < kMaxAsync; ++i) {
        ex.spawn(park(park_frame[i], ev[i]));
    }
    ex.run_once();  // let every task reach its first co_await

    constexpr unsigned kExecIters = 512;
    {
        const std::uint32_t t0 = alloy::uptime_us();
        for (unsigned it = 0; it < kExecIters; ++it) {
            ex.run_once();
        }
        const std::uint32_t us = alloy::uptime_us() - t0;
        report_cost(uart, "exec empty",
                    static_cast<std::uint32_t>(static_cast<std::uint64_t>(us) *
                                               cpu_per_us * 100u / kExecIters));
    }
    const std::uint32_t exec_n[4] = {1, 2, 4, 8};
    for (unsigned k = 0; k < 4; ++k) {
        const std::uint32_t n = exec_n[k];
        const std::uint32_t t0 = alloy::uptime_us();
        for (unsigned it = 0; it < kExecIters; ++it) {
            for (std::uint32_t i = 0; i < n; ++i) {
                ev[i].set();
            }
            ex.run_once();
        }
        const std::uint32_t us = alloy::uptime_us() - t0;
        const std::uint32_t counts_x100 = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(us) * cpu_per_us * 100u / kExecIters);
        uart.write("exec n=");
        wu(uart, n);
        uart.write(" superstep_ieq=");
        wfix(uart, to_ieq(counts_x100));
        uart.write(" per_task_ieq=");
        wfix(uart, to_ieq(counts_x100 / n));
        uart.write("\r\n");
    }

    // --- jitter ------------------------------------------------------------
    g_late_min = 0xFFFF'FFFFu;
    g_late_max = 0;
    g_jitter_done = false;
    ex.spawn(periodic(jitter_frame, 16));
    pump_until_done(2000u);
    const std::uint32_t idle_max = g_late_max;
    uart.write("jitter idle req=2000 min=");
    wu(uart, g_late_min);
    uart.write(" max=");
    wu(uart, idle_max);
    uart.write("\r\n");

    g_late_min = 0xFFFF'FFFFu;
    g_late_max = 0;
    g_jitter_done = false;
    g_hog_stop = false;
    ex.spawn(hog(hog_frame, 5000u));
    ex.spawn(periodic(jitter_frame, 16));
    pump_until_done(4000u);
    g_hog_stop = true;
    const std::uint32_t hog_max = g_late_max;
    uart.write("jitter hog5000 req=2000 min=");
    wu(uart, g_late_min);
    uart.write(" max=");
    wu(uart, hog_max);
    uart.write("\r\n");

    // --- parked ------------------------------------------------------------
    // What a SUSPENDED task costs per superstep. "Parked" is not one thing:
    // a task waiting on an event is in no list the executor owns, but a task
    // inside `co_await delay(...)` holds a timer_node that run_once() walks
    // before it touches the ready queue. Measured LAST so nothing above is
    // perturbed by the tasks spawned here.
    // Let the hog retire first — it is still sleeping on delay(1ms) and its own
    // timer node would land in the baseline.
    {
        const std::uint32_t t = alloy::uptime_ms();
        while (alloy::uptime_ms() - t < 5u) {
            ex.run_once();
        }
    }
    auto empty_poll_x100 = [&]() {
        const std::uint32_t t0 = alloy::uptime_us();
        for (unsigned it = 0; it < kExecIters; ++it) {
            ex.run_once();
        }
        const std::uint32_t us = alloy::uptime_us() - t0;
        return static_cast<std::uint32_t>(static_cast<std::uint64_t>(us) * cpu_per_us *
                                          100u / kExecIters);
    };
    const std::uint32_t parked_events = empty_poll_x100();
    report_cost(uart, "parked events=8 timers=0", parked_events);
    for (std::size_t i = 0; i < kMaxAsync; ++i) {
        ex.spawn(sleeper(sleeper_frame[i]));
    }
    ex.run_once();  // every sleeper reaches its first co_await and arms a timer
    const std::uint32_t parked_timers = empty_poll_x100();
    report_cost(uart, "parked events=8 timers=8", parked_timers);

    // --- in-band verdicts --------------------------------------------------
    // The emulation leg gates on THESE, not on the raw figures: a number that
    // moves with the emulator's speed is not a regression, but a broken
    // ordering property is. Each verdict is a claim docs/guide/async.md makes.
    //
    //  1. alloy's dispatch is a real but bounded cost on top of the raw vector,
    //     and a second handler on the line costs more again.
    //  2. an interrupts-off section delays the ISR by about its own length, and
    //     an EMPTY one costs almost nothing.
    //  3. the executor does NOT preempt: a 5 ms non-yielding task pushes a 2 ms
    //     delay out past 4 ms, while unloaded it lands within one extra tick.
    //  4. a task parked on an EVENT costs nothing per superstep, but a task
    //     parked on a TIMER does — run_once() walks the timer list every time.
    //     The guide used to claim both were free; this verdict is why it no
    //     longer does.
    // !g_irq_timeout matters: without it a raw leg that never vectored reads as
    // 0 and would SATISFY "alloy costs more than raw" — a green verdict from a
    // measurement that did not happen. (Seen for real on an STM32F722, where
    // vector 22 is CAN1_SCE.)
    const bool v_dispatch = !g_irq_timeout && via_alloy.mean_x100() > raw.mean_x100() &&
                            chained.mean_x100() > via_alloy.mean_x100();
    const bool v_mask = masked100.mean_x100() > masked0.mean_x100() * 2u;
    const bool v_cooperative = idle_max < 4000u && hog_max >= 4000u;
    // `parked_events > 0` guards the same vacuity the irq legs needed: if the
    // baseline never measured, 0 < anything would read as a pass.
    const bool v_parked = parked_events > 0u && parked_timers > parked_events * 2u;
    uart.write(v_dispatch ? "verdict dispatch: ok\r\n" : "verdict dispatch: FAIL\r\n");
    uart.write(v_mask ? "verdict mask: ok\r\n" : "verdict mask: FAIL\r\n");
    uart.write(v_cooperative ? "verdict cooperative: ok\r\n"
                             : "verdict cooperative: FAIL\r\n");
    uart.write(v_parked ? "verdict parked: ok\r\n" : "verdict parked: FAIL\r\n");
    uart.write("concurrency_probe done\r\n");

    for (;;) {
        alloy::sleep_for(1s);
    }
}
