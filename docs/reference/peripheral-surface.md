# The peripheral surface

Alloy's lead chip just went from 23 to 65 peripherals, 28 of them uncurated, and
the drivers and config surface for the rest are about to be written. This page
decides the *shape* of that surface first, because deciding it after 28 drivers
is the expensive order.

It answers four questions that every one of those drivers would otherwise argue
from scratch:

- where a knob lives when there are forty of them,
- what happens when the chip has less of a feature rather than none of it,
- where the line is between "portable API", "typed but vendor-shaped", and
  "raw registers",
- and what any of it costs on a 32 KB Cortex-M0+.

The short version is one sentence: **a field exists only where it is honoured,
a value only where it is reachable, a quantity is a generated number, a
peripheral has one owner, and everything else is a compile error that names the
thing you asked for.**

!!! warning "This page is at revision 2, and revision 1 was wrong in three places"
    The rule below was derived from UART and then applied, adversarially, to
    three features it had never seen: I2C 10-bit addressing, the ADC analog
    watchdog, and timer encoder mode. **It failed all three** — confidently,
    which is worse than failing loudly. The repair is not another paragraph of
    doctrine: v1 asked *"which layer does this knob live in?"* without first
    asking *"is it a knob at all?"*, and all three counterexamples are things
    that are not knobs.

    What changed: a gate (question 0) and three new categories in front of the
    layers ([the rule](#the-rule)); Layer 1 now admits VALUES and not only
    fields; Layer 2 gained a *scope*; and "`alloy::dev::` is the unconditional
    escape hatch" is retired — it was false, and
    [the three gates](#question-0-what-does-the-database-already-know) says
    what is actually true. The three counterexamples are
    [answered in full](#the-three-counterexamples-answered) and three more
    features are [stressed against v2](#stressing-v2-on-three-features-it-was-not-built-from),
    two of which break it. Every v1 claim that did not survive is corrected
    where it stood, not appended to.

---

## The decision

| Question | Answer |
|---|---|
| Where does the portable surface live? | `alloy::<periph>::config` — a runtime struct, frozen at the knobs *every* driver alloy ships programs. UART: three fields. |
| Where does the rest live? | `alloy::<periph>::opts<Inst>` — a compile-time value whose **members are declared per IP version**. A knob the silicon lacks is not a member. |
| How is "less of it, not none of it" expressed? | A generated number on the instance: `Inst::feat::rx_fifo_depth`. **0 means absent.** |
| What happens when you ask for something absent? | Compile error naming the instance, and the field when the IP has any vendor knobs at all. Never ignored, never a runtime error code. |
| What happens when you ask for too much of something present? | `static_assert` inside the driver, comparing against the generated number, with both numbers in the message. |
| What happens when you ask for a value the port cannot reach? | Compile error when the value is a literal, named trap when it is computed. Not opt-in — see [Layer 1 admits values, not only fields](#layer-1-admits-values-not-only-fields). |
| Is there a generic command door? | **No.** Not now, not under schedule pressure. `alloy::dev::` is the only door below `opts`. |
| Is `alloy::dev::` always available? | **No, and v1 said it was.** It reaches only what the chip database curates, at [three separate gates](#question-0-what-does-the-database-already-know). |
| What owns a peripheral instance? | Exactly one binder, in exactly one *personality* — enforced at generation time for board roles and at run time for everything else (`alloy::claim`). |

---

## Three layers, and the name tells you which

| Layer | Name | Promise | Who may name it |
|---|---|---|---|
| 1 | `alloy::uart::config`, `alloy::uart::handle`, the concepts in `alloy/concepts.hpp` | works on every chip alloy claims to support | portable `main.cpp`, `libs/` drivers |
| 2 | `alloy::uart::opts<Inst>` (surfaced as `Role::opts`) | typed, instance-checked, **shaped by the IP** | app code that knows its board; `libs/` only through `requires` probes |
| 3 | `alloy::dev::`, `alloy::ip::` | raw register facts from the chip database — **only for what the database curates**, see [question 0](#question-0-what-does-the-database-already-know) | anyone, with the [escape hatch](../guide/escape-hatch.md)'s warnings |

`alloy::hal::` stays what [stability.md](stability.md) already says it is: driver
internals, not a user surface. Layer 2 is *not* `alloy::hal::` — it is a
template declared next to the shared vocabulary and re-exported into
`alloy::uart::`, so a user never types `hal`.

The reason three layers exist rather than two is the cliff. ST ships HAL, LL and
CMSIS registers and publishes `Examples_MIX` projects to show them mixed; alloy
today has a portable facade and raw registers with nothing between, so a user
who wants parity bits on a chip whose driver does not offer them writes register
pokes — *in their own code*, where nothing checks them.

!!! danger "What the three layers do NOT answer, and what v1 got wrong by assuming they did"
    The layers are an axis of **portability**: how many chips can honour this
    value. That is a real question and this is a real answer to it.

    It is not the *first* question. Before "how portable is this value" comes
    "what kind of thing is this value" — and there are four kinds, only one of
    which is a knob on an open port:

    | Kind | Example | Where it goes |
    |---|---|---|
    | a **personality** | timer as PWM vs as encoder | a different **binder**, in its own namespace |
    | a **sub-resource** | one of the ADC's three analog watchdogs | its own **handle**, claimed against a `feat` count |
    | a **per-transfer** value | this transfer's I2C address width | an **argument** of the operation |
    | a **knob** | baud, parity, DE lead time | Layer 1 / `feat` / Layer 2 / Layer 3 |

    v1 routed all four through the layer questions, so it answered
    *confidently and wrongly* for the first three. The repaired rule asks the
    kind question first.

---

## Layer 1 — the portable config

```cpp
namespace alloy::uart {

enum class parity    : std::uint8_t { none, even, odd };
enum class stop_bits : std::uint8_t { one, two };

struct config {
    std::uint32_t   baud   = 115'200;
    uart::parity    parity = parity::none;
    uart::stop_bits stop   = stop_bits::one;
};
}
```

!!! note "This block was written with four fields. The code says three."
    Word length did not survive the admission test, and neither did two of the
    four `stop_bits` values. Both are measured, both are in the tree, and the
    reasoning is in [What the code changed about this page](#what-the-code-changed-about-this-page).
    The rule survived; two of its example answers did not.

Three fields, and the admission test is mechanical: **a field is Layer 1 only
when every `uart_impl<>` in the tree programs it, for every value it can
take.** Six drivers ship today (`st_usart_v2/v3/v4`, `microchip_usart_v1`,
`espressif_uart_v1`, `raspberrypi_uart_pl011`) across four vendors. Three
fields is a promise six drivers can keep. Nine is not — which is exactly the
problem this replaces.

!!! warning "The defect this fixes is live, not hypothetical"
    `src/alloy/hal/uart/uart_impl.hpp` declares one shared `serial_config` with
    nine fields, including `de_enable`, `invert_tx` and `de_assert_16ths`, for
    **every** UART driver in the tree. Exactly one driver — `st_usart_v3` —
    implements `configure()`. `bind::reconfigure()` is gated on
    `requires requires { hal::uart_impl<Inst>::configure(0u, c); }`, which
    proves the *method* exists, not that the *fields* are honoured. The day
    someone writes `configure()` for `raspberrypi_uart_pl011` — no DE register,
    no `TXINV` — `.de_enable = true` compiles and does nothing.

    That is the lie guard #7 forbids, it is in `src/` today, and it is loaded
    and pointed at the next 28 drivers.

`config` stays a **runtime argument**. Baud and parity legitimately change on a
live port — that is what `st_usart_v3::configure()`'s UE-low rewrite and
TEACK/REACK waits exist for. It also means every existing `open({.baud = …})`
call site keeps compiling; see [Migration](#migration).

Two things that look like config fields and are not:

- **Anything that needs a pin** is a binder tag, not a field. RS-485 DE, RTS and
  CTS join the bind: `uart::bind<Inst, tx<pa2_t>, rx<pa3_t>, Clock, de<pb1_t>>`.
  A config bool that only works if some other code muxed a pin is precisely the
  lie `routes::routable<>` exists to kill.
- **Anything whose unit is a register artefact.** `de_assert_16ths` is measured
  in sixteenths of a bit time because one vendor's field is five bits wide.
  That is a Layer-2 name forever.

### Layer 1 admits values, not only fields

v1 made a Layer-1 *field* honest — a field exists only where every driver
programs it — and said nothing about the *value*. The consequence was found by
trying it: `open({.baud = 0})` compiled with **no diagnostic at all**. The
driver divided by it, GCC folded the constant division to unreachable, and the
program fell into the nearest trap, which belonged to the double-open guard —
so the crash pointed at the wrong bug. With a run-time baud there was no fold
and no trap: `__aeabi_uidiv` returned quietly and the port was misconfigured.

`open_checked<Baud>` existed and was right about all of this, and being opt-in
made the **default** the unsafe one — the inverse of this project's posture.
`alloy/core/admit.hpp` closes it, with one rule in two mechanisms:

```cpp
const bool ok = baud != 0u && kernel != 0u && baud <= kernel;
if (__builtin_constant_p(ok) && !ok) { alloy::core::admit::uart_baud(); }
if (!ok) { alloy::trap<alloy::trap_code::impossible_config>(); }
```

`admit::uart_baud()` is declared with GCC's/Clang's `error` attribute and never
defined: if the impossible branch survives optimization, the call is a hard
error. Verbatim, on the G0B1RE, `-Os`, **8 non-blank lines**:

```
In function 'void alloy::uart::detail::admit_baud(uint32_t, uint32_t)',
    inlined from 'static alloy::uart::handle<Inst> alloy::uart::bind<...>::open(
        alloy::uart::config) [with ... Inst = alloy::dev::usart2_t; ...]',
    inlined from 'void probe()' at wrong.cpp:3:45:
src/alloy/uart.hpp:100:38: error: call to 'alloy::core::admit::uart_baud'
declared with attribute error: alloy::uart::open: this baud rate is impossible
on this port — it is zero, or above the UART's own kernel clock. Layer 1 admits
only rates a divisor can represent; for the accuracy question use
open_checked<Baud>()
```

Note what the inline chain buys: the message is per peripheral, and GCC's
`[with ... Inst = alloy::dev::usart2_t]` names the *instance* without the
message having to. The same two lines guard `i2c::open`'s `speed_hz`,
`spi::open`'s `clock_hz` and `pwm::open`'s `freq_hz` — three more facades that
had the identical hole.

**What it checks is deliberately narrow**: only values *no divisor can
represent* — zero, and a rate above the peripheral's own kernel clock. Both are
true of every dividing peripheral on every vendor, so the check needs no
per-driver knowledge and can never refuse a port that works.

**What it does not check is accuracy**, and that is a decision rather than an
omission. Whether 3 Mbaud off a 16 MHz kernel lands within 2% is
`open_checked<Baud>`'s job and stays opt-in: a tolerance applied silently to a
*runtime* value is a policy, and a policy that refuses a port a product has
shipped on for three years is its own kind of lie. **The seam is here** —
representable is checked always, accurate is checked when you ask.

Measured (`-Os`, Cortex-M0+, `.text`): **0 bytes** when the value is a
constant, which every literal call site and every `product.hpp` constant is;
**20 bytes** when it is genuinely computed at run time, which is the case that
previously had no diagnostic whatsoever.

#### Two spellings of one fact cannot both be typed

The same review found `open_checked<115'200_baud>({.baud = 9'600})` compiling
clean and running at 115 200 — the template argument overwrote the field and
the loser was never mentioned. `open_checked` now takes `alloy::uart::frame`,
which is Layer 1 *minus the rate*:

```cpp
struct uart_frame {
    detail::baud_is_the_template_argument_of_open_checked baud = {};
    hal::parity     parity = parity::none;
    hal::stop_bits  stop   = stop_bits::one;
};
```

`baud` is still a member, of a type nothing converts to, because that is what
makes the diagnostic say *why*: the type's name is the message. Written out,
`alloy::uart::frame f{.baud = 9'600};` is one line —
`error: initializer for 'alloy::hal::detail::baud_is_the_template_argument_of_open_checked' must be brace-enclosed`.
Written inline at the call, GCC reports it as an overload failure instead: 13
lines, ending in `cannot convert '{9600}' … to type 'alloy::uart::frame'`.
Still one screen, still names the value and the type, and measurably worse than
the direct form — stated because it is the shape a user will actually hit.

---

## Layer 2 — `opts<Inst>`, where absence is the capability

```cpp
// alloy/hal/uart/uart_impl.hpp — the shared vocabulary header
template <class Inst> struct uart_opts {};          // primary: empty, always usable

// alloy/uart.hpp — re-exported so users never type `hal`
namespace alloy::uart { template <class Inst> using opts = hal::uart_opts<Inst>; }

// alloy/hal/uart/st_usart_v3.hpp — beside the driver, constrained identically
template <class Inst> requires std::same_as<typename Inst::ip, ip::st::usart_v3>
struct uart_opts<Inst> {
    std::uint8_t data_bits         = 8;    // {7,8,9} on THIS IP
    bool         invert_tx         = false;
    bool         invert_rx         = false;
    bool         swap_rx_tx        = false;
    std::uint8_t de_assert_16ths   = 8;
    std::uint8_t de_deassert_16ths = 8;
};
```

Note there is no `de_enable` bool: hardware driver-enable needs a PIN, so what
switches it on is the binder tag `uart::de<pin>` (rule 1). The members here are
the timings only. And note the IP: it is **v3**, not v4 — see the boxed
correction below.

Applied as a template argument, defaulted, so the empty case is the call you
already write:

```cpp
template <opts<Inst> Opts = {}>
static handle<Inst> open(config c = {});
```

```cpp
// portable, every board
auto dbg = board::debug_uart::open({.baud = board::debug_uart_baud});

// the one door, greppable as `::opts{`. DE is switched on by the bind's
// `uart::de<pin>` tag; these are the timings it uses.
auto bus = board::rs485::open<board::rs485::opts{.de_assert_16ths = 12}>(
               {.baud = 19'200, .parity = alloy::uart::parity::even});
```

Three deliberate choices:

**It is a template parameter, not a field.** Whether the port drives DE, is
inverted, or has a FIFO threshold is a board and silicon fact fixed at design
time. Making it compile-time is what lets the driver `static_assert` the value
against generated data, and what makes an unused feature emit nothing.

**The type is named at the call site** (`Role::opts{…}`, not `open<{…}>()`).
Six extra characters buy the diagnostic; see [Failure modes](#failure-modes).

**A driver with no vendor knobs writes nothing.** `raspberrypi_uart_pl011`
inherits the empty primary, and its `configure<O>` ignores `O`. Adding Layer 2
costs zero lines in drivers that have nothing to add.

### Every Layer-2 knob has a SCOPE, and v1 never asked

A UART is one thing, so on UART the question never came up. A timer is not: it
has one prescaler, one auto-reload, one dead-time generator — and **four
channels**. `alloy::pwm::bind` is per channel. So a Layer-2 knob on a timer is
one of two different things:

| Scope | Meaning | May be stated at |
|---|---|---|
| **block** | one register for the whole peripheral (`PSC`, `ARR`, `BDTR.DTG`) | one call site per block, and every other claimant must agree |
| **channel** | one register field per channel (`CCMR1.OC1M`, `CCER.CC1P`) | that channel's own call site |

Declaring a **block-scoped** value at a **channel-scoped** call site is a lie
with the same shape as hole (A): two claimants, one register, last writer wins,
no diagnostic. It was live in shipped code — `pwm::bind::open({.freq_hz = …})`
on two channels of one timer wrote `PSC`/`ARR` twice and the second silently
retuned the first, both handles still "working". `alloy::claim::shared` refuses
it now, with the block-scoped value as the witness:

```cpp
alloy::claim::shared<Inst, personality::pwm>(c.freq_hz);   // agree, or trap
```

The witness covers Layer 1's `freq_hz` today. It does **not** yet cover a
block-scoped Layer-2 value, because no timer driver has one — and that is
exactly the gap [dead time walks into](#3-timer-complementary-outputs-with-dead-time-breaks-v2).

### The naming rule that makes Layer 2 portable enough

A per-IP extension type is per-IP, which means nothing written against it is
automatically reusable — a `libs/` Modbus driver that names
`opts<st_usart_v3_t>` does not compile on a Microchip USART that *does* have DE.
The fix is not to generate the vocabulary; API design is behaviour, and the
governing rule says behaviour is hand-written. The fix is a discipline:

> **The same silicon feature gets the same field name and the same unit in
> every driver that has it.** `de_assert_16ths` means the same thing on ST and
> on Microchip or it is not called `de_assert_16ths`.

A `libs/` driver then probes by name, with no preprocessor:

```cpp
if constexpr (requires { Opts{}.de_assert_16ths; }) { /* hardware DE */ }
else                                                { /* GPIO DE */ }
```

This is a review rule today, not a gate. A grep that checks Layer-2 field names
against a registered vocabulary list is the obvious enforcement and **has not
been built** — do not read the rule as mechanised.

---

## Degree — a generated number, where 0 means absent

Presence and degree are different questions and get different mechanisms.

`board::caps::*` answers neither, and must never grow either. It is generated
from `PRESENCE_ROLES` in `emit/board.py` — nineteen booleans about which
**roles a board wired up**. FIFO depth is a die fact that has to be true for a
peripheral no role touches; putting it in `caps` would let two boards on one
chip disagree about the silicon.

Degree lands on the instance descriptor, beside `Inst::ip` and `Inst::dmareq_tx`:

```yaml
# alloy-devices/chips/st/stm32g0b1re.yaml
usart1:
  base: '0x40013800'
  ip: st/usart_v4
  gate: {peripheral: rcc, register: APBENR2, bit: 14}
  irq: USART1
  kernel_clock: apb
  dma_requests: {rx: 50, tx: 51}
  feat:                      # NEW
    rx_fifo_depth: 8
    tx_fifo_depth: 8
    de_time_max_16ths: 31
    max_data_bits: 9
```

```cpp
struct usart1_t {
    using ip = alloy::ip::st::usart_v4;
    // ... base, gate, irq, kernel, dmareq_* (unchanged) ...
    struct feat {
        static constexpr std::uint8_t rx_fifo_depth     = 8u;
        static constexpr std::uint8_t de_time_max_16ths = 31u;
        static constexpr std::uint8_t max_data_bits     = 9u;
    };
};
```

Four rules, and they matter more than the syntax:

1. **Zero means absent.** There is no `has_fifo` beside `rx_fifo_depth`. A bool
   and a count that can disagree is a bug class; deleting the bool deletes the
   class. A bool appears only where the fact is genuinely ungraded.
2. **It is per instance, not per IP.** Alloy's descriptors already are, so this
   rides for free — and it is the axis that matters. `usart1` and `lpuart1` on
   the G0B1 share an IP tag and differ in oversampling and flow control.
3. **`feat` numbers are only ever generated.** A hand-written header asserting a
   silicon quantity is guard #1 with a different literal. `check_contract.sh`
   gains one grep: `struct feat` may not appear under `src/`. Concretely,
   `st_i2c_v2.hpp`'s `kMaxNbytes = 255` is a silicon fact hand-written today and
   becomes `Inst::feat::max_transfer_bytes`.
4. **Absence in the data is a lint failure, not a default.** A missing `feat`
   block must fail the plausibility lint, never be read as zero — a generated
   lie is worse than a hand-written one, because nobody reviews it.

Portable code branches on the number, with no preprocessor:

```cpp
if constexpr (Inst::feat::rx_fifo_depth >= 8) { burst_fill(); }
else                                          { byte_at_a_time(); }

static_assert(alloy::dev::adc1_t::feat::oversample_bits >= 14,
              "this product needs 14-bit oversampling");
```

This subsumes most of the documented gap list: the I2C 255-byte transfer cap
(today an invisible driver constant), ADC resolution and oversampling, SPI frame
width, timer counter width.

### Degree that is a shape, not a quantity

Input capture, encoder mode and complementary outputs with dead time are not
*more timer*. v1 said they were "more registers — a different IP tag, therefore
a different driver", and that is **half right and the wrong half is the one
that matters**: a different IP tag is how alloy picks a *driver*, and it cannot
express that one block runs in one of several mutually exclusive modes at a
time. `tim3` on the G0B1RE is one IP, one instance, and it is a PWM generator
*or* an encoder counter, never both. The IP tag has nothing to say about that.

The right category is a [personality](#personalities-a-block-runs-in-one-mode-at-a-time),
below. What survives of v1's paragraph is the data half: `st/tim_gp16` is
under-curated for any of those modes and needs mining before the first timer
driver beyond PWM is written, not after the seventh. Seven of the 28 uncurated
peripherals on the G0B1 are timers.

---

## Personalities: a block runs in one mode at a time

A **personality** is a mutually exclusive whole-block mode. A timer is a PWM
generator, an encoder counter, or an input-capture unit. A USART is a UART, a
synchronous SPI master, a LIN node, or an IrDA endpoint. An I2C is a master or
a slave. These are not knobs, they are not degrees, and they have no home
anywhere in v1's five questions — which is why v1 answered encoder mode with
"it needs a pin mux, therefore a binder tag" and was wrong in one step.

**Two mechanical tests, and they agree:**

1. *Does turning it on change which OPERATIONS the peripheral offers?* PWM
   offers `set_duty()`; an encoder offers `count()`. Baud changes no operation
   a UART offers. → personality.
2. *Can two of them be active on the block at once?* No → personality. Two PWM
   channels on one timer: yes → one personality, several claimants.

Where a personality goes: **its own facade and its own binder type**, in its
own namespace, with its own `config`, its own `opts<Inst>` and its own binder
tags. `alloy::pwm::bind<tim3_t, …>` and `alloy::encoder::bind<tim3_t, …>` are
different types that happen to name one instance.

A personality may **share Layer 1 with its sibling where the value means the
same thing on the wire** — SPI's CPOL/CPHA mean exactly what they mean whether
you are the master or the slave — and must **drop the fields it cannot honour**
rather than ignore them. That is the `uart::frame` shape again: an SPI slave is
clocked by the master, so `spi::slave::config` has no `clock_hz`, and asking
for one is a compile error naming the member instead of a value that goes
nowhere.

### What stops a program binding two personalities to one block

Nothing did, and that is [hole (A)](#the-three-holes-two-now-closed). It is
closed in two places, neither of which subsumes the other:

**At generation time**, for the binds a board file states. `emit/board.py`
carries `ROLE_PERSONALITY` and refuses a `board.json` that gives one peripheral
to two of them:

```
board synth: peripheral 'tim3' is claimed by role 'led_pwm' as a pwm and by
role 'rtc' as a rtc — a block runs in one personality at a time, so one of the
two roles has to name a different peripheral
```

`nvm` and `fs` deliberately share the personality `flash`: two regions carved
out of one flash controller are one driver serving two roles, which the
`nucleo_g0b1re` board declares today, and a blanket "one role per peripheral"
rule would have broken it.

**At run time**, for everything a board file never sees — hand-written binds,
`libs/` code, a second translation unit. `alloy/core/claim.hpp` puts one byte
on the **instance**:

```cpp
template <class Inst> inline claim::personality owner = personality::none;

claim::exclusive<Inst, P>();      // one owner, full stop
claim::shared<Inst, P>(witness);  // several claimants, one personality,
                                  // agreeing on the block-scoped value
```

`inline` variable template ⇒ one object per instance across the whole image,
however many TUs, binders or roles name it. That is the part the old
`detail_opened` could not do: it was a static of the **binder type**, so two
binders on one instance had two flags and both "succeeded".

Run time and not compile time, for the reason NORTH_STAR guard #7 already
concedes: C++ cannot see across translation units while compiling. What is new
is not the mechanism, it is the *key* — instance, not binder.

### Two keys, because a peripheral is not always the contested resource

A whole peripheral is one scope. A **numbered part** of it is another, and
`owner<Inst>` cannot express it: it answers "who owns TIM2" and there is no
value it can hold that answers "who owns TIM2 **channel 1**". So there is a
second variable template, keyed on the instance **and the ordinal**:

```cpp
template <class Inst, unsigned Sub> inline claim::personality sub_owner = …;

claim::sub_exclusive<Inst, Sub, P>();     // one owner of one channel, full stop
claim::sub_shared<Inst, Sub, P>(w);       // several claimants, one witness
claim::sub_release<Inst, Sub, P>(w);      // …and the one resource with a disarm
```

!!! warning "`sub_shared` was argued away when this scope was built, and the EXTI line refuted the argument"
    The first version of this section said, in as many words: *"There is
    deliberately no `sub_shared`. A sub-resource is a sub-resource because it
    has no block-scoped register for two claimants to disagree about."* That
    was derived from the two sub-resources alloy had — a timer channel and a
    DMA channel — and it is false of the third. An **EXTI line** has its own
    port-select field, its own trigger pair and its own callback slot, and the
    claimants competing for it are *pins*: PA5 and PB5 are one line. The witness
    is the port index, so re-arming one pin is admitted and a second pin traps.
    [Hole (A3)](#a3-the-exti-line) has the measurement.

    The same feature refuted *"claims are never released"*. `clear_on_edge()`
    is a shipped call that genuinely frees a line, so `sub_release` exists — at
    this scope only, because nothing at block scope in alloy can be given back.

**The ordinal, and nothing else, is the key.** That is the whole lesson of this
scope, and getting it wrong is how the defect survived the first repair:
`pwm::bind` is templated on `<Inst, Channel, Pin, Signal, Clock>`, so the
`static bool detail_channel_opened` that used to guard the channel was keyed on
the PIN as well — one channel, one flag per route. TIM2_CH1 has four routes on
the G0B1RE (PA0, PA5, PA15, PC4), so two binds on one channel were legal to
write, compiled clean, both opened, both muxed their pin onto the same output
compare, and neither said a word. Measured: the pre-fix `nucleo_g0b1re` image
printed its "second bind also opened" banner under Renode in 1.4 s. It is
[hole (A)](#the-three-holes-two-now-closed) exactly, one level down, and the
fix is the same one: move the key off the binder.

### Which facade claims what {#which-facade-claims-what}

Thirteen rows, two scopes, and the rule that decides is one line: **a facade
claims at its CONFIGURING entry point — the call that programs the block — and
never on a call that only moves data.** `uart::write`, `pwm::set_duty`,
`can::send` and `wdt::feed` are on the hot path and claim nothing.

**This table is a test.** `tools/alloy/tests/test_claim_surface.py` parses it
and checks each row against the file it describes. A facade that claims *less*
than its row says fails; a peripheral class that ships a driver directory with
no row at all fails; and the two facades that promise **nothing** — `flash` and
`gpio` — fail if a claim appears in them. The one direction it does not fail on
is a facade claiming *more* than the table says, which is this page lagging the
code rather than the code lying. That asymmetry is deliberate: hole (A2) below
is exactly a row this page asserted and the code did not have, twice.

| Facade | Entry point | Instance scope | Sub-resource scope | What the claim catches |
|---|---|---|---|---|
| `uart` | `open`, `rom_bind::open` | `exclusive` | — | a second pin pair on one port |
| `i2c` | `open` | `exclusive` | — | a second pin pair on one bus |
| `spi` | `open` | `exclusive` | — | a second pin trio on one bus |
| `adc` | `open` | `exclusive` | — | two claimants of one converter |
| `pwm` | `open` | `shared(freq_hz)` | `sub_exclusive(Channel)` | two frequencies on one timer; **two pins on one channel** |
| `dma` | `channel::claim` | — | `sub_exclusive(Ch)` | one channel handed out twice |
| `wdt` | `start` | `shared(timeout_ms)` | — | **two deadlines on one watchdog** |
| `dac` | `enable` | `shared(0)` | — | personality only (see below) |
| `can` | `enable` | `shared(0)` | — | personality only |
| `rtc` | `set` | `shared(0)` | — | personality only |
| `encoder` | `open` | `exclusive` | — | a timer already generating PWM |
| `exti` (a line) | driver `arm` / `disarm` | — | `sub_shared(port)` | **two pins on one interrupt line** |
| `flash` | *none* | — | — | nothing; deliberately (see below) |
| `gpio` | *none* | — | — | the pin, nothing; its EXTI *line*, the row above |
| ethernet | *no facade* | — | — | nothing at run time; generation time only |

There is no row for `spi::slave` or `capture`: those personalities
are named in the enum and have no facade yet. **Ethernet has the opposite
shape** — a role, a `ROLE_PERSONALITY` entry and a generated `board::eth`, but
no `alloy::eth` facade: the board exposes the HAL MAC type directly, so there
is no portable entry point for a claim to live in. Generation time is the only
half it has, and the `ethernet` enumerator exists for the day that changes.

**`shared(0)` is not ceremony and not a fix.** `dac`, `can` and `rtc` carry no
configuration — `enable()` takes no arguments, and a wall clock is data — so
the "contradictory config, last writer wins" half of hole (A) genuinely cannot
happen there: two `can::controller<fdcan1_t>` objects are indistinguishable.
What the claim buys is the *other* half. The block goes on record in its
personality, so an `alloy::dev::`-level driver or an out-of-tree facade
claiming `user_a` on the same instance traps instead of quietly coexisting —
which is a supported path (`examples/escape_hatch`), not a hypothetical. A
constant witness is what keeps `enable()` safe to call twice; `exclusive` there
would invent a bug that does not exist. The honest cost of reusing `shared`
rather than inventing a third shape: `witness<Inst>` is instantiated for these
three, so they pay 4 bytes of `.bss` for a value that can only ever be zero.
Four bytes on a peripheral the program is already using was judged cheaper than
a fourth entry point in a Tier-1 header.

**`flash` claims nothing, by the same rule that exempts `uart::write`.** The
flash controller has no configuring entry point at all: the array is on at
reset and memory-mapped, and `erase_page`/`program` only move data. Two
claimants of one controller is not merely legal, it is what every board with
both an `nvm` and an `fs` role ships — and `ROLE_PERSONALITY` already gives
both roles the `flash` personality so the generator admits exactly that pair.

**`gpio` is a third scope this mechanism does not model, and that is a limit,
not a rule.** A pin is neither an instance nor a numbered part of one; it is a
resource several peripherals compete for through the mux. On the
`nucleo_g0b1re`, PA5 is both the `led` role and the `led_pwm` role, and
PB3/PB4/PB5 are both `gpio_bus` and `spi`: the board offers alternatives and
the application picks one. A pin claim would refuse boards alloy ships on
purpose. What the type system does enforce is that a pin reaches the peripheral
at all — `routes::route` is a compile error otherwise (guard #7). Who wins when
two routes are both legal is, today, the programmer's problem.

### The two halves cannot drift

`ROLE_PERSONALITY` (Python, generation time) and `claim::personality` (C++, run
time) are one closed vocabulary written twice, and until now nothing checked
that they agreed. `ethernet` was in the generator and not in the enum; `dma`
was a facade with no enumerator at all. `test_emit.py` now parses the enum out
of `claim.hpp` and fails if the generator names a personality the firmware
cannot express.

The generator half covers **instance** scope only, and completely: all twelve
board roles that name a peripheral are in `ROLE_PERSONALITY`. It cannot cover
sub-resource scope because no board file can express the conflict — `led_pwm`
names a `channel`, but a board has exactly one `led_pwm` role, so there is no
second claimant for the generator to see. At that scope the runtime claim is
the only half there is.

---

## Sub-resources: a thing inside the peripheral with its own lifetime

The third category v1 had no word for. An ADC's analog watchdog is not a knob
on `open()`: there are one or three of them depending on the IP version, each
has its own enable, its own thresholds, its own channel selection and its own
interrupt, and a program arms one long after the port is open and disarms it
later. So does an ADC's injected sequencer, a timer's capture channel, a CAN
filter bank, a DMA stream.

**Mechanical test:** *does it exist N times inside one peripheral, with its own
enable and its own lifetime?* → sub-resource.

And it is a resource, so it is *owned*:
[`claim::sub_exclusive<Inst, Sub, P>()`](#two-keys-because-a-peripheral-is-not-always-the-contested-resource),
keyed on the instance and the ordinal. A timer channel and a DMA channel use it
today; an analog watchdog would be the third the day the handle below exists.

Where it goes: **its own handle, obtained from the port's handle, with the
index checked against a generated `feat` count.**

```cpp
auto adc = board::adc::open();
auto wd  = adc.watchdog<0>({.low = 300, .high = 3600});   // 0 <= feat count
```

The decomposition then falls out of the *existing* questions instead of being
argued per peripheral — which is the whole point, because v1's failure on the
analog watchdog was that it had no prescribed decomposition and improvised one:

| Part of the feature | Question it answers to | Answer |
|---|---|---|
| how many there are | is it a count the database knows? | `Inst::feat::analog_watchdogs`, 0 means absent |
| the thresholds | can every driver that has one program them? | the sub-resource's own Layer-1 `config` |
| which channels it guards | does it vary per call on one armed watchdog? | an argument of `guard()` |
| what happens on a trip | — | the interrupt-callback shape alloy already uses |

---

### Ask what you actually got

Alloy already has the best version of this and it is currently a UART special
case: `open_checked<115'200_baud, TolPermille>()` computes the achieved rate with
the *same divisor the driver programs*, so the check cannot disagree with the
silicon, and the failing `rate_check<requested, achieved, tol>` names both
numbers. Generalise it as a house rule: **every rate-programmed peripheral gets a
constexpr `achieved_*()` computed by the same expression the driver uses, plus an
`open_checked` companion.** SPI's prescaler and I2C's SCL divider silently round
today and nobody is told.

---

## Failure modes

Compile error wherever the information exists at compile time. Never silently
ignored, never a runtime error code, never a clamp — and where the information
genuinely does not exist until the program runs, a
[named trap](#the-failures-that-are-honestly-runtime) rather than a pretence.
Everything below is **verbatim compiler output** from a reduction
compiled with `arm-none-eabi-g++ 14.2.1`, `-std=c++23 -Os -mcpu=cortex-m0plus
-fmax-errors=1` — a probe with fake register blocks, not the alloy tree.

**The field does not exist on this IP** (4 lines, 288 characters):

```
h_absent2.cpp:10:46: error: 'O' {aka 'alloy::hal::uart_opts<dev::usart9_t>'}
    has no non-static data member named 'de_enable'
   10 | extern "C" void app() { O o{.de_enable = true}; (void)o; }
      |                                              ^
```

**The feature exists but not this much of it** (9 lines, 853 characters) — note
that GCC prints the comparison itself, and the `31` came from generated data, so
the check cannot drift from the silicon:

```
mini.hpp:62:45: error: static assertion failed: DE lead time exceeds this
    UART's DEAT field width
   62 |             static_assert(O.de_assert_16ths <= Inst::feat::de_time_max_16ths,
      |                           ~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mini.hpp:62:45: note: the comparison reduces to '(40 <= 31)'
```

**No driver for this IP at all** — unchanged: `uart_impl<Inst>`'s primary is
undefined, so the diagnostic is an incomplete type naming the instance.

Two measured warts, stated because an adversarial reviewer will find them:

- **Spell the type.** `Role::open<Role::opts{…}>(…)` gives the diagnostic above.
  The terser `Role::open<{…}>(…)` degrades to "no matching function … template
  argument deduction/substitution failed", with the designated initializer
  rendered positionally as `{6}` — longer, and actively misleading. Six
  characters buy guard #7's one-screen acceptance test.
- **An IP with *zero* vendor knobs degrades.** With an empty `opts`, GCC says
  `too many initializers for 'O' {aka alloy::hal::uart_opts<dev::usart9_t>}` —
  4 lines, names the instance, does **not** name the field. It is still a
  one-screen compile error and it is still not a lie, but it is worse than the
  common case.

Runtime errors keep the jobs that are genuinely runtime: NACK, overrun, framing,
timeout, arbitration loss. "This silicon cannot do that" never is. Alloy's
`bool` returns are under-informative for the real runtime cases and should grow
an `enum class : std::uint8_t` — same size, more information — but that is a
separate change.

### The failures that are honestly runtime

Three of them, and each carries an `alloy::trap_code` in a register so a fault
report can say which. They are runtime for the reason NORTH_STAR guard #7 gives
— a compiler cannot see across translation units — and not because checking
them at compile time was inconvenient.

| Guard | `trap_code` | Fires when |
|---|---|---|
| instance ownership | `instance_owned` | a second binder opens a port already open |
| sub-resource ownership | `sub_resource_owned` | a second binder opens one *channel* already open |
| personality | `personality_conflict` | one block, two mutually exclusive modes |
| block config | `block_config_conflict` | two channels of one timer, two frequencies; two `wdt::start` deadlines |
| sub-resource config | `sub_config_conflict` | two *pins* on one EXTI line; a line disarmed by a pin that does not hold it |
| Layer-1 value | `impossible_config` | a *computed* value no divisor can reach |
| port state | `not_open` / `opts_mismatch` | `reconfigure<Opts>` on a port nobody opened, or with `Opts` that disagree with `open()`'s |

Every one of them is a compile error instead, wherever the information exists
at compile time: a *literal* impossible value is `admit::uart_baud`, and a
personality conflict between two **board roles** is a generation error that
never reaches a compiler.

---

## The rule

This is the load-bearing output, at **revision 2**. v1's five questions are
questions 5 to 9 below, essentially unchanged and still right — what was
missing was everything in front of them. For any value on any of the 65
peripherals, ask these in order and stop at the first yes. Each test is
mechanical, and each one names a thing a compiler or a generator can check.

### Question 0: what does the database already know?

Not a layer question — a **gate**, and the one whose absence made v1 promise an
escape hatch that is not there. Ask it first, because the answer decides
whether the rest of the rule can even run.

| What alloy-devices has | What you can reach | Verified by |
|---|---|---|
| nothing — the peripheral is `uncurated` | **nothing.** `alloy::dev::<name>_t` is not emitted at all, and neither are its routes | `emit/device.py::curated_peripherals` drops them; the G0B1RE's generated `device.hpp` has `tim2/3/4` and no `tim1` |
| the peripheral, but not the register | the base, gate, IRQ and DMA facts. **Not the register**: `IP::regs` has no member for it | the G0B1RE's ADC is `st/adc_v2`, whose curated map stops at `CCR` — there is no `TR1` member to write |
| the register, but not the field | the register as a typed member. **No named accessor**, so a driver would hand-write bit numbers, which guard #1 forbids | `tim_gp16`'s `SMCR` is a register with zero fields |
| the field | everything. Layer 2 is open | `i2c_v2`'s `CR2.ADD10` and 10-bit `CR2.SADD` are both in the data |

**28 of the G0B1RE's 65 peripherals are at row 1** — `alloy chip-status
st/stm32g0b1re` prints `37 of 65 peripherals curated` — including the only
advanced timer, both LPUARTs, all three comparators and the USB device. For
those, the rule's output is not a layer at all: it is **"curate first"**, a
task in `alloy-devices` and not a decision about C++.
`emit/board.py::_require_curated` already refuses a board role that names one.

!!! warning "Rows 2, 3 and 4 have no tool"
    `chip-status`'s `REG` column answers row 1 and nothing finer: it says the
    peripheral has curated register data, not that the data has the register
    you need or the field inside it. Both of the counterexamples below that are
    blocked are blocked at rows 2 and 3, and both were found by *opening the
    IP yaml by hand*. A `chip-status --registers <ip>` that lists which
    registers and fields a curated IP actually carries is the obvious tool and
    **is not built**; until it is, question 0 costs a reader one file.

The honest unconditional escape is the one
[escape-hatch.md](../guide/escape-hatch.md) calls **Route C** — a raw literal
address in *your* code, outside alloy, with no gate, no IRQ number and no route
check. That is a real door and it is not `alloy::dev::`.

### Questions 1–4: what kind of thing is it?

**1. Does it change which OPERATIONS the peripheral offers, and exclude the
other modes while it is on?**
*Test: does the handle grow or lose methods? Can two of them run at once?*
→ A [**personality**](#personalities-a-block-runs-in-one-mode-at-a-time). Its
own facade, its own binder type, an exclusive claim on the instance. Not a knob
at any layer.

**2. Does it exist N times inside the peripheral, with its own enable and its
own lifetime?**
*Test: would you arm it and disarm it independently of `open()`? Is there a
count that differs by IP version?*
→ A [**sub-resource**](#sub-resources-a-thing-inside-the-peripheral-with-its-own-lifetime).
Its own handle, obtained from the port's, indexed against `Inst::feat::<count>`.

**3. Does it vary from one transfer to the next on a port nobody reopened?**
*Test: could two consecutive calls legitimately want different values?*
→ **An argument of the operation** — and if a bus is shared by devices that
each want their own, a *device* type that carries the bus plus that device's
settings. See [the transfer axis](#the-transfer-axis-and-the-zephyr-answer).

**4. Does honouring it require a pin?**
*Test: does it need a route or an AF programmed?*
→ A **binder tag**, not a field. `uart::de<pb1_t>`, `spi::nss<pa4_t>`.

### Questions 5–9: which layer does the knob live in?

**5. Is the answer a count, a limit, a width, or a rate the die fixes?**
*Test: is it a number the chip database knows, rather than something the app
chooses?*
→ Not a field at all. **`Inst::feat::<name>`**, generated, 0 means absent.

**6. Can every driver alloy ships program it, with the same semantics, the same
unit, over its whole value domain?**
*Test: would adding it force any current driver to trap, reject, or silently
ignore — for any value it can take? Is its unit free of vendor register
artefacts?*
→ **Layer 1**, `alloy::<periph>::config`. And the values it accepts are
[admitted, not assumed](#layer-1-admits-values-not-only-fields).

**7. Is there a register field that means exactly this, on some IPs?**
*Test: can you name the register field it programs — **in curated data**, which
question 0 already told you?*
→ **Layer 2**, `alloy::<periph>::opts<Inst>`, declared only in the drivers
whose IP has it, under the same name and unit everywhere it appears — and
**state its [scope](#every-layer-2-knob-has-a-scope-and-v1-never-asked)**, block
or channel.

**8. Is the register curated but the field not?**
→ Not a C++ question. **Mine the field**, in `alloy-devices`, and go back to 7.
The compiler will have told you: a Layer-2 member can only exist where a
generated field backs it, which is how `usart_v4`'s missing `DEAT/DEDT` turned
into a data task with an error pointing at it.

**9. Nothing above fits** — an erratum poke, a one-off, a block alloy models
but this feature is not worth a driver for.
→ **Layer 3**, `alloy::dev::` — *if question 0 said the peripheral is curated*.
And **no generic command door, ever**: no `drv_cmd(dev, uint32_t, uint32_t)`,
no `ioctl`, no `cr1/cr2/cr3` words inside a config struct. An untyped command
pair is invisible to review, to grep, to the contract gate and to the compiler;
it is strictly worse than a typed register on a named peripheral. Zephyr's own
Kconfig help for its version says *"Says no if not sure"* — a framework warning
you about its own escape hatch.

**Promotion and demotion, so the layers are not a one-way ratchet:**

- Layer 2 → Layer 1 when every shipped driver implements it. The cost is real
  work — teaching six (then N) drivers — and that friction is the point: it is
  what makes the Layer-1 promise mean something.
- Layer 3 → Layer 2 when the same register poke shows up twice.
- Nothing is ever demoted silently; a Layer-1 field that a new vendor cannot
  honour is a deprecation, on the [stability](stability.md) window.

### The transfer axis, and the Zephyr answer

Question 3 is the one v1 did not have, and the counterexample that found the
gap was I2C 10-bit addressing: **a 10-bit address is a property of the
TRANSFER, not of the port.** One bus talks to a 7-bit sensor and a 10-bit
sensor in consecutive calls, and no amount of layering a *port* config can
express that. v1 routed it through question 6, decided "every I2C driver can
program an address width, therefore Layer 1", and produced a `config` field
that would be wrong on the next line of user code.

The value belongs to the operation. There are two shapes, and the choice
between them is mechanical:

**It is already an argument → widen its type.** The I2C address is passed to
`write()` today; making the *width* part of the address type costs no new
concept:

```cpp
namespace alloy::i2c {
struct addr7  { std::uint8_t  v; };   // 0x00..0x7F
struct addr10 { std::uint16_t v; };   // 0x000..0x3FF
}
```

**Two devices on one bus disagree, and the disagreement has to be reprogrammed
per transfer → bundle the device.** SPI is the case: word size, mode and clock
are per *peripheral device*, not per bus, and any correct multi-device SPI
rewrites `CR1` between chip-selects. Zephyr found this and its answer is
`spi_dt_spec`, which bundles bus + config + CS so every transfer carries its
own. **Judgement: the pattern is right and alloy should adopt it** — with one
difference that matters here. Zephyr's is a runtime struct built from
devicetree; alloy's should be a *type*:

```cpp
using flash = alloy::spi::device<board::spi, board::flash_cs,
                                 alloy::spi::opts<board::spi::inst>{.word_bits = 16}>;
```

so a per-device word size that the silicon fixes stays compile-time and unused
combinations still cost nothing. `src/alloy/spi.hpp` has carried a comment
promising exactly this layer ("the shared SpiDevice layer arrives later,
embedded-hal style") since before this page existed; question 3 is what says
where it goes and why. **Named give-up, the same one Zephyr pays:** a device
chosen at run time — a bus scanner, a hot-plugged module — needs the runtime
form, and a compile-time `device` type cannot give it one.

### The second axis: who knows the fact

Orthogonal to which layer, and just as load-bearing. Every value must be
assignable to exactly one of three sources:

| Source | What it carries | Example |
|---|---|---|
| `board.json` → `board.hpp` | how the board is **wired** | DE timing, pin swap, line inversion, CS polarity, ADC vref, *which devices sit on the bus* |
| `products/*.toml` → `product.hpp` | what varies between **SKUs** on that board | default baud, oversampling depth, watchdog period |
| the call site | what this **call** legitimately varies | this open's baud, *this transfer's address and word size* |

If a value cannot be assigned to exactly one, it is not designed yet. This is
what makes a wide surface readable: a forty-field surface is fine as data and
fatal as app code. An RS-485 product's parity, stop bits and DE timing belong in
a TOML, and `main.cpp` writes `board::rs485::open()`.

!!! note "The review said this axis contradicted the rule on I2C. It did not — v1 read it wrong."
    Both statements are true at once and they are about different values.
    *Which devices exist on the bus, and at what addresses,* is a **board**
    fact, and `board.json` already carries it (`board::eeprom_addr` is
    generated from the `eeprom` role today). *Which of them this call is
    talking to* is a **call-site** fact. The contradiction came from v1 forcing
    a per-transfer value through a per-port question, not from the axis.

---

## What it costs

Measured for this decision, not quoted: `arm-none-eabi-g++ 14.2.1` (xPack),
`-std=c++23 -Os -mcpu=cortex-m0plus -mthumb -ffunction-sections -fdata-sections
-fno-exceptions -fno-rtti`, `.text` of the object. Same workload in every row
(open a UART, write two bytes). **Label: a reduction with a fake register block,
not an alloy build — it measures the shape, not the framework.**

| Variant | `.text` | Δ |
|---|---|---|
| today's shape — `enable(kernel, baud)`, 1-field config | 48 B | — |
| Layer 1, everything defaulted | 52 B | **+4** |
| Layer 1, 8E2 @ 19200 (parity + stop bits + word length programmed) | 64 B | +16 |
| Layer 1 deep **plus five Layer-2 features** (inversion, DE + both timings, FIFO threshold) | 72 B | +8 over the row above |
| Layer 1, config from a **runtime** source instead of a literal | 132 B | **+80** |

Read those carefully, because three of the four differences are not overhead:

- **+4 bytes** is what a user who configures nothing pays, for a feature set the
  framework does not have today.
- **+16** is the register work of actually programming parity, stop bits and word
  length — functionality, not abstraction. There is no dispatch, no table, no
  runtime decision anywhere in the disassembly.
- **+8 for five vendor features** is the constraint-(d) proof: `if constexpr`
  chains collapse and the OR'd constants precompute. A feature not named in
  `opts` contributes no instruction, no byte, no symbol.
- **+80 is the one real cliff**, and it is a design rule rather than a defect:
  keep `open()` header-inline, keep `config` an aggregate, keep product
  parameters `constexpr` — which `products/*.toml → product.hpp` already emits,
  so the fold survives. It is also the honest price of Modbus-style runtime
  renegotiation: eighty bytes on the one port that asked for it.

Two driver-authoring rules fall out and belong in the guide:

- **`open()` may assume reset state** — it runs on a just-gated peripheral — and
  must therefore *guard* every default-valued write. An unconditional
  read-modify-write per field costs bytes for knobs nobody set.
- **`reconfigure()` may not**, and writes unconditionally.

**What is deliberately not here: a table of function pointers.** A published
vtable anchors every entry against `--gc-sections`, so unused driver functions
survive in the image, and LTO recovers the speed while saving zero bytes. That
is the mechanical reason Zephyr *needs* Kconfig — `#ifdef` is the only thing that
can physically delete a pointer, and deleting the pointer is the only thing that
can delete the function. Alloy's partial specialization on `Inst::ip` is not
merely an alternative to the vtable; it is what makes the whole Kconfig layer
unnecessary. Do not add a `uart_ops` interface "for flexibility".

### Unmeasured

- **Compile time.** The instantiation key for `opts<Inst>` and `configure<O>` is
  `Inst`, which is already the key for `uart_impl<Inst>`, so no new
  instantiation *axis* appears — but that is an argument, not a measurement. A
  TU that opens one port with three different `opts` values gets three
  instantiations of the apply path. Measure it on UART plus one non-ST driver
  before applying to the other 27, and hang the result on
  `scripts/check_static_limits.sh`. CI grows along the IP-version axis, which is
  the cheaper one — but this repo has already hit GitHub's 256-job matrix limit
  once.
- **Whether the `feat` schema survives a non-ST family.** It was designed
  against ST data. Alloy already ships `microchip_usart_v1`,
  `espressif_uart_v1`, `raspberrypi_uart_pl011`, `microchip_twihs_v1`,
  `microchip_afec_v1`, `microchip_spi_v1`. Confirm on one non-ST family before
  the schema is called settled.

---

## What it gives up

**The thing a CubeMX user will still not be able to do: change a Layer-2 knob at
run time.** `HAL_UARTEx_SetRxFifoThreshold()`, `HAL_UARTEx_SetTxFifoThreshold()`
and `HAL_RS485Ex_Init()`'s assertion times can be called at any moment on a live
port. Under this design they are template arguments, fixed when the image is
built. A product that must retune its DE lead time from a field-settable
parameter has exactly one route: `alloy::dev::`, and it will be told so by a
compiler rather than discovering it.

That is a real capability, deliberately traded, and the trade is: everything that
*is* expressible is checked, and everything unused costs nothing. If a knob turns
out to need runtime change often enough, the answer is to promote it to Layer 1
— where it is a runtime field, honoured by every driver — not to make Layer 2
dynamic.

Three smaller give-ups, named so nobody rediscovers them as bugs:

- **Layer 2 is not portable by construction**, only by discipline. The naming
  rule is a review rule; the gate that would enforce it is not built.
- **A peripheral cannot be chosen at run time.** `Inst` is a type, where a
  vendor HAL's `huart` is a pointer. Already true today; this decision does not
  change it, and does not intend to.
- **Auto-baud detection** — hardware writing config back into the driver — has
  no home in this shape yet. It is a read-back path, not a knob.

---

## Migration

Nothing here is a breaking change under [stability.md](stability.md), and the
one Tier-1 name that has to go gets its window.

| Surface | What happens |
|---|---|
| `alloy::uart::config` | gains three fields **with defaults**. All 59 `::open(` sites across the 48 examples compile unchanged — 45 of them `board::debug_uart::open`, the rest other peripherals' `open()` that this change must not disturb either. MINOR. |
| `alloy::uart::bind` | gains a defaulted variadic tail for pin-bearing tags (`de<>`, `rts<>`, `cts<>`). Existing four-argument binds keep working. MINOR. |
| `alloy::uart::opts<Inst>` | new. MINOR. |
| `Inst::feat::*` | new, generated. Its *contents* are the chip database's promise, exactly like `alloy::dev::` — pin `[devices]`. |
| `alloy::uart::serial_config` | **Deprecated.** It is a public alias in a Tier-1 header, so it keeps working for at least one MINOR and is removed no earlier than the next MAJOR. `CHANGELOG.md` names the replacement. |
| `bind::reconfigure(serial_config)` | **Deprecated** alongside it; `reconfigure<Opts>(config)` replaces it. |
| `board::caps::*` | unchanged, forever. Roles only. |
| `products/*.toml` | gains `[ports.<role>]`, additive. |
| `docs/reference/stability.md` | gains a Layer-2 row: public *name*, IP-shaped *contents*. Additive. |
| `alloy::uart::frame` | new — Layer 1 minus the rate, the argument type of `open_checked`. MINOR. |
| `bind::open_checked<Baud>(…)` | **its parameter type changes** from `config` to `frame`. Tier 1, so this is the one row here that is not purely additive. It breaks exactly one call shape — `open_checked<A>({.baud = B})` — which was the silently-wrong one; every other spelling, including `open_checked<A>()` and `open_checked<A>({.parity = …})`, compiles unchanged. No in-tree caller exists. Recorded in `CHANGELOG.md` rather than deprecated, because a window in which both spellings work is a window in which the disagreement is still silent. |
| `alloy::claim::*`, `alloy::trap_code` | new, in `alloy/core/claim.hpp`. MINOR. Not `detail`: a facade outside the tree needs `claim::exclusive` to participate in instance ownership. |

The null-role stubs in `emit/board.py` need the same template shape as the real
binds, or `if constexpr (board::caps::debug_uart)` code stops compiling on boards
without the role — guard #6.

---

## First: UART

Convert UART first, and convert it completely, before driver 1 of the remaining
28.

1. **It is the only peripheral where this fixes a live defect** rather than
   green-fielding one. `serial_config`'s nine unhonoured fields are in `src/`
   now; every driver written against the current shape replicates them.
2. **It is the only peripheral that can falsify the design immediately.** Six
   drivers, four vendors, already in the tree. The naming rule is a claim about
   cross-vendor field names, and UART is where the claim is cheap to test.
3. **`debug_uart` is in every board's role schema**, so the migration touches the
   widest blast radius first — where a mistake is cheapest to notice.
4. **Half of it is already proven.** `open_checked` / `achieved_baud` /
   `rate_check` is the degree-negotiation half, working, in production, and
   needs generalising rather than inventing.

**The falsification test, to run before the schema is frozen:** implement
`opts<Inst>` for `st_usart_v3` **and** `microchip_usart_v1`, both with RS-485 DE.
If `de_assert_16ths` does not survive to Microchip with the same meaning and the
same unit, the naming rule is wrong and the honest fallback is that Layer 2 is a
*description* rather than a contract — the three-layer naming still holds, but
`libs/` may not depend on Layer-2 field names. Better to learn that at driver 2
than at driver 22.

---

## What the code changed about this page

Everything above was decided before a line of it was written. UART has now been
converted — six drivers, four vendors, the facade, the binder, the emitter, the
chip data and a portable example — and five claims on this page did not survive
contact with the tree. They are recorded here rather than quietly edited out,
because which claims break is the useful part.

### 1. Word length is not a Layer-1 field, and `stop_bits` has two values, not four

The admission test as written asks about a FIELD. The real unit is the
(field, **value domain**) pair, and word length is where that bites. Read off
the register data, the character-length domains are:

| driver | data bits it can program | why |
|---|---|---|
| `st_usart_v2` | 8, 9 | one `M` bit, no `M1` |
| `st_usart_v3` / `st_usart_v4` | 7, 8, 9 | `M1:M0` |
| `microchip_usart_v1` | 5, 6, 7, 8 | `MR.CHRL` is 2 bits |
| `raspberrypi_uart_pl011` | 5, 6, 7, 8 | `LCR_H.WLEN` is 2 bits |
| `espressif_uart_v1` | — | ROM-configured; alloy programs no character length |

The intersection is **{8}**. A Layer-1 `word_len` would therefore be a runtime
field that some driver must trap on, reject, or ignore for most of its values —
the exact defect being removed. So data bits is a **Layer-2** knob named
`data_bits` in each driver that has one, with that IP's own domain enforced by
a `static_assert`. `stop_bits::half` and `stop_bits::one_and_half` fail the same
way (ST smartcard/IrDA encodings that no other vendor has) and are gone.

**This is the rule working, not the rule failing.** Teaching six drivers is
what Layer 1 costs, and two of the four fields could not be taught.

### 2. Layer 2 is a DESCRIPTION, not a contract — the falsification test came back negative

The test this page named was: implement RS-485 DE on `st_usart_v3` **and**
`microchip_usart_v1`, and see whether `de_assert_16ths` survives with the same
meaning and unit. **It does not, and the reason is silicon rather than naming.**
The SAM USART has no DE assert/deassert time at all — RS-485 mode drives RTS
around the frame automatically and the only tunable is `US_TTGR`, a transmitter
time *guard* in whole bit periods. There is no name under which the ST knob
could appear on Microchip.

So the honest fallback this page pre-committed to is the one in force:
`libs/` code may probe a Layer-2 member by name, but it may not assume a
feature present on one vendor appears under any name on another. The
three-layer naming still holds.

### 3. `usart_v4` has *fewer* Layer-2 knobs than `usart_v3`, which is backwards from the sketch

The sketch above hung DE and inversion on `uart_opts<usart_v4>`. The G0 silicon
does have them — and alloy's curated `usart_v4` register data does **not**
(`usart_v3.yaml` carries `DEAT/DEDT/DEM/DEP` and `TXINV/RXINV/SWAP`;
`usart_v4.yaml` carries none of them, and does carry `FIFOEN`, which v3 lacks).
Declaring Layer 2 beside the driver is what forced this to surface: a knob can
only be a member where a generated register field backs it. The consequence is
stated by a compiler, not a README —

```
error: static assertion failed: this USART's driver has no hardware
driver-enable: alloy's curated usart_v4 register data carries no DEM/DEAT/DEDT
(the silicon has them; the database has not mined them). Drive DE from a GPIO,
or reach the registers through alloy::dev::
```

— and mining those fields into `alloy-devices` is now a data task with a
compile error pointing at it.

### 4. A maximum programmable value should come from the register data, not from `feat`

This page proposed `Inst::feat::de_time_max_16ths = 31`. It is not needed and
it should not exist: the generated field accessor already knows its own width,
so the driver writes

```cpp
static_assert(O.de_assert_16ths <= IP::deat.raw_mask,
              "DE assertion time exceeds this USART's DEAT field width");
```

and GCC prints `the comparison reduces to '(40 <= 31)'`. One fewer number in
the chip database, one fewer thing that can disagree with the registers.
`feat` keeps what a register map genuinely cannot state — a FIFO **depth**, a
transfer cap, a resolution.

### 5. One combination neither layer can reject

Layer 1 is runtime and Layer 2 is compile-time, so `opts{.data_bits = 9}` plus
a runtime `parity::even` asks an ST USART for a 10-bit word. No `static_assert`
can see a runtime parity, and silently programming 8 bits would be the lie.
The drivers `__builtin_trap()`, guarded by `if constexpr (O.data_bits == 9)`
so it costs nothing otherwise — the same honest runtime guard the double-open
check already uses. **A mixed compile-time/runtime surface has a seam, and this
is where it is.**

Measured on the G0B1RE: with a compile-time-constant parity the trap folds,
GCC merges it into the double-open `udf #255`, and everything after `open()`
is deleted as unreachable — including the string the program meant to print.
So the build is silent, the binary is *correct* (it refuses), and the failure
arrives at run time on the first boot. No warning is emitted.

### 6. The holes, three now closed {#the-three-holes-two-now-closed}

Found by trying combinations this page did not anticipate, all on the G0B1RE
unless noted. None of them was a new defect introduced by the layers — they
were the shape's blind spots. Three are closed in code; the last is the seam
above and stays.

**(A2) is here because the repair for (A) was itself incomplete**, and it was
found the same way the holes were: by asking the shipped code, one facade at a
time, whether it actually had the claim the page said it had. Five of twelve
facades did, and one of two scopes.

**(A) The double-open guard was per *binder type*, not per instance. CLOSED.**
Two different `uart::bind<>` specialisations naming the same `usart2_t` — a
second, legitimately-routed pin pair — each carried their own `detail_opened`.
Opening both compiled clean, with *contradictory* Layer 1 and Layer 2
(`data_bits = 8` then `7`; `115'200/none` then `9'600/odd`), no error and no
warning; the second `open()` reprogrammed the peripheral under the first
handle, which stayed usable. Layer 2 makes a *call site* impossible to lie in
and said nothing about **who owns the instance**.

The flag lives on the instance now — `alloy::claim::owner<Inst>`, an inline
variable template, so one object per instance across the whole image — and it
carries the [personality](#personalities-a-block-runs-in-one-mode-at-a-time),
so the same mechanism refuses `pwm` and `encoder` on one timer. `uart`, `i2c`,
`spi` and `adc` claim exclusively; `pwm` claims *shared* with the block-scoped
frequency as a witness, because four channels on one timer are legitimate and
four channels asking for different frequencies are not. `reconfigure<Opts>()`
grew the two checks it was missing: it refuses a port nobody opened, and it
refuses `Opts` that disagree with the ones `open()` programmed — Layer 2
belongs to the port now, not to the call. Proven by seven death tests in
`tests/test_claim.cpp`, and at generation time by
`emit/board.py::_check_role_personalities` plus its three tests in
`tools/alloy/tests/test_emit.py`.

*Measured cost:* **+8 bytes of `.text`** on every ARM board's `uart_echo`
(1812→1820 on the G071RB and the same delta on six more), **all of it in the
cold path** — the disassembly's success path is the identical
`ldrb / cmp / beq` it was before, and the eight bytes are the second comparison
and the `movs r3, #2` that distinguishes the two failures. RAM is unchanged for
an exclusive claim: one byte per instance where it was one byte per binder.

**(A2) …and that repair was itself keyed one level too high. NOW CLOSED.**
The paragraph above described five of alloy's twelve facades and one of the
defect's two scopes, and the identical bug was still live in shipped code where
it was not looking. `pwm::bind` kept its own `static bool
detail_channel_opened` for the CHANNEL — and `bind` is templated on the pin, so
one channel had one flag per route. On the `nucleo_g0b1re`, TIM2 channel 1 is
routed to PA0, PA5, PA15 and PC4:

```cpp
auto a = board::led_pwm::open({.freq_hz = 1'000});      // TIM2_CH1 via PA5
using other = alloy::pwm::bind<alloy::dev::tim2_t, 1u, alloy::dev::pa0_t,
                               alloy::signal::ch1, board::clock_profile>;
auto b = other::open({.freq_hz = 1'000});               // TIM2_CH1 via PA0
```

Both compiled, both opened, both muxed their pin onto one output compare, and
the `claim::shared` block claim admitted them because they agreed on the
frequency — which they do. Built for `nucleo_g0b1re` and booted in Renode, the
pre-fix image printed its "second bind also opened" banner **in 1.4 s**.

The key is now `sub_owner<Inst, Sub>` — instance and ordinal, nothing else
([two keys](#two-keys-because-a-peripheral-is-not-always-the-contested-resource))
— and the same claim replaced `dma::channel`'s private `claimed_`, which was
correctly scoped but was a *third* ownership mechanism whose bare
`__builtin_trap()` a fault report could not tell from the transfer-size traps
in the same class. `wdt::start` gained the `shared` claim it never had (two
`start()` calls with different timeouts silently kept the last one), and `dac`,
`can` and `rtc` claim their personality; `flash` and `gpio` deliberately claim
nothing, for the reasons in
[which facade claims what](#which-facade-claims-what).

*Measured on `nucleo_g0b1re`, `arm-none-eabi-g++ 14.2.1 -Os`:*

| Example | `.text` | `.bss` | why |
|---|---|---|---|
| `blink`, `hello`, `uart_echo`, `nvm`, `fs`, `adc_read`, `i2c_read`, `spi_read`, `gpio_bus` | **unchanged** | unchanged | nothing they do claims anything new |
| `pwm_fade` | **unchanged** (3652) | **−8** (3256→3248) | a per-binder `bool` became a per-(instance, channel) byte |
| `dma_uart` / `dma_probe` | +8 / +36 | unchanged | the trap codes that tell the two DMA failures apart |
| `watchdog` | +48 | +8 | a claim the facade did not have at all |
| `dac` / `can` / `rtc` | +44 / +40 / +44 | +8 each | ditto — and these buy personality only |

The nine unchanged examples are the control: code that claims nothing pays
nothing. The `pwm_fade` row is the fix's own row — strictly better coverage at
strictly negative cost, because the flag it deleted was bigger than the one it
added.

**One half of this scope IS a compile error, and it was missing.** `pwm::bind`
states the channel twice — as the ordinal the driver programs and the claim
owns, and as the route signal that picks the pin mux — and nothing tied them
together. `bind<tim2_t, 2u, pa5_t, signal::ch1, …>` muxed PA5 onto CH1 while
programming and owning CH2, so two binds genuinely contesting CH1 would hold
two different keys and the runtime claim would never see them. That is a
compile-time fact and is now a `static_assert`, with an eighth case in
`scripts/check_compile_errors.py` holding the diagnostic.

*What is and is not proven.* Seven more tests in `tests/test_claim.cpp` — three
of them death tests, four asserting the claims that must be *admitted* — cover
the new scope and the watchdog, with a negative control confirming the
fork-based idiom reports *false* when nothing traps and *false* for a single,
legitimate claim. The refusal itself is read directly out of
the linked image: `sub_exclusive<dev::tim2_t, 1u, personality::pwm>` loads
`sub_owner` and, on a non-zero owner, puts `#7` (`sub_resource_owned`) or `#2`
(`personality_conflict`) in `r3` before the shared `udf #255`. The **defect**
is proven on emulated silicon (the 1.4 s above); the **trap** is not, and
cannot be with this tooling — Renode does not vector an undefined instruction
on this platform, so the fixed image wedges at the `udf` instead of reaching a
fault handler, which is the same limitation `crash_report.robot` documents for
organic faults. A wedge is not a pass and is not reported as one.

<a id="a3-the-exti-line"></a>
**(A3) …and the scope the repair invented was itself derived from two examples.
NOW CLOSED.**
(A2) closed the sub-resource scope and, in the same commit, wrote down what a
sub-resource *is*: *"There is deliberately no `sub_shared`. A sub-resource is a
sub-resource because it has no block-scoped register for two claimants to
disagree about."* Both sub-resources alloy had — a timer channel and a DMA
channel — agreed with that. **The EXTI line does not**, and it was in the tree
the whole time.

An EXTI line has its own port-select field, its own trigger pair, its own
pending bits and its own callback slot, and the claimants competing for it are
*pins*: on every ST part the same index on every port is one line. PA5 and PB5
are EXTI line 5. Nothing claimed it, so:

```cpp
const alloy::gpio::input<alloy::dev::pa5_t> a{};
const alloy::gpio::input<alloy::dev::pb5_t> b{};
a.on_edge(alloy::gpio::edge::rising, cb_a);
b.on_edge(alloy::gpio::edge::rising, cb_b);   // takes line 5 from a
```

Both compiled, both "armed", and the *first* handle was left in the worst
possible state — not dead, **wrong**. Built for `nucleo_g071rb` and booted in
Renode, driving a rising edge on **PA5** produced nothing (`a_edges=00000000`),
and driving one on **PB5** made `a.edges()` answer `00000001`: PA5's handle
reporting PB5's edge as its own, because the counter is keyed on the line.
`cb_a` never ran again. **9.1 s**, exact counters asserted, no polling anywhere.

The key is `sub_owner<exti_t, Line>` with `sub_witness<exti_t, Line>` holding
the **port index**: re-arming one pin is admitted (the driver documents that it
replaces the edge and the callback), a second pin traps `sub_config_conflict`.
The claim lives in `hal/exti/exti_impl.hpp` and not in `gpio.hpp`, because the
line number only exists down there — `gpio::input` knows about a pin, and a pin
is [still an unmodelled scope](#which-facade-claims-what).

**And it needed a release, which is the second thing this feature refuted.**
"Claims are never released" was true of every claim made by an `open()` with no
`close()`. `clear_on_edge()` is a shipped call that genuinely frees a line, so
refusing PB5 forever because PA5 had once been armed would have replaced a
silent wrong answer with a loud wrong one. `claim::sub_release<Inst, Sub, P>(w)`
is the one release in the mechanism, offered at the sub scope only — and it
catches the mirror-image bug on the way: `pb5.clear_on_edge()` while PA5 holds
line 5 used to silently disarm PA5's interrupt, and now traps.

*What is and is not proven.* The **defect** is proven on emulated silicon —
`exti_probe`, the counters above. The **handover** is proven the same way and
positively: PA5 arms, releases, PB5 arms, and PB5's edge fires
(`cb_a=00000000 cb_b=00000001`) in 7.8 s, so the claim does not over-refuse.
The **refusal** is read out of the linked image, where
`claim::sub_shared<dev::exti_t, 5u, personality::exti>` loads `sub_owner`,
branches to `movs r3, #2` (`personality_conflict`) on a foreign owner and to
`movs r3, #8` (`sub_config_conflict`) on a disagreeing witness, both into the
shared `udf #255` — plus six host tests in `tests/test_claim.cpp`, three of them
death tests. On silicon the refusal is a **wedge**: the fixed `exti_probe`
never prints its second banner and `renode-test` runs out its 180 s timeout,
because Renode does not vector an undefined instruction on this platform. That
is the same limitation (A2) recorded, and a wedge is still not a pass.

**The inventory is a test now, because prose was what failed twice.** (A2) was
found by hand, by asking each facade whether it had the claim this page said it
had. `tools/alloy/tests/test_claim_surface.py` asks mechanically: it parses the
[table](#which-facade-claims-what), checks that each row's claims are really in
the file it names, and fails if a peripheral class has a driver directory and
no row at all. Negative control: deleting `pwm`'s `sub_exclusive` call fails the
`pwm` row. It has already caught two things it was not written for — a new
`hal/encoder/` directory with no row, and an `adc` that had grown a
sub-resource claim the table did not know about.

**And the claim is now shown to cross translation units, which is the entire
point of it.** Every test written for (A) and (A2) lived in one `.cpp` — the
one arrangement in which an `inline` variable template and the per-binder
`static` it replaced are indistinguishable. `tests/test_claim_tu2.cpp` is a
second TU naming the same instances: a claim made there is `held()` here, and a
second owner across the TU boundary traps, at both scopes.

**(B) Layer 1 admitted values it could not reach. CLOSED.**
`open({.baud = 0})` compiled with no diagnostic; `baud_div()` divided by it,
GCC folded the constant division to unreachable, and the emitted code fell into
the same `udf` as the double-open guard, so the *diagnosis* named the wrong
bug. With a run-time baud there was no fold and no trap at all. And
`open_checked<115'200_baud>({.baud = 9'600})` compiled clean and ran at
115 200, the loser never mentioned.

Both are closed, and not opt-in:
[Layer 1 admits values, not only fields](#layer-1-admits-values-not-only-fields).
Zero bytes for a constant value, 20 bytes for a computed one, a compile error
naming the instance for the literal case, and `.baud` is not a member of the
type `open_checked` takes. Two new cases in
`scripts/check_compile_errors.py` (now 8 with the channel case below, all
green) hold both.

**(C) The compile-time/runtime seam stays open.** Item 5 above — a
compile-time `opts{.data_bits = 9}` plus a runtime `parity::even` — is not
closed and is not closable by either mechanism here: it is the price of a
surface that is deliberately half compile-time. The runtime trap it produces
now carries `trap_code::impossible_config` in a register like the others, which
is a better diagnosis and not a fix.

!!! warning "What the trap codes do and do not buy"
    Each guard puts its own `alloy::trap_code` in a register immediately before
    the trap, so a disassembly or a fault report that stacks the register set
    can tell them apart. **It does not give each guard its own PC**: GCC
    tail-merges the `udf` itself, and measurement on the G071RB confirms it
    does. What is no longer shared is the basic block and the register value —
    which is enough to stop a baud-rate bug reading as a double-open, and less
    than "every guard has its own fault address" would be.

---

## The three counterexamples, answered {#the-three-counterexamples-answered}

The method that produced rule v2: for each feature, write down what a user
should be able to type **first**, then find the rule that produces it. A rule
derived from desired outcomes beats a rule derived from taxonomy — v1 was
derived from UART, which is why it broke on the three peripherals that are not
UART-shaped.

Everything below is on the **STM32G0B1RE**, the lead chip.

### 1. I2C 10-bit addressing

**What the user should write.** The bus is one thing; the devices on it are
not. Two consecutive calls, two address widths:

```cpp
auto bus = board::sensors::open({.speed_hz = 400'000});

std::uint8_t temp[2];
(void)bus.write_read(alloy::i2c::addr7{0x48},  cmd, temp);   // 7-bit part
(void)bus.write_read(alloy::i2c::addr10{0x21F}, cmd, temp);  // 10-bit part
```

**The rule that produces it.** Question 0: `i2c1` is curated as `st/i2c_v2`,
and the data carries `CR2.ADD10` *and* a 10-bit-wide `CR2.SADD` — this is the
one counterexample where the database is **not** the blocker, and the review's
blanket "curation blocks all three" is wrong here. Question 1: no, the handle
keeps every method. Question 2: no. **Question 3: yes** — two consecutive calls
legitimately differ, so it is an argument of the operation, and since the
address is already an argument, the fix is to widen its *type*. `addr7` and
`addr10` are distinct types, so a driver that cannot do 10-bit does not declare
that overload, and the `static_assert` behind it names the instance and the IP.

**Why v1 got it wrong.** v1's question 2 asked "can every driver program an
address width?", answered yes, and produced a Layer-1 `config` field — which is
a property of the *port*, so the second line above would have to reopen the bus
to talk to the second device. The rule had no step for a value that belongs to
an operation. Question 3 is that step.

*Status: specified, not built. `alloy::i2c::config` has one field today and the
handle takes `std::uint8_t addr`. Nothing here is blocked by data.*

### 2. The ADC analog watchdog

**What the user should write.**

```cpp
auto adc = board::adc::open();

// One of this ADC's analog watchdogs. The index is checked against generated
// degree, so watchdog<2> on an IP with one of them is a compile error with
// both numbers in it.
auto wd = adc.watchdog<0>({.low = 300, .high = 3600});
wd.guard(board::vbus_channel);
wd.on_trip(+[](void*) { emergency_stop(); }, nullptr);
```

**The rule that produces it.** Question 1: no — a watchdog adds `count`-style
operations without excluding conversion. **Question 2: yes** — there are three
of them on ST's v3 ADC and one on v2, each with its own enable, thresholds,
channel selection and interrupt, armed and disarmed independently of `open()`.
So it is a sub-resource, and its four parts route
[as tabulated above](#sub-resources-a-thing-inside-the-peripheral-with-its-own-lifetime):
the count is `Inst::feat::analog_watchdogs` (a genuine `feat` — a number the
register map cannot state), the thresholds are the sub-resource's own Layer 1,
the channel selection is an argument, the response is the existing callback
shape.

**Where it actually stops, and this is the part v1 could not say.** Question 0,
row 2. The G0B1RE's `adc1` is curated as `st/adc_v2`, and that IP's register
map runs `ISR, IER, CR, CFGR1, CFGR2, SMPR, CHSELR, DR, CCR` — **there is no
`TR1` and no `AWD1CR` at all**, and `ISR` carries no `AWD` flag. So this is not
"Layer 2 or Layer 3": Layer 2 cannot name a field, and Layer 3's typed route
cannot name the *register* either, because `IP::regs` has no member for it. The
rule's output is **curate `adc_v2.yaml` first** — then question 7 answers, and
the driver becomes ordinary work.

**Why v1 got it wrong.** It half-decided (correctly identifying the count as
degree), improvised the rest of the decomposition, and then routed the
remainder to a Layer-3 door it had asserted was always open. The door is
locked, and question 0 is what says so before anyone walks into it.

### 3. Timer encoder mode

**What the user should write.** The personality is chosen by naming a different
binder, not by setting a field:

```cpp
using Enc = alloy::encoder::bind<alloy::dev::tim3_t,
                                 alloy::encoder::a<alloy::dev::pa6_t>,
                                 alloy::encoder::b<alloy::dev::pa7_t>>;

auto enc = Enc::open({.period = 1'440});
std::uint32_t pos = enc.count();     // 0 .. period-1, wraps in hardware
std::int32_t  moved = enc.delta();   // signed, wrap-aware, since last call
```

!!! note "This block was predicted with a clock profile and `counts_per_rev`. The shipped code has neither."
    It was then BUILT (`src/alloy/encoder.hpp`, `src/alloy/hal/encoder/`,
    `examples/encoder/`), and two of the three lines above changed:

    * **no `Clock` parameter.** Every other binder takes one because its
      peripheral divides a kernel clock to reach the rate asked for. An
      encoder divides nothing — the driver forces `PSC = 0` because the
      counter is clocked by the shaft — so a `Clock` here would have been a
      dependency the peripheral does not have, and the sort that survives for
      years because nothing ever breaks. The one thing that would bring it
      back is the input filter, whose sampling rate IS derived from the
      kernel clock, and that field is not curatable (below).
    * **`period`, not `counts_per_rev`.** The field is a counter modulo. Its
      most common value is the encoder's counts per revolution, and naming it
      after the most common use would have made a linear axis with a 10 000
      count travel read as a lie.

    `count()` is unsigned because the hardware counter is; the signed number
    a caller actually wants is `delta()`, which is a separate operation
    because it consumes state (see hole (D) — "how far since I last looked"
    is a property of the looker, so it lives in the handle).

and if the same program also opens `alloy::pwm::bind<alloy::dev::tim3_t, …>`,
the second `open()` traps with `trap_code::personality_conflict` — because both
claim `tim3_t` and `pwm != encoder`. If the two came from *board roles* instead
of hand-written binds, `alloy build` refuses the board file and never starts a
compiler.

**The rule that produces it.** **Question 1: yes.** The handle loses
`set_duty()` and gains `count()`; the two modes cannot be on at once (`SMS`
selects one). So it is a personality: a different binder type, an exclusive
claim. The pins are question 4 — `encoder::a<>` / `encoder::b<>` are binder
tags on the `ch1`/`ch2` routes, which the G0B1RE's route table already carries.

**Where it actually stops.** Question 0, row 3. `tim3` is curated as
`st/tim_gp16`, whose `SMCR` **is** a register member but has **zero fields** —
no `SMS` — and whose `CCMR1` carries only the output-compare fields
(`OC1M/OC1PE/OC2M/OC2PE`), with no `CC1S/CC2S` input selection. A driver could
reach `SMCR` through `IP::regs` and write `3` into bits 2:0 by hand, which is
exactly the hex-literal-silicon-fact that guard #1 exists to forbid inside
`src/`. So: **mine `SMCR.SMS` and `CCMR1.CC1S/CC2S` into `tim_gp16.yaml`
first.** (`tim1`, the advanced timer, is at row 1 — uncurated, no
`alloy::dev::tim1_t` at all.)

That mining was done, and it hit two walls the layer question cannot see,
both now recorded in `registers/st/tim_gp16.yaml` itself:

* **`SMS` is one field in two places** — `SMS[2:0]` at bits 2:0 and `SMS[3]`
  at bit 16 — and a field in `alloy.registers.v1` has one `bit` and one
  `width`. It is curated as two entries named after the manual's own diagram,
  because both single-entry spellings lie (one claims bits SMS does not own,
  the other hides a bit it does).
* **`CCMR1` is two registers at one address**, and which one it is depends on
  a field inside it. The input view's `ICxF` (bits 7:4 / 15:12) sits exactly
  on top of the output view's `OCxPE`/`OCxM`, fields of one register may not
  overlap, so the digital input filter is **unreachable at every layer at
  once — `alloy::dev::` included**, because Layer 3 gives a named accessor
  only to a curated field. Not a layering decision; a limit of the data
  model. A bouncing mechanical encoder will feel it.

**Why v1 got it wrong.** It decided in one step — "needs a pin mux → binder
tag" — which conflates the pins with the mode. The pins *are* binder tags and
that part was right; what it missed is that encoder mode **excludes PWM on the
same block**, and v1 had no category for a mutually exclusive whole-block mode.
"A different IP tag, therefore a different driver" (v1's other attempt, in the
degree section) cannot express it either: `tim3` has one IP tag and two
possible personalities.

---

## Stressing v2 on three features it was not built from {#stressing-v2-on-three-features-it-was-not-built-from}

A rule tested only on the counterexamples that produced it has been fitted, not
validated. Three more, chosen because they look awkward. **One survives with a
refinement; two break v2**, and the breaks are recorded here rather than
smoothed over.

### 1. SPI slave mode — survives, with one refinement

Question 1 fires: the operations change (a slave cannot initiate; `xfer()`
means "load TX and wait to be clocked", with failure modes a master does not
have), the modes are exclusive (`CR1.MSTR` selects one), and **every pin
reverses direction** — MISO becomes an output, MOSI and SCK become inputs, NSS
becomes an input. → personality: `alloy::spi::slave::bind<…>`, exclusive claim.

Two things v2 had to be sharpened to say, both found here:

- **Layer 1 is shared where the wire meaning is shared, and trimmed where it is
  not.** CPOL/CPHA mean the same thing in both personalities and stay one type;
  `clock_hz` is meaningless to a slave, so `spi::slave::config` does not have
  it — the `uart::frame` shape, for the same reason.
- **The personality vocabulary is one enumerator per FACADE, not per bus.**
  `claim::personality::spi` cannot cover both, or a master bind and a slave
  bind on one instance would agree and neither would trap. `spi_slave` becomes
  its own enumerator the day the facade lands. This is written into
  `claim.hpp`'s comment so the next facade does not get it wrong.

### 2. ADC injected channels — breaks v2

Routing is easy and v2 gets it right: question 1 no (it adds operations without
excluding regular conversion), **question 2 yes** — four injected ranks, their
own trigger, their own `JDR` result registers, their own end-of-conversion,
armed independently. A sub-resource, count from `Inst::feat::injected_ranks`,
0 where the IP has none.

**Where it breaks: injected conversions PREEMPT regular ones.** v2 gives each
sub-resource its own handle and has **no vocabulary at all for sub-resources
that interact**. After `adc.injected<0>(…)` is armed, a plain `adc.read(ch)`
can be delayed or aborted by a trigger that is nowhere in its call, and nothing
in the type system, the `feat` numbers or the claim says so. The same hole
appears for two analog watchdogs guarding overlapping channels, and for a DMA
channel shared between a timer's update and its capture events.

This is a **genuine gap in v2**, not a data problem: curating the registers
would not fix it. It is named in
[what v2 does not decide](#what-v2-deliberately-does-not-decide).

*(On the lead chip the question is theoretical twice over: the G0B1RE's
curated `adc_v2` map has no injected registers, and the F767's `adc1` is
uncurated outright — question 0, row 1.)*

### 3. Timer complementary outputs with dead time — breaks v2 {#3-timer-complementary-outputs-with-dead-time-breaks-v2}

Question 0 answers first and answers row 1: complementary outputs need an
advanced timer, `tim1` on the G0B1RE is uncurated, and there is no
`alloy::dev::tim1_t`. Suppose it were curated. Then v2 routes the parts
cleanly — CH1N is a pin (question 4, a binder tag `pwm::outn<>`), the break
input is a pin (`pwm::brk<>`), and the dead time is a register field in a
vendor-shaped unit (`BDTR.DTG`'s piecewise encoding), so question 7, Layer 2.

**Where it breaks: dead time is BLOCK-scoped and `pwm::bind` is CHANNEL-scoped.**
Two channel binds on one timer, each carrying
`opts{.dead_time_ns = …}` with different values, is hole (A) in a new dress —
and the fix that closed (A) does **not** catch it. `claim::shared`'s witness is
the block-scoped *Layer-1* value (`freq_hz`); a block-scoped *Layer-2* value is
not in it, because no shipped timer driver has one yet.

Two candidate repairs, and v2 does not choose between them:

1. widen the witness to cover the block-scoped `opts` as well — cheap, and it
   turns the conflict into a trap rather than preventing it;
2. give the timer a **block handle** that channels are opened *from*
   (`auto tim = board::motor::open<opts{...}>({.freq_hz = 20'000}); auto a = tim.channel<1>();`)
   — structurally right, makes the conflict untypeable, and is a breaking
   change to `alloy::pwm`.

Naming the choice is a maintainer's call, and the argument for (2) is that the
*same* structural defect is live in shipped code today for `freq_hz` — the
claim makes it loud, it does not make it impossible.

---

## What v2 deliberately does not decide {#what-v2-deliberately-does-not-decide}

A rule that needs a human argument per peripheral is not a rule. A rule that
pretends to decide what it cannot is worse — that is exactly how v1 produced a
confident wrong answer on I2C. So these two are stated as **questions the page
asks**, not answers it guesses, and each one is a decision record a maintainer
owes before the driver that needs it is written.

**1. Sub-resources that interact.** v2 says where a sub-resource lives and says
nothing about what happens when two of them contend for the same silicon — ADC
injected preempting regular, two analog watchdogs on overlapping channels, one
DMA channel serving a timer's update and its capture. Every option is bad in a
different way: a runtime claim (like `alloy::claim`, but per sub-resource, and
paying RAM per instance), a type-level exclusion (unusable once the set is
three or more), or documenting the interaction and checking nothing.

> **The question the page asks:** *does arming this sub-resource change the
> observable behaviour of an operation whose call site does not mention it?* If
> yes, the sub-resource may not be added until the answer to "and what tells
> the user" is written down. There is no default.

**2. A "mode" whose operations do not change shape.** Question 1 is mechanical
when the handle gains or loses methods — PWM versus encoder, master versus
slave. It is **not** mechanical when both modes offer the same calls with
different meanings: a timer counting up versus down, an ADC in single versus
continuous mode, a UART in half-duplex versus full-duplex on one wire. Calling
each of those a personality would multiply facades until the framework is
unreadable; calling each of them a knob puts mutually exclusive silicon states
behind a field.

> **The question the page asks:** *can the two modes be swapped on a live block
> without re-binding a pin and without invalidating a handle already handed
> out?* If yes it is a knob; if no it is a personality. The tie-breaker is not
> mechanical enough to be a test — a maintainer states the answer, in the
> driver, with the reason, and the reason is the direction the pins point.

**What is NOT on this list, deliberately.** "Which layer" is decidable, and so
is "is the database ready" — question 0 is four rows of fact, each one readable
straight out of `alloy-devices`. If a future feature makes either of those
require a judgement call, that is a defect in the rule and belongs here, not in
a driver's comment.

---

## What it actually cost

Measured, not modelled: `examples/uart_echo` — the example that configures
nothing but a baud rate — built for every board before and after, same
toolchain (`arm-none-eabi-g++ 14.2.1` xPack for the Cortex-M boards), `.text`
of the linked image in bytes.

| board | UART driver | before | after | Δ |
|---|---|---|---|---|
| nucleo_g071rb | `st_usart_v4` | 1812 | 1812 | **0** |
| nucleo_g0b1re | `st_usart_v4` | 2736 | 2736 | **0** |
| same70_xplained | `microchip_usart_v1` | 1944 | 1944 | **0** |
| esp32_devkit | `espressif_uart_v1` | 2641 | 2641 | **0** |
| esp_wrover_kit | `espressif_uart_v1` | 2641 | 2641 | **0** |
| nucleo_f722ze | `st_usart_v3` | 2284 | 2120 | **−164** |
| nucleo_f767zi | `st_usart_v3` | 2428 | 2264 | **−164** |
| raspberry_pi_pico | `raspberrypi_uart_pl011` | 2360 | 2280 | **−80** |
| rp2040_zero | `raspberrypi_uart_pl011` | 2420 | 2340 | **−80** |

Nothing grew. Five boards are byte-identical, and the two ST F7 boards each
lost 164 bytes while *gaining* parity, stop bits, word length, line inversion,
RX/TX swap and RS-485 DE timing. The −164 is not the abstraction paying for
itself; it is the old shape being wasteful: `st_usart_v3::enable()` used to
call `configure(serial_config{})` and program all nine fields
unconditionally, including seven nobody asked for. The driver-authoring rule
this page states — **`enable()` may assume reset state and must guard every
default-valued write** — is what deleted them.

The PL011's −80 has the same cause in miniature and one caveat worth stating:
the writes are the same in both versions for a default config, so part of that
delta is inlining shape rather than deleted stores. It was not disassembled.

### What a *deep* configuration costs

The table above is the example that configures nothing. Here is the other end,
measured the same way in the same tree: `uart_echo` with **Layer 1 in use**
(`parity::even` + `stop_bits::two`), and then with **Layer 2 on top** — every
knob the IP offers, set away from its default (`data_bits`, `invert_tx`,
`invert_rx`, `swap_rx_tx`, `de_*_16ths`, `fifo_enable`), selected by the same
`requires`-concept idiom `examples/uart_frame` uses. Δ is against the *after*
column above, so it is the price of asking, not the price of the redesign.

| board | driver | default | +Layer 1 | +Layer 1 & 2 |
|---|---|---|---|---|
| nucleo_g071rb | `st_usart_v4` | 1812 | 1848 (**+36**) | 1852 (**+40**) |
| nucleo_g0b1re | `st_usart_v4` | 2736 | 2772 (**+36**) | 2776 (**+40**) |
| nucleo_f767zi | `st_usart_v3` | 2264 | 2304 (**+40**) | 2328 (**+64**) |
| raspberry_pi_pico | `raspberrypi_uart_pl011` | 2280 | 2304 (**+24**) | 2308 (**+28**) |
| same70_xplained | `microchip_usart_v1` | 1944 | 1944 (**0**) | 1944 (**0**) |
| esp32_devkit | `espressif_uart_v1` | 2641 | 2689 (**+48**) | 2689 (**+48**) |

Two things to read off it. A full non-default frame costs **24–64 bytes** —
the bound the "unused features cost nothing" claim needed, since a claim about
zero is only half an answer without the other end. And the SAM's **0** is not
a rounding artefact: `microchip_usart_v1` composes the whole of `MR` in one
unconditional store, so the fields were always being written and asking for
different values moves no code. The guarded-write rule buys nothing there,
and costs nothing either.

**Not measured:** compile time. The reduction figures earlier on this page
stand as they were labelled — a probe, not a build.

### What revision 2 cost on top of that

Instance ownership plus Layer-1 value admission, measured the same way:
`examples/uart_echo` built from a clean `git archive` of `aa95f94` (with
`alloy-devices` at `70b288f`) and from this tree, same toolchain, `.text` of
the linked image.

| board | UART driver | before | after | Δ |
|---|---|---|---|---|
| nucleo_g071rb | `st_usart_v4` | 1812 | 1820 | **+8** |
| nucleo_g0b1re | `st_usart_v4` | 2736 | 2744 | **+8** |
| nucleo_f722ze | `st_usart_v3` | 2120 | 2128 | **+8** |
| nucleo_f767zi | `st_usart_v3` | 2264 | 2272 | **+8** |
| raspberry_pi_pico | `raspberrypi_uart_pl011` | 2280 | 2288 | **+8** |
| rp2040_zero | `raspberrypi_uart_pl011` | 2340 | 2348 | **+8** |
| same70_xplained | `microchip_usart_v1` | 1944 | 1948 | **+4** |

Eight bytes on six boards across three IP versions and two vendors, because the
code added is the same everywhere. The SAM's four is not a rounding artefact:
its disassembly folds the second failure path into a Thumb-2 `it ne` /
`movne r3, #2` immediately before a single `udf`, so the branch and the second
trap disappear. Same source, same rule, half the cold path — checked, because
the alternative was guessing.

The G071RB (Cortex-M0+, no IT blocks) is the other end and says where the eight
bytes went:

```
  before                          after
  ldrb r3, [r3, #0]               ldrb r3, [r3, #0]
  cmp  r3, #0                     cmp  r3, #0
  beq  ok                         beq  ok
  udf  #255                       cmp  r3, #1        <- same personality?
                                  bne  .Lconflict
                                  udf  #255
                          .Lconflict:
                                  movs r3, #2        <- trap_code in a register
                                  b    .-4
```

The success path is byte-for-byte what it was. All eight bytes are in the
branch that traps, and the value-admission check contributes **zero**, because
`board::debug_uart_baud` is a `constexpr` and the whole predicate folds. A port
whose baud comes from a runtime source pays 20 bytes for the check that
previously did not exist.

*One honest wrinkle:* an exclusive claim is one byte of `.bss` per **instance**
where `detail_opened` was one byte per **binder type** — the same for every
program in the tree, and cheaper for any program that binds one instance twice
(which is now refused anyway). `alloy::pwm` pays four more bytes per timer for
the shared claim's witness, since that is the value the channels must agree on.

---

## What is proven, and how

| Claim | Evidence |
|---|---|
| `open({.baud = …})` still compiles everywhere | all 48 examples build; `uart_frame` builds **9 of 9 boards**, `alloy matrix` |
| a knob the IP lacks cannot be typed | `scripts/check_compile_errors.py::check_opts_absent_field` — 16-line error, first line names `uart_opts<alloy::dev::usart2_t>` and `de_assert_16ths` |
| an over-ask is rejected against generated data | `check_opts_over_ask` — `static_assert` + `the comparison reduces to '(40 <= 31)'` |
| a pin-bearing knob is rejected where the IP has none | `check_de_tag_unsupported` — on the G0B1RE, where the DE pin *does* route, so the route check passes and the driver's refusal is what is left |
| degree reaches the image and changes behaviour | `tests/emulation/uart_frame_surface.robot` on **two** boards: the same source prints `rx-fifo: shallow` on the G071 (`feat::rx_fifo_depth == 8`) and `rx-fifo: none` on the F722 (`== 0`). With a **negative control**: run the F722 leg without its `--variable FIFO:` override and it fails in ~102 s, so the two legs genuinely discriminate rather than both matching a line that happens to be printed |
| nothing else in the tree grew | `.text` before/after on six more examples × three boards (`hello`, `dma_uart`, `bootloader_uart`, `modbus_rtu_client`, `irq_echo`, `async_io`): every figure unchanged or smaller, none larger |
| the ports still work | `firmware_boots` + `uart_echo_roundtrip` green under Renode on g071rb, g0b1re, f722ze, same70_xplained |
| a hand-written degree claim cannot creep in | `scripts/check_contract.sh` greps for `struct feat` under `src/` |

### Added at revision 2

| Claim | Evidence |
|---|---|
| a second binder cannot open a port that is already open | `tests/test_claim.cpp::claim_second_binder_on_one_instance_traps` — a **death test**: the child must not exit cleanly. Six more beside it |
| two personalities cannot claim one block | `claim_two_personalities_on_one_block_trap` at run time; `tools/alloy/tests/test_emit.py::test_one_peripheral_in_two_personalities_fails_generation` at generation time, asserting the message names the peripheral **and both personalities** |
| several channels of one timer are still legal | `claim_shared_admits_agreeing_claimants` (three claimants, one witness) and `test_two_roles_may_share_a_peripheral_within_one_personality` (the `nvm` + `fs` shape the `nucleo_g0b1re` board ships) — the negative control on the rule above, without which "refuse everything" would pass |
| two channels of one timer cannot disagree about the block | `claim_shared_refuses_disagreeing_claimants` — the live defect in shipped `alloy::pwm` this found |
| a Layer-1 value no divisor can reach is a compile error | `scripts/check_compile_errors.py::check_baud_zero` — 8 non-blank lines, names `alloy::core::admit::uart_baud`, the sentence, and `Inst = alloy::dev::usart2_t`. Compiled to an **object**, not `-fsyntax-only`: the diagnostic rides on `__builtin_constant_p`, which is the same reason it costs nothing |
| two disagreeing spellings of the baud cannot both be typed | `check_open_checked_conflict` — 13 lines, names `alloy::uart::frame` and `9600` |
| none of it costs a working program anything | `uart_echo` `.text` on seven ARM boards, built from a clean `git archive` of `aa95f94` and from this tree with the same toolchain: **+8 bytes on six, +4 on the SAM**, and the G071RB disassembly shows the success path is byte-identical (`ldrb / cmp / beq`) with every added byte in the branch that traps |
| nothing in the tree stopped building | every example on the CI board×example list, plus `uart_frame`, `crash_report`, `concurrency_probe`, `product_demo` and `product_line`, built on **four boards** (`nucleo_g0b1re`, `nucleo_f767zi`, `raspberry_pi_pico`, `same70_xplained`). The only failures are `escape_hatch`, `factory` and `bootloader_uart` on the boards they were never portable to — **each one verified to fail identically at `aa95f94`**, which is the control that makes the sweep mean anything |
| the guarded `open()` still boots and still moves bytes on emulated silicon | `firmware_boots.robot` green on **`nucleo_g0b1re`, `nucleo_f722ze` and `same70_xplained`** — three families, two vendors — and `uart_echo_roundtrip.robot` green on `nucleo_g071rb`, which injects a line and asserts the firmware echoes it. Run on this host against the native arm64 Renode 1.16.1, on the tree at `1aa512f`. This is the leg the shipped revision said could not be run here; see the box below |
| the claim guard is not a boot-time regression | the G0B1RE image under test is the +8-byte one (`.text` 2744, versus 2736 at `aa95f94`), so the leg that boots is the leg carrying the guard, not a stale artefact |

**Not proven, and worth saying plainly.**

- **No leg asserts that the claim TRAPS on emulated silicon.** The seven
  refusal tests are host-side `fork()` death tests. What the emulation legs
  below prove is the *other* half — that the guard on the success path does not
  break a working port — and those two halves are not the same claim. A leg
  that binds one instance twice on an emulated part and asserts the `udf` is
  reachable-but-not-reached is unwritten.
- **The generator half is proven only against a synthetic board dict**, because
  no shipped `board.json` can express the conflict yet: today's role schema has
  one UART role, one SPI role and one PWM role. The gate is guarding a door
  nothing can currently walk through — it is there for the encoder role, not
  for the boards in the tree.
- **Nothing here was measured on ESP32 or RP2040 hardware, or on any board at
  all.** Every number on this page is a cross-compile or an emulator.

!!! note "One 'not proven' on this page was retired by checking it, and the reason it was written is worth keeping"
    Revision 2 shipped saying *"no emulation evidence at all for this
    revision"*, and blamed the host: *"the local Renode is an x86_64 build whose
    ARM translation library will not load on this arm64 host."* **That was
    false, and the four legs in the table below were run on this machine.**

    The claim was not invented — it was read off the wrong install. There are
    two Renodes here: `/Applications/Renode.app`, which the `renode-test` shell
    alias points at and which *is* the x86_64 build, and
    `~/renode/Renode.app`, a native arm64 1.16.1 (`libllvm-disas.dylib:
    Mach-O 64-bit … arm64`) that `alloy emulate` finds on its own. Checking the
    binary the alias resolved to, and stopping there, is how a true sentence
    about one file became a false sentence about the host.

    The lesson is the cheap one and it is the same one this page keeps
    relearning: **an unproven claim and a claim that could not be proven are
    different, and only one of them is an excuse.** `firmware_boots` on three
    boards and `uart_echo_roundtrip` on a fourth cost four minutes.

**Unproven and labelled:** `espressif_uart_v1` now programs `CONF0` parity and
stop bits, from TRM-derived field positions, with no ESP32 on the bench — and
its two-stop-bit encoding is the known one that needs `UART_RS485_CONF.DL1_EN`
on classic ESP32 silicon, a register alloy does not model. `st_usart_v2`'s
frame programming is likewise compile-checked only (no F4 board), as that
driver already was. Renode models no parity, so no leg asserts a parity bit on
a wire.
