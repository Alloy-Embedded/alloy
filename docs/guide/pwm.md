# PWM and motor control

Three things share one timer block and alloy gives them three different
facades, because they are three different jobs with three different ways to
get hurt:

| You want | Facade | Header |
|---|---|---|
| One channel, a duty, an LED or a heater | `alloy::pwm` | `alloy/pwm.hpp` |
| Three complementary pairs with a dead time | `alloy::bridge` | `alloy/bridge.hpp` |
| A shaft position from two phases | `alloy::encoder` | `alloy/encoder.hpp` |

Start at the top. Drop to `alloy::bridge` only when you are switching a half
bridge, because that is the facade that refuses to let you leave a dead time
unstated — and on a real inverter, an unstated dead time is a short circuit
through both transistors.

---

## Simple PWM

```cpp
#include "alloy/board.hpp"
#include "alloy/pwm.hpp"

int main() {
    board::init();
    auto pwm = board::led_pwm::open({.freq_hz = 20'000});
    pwm.set_duty(0x8000);     // half
    pwm.off();                // 0
}
```

Duty is **normalized**: `0` is off, `0xFFFF` is always on. It is not a
percentage and not a raw compare value. That is deliberate — the number of
counts the timer actually has depends on the clock and the carrier, and a
caller who writes `0x4000` for a quarter should not have to know it.

### `config`

```cpp
struct config {
    std::uint32_t freq_hz = 1'000;
    alignment     align   = alignment::edge;
    trigger       trigger = trigger::none;
};
```

`freq_hz` is **block-scoped**, not channel-scoped, and this trips people. One
timer has one prescaler and one auto-reload behind all four of its channels,
so opening CH1 at 1 kHz and CH2 at 20 kHz is a contradiction. alloy refuses
it at `open()` rather than letting the second call silently retune the first:

```cpp
auto a = board::fan1::open({.freq_hz =  1'000});
auto b = board::fan2::open({.freq_hz = 20'000});   // traps: same timer
```

Same frequency on both channels is fine and is the normal case.

### How much resolution you actually get

```cpp
auto pwm = board::led_pwm::open({.freq_hz = 20'000});
const std::uint32_t steps = pwm.period_ticks();   // counts per period
```

The prescaler is **derived** from the carrier so the counter gets as many
ticks per period as it can hold:

```
steps = kernel_hz / freq_hz        (capped at the counter's width)
```

At 64 MHz and 20 kHz that is **3200 steps** — about 11.6 bits. `set_duty()`
still takes 16 bits and scales into whatever is really there, so you never
have to do this arithmetic; `period_ticks()` is how you check it when you care.

!!! note "This used to be much worse"
    Until recently the driver pinned a 1 MHz tick, which gave a 20 kHz carrier
    **50 steps** — under six bits — while `set_duty()` still accepted a
    `uint16_t` and appeared to work. The bottom ten bits simply did nothing.
    If you have numbers from an older tree, re-measure them.

### Centre-aligned

```cpp
auto pwm = board::led_pwm::open({
    .freq_hz = 20'000,
    .align   = alloy::pwm::alignment::center,
});
```

The counter walks up and back down inside one period instead of sawtoothing.
For a single LED it changes nothing you can see. It matters when several
channels of one timer feed one load: their edges stop coinciding, so the
current ripple and the conducted noise drop.

**It halves your resolution** at a given carrier — 1600 steps instead of 3200
at 64 MHz / 20 kHz — because one period is now two counter traversals. That
is the trade, and it is yours to make; alloy does not pick it for you.

### Trigger output — sampling where you meant to

```cpp
auto pwm = board::led_pwm::open({
    .freq_hz = 20'000,
    .trigger = alloy::pwm::trigger::on_update,
});
```

This publishes the timer's own event so a converter can be started by the
counter instead of by the CPU. The difference is between sampling at a chosen
point in the switching period and sampling wherever software happened to get
to — on a current shunt, that is the difference between a reading and a guess.

`trigger::on_update` fires at the reload; `trigger::on_compare` fires at a
compare match.

A block whose curated data reports no trigger output **refuses the request**
instead of accepting it and never firing:

```
error: this PWM block has no trigger output
```

That gate reads `feat::trgo`, a generated number from the chip's data — so it
is right per-instance without anything in the driver being edited.

!!! warning "Wiring the trigger to an ADC is not done for you"
    `trigger` makes the timer publish the event. The consumer still has to be
    told to listen, and on some families alloy cannot say it yet — the F4/F7
    ADC driver is software-started only, because the `EXTSEL` encoding that
    names which timer event starts a conversion is not in this project's
    served data. On the STM32G0 the ADC's trigger selection **is** curated.

---

## The complementary bridge

One call opens three phases, their three complements, the dead time between
them and the break input:

```cpp
#include "alloy/board.hpp"
#include "alloy/bridge.hpp"

int main() {
    board::init();

    // `bridge_defaults` is what the BOARD file says the power stage needs —
    // switching rate and gate-driver dead time are project facts, so they live
    // there rather than at this call site (and are overridable from alloy.toml).
    auto inv = board::bridge::open_checked<board::bridge_defaults>();

    inv.set_duty<1>(0x8000);
    inv.set_duty<2>(0x8000);
    inv.set_duty<3>(0x8000);

    inv.enable_outputs();     // nothing switches until this line
}
```

Duties first, outputs second, and `open()` deliberately leaves the outputs off
so the bridge is never live carrying a duty nobody chose.

### `config`

```cpp
struct config {
    std::uint32_t  freq_hz      = 20'000;
    dead_time      dead_time    = {};                        // NO DEFAULT
    alignment      align        = alignment::center;
    off_state      when_off     = off_state::drive_idle_level;
    trigger        trigger      = trigger::none;
    std::uint16_t  update_every = 1;
};
```

`align` defaults to **centre** here, the opposite of `alloy::pwm`, because
that is what a three-phase modulator wants.

### The dead time has no default, on purpose

`dead_time` is not a `uint32_t` with a `0` default. It is a small type with
three states, and the unstated one does not compile:

```cpp
using dt = alloy::bridge::dead_time;

dt::ns(500)                          // 500 ns, inserted by the timer
dt::inserted_by_the_gate_driver()    // the gate driver already does it
{}                                   // unstated — compile error
```

The reason is the failure mode. A default of zero is a perfectly ordinary
number that means *turn both transistors of a pair on at the same time*, and
the board it destroys will not tell you why. So "I have not thought about
this" is a different state from "I chose zero", and only the second one builds.

`dead_time_ns()` reports what the hardware **actually programmed**, which is
not always what you asked for — the encoding is piecewise and alloy rounds
**up**, never down:

```cpp
inv.dead_time_ns();   // e.g. 507 for a requested 500
```

Too much dead time distorts the waveform. Too little destroys the bridge. The
rounding direction is the one that is survivable.

### The break input

The break input switches all six outputs off **in hardware**, with no software
in the path — that is the whole point of it, and it is why an over-current
comparator belongs on that pin rather than on an interrupt.

```cpp
if (inv.faulted()) {
    // The outputs are ALREADY off. The hardware did that before this line ran.
    inv.acknowledge_fault();   // clears the flag; does NOT re-enable outputs
}

inv.emergency_stop();          // trip it from software
```

Re-arming after a fault is a decision, not a default, so nothing re-enables
the outputs for you.

### Writing three duties without tearing

The three compare registers are preloaded, so each write lands at the next
update event. Three writes that straddle one produce a **torn frame**: two
phases from the new set, one from the old. On a motor that is one vector
nobody asked for, and it is invisible afterwards because the next frame is
correct.

```cpp
if (!inv.set_duties_coherent(d1, d2, d3)) {
    // A reload happened mid-write. Retry if you care.
}
```

Read the return value honestly: **this does not make the write atomic**, and
nothing on this timer can — there is no commit register for compare *values*.
What it does is *witness* the straddle, so a caller who cares can retry. A
loop that already runs from the update interrupt is inside the window by
construction and can ignore this entirely and use `set_duty<N>()`.

### Slowing the update cadence

```cpp
auto inv = board::bridge::open_checked<config{
    .freq_hz = 20'000,
    .dead_time = dt::ns(500),
    .update_every = 4,          // duties land every 4th switching period
}>();
```

Stated in **switching periods**, not counter units, because a centre-aligned
counter reaches its update condition twice per period and an edge-aligned one
once — so "every period" is not the same register value in the two modes. Your
control loop thinks in periods; the driver does the conversion.

Raising it is how a loop slower than the carrier stops being interrupted. It
is also how a duty stays on the bridge for N periods instead of one, which is
torque ripple if you did not mean it. Hence the default of 1.

### `open_checked` vs `open`

```cpp
auto inv = board::bridge::open_checked<board::bridge_defaults>();  // prefer
auto inv = board::bridge::open(cfg);                               // runtime
```

`open_checked` takes the config as a **template** parameter, which turns the
admissions into `static_assert`s that fire at every optimization level.
`open()` takes it as a value, and its checks are `[[gnu::error]]`s that need
the optimizer to have folded the constant — measured, they fire at `-O2` and
`-Os` but not at `-O0` or `-Og`. Use `open_checked` unless the config is
genuinely a runtime value.

---

## Boards that have one

| Board | `led_pwm` | `bridge` |
|---|---|---|
| `nucleo_g0b1re` | ✅ | ✅ TIM1 |
| `nucleo_f767zi` | — | ✅ TIM1 |
| `nucleo_g071rb` | ✅ | — |
| `esp32_devkit`, `esp_wrover_kit` | ✅ | — |

Code compiles on all of them either way — a board with no bridge takes the
other branch of an `if constexpr`, with no preprocessor anywhere:

```cpp
if constexpr (board::caps::bridge) {
    auto inv = board::bridge::open_checked<board::bridge_defaults>();
    // ...
} else {
    uart.write("no bridge role on this board\r\n");
}
```

To put a bridge on your own board, declare the role — seven pads and a dead
time — and see **[Adding a board](adding-a-board.md)**:

```json
"bridge": {
  "peripheral": "tim1",
  "ah": "pa8",  "al": "pa7",
  "bh": "pa9",  "bl": "pb0",
  "ch": "pa10", "cl": "pb1",
  "brk": "pb12", "break_active": "low",
  "freq_hz": 20000, "dead_time_ns": 500
}
```

`alloy board-validate <board>` checks every pad routes to that timer, and
warns when two roles want the same pad.

---

## What has actually been proven, and what has not

This section is not boilerplate. Motor control is the one area of this
framework where believing an untested claim can destroy hardware.

**Tested on the host.** The prescaler arithmetic, the duty-step counts quoted
above, the centre-aligned halving, the dead-time encoding and its round-up
direction, the CR2 allowlist, the trigger refusal, and the torn-frame
witness — all have host tests, including negative controls.

**Compiled on two families.** `alloy::bridge` opens on the STM32G0 and, as of
this work, the STM32F7.

**Never run on silicon.** No `alloy::bridge` driver has switched a transistor,
on any board. There is no emulation leg either, and the reason is checked
rather than assumed: alloy's generated Renode platform does not instantiate
TIM1 at all, so the dead time, the break input and the main output enable are
**unmodelled** — running the bridge example under Renode logs the register
writes and simulates none of their effects.

!!! danger "The F7's carrier frequency is not verified"
    On the two F7 boards — and only those — the APB bus is divided (PCLK2 is
    90 MHz against a 180 MHz AHB). The pinned chip data gives TIM1
    `bus_clock: PCLK2` and `kernel_clock: PCLK2_TIM`: **two different nodes**.
    alloy's clock model has no timer node, so it uses the bus one, and the
    relation between the two is not in this project's served data.

    If the well-known ST doubler is real, a bridge asked for 20 kHz switches at
    **40 kHz** on an F7. Every other board in the tree runs APB undivided,
    where both readings give the same number — which is why nothing caught it
    sooner. Treat the F7 carrier as unverified until measured on a board.

If you are attaching this to a real power stage: put a scope on CH1 and CH1N
first and measure the gap at both edges against what `dead_time_ns()` reports.
Do that before any DC bus is energized.
