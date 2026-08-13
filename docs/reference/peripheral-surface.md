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

!!! warning "This page is at revision 3, and the two earlier rules were refuted by being applied"
    **v1** was derived from UART and then applied, adversarially, to three
    features it had never seen — I2C 10-bit addressing, the ADC analog watchdog,
    timer encoder mode. It failed all three, confidently, which is worse than
    failing loudly: it asked *"which layer does this knob live in?"* without
    first asking *"is it a knob at all?"*

    **v2** was derived from those three and stressed against features it had
    not seen; the review that retired it reported **four of five broke it**,
    and [three of those trials are recorded below](#stressing-v2-on-three-features-it-was-not-built-from)
    — one survival, two breaks, both breaks still open. The part that survived
    both reviews is **question 0**, because it asks a verifiable fact about the
    chip database rather than a category judgement.

    So the a-priori method was abandoned. **v3 is derived from three
    peripherals that were built** — a cross-peripheral feature
    ([`can`](#question-5-cross-peripheral)), a personality
    ([`encoder`](#personalities-a-block-runs-in-one-mode-at-a-time)) and a
    sub-resource ([`adc`](#sub-resources-a-thing-inside-the-peripheral-with-its-own-lifetime))
    — and is nothing but the generalisation of what those builds demanded.
    Question 0 is kept and now has **five rows**: the FDCAN filters found a
    curated field whose *encoding* was not curated, which is a magic number
    wearing an accessor.

    [**The rule is here**](#the-rule); what it refuses to decide, and what the
    three builds never showed it, is
    [here](#what-v3-does-not-decide-and-what-it-has-not-seen). The refutations
    that produced it are kept as
    [history](#history-how-this-rule-was-refuted-twice), because which claims
    break is the useful part — but the rule is stated in exactly one place.

---

## The decision

The load-bearing one first, because three revisions of this page buried it:

| Question | Answer |
|---|---|
| What actually decides where a feature lives? | **[Question 0](#question-0-what-does-the-database-already-know) plus the reference manual.** Question 0 is mechanical and has answered 5 of 5 in every review; the taxonomy that used to sit here answered 1 of 5 twice and is now a [checklist](#the-checklist-questions-110). |

Everything below is what the built peripherals settled, and holds:

| Question | Answer |
|---|---|
| Where does the portable surface live? | `alloy::<periph>::config` — a runtime struct, frozen at the knobs *every* driver alloy ships programs. UART: three fields. |
| Where does the rest live? | `alloy::<periph>::opts<Inst>` — a compile-time value whose **members are declared per IP version**. A knob the silicon lacks is not a member. |
| How is "less of it, not none of it" expressed? | A generated number on the instance: `Inst::feat::rx_fifo_depth`. **0 means absent.** |
| What happens when you ask for something absent? | Compile error naming the instance, and the field when the IP has any vendor knobs at all. Never ignored, never a runtime error code. |
| What happens when you ask for too much of something present? | `static_assert` inside the driver, comparing against the generated number, with both numbers in the message. |
| What happens when you ask for a value the port cannot reach? | Compile error when the value is a literal, named trap when it is computed. Not opt-in — see [Layer 1 admits values, not only fields](#layer-1-admits-values-not-only-fields). |
| Is there a generic command door? | **No.** Not now, not under schedule pressure. `alloy::dev::` is the only door below `opts`. |
| Is `alloy::dev::` always available? | **No, and v1 said it was.** It reaches only what the chip database curates, at [four separate gates](#question-0-what-does-the-database-already-know) — peripheral, register, field, and the field's *encoding*. |
| What owns a peripheral instance? | Exactly one binder, in exactly one *personality* — enforced at generation time for board roles and at run time for everything else (`alloy::claim`). |
| Where does a feature live when it needs **two blocks**? | On the facade of the block the user names; the second is an edge on the instance descriptor (`Inst::ram_t`). Curation and the capacity check cross the pair — [question 5](#question-5-cross-peripheral). |
| Where does a **maximum** come from? | Whichever artefact physically bounds it — a field's own mask, a `feat` count, or *another peripheral's* element count — with a `static_assert` tying them together when more than one names it ([here](#where-a-maximum-comes-from)). |

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
    "what kind of thing is this value" — and there are five kinds, only one of
    which is a knob on an open port:

    | Kind | Example | Where it goes |
    |---|---|---|
    | a **personality** | timer as PWM vs as encoder | a different **binder**, in its own namespace — and the register data has to name it |
    | a **sub-resource** | one of the ADC's three analog watchdogs | its own **handle**, claimed against a `feat` count |
    | a **per-transfer** value | this transfer's I2C address width | an **argument** of the operation |
    | a **cross-peripheral** feature | a CAN acceptance filter: elements in the companion RAM, size in the controller | the facade of the block the **user names**; the other block is an edge on the descriptor |
    | a **knob** | baud, parity, DE lead time | Layer 1 / `feat` / Layer 2 / Layer 3 |

    v1 had only the last row, so it answered *confidently and wrongly* for the
    first three; v2 added three of them and still had no row for the fourth,
    which is what the FDCAN build found. The rule asks the kind question first,
    and asks it in [five steps](#questions-15-what-kind-of-thing-is-it).

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
    reasoning is in [the history section](#history-how-this-rule-was-refuted-twice).
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
  feat:
    rx_fifo_depth: 8
    tx_fifo_depth: 8
```

```cpp
struct usart1_t {
    using ip = alloy::ip::st::usart_v4;
    // ... base, gate, irq, kernel, dmareq_* (unchanged) ...
    struct feat {
        static constexpr std::uint32_t rx_fifo_depth = 8u;
        static constexpr std::uint32_t tx_fifo_depth = 8u;
    };
};
```

!!! note "This block is the file, not a sketch — and it used to carry two keys that are refuted"
    Earlier revisions of this page printed `de_time_max_16ths: 31` and
    `max_data_bits: 9` in the same `feat` block, with a real chip-file path
    above them. Neither has ever been in `chips/st/stm32g0b1re.yaml`, and
    `de_time_max_16ths` is exactly the answer
    [where a maximum comes from](#where-a-maximum-comes-from) says v1 got
    wrong: the DE assertion-time bound is read from the generated field's own
    `raw_mask`, which is what the `(40 <= 31)` acceptance case in
    `scripts/check_compile_errors.py` pins. A worked example that shows the
    refuted design as though it were the shipped data is how a rule survives
    being wrong, so it is deleted rather than annotated.

Five rules, and they matter more than the syntax:

1. **Zero means absent.** There is no `has_fifo` beside `rx_fifo_depth`. A bool
   and a count that can disagree is a bug class; deleting the bool deletes the
   class. A bool appears only where the fact is genuinely ungraded.
2. **It has two homes.** A number that differs
   between two instances on one die lives on the **instance**, in the chip file
   — `usart1` and `lpuart1` on the G0B1 share an IP tag and differ in
   oversampling and flow control. A number the **IP version** fixes lives on the
   register file, or it is the same integer copied into every chip that names
   the IP and free to drift in any of them: `st/adc_v2` carries
   `analog_watchdogs: 3` once. Both land in `Inst::feat`.
3. **The two homes may not disagree.** The same name in both places with
   *different* values is an error, not an override (`emit/device.py`). An
   override would make the chip file able to contradict the register file
   silently, which is the drift the second home exists to remove.
4. **`feat` numbers are only ever generated.** A hand-written header asserting a
   silicon quantity is guard #1 with a different literal. `check_contract.sh`
   gains one grep: `struct feat` may not appear under `src/`. Concretely,
   `st_i2c_v2.hpp`'s `kMaxNbytes = 255` is a silicon fact hand-written today and
   becomes `Inst::feat::max_transfer_bytes`.
5. **Absence in the data is a lint failure, not a default.** A missing `feat`
   block must fail the plausibility lint, never be read as zero — a generated
   lie is worse than a hand-written one, because nobody reviews it.

!!! danger "Two of those five rules are aspirations, and the tree contradicts both"
    Written down here because a rule stated in the indicative that nothing
    enforces is how v1 and v2 read right for months.

    - **Rule 5 is not implemented, and rule 5 is contradicted by the shipped
      `watchdog_count<Inst>`.** No lint in `alloy-devices` requires a `feat`
      block anywhere; `same70_xplained`'s `afec0_t` is generated with no `feat`
      at all, and `alloy::adc::watchdog_count<afec0_t>` reads that absence as
      **0** — deliberately, because
      [question 6](#questions-610-which-layer-does-the-knob-live-in) needs one
      answer for "not reachable from here". So "never be read as zero" and "one
      answer folds both into 0" are the same page disagreeing with itself. The
      resolution is a maintainer's call and is not made here.
    - **There is a THIRD home, already in the data.** `dma1` on the G0B1RE
      carries `channels: {count: 7, mux_offset: 0, …}`, emitted as
      `Inst::ch_count` — a per-instance degree number in a bespoke key outside
      `feat`, for the tree's *oldest* sub-resource. `st_dma_v1.hpp` bounds
      `setup<Ch>()` against it. It is generated, so it is not guard #1, but
      `check_contract.sh`'s `struct feat` grep cannot see it and rule 3's
      "may not disagree" check does not cover it.

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
below. What survives of v1's paragraph is the data half: `st/tim_gp16` was
under-curated for any of those modes and needed mining before the first timer
driver beyond PWM was written, not after the seventh. **For encoder mode that
mining is done** — `SMCR.SMS` and `CCMR1.CC1S/CC2S` landed in `alloy-devices`
and `f1f6833` consumes them — and it hit two limits the layer question cannot
see, both now [data-model proposals](#what-a-data-model-change-would-have-to-say).
Input capture and dead time are still unmined. Seven of the 28 uncurated
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

!!! note "Three things the encoder build added, because a personality is not only a C++ shape"
    Building the timer's second personality (`f1f6833`) found that the two
    tests above are necessary and not sufficient. In full, with the reasons, in
    [question 1 of the rule](#questions-15-what-kind-of-thing-is-it); in short:

    1. **The register data must DECLARE it.** `class` is single-valued and four
       separate consumers read it as "a block has one job" — codegen, the role
       matcher, `chip-info`, and a fourth copy of the rule found inline in
       `chips.py`. All four ask **membership** now, through `roles.ip_classes`,
       and `alloy.registers.v1` gained an optional `personalities:` list whose
       first user is `st/tim_gp16`. A personality the data does not name cannot
       be reached from a board role at all.
    2. **Program it with whole-register writes.** The bits `encoder::open()`
       does not name in `CCMR1` are the *other* personality's layout of the same
       word — the input view's `ICxPSC/ICxF` are the output view's `OCxPE/OCxM`.
       A read-modify-write carries a PWM mode field into an input-filter
       setting. This is the one place where RMW is the wrong default.
    3. **The binder takes only tags that carry a fact it programs.** This page
       predicted a `Clock` parameter on `encoder::bind` because every other
       binder has one. An encoder divides nothing, so it was dead, and it is
       gone.

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
| `wwdt` | `start` | `shared(window)` | — | **two windows on one WINDOW watchdog**, and it is a SEPARATE row from `wdt` on purpose: the IWDG and the WWDG are two blocks with two contracts, so one enumerator for both would make them agree and neither trap |
| `dac` | `enable` | `shared(0)` | — | personality only (see below) |
| `can` | `enable` | `shared(0)` | — | personality only |
| `rtc` | `set` | `shared(0)` | — | personality only |
| `encoder` | `open` | `exclusive` | — | a timer already generating PWM |
| `tick` | `open` | `exclusive` | — | **a timer already generating PWM, and a second time base on one block.** `exclusive` and not `shared(hz)` like `pwm`: PWM shares a timer because four channels are four legitimate owners of one personality, while a time base has no channels to share — a second opener is not a co-owner asking for the same rate, it is a second program overwriting PSC and ARR |
| `bridge` | `open` | `exclusive` | — | **a timer already generating PWM, or a PWM channel stolen out of a dead-time pair** |
| `exti` (a line) | driver `arm` / `disarm` | — | `sub_shared(port)` | **two pins on one interrupt line** |
| `crc` | `open` | `exclusive` | — | **two owners of one shift register.** Not "last writer wins" — interleaved `update()`s arithmetically corrupt each other and both callers read back a plausible 32-bit number that nothing downstream can question |
| `flash` | *none* | — | — | nothing; deliberately (see below) |
| `gpio` | *none* | — | — | the pin, nothing; its EXTI *line*, the row above |
| `uid` | `read` | — | — | nothing, and this is the ONLY row where "nothing" is a property of the SILICON rather than a decision: every register is read-only, there is no gate to enable and no mode to select, so a second reader cannot disturb a first. `flash` and `gpio` claim nothing by choice; this one has nothing to claim |
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
keyed on the instance and the ordinal. A timer channel, a DMA channel and — since
`de6e59b` — an analog watchdog use it today.

Where it goes: **its own handle, obtained from the port's handle, with the
index checked against a generated `feat` count.** This is shipped code, not a
sketch:

```cpp
auto adc = board::adc::open();
auto wd  = adc.watchdog<0>({.channel = 3, .low = 1000, .high = 3000});
...
if (wd.tripped()) { wd.clear(); }
```

`watchdog<3>()` on an ADC with three of them is a compile error carrying **both**
numbers, because the bound is `Inst::feat::analog_watchdogs` and GCC prints the
comparison. An ADC whose data records no count at all is a compile error naming
the instance, never a silent zero.

The decomposition falls out of the *existing* questions rather than being argued
per peripheral — v1's failure here was that it had no prescribed decomposition
and improvised one. The build then refuted one of v2's four rows, which is why
the third column now cites the silicon:

| Part of the feature | Question it answers to | Answer |
|---|---|---|
| how many there are | is it a count the database knows? | `Inst::feat::analog_watchdogs`, 0 means absent — and on the register file, not the chip file, because the IP version fixes it |
| the thresholds | can every driver that has one program them? | the sub-resource's own Layer-1 `config` |
| ~~which channels it guards~~ **which channel it guards** | ~~does it vary per call on one armed watchdog?~~ | **part of ARMING, not a later call.** `CFGR1.AWD1CH` sits next to `AWD1EN` and both are writable only with the ADC disabled, so there is no `guard(channel)` a running port could honour. It is a member of `watchdog_config` |
| what happens on a trip | — | the sticky flag, polled (`tripped()` / `clear()`). The interrupt path exists in silicon and alloy does not arm it: `AWDnIE` is never set |

**Two more things the build made true that the category does not predict**, both
visible in `src/alloy/adc.hpp`'s own header comment:

- **Arming cycles the port.** The driver stops any conversion, disables the ADC,
  programs the watchdog and re-enables it. "Its own lifetime" turns out to mean
  *its own*, not *non-interfering*.
- **The N are not interchangeable.** AWD1 guards one channel or all of them;
  AWD2 and AWD3 guard an arbitrary channel *set* through a 19-bit bitmask whose
  non-zero value is itself the enable — a different register shape, with no
  `AWDnEN` bit. Layer 1 for a sub-resource is therefore the intersection over
  the **N** as well as over the drivers.

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

**One question decides. The rest is a guide.**

That sentence is the outcome of three attempts and three adversarial reviews,
and it is worth more than any of the taxonomies it replaces. The record:

| Revision | Derived from | Applied to features it had not seen | Score |
| --- | --- | --- | --- |
| v1 | UART | I2C 10-bit, ADC watchdog, timer encoder | 0 of 3 |
| v2 | those three | FDCAN filters, RTC alarms, DAC waveform, LPTIM, USB endpoints | 1 of 5 |
| v3 | three peripherals actually **built** | ADC oversampling, I2C PEC, DMA request routing, flash option bytes, GPIO slew rate | 1 of 5 |

Question 0 answered **5 of 5** every time it was applied, including in the review
that retired v3.

The pattern is not bad luck three times. Question 0 asks a **verifiable fact
about the database** — is the peripheral curated, the register, the field, the
field's encoding? Anyone can check it in a minute, and it is either true or not.
Questions 1–10 ask for a **judgement about silicon nobody has read yet**, and
every review found the same failure shape: the rule refuses the exotic cases it
was taught to refuse, and answers confidently and wrongly on the ordinary ones.
The fourth reviewer put it best — the not-yet-seen table lists cases that are
hard to *reach*, not the cases that are easy to reach and hard to *place*.

So the procedure for the ~25 peripherals still uncurated on this die is:

1. **Run Question 0.** It is a gate, it is mechanical, and its output is often
   "curate first" — a task in alloy-devices, not a layer.
2. **Read the reference manual for that block.** There is no substitute and the
   rule was pretending there was.
3. **Follow a precedent.** Three peripherals were built precisely so that the
   next one has something to copy rather than something to deduce:
   a cross-peripheral feature (`can`, `3ef5130`), a personality
   (`encoder`, `f1f6833`) and a sub-resource (`adc`, `de6e59b`). Each commit
   records what its own prediction got wrong.
4. **Consult questions 1–10 as a checklist**, not as a decision procedure. They
   are a good list of the things to think about — is it a knob at all? whose is
   it? where does its maximum come from? — and they are, measured, wrong about
   one ordinary feature in five.

Everything below stays because it is useful; what changed is its **status**.

### The checklist (questions 1–10)

For any feature, these are worth asking in order. Each names the **artefact**
that would answer it, and each answer is a thing a compiler, a generator or a
lint can check — which is what makes them worth asking even when they are wrong.
They do not decide; you do, with the manual open.

### Question 0: what does the database already know? {#question-0-what-does-the-database-already-know}

A **gate**, not a layer — and the one whose absence made v1 promise an escape
hatch that is not there. It asks a verifiable fact about `alloy-devices`, never
a category judgement, which is why it is the one part of v2 that survived both
reviews intact. What it did not survive is the FDCAN build's count: **it has
five rows, not four.**

| What `alloy-devices` has | What you can reach | Output if you stop here | Witness |
|---|---|---|---|
| **nothing** — the peripheral is uncurated | **nothing.** `alloy::dev::<name>_t` is not emitted at all, and neither are its routes | **curate the peripheral** | `emit/device.py::curated_peripherals` drops them; 28 of the G0B1RE's 65, including `tim1`, both LPUARTs and the USB device |
| the peripheral, **not the register** | base, gate, IRQ, DMA and companion facts. **Not the register**: `IP::regs` has no member for it | **curate the register** | `st/adc_v2`'s curated map stopped at `CCR` — no `TR1`, no `AWD1CR`, no `AWD` flag in `ISR` |
| the register, **not the field** | the register as a typed member. **No named accessor**, so a driver would hand-write bit numbers — guard #1 | **curate the field** | `st/tim_gp16`'s `SMCR` was a register member with zero fields |
| the field, **not its ENCODING** | a named accessor you have to feed a bare integer. **A curated field whose encoding is not curated is a magic number wearing an accessor** | **curate the values** | FDCAN `RXGFC.ANFS` was a curated field, and "reject every non-matching frame" was still the literal `2` until `values:` landed (`alloy-devices` `fb46ebb`, `7cc8ff8`) |
| the field **and** its encoding | everything. The layer questions below can run | — | `st/i2c_v2`'s `CR2.ADD10`; `IP::rxgfc::anfs_reject` |

**Rows 1–4 are not a layer. Their output is a task in `alloy-devices`**, and the
C++ question has no answer until that task is done. Saying so is the whole value
of asking question 0 first: three of the four features this page has been
adversarially tested with stopped at a row, and each time the honest output was
a data ticket rather than a design argument.

**Ask it of the CLOSURE, not of one block.** This is the FDCAN build's second
correction. An acceptance filter's elements live in the companion message RAM
and its list size lives in the controller, so *the controller being curated is
not enough*. Curation travels across the pair: an FDCAN whose message RAM is
uncurated is not a working FDCAN, it is a controller whose filters and FIFOs are
unreachable. Before the build, an uncurated companion produced
`companion cycle among peripherals: ['fdcan1']` — a true sentence about the
wrong thing. `emit/device.py` now names the companion and says why:

```
fdcan1: companion 'ram' names fdcanram1, which this chip does not emit — it is
uncurated, or absent from the data. A peripheral is only as curated as its
companions; curate fdcanram1 in alloy-devices, or drop the companion
```

**And there is one answer that is not a row, because it is not a task.** A field
the register schema *cannot express* has no curation ticket to file. Two are
known, both found by mining `st/tim_gp16` for the encoder:

- **Two registers at one address.** `CCMR1`'s input view's `IC1F/IC2F` sit
  exactly on top of the output view's `OC1PE/OC1M`, and fields of one register
  may not overlap in `alloy.registers.v1`. The digital input filter is therefore
  unreachable **at every layer at once, `alloy::dev::` included** — Layer 3 hands
  out named accessors only for curated fields. Not a layering decision; a limit
  of the data model.
- **One field in two places.** `SMS` is `SMS[2:0]` at bits 2:0 *and* `SMS[3]` at
  bit 16, where a field has one `bit` and one `width`. It is curated as two
  entries named after the manual's own diagram, because both single-entry
  spellings lie.

The output there is a **data-model proposal** for the maintainer, not a
curation ticket and not a C++ workaround. Both are written up in
[what a data model change would have to say](#what-a-data-model-change-would-have-to-say).

!!! warning "Rows 2, 3 and 4 still have no tool"
    `alloy chip-status` answers row 1 and nothing finer: it says a peripheral
    has curated register data, not that the data has the register you need, the
    field inside it, or that field's encoding. Every row-2/3/4 answer on this
    page was found by *opening the IP yaml by hand*. A
    `chip-status --registers <ip>` that lists what a curated IP actually carries
    is the obvious tool and **is not built**; until it is, question 0 costs a
    reader one file. Row 4 makes that worse, not better: an encoding is easier
    to miss than a field, because the accessor you wanted already exists.

The honest unconditional escape is the one
[escape-hatch.md](../guide/escape-hatch.md) calls **Route C** — a raw literal
address in *your* code, outside alloy, with no gate, no IRQ number and no route
check. That is a real door and it is not `alloy::dev::`.

#### Questions 1–5: what kind of thing is it?

Before "how portable is this value" comes "what kind of thing is this value".
v1 had only the layer questions, so it answered *confidently and wrongly* for
everything that is not a knob. Question 5 is the axis that killed v2.

These five are the useful half of the checklist: three of the four features v3
got wrong were mis-sorted here, not in the layer questions below — the reviewer
found ADC oversampling, I2C PEC and DMA request routing all placed confidently
in the wrong kind. Read them as prompts, and expect to overrule them.

**1. Does it change which OPERATIONS the peripheral offers, and exclude the
other modes while it is on?**
*Test: does the handle grow or lose methods? Can two of them run at once?*
→ A [**personality**](#personalities-a-block-runs-in-one-mode-at-a-time): its
own facade, its own binder type, an exclusive claim on the instance. Not a knob
at any layer.

The encoder build added three clauses to this answer, all three from things
that broke:

- **The register data has to DECLARE the personality**, because "one block, one
  job" was encoded in four separate consumers as an equality test on a
  single-valued `class`. Codegen included one `hal/<class>/…` header, the role
  matcher offered an instance to one kind of role, `chip-info` answered with one
  string, and a fourth copy of the rule was found inline in `chips.py`. All four
  now ask **membership** through one helper (`roles.ip_classes`), and
  `alloy.registers.v1` grew an optional `personalities:` key whose first user is
  `st/tim_gp16`. **A personality that the data does not name is unreachable from
  any board role**, however good the C++ is.
- **A personality switch writes whole registers; it does not read-modify-write.**
  The bits `encoder::open()` does not name in `CCMR1` are the *other*
  personality's layout of the same word. Preserving them carries a PWM mode
  field into an input-filter setting. RMW is the wrong default exactly here.
- **The binder takes only the tags that carry a fact it programs.** This page
  predicted `encoder::bind<tim3_t, a<>, b<>, board::clock_profile>` because every
  other binder takes a clock profile. An encoder divides nothing — `PSC` is
  forced to zero and the counter is clocked by the shaft — so the parameter was
  dead and is gone. A dead binder parameter is a dependency the peripheral does
  not have, and the kind that survives for years because nothing ever breaks.

**2. Does it exist N times inside the peripheral, with its own enable and its
own lifetime?**
*Test: would you arm it and disarm it independently of `open()`? Is there a
count that differs by IP version?*
→ A [**sub-resource**](#sub-resources-a-thing-inside-the-peripheral-with-its-own-lifetime):
its own handle, obtained from the port's, its ordinal checked against
`Inst::feat::<count>`, and `claim::sub_exclusive<Inst, N, P>()` on the pair.

The watchdog build says the routing is the easy half and adds three questions
that must be answered *before the handle is designed*, because v2 guessed all
three and was wrong on two:

| Follow-up | v2's guess | What the silicon said |
|---|---|---|
| is the selector a later call or part of arming? | `wd.guard(channel)` — an argument of a later operation | **part of arming.** `CFGR1.AWD1CH` sits next to `AWD1EN` and ST documents both as writable only with the ADC disabled, so `watchdog_config` carries the channel next to the window |
| is its lifetime really independent of the port's? | yes, that is what makes it a sub-resource | **no.** Arming stops any conversion, disables the ADC, programs the watchdog and re-enables it. A sub-resource can interrupt the port it lives in |
| are the N interchangeable? | assumed yes | **no.** AWD1 guards one channel or all; AWD2/AWD3 guard an arbitrary channel *set*, through a 19-bit bitmask whose non-zero value IS the enable. Layer 1 for a sub-resource is the intersection **over the N as well as over the drivers** — one channel — because a portable field that only works at ordinal 0 is the lie this surface exists to remove |

**3. Does it vary from one transfer to the next on a port nobody reopened?**
*Test: could two consecutive calls legitimately want different values?*
→ **An argument of the operation** — and if a bus is shared by devices that each
want their own, a *device* type that carries the bus plus that device's
settings. See [the transfer axis](#the-transfer-axis-and-the-zephyr-answer).

**4. Does honouring it require a pin?**
*Test: does it need a route or an AF programmed?*
→ A **binder tag**, not a field. `uart::de<pb1_t>`, `spi::nss<pa4_t>`,
`encoder::a<pa6_t>`.

<a id="question-5-cross-peripheral"></a>
**5. Does honouring it require programming a SECOND BLOCK?**
*Test: does the feature's data live at an address the facade's own `Inst::base`
does not cover?*
→ **Cross-peripheral.** This is the shape that killed v2, and the FDCAN build is
exactly it: one line of user code, two peripherals with different curation
states, one of which states the other's capacity.

The answer has one structural half and four obligations:

**The feature stays on the facade of the block the user names.**
`board::can.accept_only(match(0x123), match_masked(0x200, 0x7F0))` is a CAN call.
The second block is reached through an **edge on the instance descriptor**
(`Inst::ram_t`, generated from the chip file's `companions:`), never through a
second handle the user has to hold and never through a second facade. A user who
must own two objects to configure one feature has been handed the datasheet's
decomposition instead of a surface.

Then four things no layer states, so the **driver** states them, where the
driver is:

1. **Curation is a closure** — question 0, above.
2. **The capacity may be stated by the OTHER block.** Read a maximum from the
   block that physically stores the things, and pin the relationship to the
   block that counts them; see
   [where a maximum comes from](#where-a-maximum-comes-from).
3. **Order is part of the feature.** Elements first, size second, always: the
   core scans `LSS` elements the instant `LSS` is non-zero, so a size published
   before its elements exist is a core scanning garbage. Nothing in the type
   system can say this; it is a comment and a code shape.
4. **A config window shapes the API.** `RXGFC` takes writes only while
   `CCCR.INIT` and `CCCR.CCE` are set — which means the node is off the bus —
   so the call is variadic and *singular* rather than a builder: every extra
   call would be another window and another moment off the bus, and
   `sizeof...(F)` is also the compile-time count the capacity check needs.

<a id="the-sixth-kind"></a>
##### The kind the five questions have no row for: a second BLOCK that would fill a role alloy already has

Found by the **WWDG** build, and reported here because it is the fourth
consecutive time the kind question was the one that mattered and the checklist
was the wrong instrument for it.

The feature: the STM32's *window* watchdog, on a chip whose *independent*
watchdog alloy has driven since the OTA rollback work. Both are watchdogs, both
are fed, both reset the part. The question the build had to answer first was not
which layer anything lives in — it was **is this a second role, a personality of
the first, or an option on it?**

Questions 1–5 answer it **wrongly, and confidently, in two different
directions**:

| Question | What it says for this feature | Why that is wrong |
|---|---|---|
| **1 — personality?** *"changes which operations the peripheral offers, and excludes the other modes while it is on"* | the strongest match in the table. Two watchdogs, one abstraction, and each one's contract excludes the other's — that reads as textbook mutual exclusion | a personality is **one block in one of several modes**. These are **two blocks, at two addresses, on two different clocks**, and they run **at the same time** — which is the configuration a safety product actually wants: the IWDG as the coarse *the core is alive* backstop, the WWDG as the tight *the loop runs at its rate* monitor. Modelling them as one block's two modes makes the useful case **unrepresentable** |
| **5 — cross-peripheral?** *"does honouring it require programming a SECOND BLOCK?"* | yes, literally: the feature lives at an address `iwdg_t::base` does not cover | question 5's answer is for a block the feature **needs** (a companion). This is a block that is an **alternative**. Its prescription — keep the feature on the facade of the block the user names, reach the other through `Inst::ram_t` — would produce `board::watchdog.feed_within_window()`, which is exactly the lie the split exists to prevent |

**What actually decided it was not on the checklist at all.** It was the `class:`
key in the register file — the substitutability gate, which is Question 0's own
artefact. `st/wwdg_v2` is `class: window_watchdog`, not `class: watchdog`, and
once the data says that, the role table, the codegen and the personality
enumerator all follow without anybody making a taxonomy judgement. So this is
one more data point for the conclusion this page already reached: **Question 0
plus the manual decides, and the kinds are a checklist.** The novelty is only
that the trap was in the *kind* column rather than the *layer* column, and that
the correct answer was reachable by reading two `class:` lines.

!!! danger "And the gate is softer than three separate places said it was"
    roles.py, `st/wwdg_v2`'s commit body and the first draft of the guide all
    said the same sentence: naming the wrong watchdog for the role is *"a
    validation error that says which class it found."* Tried, on a real board
    file, and it is **not**: `board_validate._check_peripheral` grades an
    `ip_class` mismatch as a **warning** — *"the driver may not match"* — for
    every role in the table, so `alloy build` proceeds and the failure lands as
    an incomplete-type cascade in C++.

    Two consequences, and the second is the general one. Locally, `alloy/wwdt.hpp`
    now carries a `static_assert` on the absent `wwdt_impl<>` specialization that
    names the confusion, and `scripts/check_compile_errors.py` compiles it.
    Generally: **this page's `ip_class` column describes an advisory check.**
    Every argument on this page that leans on a role refusing a peripheral of the
    wrong class is leaning on a warning plus whatever the driver's own C++
    happens to say. Promoting it is one branch shared by every role and was not
    changed here.

#### Questions 6–10: which layer does the knob live in?

Only reached by things that really are knobs, on a peripheral question 0 cleared.

**6. Is the answer a count, a limit, a width, or a rate the die fixes?**
*Test: is it a number the chip database knows, rather than something the app
chooses?*
→ Not a field at all. **`Inst::feat::<name>`**, generated, **0 means absent**.
Two clauses the builds added:

- **`feat` has two homes and they may not disagree.** A number that differs
  between two instances on one die (a FIFO depth) lives on the **instance**, in
  the chip file. A number the **IP version** fixes — how many analog watchdogs
  an `st/adc_v2` has — lives on the register file, or it is the same integer
  copied into every chip that names the IP and free to drift in any of them.
  Both land in `Inst::feat`. The same name in both places with *different*
  values is an error, not an override.
- **"Not reachable from here" needs ONE answer.** Portable code must not have to
  ask two questions — "does the data carry a count?" and "does this driver have
  the hooks?" — to decide whether to compile a branch.
  `alloy::adc::watchdog_count<Inst>` is a `consteval` that folds both into `0`,
  and one `if constexpr` on it then serves three *different* board outcomes:
  three watchdogs (`nucleo_g0b1re`, `nucleo_g071rb`), an ADC with none reachable
  (`same70_xplained`'s `afec0_t`), and no ADC at all. `board::caps::` can only
  express two of those three, which is the clearest demonstration on this page
  of why degree is a number and not a bool.

**7. Can every driver alloy ships program it, with the same semantics, the same
unit, over its whole value domain?**
*Test: would adding it force any current driver to trap, reject, or silently
ignore — for any value it can take? Is its unit free of vendor register
artefacts?*
→ **Layer 1**, `alloy::<periph>::config`. And the values it accepts are
[admitted, not assumed](#layer-1-admits-values-not-only-fields) — with the
FDCAN caveat that an admission diagnostic must say what the hardware actually
does: an identifier above eleven bits does not produce a filter matching
*nothing*, it produces one matching a *different* identifier, because no
controller compares the twelfth bit. `match(0x800)` is `match(0x000)` under
another name, and the first draft of that message said the opposite.

**8. Is there a register field that means exactly this, on some IPs?**
*Test: can you name the register field it programs — **in curated data**, which
question 0 already told you?*
→ **Layer 2**, `alloy::<periph>::opts<Inst>`, declared only in the drivers whose
IP has it, under the same name and unit everywhere it appears — and **state its
[scope](#every-layer-2-knob-has-a-scope-and-v1-never-asked)**, block or channel.

**9. Is the register curated but the field, or the field's encoding, not?**
→ Not a C++ question. **Mine it** in `alloy-devices` and come back to 8. The
compiler will have told you: a Layer-2 member can only exist where a generated
field backs it, which is how `usart_v4`'s missing `DEAT/DEDT` became a data task
with an error pointing at it. This is question 0's rows 3 and 4 arriving late,
and it arrives late often enough to be worth a number of its own.

**10. Nothing above fits** — an erratum poke, a one-off, a block alloy models
but this feature is not worth a driver for.
→ **Layer 3**, `alloy::dev::` — *if question 0 said the peripheral is curated,
down to the field*. And **no generic command door, ever**: no
`drv_cmd(dev, uint32_t, uint32_t)`, no `ioctl`, no `cr1/cr2/cr3` words inside a
config struct. An untyped command pair is invisible to review, to grep, to the
contract gate and to the compiler; it is strictly worse than a typed register on
a named peripheral. Zephyr's own Kconfig help for its version says *"Says no if
not sure"* — a framework warning you about its own escape hatch.

**Promotion and demotion, so the layers are not a one-way ratchet:**

- Layer 2 → Layer 1 when every shipped driver implements it. The cost is real
  work — teaching six (then N) drivers — and that friction is the point: it is
  what makes the Layer-1 promise mean something.
- Layer 3 → Layer 2 when the same register poke shows up twice.
- Nothing is ever demoted silently; a Layer-1 field that a new vendor cannot
  honour is a deprecation, on the [stability](stability.md) window.

### Where a maximum comes from {#where-a-maximum-comes-from}

This page has now been wrong about this twice in opposite directions, so v3
states it as its own clause with three witnesses rather than a preference.

Revision 1 said a maximum belongs in `feat`
(`de_time_max_16ths = 31`). The UART conversion refuted that: the generated
field accessor already knows its own width, so one fewer number in the database
is one fewer thing that can disagree with the registers. Revision 2 replaced it
with *"a maximum programmable value must be read from the field's `raw_mask`"*.
**The FDCAN build refuted that in turn**, and the honest rule is neither:

> **A maximum comes from whichever artefact physically bounds the quantity —
> and when more than one artefact names it, the driver `static_assert`s the
> relationship between them.**

Three witnesses, all in the tree:

| The bound is really… | Read it from | Witness |
|---|---|---|
| how wide the field is | the field's own mask | `st_tim_gp16.hpp`: `max_period = IP::arr.wide_raw_mask + 1u` |
| a count the IP version fixes, that no register states | `feat`, on the register file | `Inst::feat::analog_watchdogs` — a register map cannot say "there are three of these" |
| **how much room another peripheral has** | that peripheral | `st_fdcan_v1.hpp`: `filter_capacity = RAM::FLSSA_count`. `RXGFC.LSS` is five bits, so `raw_mask` says **31**; the real capacity is **28**, because a standard filter element is one word of the companion's message RAM |

And the pinning, which is the part that makes it safe to read a number from a
block other than the one that counts it:

```cpp
static_assert(filter_capacity <= IP::lss.raw_mask,
              "this instance's message RAM holds more standard filters than "
              "the controller's RXGFC.LSS field can count — the chip data "
              "has the controller and its companion from different dies");
```

A future die that widens one and not the other fails to build, which is the only
form of "these two facts agree" that survives a data update nobody reviews.

### The transfer axis, and the Zephyr answer

Question 3 is the one v1 did not have, and the counterexample that found the
gap was I2C 10-bit addressing: **a 10-bit address is a property of the
TRANSFER, not of the port.** One bus talks to a 7-bit sensor and a 10-bit
sensor in consecutive calls, and no amount of layering a *port* config can
express that. v1 routed it through what is now question 7, decided "every I2C
driver can program an address width, therefore Layer 1", and produced a `config`
field that would be wrong on the next line of user code.

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

## What the checklist does not decide {#what-v3-does-not-decide-and-what-it-has-not-seen}

A rule that needs a human argument per peripheral is not a rule — and the
measured conclusion of three reviews is that the checklist below IS that, so it
is documented as a checklist and Question 0 carries the decision alone.

This section keeps its two halves because both are still true and useful:
**questions the page asks and does not answer**, and **cases the three builds
never showed it**. One correction from the review that retired v3, because it is
the most useful sentence anyone wrote about this page: the not-yet-seen list
below is long, but it lists cases that are hard to REACH — three personalities
on one block, a run-time personality switch, a companion with its own facade —
and not the cases that are easy to reach and hard to place. None of the five
ordinary features the reviewer tried landed on it. A refusal list only protects
you from what it anticipated, and this one anticipated the exotic.

### Open questions, unchanged by the three builds

**1. Sub-resources that interact.** v3 says where a sub-resource lives and says
nothing about what happens when two of them contend for the same silicon. The
watchdog build made this *sharper* rather than solving it: the G0B1RE has three
analog watchdogs, and AWD2/AWD3 guard an arbitrary channel *set*, so two
watchdogs guarding overlapping channels is a shape the lead chip can express
today. So is an injected conversion preempting a regular one, and a DMA channel
serving a timer's update and its capture events.

> **The question the page asks:** *does arming this sub-resource change the
> observable behaviour of an operation whose call site does not mention it?* If
> yes, the sub-resource may not be added until the answer to "and what tells the
> user" is written down. There is no default.

Note that the ADC build had to answer a *weaker* version of this for itself —
arming cycles the port — and answered it in a header comment. That is the honest
state: prose, in the one place a driver author will read it, and no mechanism.

**2. A "mode" whose operations do not change shape.** Question 1 is mechanical
when the handle gains or loses methods — PWM versus encoder, master versus
slave. It is **not** mechanical when both modes offer the same calls with
different meanings: a timer counting up versus down, an ADC in single versus
continuous mode, a UART half-duplex on one wire.

> **The question the page asks:** *can the two modes be swapped on a live block
> without re-binding a pin and without invalidating a handle already handed
> out?* If yes it is a knob; if no it is a personality. The tie-breaker is not
> mechanical enough to be a test — a maintainer states the answer, in the
> driver, with the reason, and the reason is the direction the pins point.

**3. A block-scoped Layer-2 knob under a channel-scoped binder.** Timer dead
time (`BDTR.DTG`) is the case, `pwm::bind` is per channel, and
`claim::shared`'s witness covers the block-scoped *Layer-1* value (`freq_hz`)
only. The two candidate repairs — widen the witness, or give the timer a block
handle that channels are opened from — are
[stated in full below](#3-timer-complementary-outputs-with-dead-time-breaks-v2)
and v3 does not choose between them, because no shipped timer driver has a
block-scoped Layer-2 knob yet and choosing would be inventing again.

### Not yet seen — and therefore not decided

Each row is a case v3 has **no evidence about**. They are listed so that the
next feature that hits one knows it is off the map, rather than finding a
confident sentence that was never tested. This list is the difference between v3
and its two predecessors.

| Case | Nearest thing that WAS built | Why the built thing does not answer it |
|---|---|---|
| a cross-peripheral feature whose **second block has its own facade** and its own user-visible handle | FDCAN + `fdcanram1` | the message RAM has no facade, no driver and no role. "The second block is an edge on the descriptor" is proven for a companion that is *pure storage*, and for nothing else |
| a companion whose two owners **disagree** about how to program it | `dmamux1`, shared by `dma1` and `dma2` — see the box below | sharing is *built*; what is unseen is a shared companion whose owners want contradictory settings in one word. `ch_mux_offset` gives the two DMA controllers disjoint index ranges, so they never write the same register |
| a personality **switched at run time** on a live block | `pwm` vs `encoder` on `tim3` | both are chosen by naming a binder, and the claim is taken at `open()`. Alloy has no `release` at instance scope at all, so "switch back" is untypeable rather than decided |
| **three or more** personalities on one block | two (`pwm`, `encoder`) | `personalities:` is a list and the membership test is general, but nothing has exercised a third |
| a sub-resource whose **interrupt** alloy arms | the watchdog's sticky flag | `AWDnIE` is never set; the trip path is polled. The callback half of the sub-resource shape is unwitnessed in code and in emulation |
| a sub-resource ordinal that can do **more** than the intersection | AWD2/AWD3's channel-set | the driver offers the intersection (one channel) and there is no mechanism that says "ordinal 2 can do more". A product that needs the set has Layer 3 or nothing, and v3 does not decide which |
| `feat`'s two homes, or `personalities:`, on **non-ST data** | `st/adc_v2`, `st/tim_gp16` | both mechanisms were designed and exercised against ST register files only. Alloy ships six non-ST drivers |
| a peripheral chosen at **run time** | — | `Inst` is a type. Already true before this page; not changed and not intended to change |
| **auto-baud** and other hardware-writes-config-back paths | — | a read-back path, not a knob. No home in this shape yet |

!!! warning "One row of this table used to be false, and the counter-evidence is the oldest cross-peripheral driver in the tree"
    It said *"a companion shared by two controllers … nothing has tested two
    owners"*. `chips/st/stm32g0b1re.yaml` gives **both** `dma1` and `dma2`
    `companions: {mux: dmamux1}`; the generated `device.hpp` emits
    `using mux_t = alloy::dev::dmamux1_t;` on both, with
    `ch_mux_offset = 0` and `= 7` to keep their index spaces apart; and
    `hal/dma/st_dma_v1.hpp` has reached the router through that edge since
    `145f05e`, long before the FDCAN build.

    Which means the structural half of [question 5](#question-5-cross-peripheral)
    — *the second block is an edge on the instance descriptor, never a second
    handle* — was **not** derived from the FDCAN build. It was already
    shipped, for a companion that is not storage at all but an active request
    router with its own register map, and the FDCAN build reproduced it. The
    clause is stronger than v3 claimed for it and its audit trail named the
    wrong witness. What FDCAN genuinely added is the rest of question 5: the
    curation closure, the capacity stated by the other block, the ordering
    obligation and the config window — none of which `mux_t` had to face.

**What is NOT on this list, deliberately.** "Which layer" is decidable, and so is
"is the database ready" — question 0 is five rows of fact, each readable straight
out of `alloy-devices`. If a future feature makes either of those require a
judgement call, that is a defect in the rule and belongs here, not in a driver's
comment.

---

## What a data-model change would have to say {#what-a-data-model-change-would-have-to-say}

The encoder build hit three limits that are not layering decisions and not
curation tickets: the register schema **cannot express** the fact. v3 cannot fix
them, because a rule about where a knob lives does not get to change
`alloy.registers.v1`. So they are stated here as **proposals for the maintainer,
not changes made** — with what each buys, what it costs, and the concrete field
that is unreachable until it lands.

One of the four *was* made, additively, by the encoder build, and is described
first so the proposals are read against what already exists.

**LANDED (`f1f6833` + `alloy-devices`): a register file may declare
`personalities:`.** `class` stays, single-valued, as the primary; the new
optional list carries the others; `roles.ip_classes()` returns the tuple and
four consumers ask membership instead of equality. First user: `st/tim_gp16`
declares `personalities: [encoder]`. Without it, the encoder personality was
unreachable from any board role no matter how good the C++ was.

**PROPOSAL 1 — collapse `class` + `personalities:` into one ordered list.**
Every consumer now asks membership; the primary is only needed by `chip-info`,
which wants "what is this block called". Two keys that together mean one ordered
list can drift in a way one key cannot — a value in both, or a `personalities:`
on a file with no `class`. *Buys:* one fact in one place, and a schema test that
is a `len()` rather than a set comparison. *Costs:* every register yaml in
`alloy-devices` and a schema major. *Why not made here:* it is a repo-wide data
migration with no user-visible symptom today, and this page's job was the rule.

**PROPOSAL 2 — two register views at one address.** `CCMR1` is two registers at
one offset and which one it is depends on a field inside it; the input view's
`IC1F/IC2F` sit exactly on top of the output view's `OC1PE/OC1M`, and fields of
one register may not overlap. **Concretely unreachable today: the encoder's
digital input filter, at every layer at once including `alloy::dev::`** — a
bouncing mechanical encoder will feel it, and there is no workaround inside
alloy that is not a hand-written bit number. A `views:` map keyed by personality
would let the same offset carry both field sets, and the personality the binder
already knows would select one. *Costs:* the emitter's register struct becomes a
union, and every consumer of `IP::regs` has to name a view.

**PROPOSAL 3 — a field in two places.** `SMS` is `SMS[2:0]` at bits 2:0 and
`SMS[3]` at bit 16, where `alloy.registers.v1` gives a field one `bit` and one
`width`. It is curated today as two entries named after the manual's own
diagram, because both single-entry spellings lie — one claims bits `SMS` does
not own, the other hides a bit it does. A field with a list of `bits:` segments
would let the accessor shift and mask correctly and let a driver write
`sms.set(encoder_mode)` once. *Costs:* accessor codegen, and every `raw_mask`
consumer has to mean "the value domain" rather than "the mask at one position".

**PROPOSAL 4 — make an enumerated field's encoding non-optional.** Question 0's
fifth row exists because `values:` is optional: `RXGFC.ANFS` was a curated field
whose encoding was not curated, and the driver wrote the integer `2`. A lint
that fails when a field's upstream source carries an enumeration and the yaml
does not would close the row instead of documenting it. There is a second half:
the IP emitter **accepted `values:` on fields of ARRAY registers and silently
dropped them** until `3ef5130` — schema-accepted, emitter-ignored, no
diagnostic. A schema-vs-emitter round-trip test is the cheaper half of this
proposal and would have caught it.

**PROPOSAL 5 (a tool, not a schema change) — `alloy chip-status --registers
<ip>`.** Rows 2, 3 and 4 of question 0 have no tool: every row-2/3/4 answer on
this page was found by opening a yaml by hand. Row 4 is the worst of the three
to find that way, because the accessor you wanted already exists and reads
perfectly well at the call site.

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

### Added at revision 3 — where each clause of the rule comes from {#added-at-revision-3}

v3 is not a design; it is a generalisation of three build reports. This table is
the audit trail, so a reader can attack any clause at its source — including the
four clauses whose witness is a comment rather than a test.

| Clause of [the rule](#the-rule) | From | What witnesses it |
|---|---|---|
| question 0 has a **fifth row** — a curated field whose encoding is not curated | `3ef5130` | `RXGFC.ANFS`'s `values:` in `alloy-devices` (`fb46ebb`, `7cc8ff8`), and `emit/ip.py` learning to emit `values:` for fields of ARRAY registers — which it had been accepting and silently dropping |
| curation is a **closure over companions** | `3ef5130` | `emit/device.py` raises naming the companion and why, with two tests in `tools/alloy/tests/test_emit.py` — one for the diagnostic, one negative control that generates the curated pair. Disabling the check reproduces the message it replaced, `companion cycle among peripherals: ['fdcan1']`: a true sentence about the wrong thing |
| **where a maximum comes from** | `3ef5130`, and `st_tim_gp16.hpp` for the contrast | `filter_capacity = RAM::FLSSA_count` with two `static_assert`s tying it to `IP::lss.raw_mask` and to the element format; `max_period = IP::arr.wide_raw_mask + 1u` |
| the second block is an **edge on the descriptor**, not a second facade | `3ef5130` | `Inst::ram_t` in `st_fdcan_v1.hpp`; there is no `fdcanram` facade, driver or role |
| **order** (elements before size) and the **config window** are the driver's obligation | `3ef5130` | **a comment and a code shape, and nothing else.** No test asserts the order; Renode maps `fdcan1` to a bxCAN model, so no leg can exist, and the host suite is board-free — it pins what a filter *means*, not how it is spelled |
| a personality must be **declared in the data** | `f1f6833` | `personalities:` in `alloy.registers.v1`, `roles.ip_classes()`, and `tools/alloy/tests/test_encoder_role.py` with the lead board as its negative control |
| a personality switch writes **whole registers** | `f1f6833` | the driver, plus the programmed sequence read out of the built image's disassembly in that commit (`SMCR = 3`, `CCMR1 = 0x101`, …). **Not re-measured since**, and no silicon or emulator has run it |
| a binder takes **only tags carrying a fact it programs** | `f1f6833` | `encoder::bind<Inst, A, B>` — three parameters where every other binder has four |
| a sub-resource's ordinal is checked against a **generated count** | `de6e59b` | `static_assert(N < Inst::feat::analog_watchdogs)` in `alloy/adc.hpp`; GCC prints the comparison |
| the selector is part of **arming**; arming **cycles the port**; the N are **not interchangeable** | `de6e59b` | `hal/adc/st_adc_v2.hpp`'s `awd_arm<N>`, and `tests/emulation/adc_watchdog.robot` (`75be7ad`) — which exercises **ordinal 0 only**, because Renode's model is built with `watchdogCount: 1`. Ordinals 1 and 2, and `AWDnIE`, are unexercised |
| `feat` has **two homes** and they may not disagree | `3ef5130`'s `emit/device.py` | the merge raises on a conflicting name. Exercised on ST register files only |
| "**not reachable from here**" is one number | `de6e59b` + `03fde66` | `alloy::adc::watchdog_count<Inst>`; the matrix shows `same70_xplained` taking the ADC-with-no-reachable-watchdog branch, which `caps::` cannot express |
| an unused feature **costs zero** | `fccaec5` | twelve byte-identical `.text` comparisons with a positive control that moves — [the measurement](#cost-zero-for-unused-features-measured-to-the-byte) |

!!! danger "The one thing revision 3 has NOT survived"
    v1 and v2 both looked right until a reviewer **applied** them to a feature
    they were not derived from. v3 has been applied to exactly the three
    features it was derived from, which is not a test — it is a restatement.
    The first honest trial is the next peripheral somebody builds with it, and
    the places it is most likely to break are already named in
    [what it has not seen](#what-v3-does-not-decide-and-what-it-has-not-seen).
    A rule's credibility is the list of cases it refuses to answer, not the
    length of the ones it does.


## Added by the three BUILT peripherals {#added-by-the-three-built-peripherals}

Two versions of the layer rule were written from a chair and both were killed by
a reviewer who applied them. The third attempt started by building three
deliberately different peripherals — a cross-peripheral feature (`can`), a
personality (`encoder`) and a sub-resource (`adc`) — so the rule would be
derived from what a build actually demanded. Those three landed at `3ef5130`,
`f1f6833` and `de6e59b`, each naming what its own prediction got wrong; the
generalisation of those three lists is [the rule](#the-rule), and every clause
of it points back into this section or into one of those commits.

Each of those commits also listed proofs it had not run. This section is those
proofs, run.

### Portability: all three examples, every board {#portability-all-three-examples-every-board}

`examples/can`, `examples/encoder` and `examples/adc_watchdog` are one
`main.cpp` each with no preprocessor conditional. Built for every board on CI's
`build` matrix — `nucleo_g071rb`, `nucleo_g0b1re`, `nucleo_f722ze`,
`same70_xplained`, `rp2040_zero`, `raspberry_pi_pico`, `esp_wrover_kit`,
`esp32_devkit` — plus `nucleo_f767zi`, which rides the `build-net` job instead:
**27 of 27 green**, no `requires` guard added and no source changed.

The interesting column is not the pass, it is *which branch* each board takes,
because "it builds everywhere" is worth nothing if everywhere took the same
empty road:

| Example | boards on the REAL branch | boards on the honest fallback |
|---|---|---|
| `can` | `nucleo_g0b1re` (`caps::can`) | 8 — `"this board declares no can role"` |
| `encoder` | `nucleo_g0b1re` (`caps::encoder`) | 8 — `"no encoder role on this board"` |
| `tick` | `nucleo_g0b1re` (`caps::tick`, TIM6) | 8 — `"no tick role on this board"`. The example ALSO branches on `feat::trgo`, which is 1 on TIM6 and 0 on TIM14/16/17, so a board that binds the role to a small timer takes a third road that no board here exercises yet |
| `adc_watchdog` | `nucleo_g0b1re` **and** `nucleo_g071rb` (`feat::analog_watchdogs == 3`) | 7 — of which `same70_xplained` is the informative one: it HAS an ADC (`afec0_t`) and no reachable watchdog, so `Adc::watchdogs` is 0 and the same `if constexpr` that serves a board with no ADC at all serves it |

`adc_watchdog` is the one that shows the degree mechanism doing work rather than
`caps` doing it: three different board outcomes (three watchdogs, an ADC with
none, no ADC) from one `if constexpr` on a generated number.

That the fallback branch is genuinely empty is measurable, not a matter of
reading the source. On `esp32_devkit` the three examples have **the same
`.text` to the byte — 1188** — and differ only in `.rodata` (248, 256, 264
bytes for `encoder`, `adc_watchdog`, `can`), which is the length of their
banners and their one "not available" line. Three different peripherals, three
discarded branches, and not one instruction between them.

### Cost: "zero for unused features", measured to the byte {#cost-zero-for-unused-features-measured-to-the-byte}

The claim under every layer decision on this page is that a feature nobody calls
costs a program nothing. It was asserted by all three of these commits and
measured by one of them. It is measured for all three now, the same way each
time.

**Method**, so it can be re-run or attacked. For each landing, `git archive` the
PARENT commit of `alloy` and pair it with the `alloy-devices` commit that was
`alloy-devices` HEAD at that moment — the register data is half of the change,
and comparing a new emitter against old data would measure the wrong thing.
Then `git archive` the landing itself with its own data commit. Build four
examples that never mention the new feature — `blink`, `uart_echo`, `pwm_fade`,
`adc_read` — for `nucleo_g0b1re`, same host toolchain (xPack
`arm-none-eabi-gcc` 14.2.1, `-Os`). Compare `size`, then extract `.text` with
`objcopy -O binary --only-section=.text` and compare the bytes.

| Landing | before (alloy + alloy-devices) | after |
|---|---|---|
| `can` — FDCAN acceptance filters | `b353304` + `70b288f` | `3ef5130` + `7cc8fff` |
| `encoder` — the timer's second personality | `16cbae9` + `7cc8fff` | `f1f6833` + `2ec8c99` |
| `adc` — the analog watchdog | `f1f6833` + `2ec8c99` | `de6e59b` + `14b6976` |

**Result: `.text` byte-identical in all twelve comparisons.** Not "the same
size" — the same bytes, same MD5:

| Example | `size` text/data/bss | `.text` bytes | MD5, all six trees |
|---|---|---|---|
| `blink` | 3396 / 8 / 3240 | 3208 | `565f5cfc7db6ac62dfe235fc9b1c30d2` |
| `uart_echo` | 2744 / 8 / 3240 | 2556 | `b27dfa7f10226bf7be0c9b527c721da6` |
| `pwm_fade` | 3652 / 8 / 3248 | 3464 | `e06d3af5ab1969a9fdeff053fc9d2854` |
| `adc_read` | 3996 / 8 / 3240 | 3808 | `312fb5e06c50c394bf49dd9c317b4e18` |

Two of those four rows are the ones that could actually have moved, and they are
the reason the set is not just `blink` four times:

- **`pwm_fade` is the encoder's exposure.** It binds the same `st_tim_gp16` IP
  the encoder personality binds, and `f1f6833` changed how `class` is matched
  across four consumers *and* added an encoder role to
  `boards/nucleo_g0b1re/board.json`, so the board codegen that produced this
  image is a different program than the one before it. Same bytes out.
- **`adc_read` is the watchdog's exposure.** `de6e59b` added `awd_arm`,
  `awd_tripped`, `awd_clear` and `awd_disarm` to the very header `adc_read`
  compiles (`hal/adc/st_adc_v2.hpp`) and a `watchdog<N>()` member to
  `alloy::adc::handle`. Same bytes out — and also on `nucleo_g071rb`, the other
  board with `analog_watchdogs == 3` (`.text` 2880 before and after,
  `83869ae2e8971b986abb96467f122068`; `blink` 2280, `c79f1b0ecb9b6482fd8f7bb62b76024d`).

**The controls, because twelve identical hashes are equally consistent with a
broken measurement.**

- *Positive control on the harness.* The same before/after pair applied to
  `examples/can`, which DOES use the feature, moves: `.text` 4044 → 4244 bytes
  and a different MD5. The comparison can see a change; it saw none in the four
  above.
- *The header cost, isolated from the example rewrite.* `3ef5130` rewrote
  `examples/can/src/main.cpp` as well as adding the filters, so the +200 above
  is not the price of `accept_only` — it is the price of the whole new main.
  Building the OLD `main.cpp` against the NEW headers gives `.text` 4044 and
  `83236c5ad8ba3495116bfdddba9f486e`, byte-identical to the old tree. So the
  filter machinery costs a program that does not call it exactly zero, which is
  what `3ef5130` claimed from a disassembly diff and what this reproduces from
  the image.

**What this does not say.** Every figure above is `-Os`, one toolchain, and
Cortex-M0+ only — `nucleo_g0b1re` and `nucleo_g071rb`. No `-O2`, no other
compiler, no other ISA, and no RAM-at-runtime claim beyond the `bss` column.
The 27-build sweep in the section above covers the other architectures; the
byte comparison does not. The +200 figure for `examples/can` is the whole
example's delta and is *not* the same quantity as `3ef5130`'s "+112 B for the
two-filter call", which was measured on the function rather than the image; that
figure was not re-derived here.

---

## History: how this rule was refuted twice {#history-how-this-rule-was-refuted-twice}

Everything from here down is a **record of refutation**, not a rule. Three
narratives used to sit at the top level of this page and a reader meeting them
cold could take them for three competing rules; they are one story, told in the
order it happened. Where anything below disagrees with
[the rule](#the-rule), **the rule wins** — and the disagreements are marked
where they occur rather than edited out, because which claims break is the
useful part.

Most of what follows is not argument. It is measurement: live defects found by
applying a rule to shipped code and then looking at what came out.

### v1, derived from UART, and the five things the code changed about it

v1 was decided before a line of it was written. UART was then converted — six
drivers, four vendors, the facade, the binder, the emitter, the chip data and a
portable example — and five of its claims did not survive contact with the tree.

**1. Word length is not a Layer-1 field, and `stop_bits` has two values, not
four.** The admission test as written asks about a FIELD; the real unit is the
(field, **value domain**) pair. Character length domains are 8–9 on
`st_usart_v2`, 7–9 on `v3`/`v4`, 5–8 on `microchip_usart_v1` and on
`raspberrypi_uart_pl011`, and nothing at all on `espressif_uart_v1`, which is
ROM-configured. **The intersection is {8}**, so data bits is a Layer-2 knob with
each IP's own domain behind a `static_assert`. `stop_bits::half` and
`one_and_half` (ST smartcard/IrDA encodings) failed the same way and are gone.
This is the rule working: teaching six drivers is what Layer 1 costs, and two of
the four proposed fields could not be taught.

**2. Layer 2 is a DESCRIPTION, not a contract.** The falsification test this
page pre-committed to — implement RS-485 DE on `st_usart_v3` *and*
`microchip_usart_v1` and see whether `de_assert_16ths` survives with the same
meaning and unit — **came back negative**, for a reason that is silicon rather
than naming: the SAM USART has no DE assert/deassert time at all, only
`US_TTGR`, a transmitter guard time in whole bit periods. So `libs/` code may
probe a Layer-2 member by name and may **not** assume a feature present on one
vendor appears under any name on another.

**3. `usart_v4` has *fewer* Layer-2 knobs than `usart_v3`, which is backwards
from the sketch.** The G0 silicon has DE and inversion; alloy's curated
`usart_v4` data does not (`usart_v3.yaml` carries `DEAT/DEDT/DEM/DEP` and
`TXINV/RXINV/SWAP`; `usart_v4.yaml` carries none of them, and does carry
`FIFOEN`, which v3 lacks). Declaring Layer 2 beside the driver is what forced
this to surface, and the consequence is stated by a compiler rather than a
README:

```
error: static assertion failed: this USART's driver has no hardware
driver-enable: alloy's curated usart_v4 register data carries no DEM/DEAT/DEDT
(the silicon has them; the database has not mined them). Drive DE from a GPIO,
or reach the registers through alloy::dev::
```

That is question 0's row 3, arriving as a build error with a data ticket
attached.

**4. A maximum should come from the register data, not from `feat` — and that
correction was itself over-general.** v1 proposed
`Inst::feat::de_time_max_16ths = 31`; the generated field accessor already knows
its own width, so `O.de_assert_16ths <= IP::deat.raw_mask` is one fewer number
in the database and GCC prints `the comparison reduces to '(40 <= 31)'`. **v2
then wrote that up as "a maximum must be read from the field's `raw_mask`", and
the FDCAN build refuted it**: `RXGFC.LSS` is five bits, so `raw_mask` says 31,
and the real capacity is 28 because the companion message RAM holds 28
one-word filter elements. The rule now in force is
[where a maximum comes from](#where-a-maximum-comes-from) — whichever artefact
physically bounds the quantity, with a `static_assert` between them when more
than one names it.

**5. One combination neither layer can reject.** Layer 1 is runtime and Layer 2
is compile-time, so `opts{.data_bits = 9}` plus a runtime `parity::even` asks an
ST USART for a 10-bit word. No `static_assert` can see a runtime parity, and
silently programming 8 bits would be the lie, so the drivers `__builtin_trap()`
under `if constexpr (O.data_bits == 9)`. Measured on the G0B1RE: with a
compile-time-constant parity the trap folds, GCC merges it into the double-open
`udf #255`, and everything after `open()` is deleted as unreachable — including
the string the program meant to print. The build is silent, the binary is
*correct* (it refuses), and the failure arrives on the first boot. **A mixed
compile-time/runtime surface has a seam, and this is where it is.**

### The three counterexamples that killed v1 {#the-three-counterexamples-answered}

The method that produced v2: for each feature, write down what a user should be
able to type **first**, then find the rule that produces it. All three are on the
STM32G0B1RE, and two of the three have since been **built**, which is how their
predictions came to be scored rather than argued.

| Feature | What v1 answered | What was actually true | Where it is now |
|---|---|---|---|
| I2C 10-bit addressing | "every I2C driver can program an address width → Layer 1 `config`" | a 10-bit address is a property of the **transfer**, not the port: one bus talks to a 7-bit and a 10-bit device in consecutive calls | question 3, [the transfer axis](#the-transfer-axis-and-the-zephyr-answer). **Specified, not built** — `alloy::i2c::config` still has one field and the handle still takes `std::uint8_t addr` |
| the ADC analog watchdog | count is degree, "and the rest through `alloy::dev::`" | the Layer-3 door was **locked**: `st/adc_v2`'s curated map ran `ISR, IER, CR, CFGR1, CFGR2, SMPR, CHSELR, DR, CCR` — no `TR1`, no `AWD1CR`, no `AWD` flag in `ISR`. Question 0, row 2 | curated, then **BUILT** (`de6e59b`). What the build changed about the prediction is in [sub-resources](#sub-resources-a-thing-inside-the-peripheral-with-its-own-lifetime) |
| timer encoder mode | "it needs a pin mux → a binder tag" | the pins *are* binder tags; what v1 missed is that encoder mode **excludes PWM on the same block**, and v1 had no category for a mutually exclusive whole-block mode. "A different IP tag, therefore a different driver" cannot express it either: `tim3` has one IP tag and two personalities | curated (question 0, row 3 — `SMCR` was a register with zero fields), then **BUILT** (`f1f6833`) |
| the basic and small timers (TIM6/7/14/15/16/17) | the checklist was never asked — question 0 answered first and answered everything: all six were `uncurated`, which is row 1, so the only available move was data | **the interesting judgement was not a LAYER, it was how many IPs.** "A timer is a timer" says reuse `st/tim_gp16`; the register data says four separate blocks, and the load-bearing difference is `BDTR.MOE` — TIM15/16/17 leave the pin inactive with every other register correct, so reuse would have produced silence rather than an error. Nothing on this page has a question for "is this the same IP"; the answer came from diffing upstream's block hierarchy | curated (question 0, row 1) in `83bcbc2`, then **BUILT** (`e8d433d`) |

Two details worth keeping, because they are the parts a summary loses.

**What the user writes for the two that were built**, and the prediction's score:

```cpp
// encoder — predicted with a clock profile and `counts_per_rev`; shipped with
// neither. An encoder divides nothing (PSC is forced to 0, the shaft clocks the
// counter), so a Clock parameter was a dependency the peripheral does not have;
// and `period` is a counter modulo, which is only usually a rev.
using Enc = alloy::encoder::bind<alloy::dev::tim3_t,
                                 alloy::encoder::a<alloy::dev::pa6_t>,
                                 alloy::encoder::b<alloy::dev::pa7_t>>;
auto enc = Enc::open({.period = 1'440});
std::uint32_t pos   = enc.count();   // unsigned, because the counter is
std::int32_t  moved = enc.delta();   // signed, wrap-aware, consumes state

// the watchdog — predicted with `wd.guard(channel)` and `wd.on_trip(cb)`;
// shipped with the channel inside the arming config and a polled sticky flag.
auto adc = board::adc::open();
auto wd  = adc.watchdog<0>({.channel = 3, .low = 1000, .high = 3000});
if (wd.tripped()) { wd.clear(); }
```

If the same program also opens `alloy::pwm::bind<alloy::dev::tim3_t, …>`, the
second `open()` traps with `trap_code::personality_conflict`; if the two came
from *board roles*, `alloy build` refuses the board file and never starts a
compiler. Both directions have tests, and both have negative controls
(`tests/test_encoder.cpp`, `tools/alloy/tests/test_encoder_role.py`).

**The two walls the encoder's mining hit**, which no layer question can see and
which are now [data-model proposals](#what-a-data-model-change-would-have-to-say):
`SMS` is one field in two places (`SMS[2:0]` at bits 2:0 and `SMS[3]` at bit 16),
and `CCMR1` is two registers at one address whose views overlap — which is why
the encoder's digital input filter is unreachable at every layer at once,
`alloy::dev::` included.


### The holes: four combinations this page did not anticipate {#the-three-holes-two-now-closed}

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


### v2, and the features it was stressed against {#stressing-v2-on-three-features-it-was-not-built-from}

A rule tested only on the counterexamples that produced it has been fitted, not
validated. v2 was applied to features it was not built from; **the review that
retired it reported four of five broke**, and three of those trials are recorded
here — one survival and two breaks. The breaks are the reason v3 exists, and one
of them is still open.

#### 1. SPI slave mode — survives, with one refinement

Question 1 fires: the operations change (a slave cannot initiate), the modes are
exclusive (`CR1.MSTR`), and **every pin reverses direction**. → personality:
`alloy::spi::slave::bind<…>`, exclusive claim. Two things v2 had to be sharpened
to say, both found here:

- **Layer 1 is shared where the wire meaning is shared, and trimmed where it is
  not.** CPOL/CPHA mean the same thing in both personalities and stay one type;
  `clock_hz` is meaningless to a slave, so `spi::slave::config` does not have it
  — the `uart::frame` shape, for the same reason.
- **The personality vocabulary is one enumerator per FACADE, not per bus.**
  `claim::personality::spi` cannot cover both, or a master bind and a slave bind
  on one instance would agree and neither would trap. `spi_slave` becomes its own
  enumerator the day the facade lands; this is written into `claim.hpp`'s comment
  so the next facade does not get it wrong.

#### 2. ADC injected channels — broke v2, and is still open

Routing is easy and v2 got it right: question 1 no, **question 2 yes** — four
injected ranks, their own trigger, their own `JDR` registers, their own
end-of-conversion, armed independently. **Where it breaks: injected conversions
PREEMPT regular ones**, and v2 had no vocabulary at all for sub-resources that
interact. After `adc.injected<0>(…)` is armed, a plain `adc.read(ch)` can be
delayed or aborted by a trigger that is nowhere in its call.

The watchdog build did **not** close this — it sharpened it, because AWD2/AWD3
guard channel *sets*, so two watchdogs on overlapping channels is the same shape
on the same converter. It is
[question 1 of what v3 does not decide](#what-v3-does-not-decide-and-what-it-has-not-seen).

*(On the lead chip the injected question is theoretical twice over: the
G0B1RE's curated `adc_v2` map has no injected registers, and the F767's `adc1`
is uncurated outright — question 0, row 1.)*

#### 3. Timer complementary outputs with dead time — broke v2, and is still open {#3-timer-complementary-outputs-with-dead-time-breaks-v2}

Question 0 answers first and answers row 1: complementary outputs need an
advanced timer, `tim1` on the G0B1RE is uncurated, and there is no
`alloy::dev::tim1_t`. Suppose it were curated. Then the parts route cleanly —
CH1N is a pin (question 4, `pwm::outn<>`), the break input is a pin
(`pwm::brk<>`), and the dead time is a register field in a vendor-shaped unit
(`BDTR.DTG`'s piecewise encoding), so question 8, Layer 2.

**Where it breaks: dead time is BLOCK-scoped and `pwm::bind` is CHANNEL-scoped.**
Two channel binds on one timer, each carrying `opts{.dead_time_ns = …}` with
different values, is hole (A) in a new dress — and the fix that closed (A) does
not catch it, because `claim::shared`'s witness is the block-scoped *Layer-1*
value (`freq_hz`) and no shipped timer driver has a block-scoped Layer-2 one.

Two candidate repairs, and v3 still does not choose between them:

1. widen the witness to cover the block-scoped `opts` — cheap, and it turns the
   conflict into a trap rather than preventing it;
2. give the timer a **block handle** that channels are opened *from*
   (`auto tim = board::motor::open<opts{...}>({.freq_hz = 20'000}); auto a = tim.channel<1>();`)
   — structurally right, makes the conflict untypeable, and is a breaking change
   to `alloy::pwm`.

Naming the choice is a maintainer's call. The argument for (2) is that the
*same* structural defect is live in shipped code today for `freq_hz`: the claim
makes it loud, it does not make it impossible.
