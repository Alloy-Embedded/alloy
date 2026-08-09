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
a quantity is a generated number, and everything else is a compile error that
names the thing you asked for.**

---

## The decision

| Question | Answer |
|---|---|
| Where does the portable surface live? | `alloy::<periph>::config` — a runtime struct, frozen at the knobs *every* driver alloy ships programs. UART: four fields. |
| Where does the rest live? | `alloy::<periph>::opts<Inst>` — a compile-time value whose **members are declared per IP version**. A knob the silicon lacks is not a member. |
| How is "less of it, not none of it" expressed? | A generated number on the instance: `Inst::feat::rx_fifo_depth`. **0 means absent.** |
| What happens when you ask for something absent? | Compile error naming the instance, and the field when the IP has any vendor knobs at all. Never ignored, never a runtime error code. |
| What happens when you ask for too much of something present? | `static_assert` inside the driver, comparing against the generated number, with both numbers in the message. |
| Is there a generic command door? | **No.** Not now, not under schedule pressure. `alloy::dev::` is the only door below `opts`. |

---

## Three layers, and the name tells you which

| Layer | Name | Promise | Who may name it |
|---|---|---|---|
| 1 | `alloy::uart::config`, `alloy::uart::handle`, the concepts in `alloy/concepts.hpp` | works on every chip alloy claims to support | portable `main.cpp`, `libs/` drivers |
| 2 | `alloy::uart::opts<Inst>` (surfaced as `Role::opts`) | typed, instance-checked, **shaped by the IP** | app code that knows its board; `libs/` only through `requires` probes |
| 3 | `alloy::dev::`, `alloy::ip::` | raw register facts from the chip database | anyone, with the [escape hatch](../guide/escape-hatch.md)'s warnings |

`alloy::hal::` stays what [stability.md](stability.md) already says it is: driver
internals, not a user surface. Layer 2 is *not* `alloy::hal::` — it is a
template declared next to the shared vocabulary and re-exported into
`alloy::uart::`, so a user never types `hal`.

The reason three layers exist rather than two is the cliff. ST ships HAL, LL and
CMSIS registers and publishes `Examples_MIX` projects to show them mixed; alloy
today has a portable facade and raw registers with nothing between, so a user
who wants parity bits on a chip whose driver does not offer them writes register
pokes — *in their own code*, where nothing checks them.

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
*more timer* — they are *more registers*. Those stay what alloy already does:
a different IP tag, therefore a different driver. `st/tim_gp16` needs splitting
into real tiers before the first timer driver is written, not after the seventh;
seven of the 28 uncurated peripherals on the G0B1 are timers.

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

Compile error, always. Never silently ignored, never a runtime error code, never
a clamp. Everything below is **verbatim compiler output** from a reduction
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

---

## The rule

This is the load-bearing output. For any knob on any of the 65 peripherals, ask
these in order and stop at the first yes. Each test is mechanical.

**1. Does honouring it require a pin?**
*Test: does it need a route or an AF programmed?*
→ It is a **binder tag**, not a field. `uart::de<pb1_t>`, `spi::nss<pa4_t>`.

**2. Can every driver alloy ships program it, with the same semantics and the
same unit?**
*Test: would adding it force any current driver to trap, reject, or silently
ignore? Is its unit free of vendor register artefacts?*
→ **Layer 1**, `alloy::<periph>::config`.

**3. Is the answer a count, a limit, a width, or a rate?**
*Test: is it a number the chip database knows, rather than something the app
chooses?*
→ Not a field at all. **`Inst::feat::<name>`**, generated, 0 means absent.

**4. Is there a register field that means exactly this, on some IPs?**
*Test: can you name the register field it programs?*
→ **Layer 2**, `alloy::<periph>::opts<Inst>`, declared only in the drivers whose
IP has it, under the same name and unit everywhere it appears.

**5. Nothing above fits** — an erratum poke, an unmodelled block, a one-off.
→ **Layer 3**, `alloy::dev::`. And **no generic command door, ever**: no
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

### The second axis: who knows the fact

Orthogonal to which layer, and just as load-bearing. Every value must be
assignable to exactly one of three sources:

| Source | What it carries | Example |
|---|---|---|
| `board.json` → `board.hpp` | how the board is **wired** | DE timing, pin swap, line inversion, CS polarity, ADC vref |
| `products/*.toml` → `product.hpp` | what varies between **SKUs** on that board | default baud, oversampling depth, watchdog period |
| the call site | what this **call** legitimately varies | this open's baud, this transfer's word size |

If a value cannot be assigned to exactly one, it is not designed yet. This is
what makes a wide surface readable: a forty-field surface is fine as data and
fatal as app code. An RS-485 product's parity, stop bits and DE timing belong in
a TOML, and `main.cpp` writes `board::rs485::open()`.

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

### 6. Three more holes the layers do not close

Found by trying combinations this page did not anticipate, all on the G0B1RE
unless noted. None of them is a new defect introduced by the layers — they are
the shape's blind spots, stated so nobody rediscovers them at driver 22.

* **A Layer-1 value with no valid divisor.** `open({.baud = 0})` compiles with
  no diagnostic. `baud_div()` divides by it, GCC folds the constant division by
  zero to unreachable, and the emitted code falls into the same `udf` as the
  seam above. The programme traps at run time — loudly, but at a fault PC that
  belongs to the double-open guard, so the *diagnosis* is misleading. With a
  run-time-variable baud there is no fold and no trap: `__aeabi_uidiv` returns
  quietly and the port is misconfigured. **Layer 1 has no admission check on
  the value at all**; only `open_checked<Baud>` does, and it is opt-in.
* **`open_checked` silently discards a conflicting Layer-1 baud.**
  `open_checked<115'200_baud>({.baud = 9'600})` compiles clean and runs at
  115 200: the template argument overwrites `c.baud`. Two spellings of the same
  fact disagree and the loser is never mentioned.
* **The double-open guard is per *binder type*, not per instance.** Two
  different `uart::bind<>` specialisations naming the same `usart2_t` — a
  second, legitimately-routed pin pair — each carry their own `detail_opened`.
  Opening both compiles clean, with *contradictory* Layer 1 and Layer 2
  (`data_bits = 8` then `7`; `115'200/none` then `9'600/odd`), no error and no
  warning; the second `open()` reprograms the peripheral under the first
  handle, which stays usable. Layer 2 makes a *call site* impossible to lie in
  and says nothing about **who owns the instance**. `reconfigure<Opts>()` has
  the same shape: it is callable on a port nobody opened (verified on the F767,
  whose `usart_v3` implements `configure_running`), and its `Opts` need not
  match the ones `open()` used, because Layer 2 belongs to the call and not to
  the handle.

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

**Unproven and labelled:** `espressif_uart_v1` now programs `CONF0` parity and
stop bits, from TRM-derived field positions, with no ESP32 on the bench — and
its two-stop-bit encoding is the known one that needs `UART_RS485_CONF.DL1_EN`
on classic ESP32 silicon, a register alloy does not model. `st_usart_v2`'s
frame programming is likewise compile-checked only (no F4 board), as that
driver already was. Renode models no parity, so no leg asserts a parity bit on
a wire.
