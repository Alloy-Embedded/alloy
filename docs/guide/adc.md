# Reading an analog input

This page takes you from one conversion to the advanced surfaces — scans,
oversampling, the analog watchdog, the injected group and DMA streaming — and
it is honest about which of those your chip actually has.

That honesty is the point. Alloy will not offer you a knob it cannot program on
your part: a feature the silicon lacks is **not a member**, so asking for it is
a compile error naming the thing you asked for, never a value that silently
goes nowhere. The cost is that you sometimes have to ask "does this port have
it?" — and this page shows you the one spelling of that question that works.

---

## Your first conversion

```cpp
#include "alloy/adc.hpp"
#include "alloy/board.hpp"

int main() {
    board::init();
    auto adc = board::adc::open();
    std::uint16_t raw = adc.read(3);      // channel 3, raw counts
}
```

`read()` blocks until the conversion finishes and returns **raw counts**, right
aligned. It does not return volts, and no alloy driver converts counts to volts
or to degrees: that needs a reference voltage and a transfer function which are
board and datasheet facts, not framework facts.

`board::adc` exists only on boards whose `board.json` declares an `adc` role.
Portable code checks first:

```cpp
if constexpr (board::caps::adc) {
    auto adc = board::adc::open();
    // …
}
```

---

## Internal channels, and a constant that looks odd on purpose

Many parts wire an internal reference, a temperature sensor or the battery rail
to their own channels. Where the chip database knows those channel numbers, the
board publishes them:

```cpp
if constexpr (board::adc_has_temp) {
    std::uint16_t t = adc.read(board::adc_temp_channel);
}
```

**Always guard with the `has_` flag.** On a board whose chip data does not map
that source, `board::adc_temp_channel` is still emitted — it has to be, because
the discarded branch of an `if constexpr` outside a template is still
type-checked, so omitting the symbol would break the very code that is doing
the right thing. Instead it carries `alloy::adc::channel_none` (0xFF), which is
not a channel on any part alloy supports, and reaching for it without the guard
is a compile error:

```
error: alloy::adc::read: this board has no such ADC channel. The channel number
is 0xFF, the value a board emits for an internal source that its CHIP DATA DOES
NOT MAP … Guard the read with `if constexpr (board::adc_has_vref)` …
```

So the failure mode is a diagnostic, not a reading from whatever pin happens to
sit at channel 255 on a future part.

!!! note "Which boards have the map"
    The G0 parts carry it. The F4/F7 and G4 parts do **not** — nothing in the
    curated data gives their VREFINT/temperature channel numbers, and alloy
    does not invent silicon facts. The SAME70 does (temperature on channel 11).

---

## Layer 2 — the knobs your chip actually has

Resolution, oversampling and sampling time are **not** arguments to `open()`.
They are a compile-time value whose *members are declared per IP*, so a knob
your silicon lacks is not a member:

```cpp
auto adc = board::adc::open<board::adc::opts{
    .resolution_bits  = 12,
    .oversample_ratio = 16,   // average 16 samples…
    .oversample_shift = 4,    // …back onto the 12-bit scale
}>();
```

Spell the type at the call site (`board::adc::opts{…}`, not `open<{…}>()`).
Six extra characters buy a diagnostic that names the member instead of
"template argument deduction/substitution failed".

**What each family offers today:**

| Family | resolution | oversampling | sampling time | notes |
|---|---|---|---|---|
| STM32 G0 | 12/10/8/6 | ratio 2–256 + shift | two shared times + a per-channel selector | also `trigger` / `trigger_edge` |
| STM32 G4 | 12/10/8/6 | ratio 2–256 + shift | fixed at the longest | |
| STM32 L4 | 12/10/8/6 | ratio 2–256 + shift | fixed at the longest | **defaults to 16×, no shift** — see below |
| STM32 F4/F7 | 12/10/8/6 | *none in silicon* | per channel | plus `clock_divider` |
| SAME70 AFEC | 12 only | *coupled to width* | — | see below |
| RP2040 | — | — | — | no knobs; the silicon has none |

Two rows deserve a sentence each, because their defaults are not "off":

- **L4 defaults to 16× oversampling with no shift**, so `read()` returns a
  16-bit-wide value. That has always been this driver's behaviour and changing
  it would break existing callers; it is a knob now rather than a hardcoded
  decision. If you want the plain 12-bit scale, ask for
  `{.oversample_ratio = 16, .oversample_shift = 4}`.
- **The SAME70's AFEC couples resolution and oversampling into one field** —
  every bit above 12 *is* averaging at a lower sample rate. Alloy programs that
  field correctly but its one-shot read cannot complete an averaged conversion,
  so widths above 12 are refused with that reason. This was found on hardware,
  not in a manual.

### Asking what you got

```cpp
constexpr unsigned bits = board::adc::result_bits<board::adc::opts{
    .oversample_ratio = 16, .oversample_shift = 0}>;   // == 16
```

Oversampling widens a result by `log2(ratio)` and narrows it by the shift. This
matters beyond curiosity: a port producing wider results than 12 bits **cannot
arm an analog watchdog**, because the threshold registers are 12 bits wide, and
alloy refuses that combination at compile time rather than guessing at a scale
conversion.

---

## Probing for a capability — the one spelling that works

This is the part most likely to cost you an afternoon, so it is worth stating
plainly. **Two obvious forms do not work:**

```cpp
// ✗ hard error, not `false`: a requires-expression over a CONCRETE type is
//   checked eagerly.
if constexpr (requires { adc.scan(chans, out); }) { … }

// ✗ also fails: outside a template, the DISCARDED branch of if constexpr is
//   still name-looked-up and type-checked.
if constexpr (alloy::adc::can_scan<board::adc::instance>) { adc.scan(…); }
```

**The working form puts the branch inside a template**, which a generic lambda
does in one line:

```cpp
[&]<class Bind>(Bind*) {
    auto adc = Bind::open();
    if constexpr (alloy::adc::can_scan<typename Bind::instance>) {
        adc.scan(channels, out);
    } else {
        for (unsigned i = 0; i < n; ++i) { out[i] = adc.read(channels[i]); }
    }
}(static_cast<board::adc*>(nullptr));
```

The capability constants are `alloy::adc::can_scan<Inst>`,
`alloy::adc::can_inject<Inst>` and `alloy::adc::watchdog_count<Inst>` (a count,
where 0 means "not reachable here").

---

## Multi-channel scan

Convert a list of channels **in the order you give, repeats included** — which
is precisely what a channel bitmap cannot express, and why this exists only on
parts with a real sequencer:

```cpp
const std::uint8_t channels[] = {3, 4, 3};
std::uint16_t values[3];
if (adc.scan(channels, values)) { /* values[0..2] in that order */ }
```

Available on **F4/F7**. Not a member on the G0, whose converter is driven by a
bitmap: there, `adc.scan(...)` is a compile error rather than a runtime
refusal. It returns `false` for an empty list or an output buffer too small —
truncating would hand back a tail of stale data indistinguishable from a
reading.

---

## The analog watchdog

Hardware that watches conversions against a window and latches a flag when one
falls outside it — no CPU polling of values, no interrupt required:

```cpp
if constexpr (board::adc::watchdogs > 0) {
    auto wd = adc.watchdog<0>({.channel = 3, .low = 1000, .high = 3000});
    // …
    if (wd.tripped()) { wd.clear(); }
}
```

Three things the silicon forces, all visible in the API:

- **The channel is part of arming**, not a later call — it shares a register
  with the enable, and both are writable only with the converter disabled.
- **Arming cycles the port.** The driver stops any conversion, disables the
  ADC, programs the window and re-enables it.
- **The N are not interchangeable.** On the G0, watchdog 1 guards one channel
  or all of them while 2 and 3 guard an arbitrary set. On the G4, watchdogs 2
  and 3 have *8-bit* thresholds where the converter produces 12 — alloy arms
  ordinal 0 there and refuses 1 and 2 by name, rather than guessing which bits
  a narrower threshold compares against.

Available on **G0** (three) and **G4** (ordinal 0 of three).

---

## The injected group

A second, higher-priority sequence that interrupts the regular one and leaves
its results in **separate data registers** — so a regular sequence streaming to
DMA is undisturbed by an injected conversion landing in the middle of it. That
independence is the whole feature.

```cpp
const std::uint8_t urgent[] = {3, 10};
auto g = adc.injected(urgent);
g.start();                       // software start
while (!g.ready()) { }
std::uint16_t a = g.read(0), b = g.read(1);
```

Available on **F4/F7**, software-started only: the hardware-trigger path needs
a register encoding the chip database does not carry, so alloy does not offer
it rather than half-offering it. Calling `start()` from a timer's own ISR is
the supported way to sample at a chosen instant.

!!! warning "One constant in this path is not yet witnessed on hardware"
    Which of the four sequence slots run for an N-channel group is a reference
    manual statement alloy has no served source for. Both readings are written
    out at `st_adc_f4::slot_order`, the chosen one is a single named constant,
    and `slot_order_witnessed_on_silicon` is `false` in the code. Picking wrong
    converts a *different channel* and returns a plausible number, so if you
    depend on this, run the two-channel test the driver header describes before
    trusting the order.

---

## Streaming with DMA

For continuous sampling the CPU should not touch, `ring()` hands you
hardware-stable halves of a caller-owned buffer:

```cpp
alloy::dma::ring_storage<std::uint16_t, 256> storage;
auto stream = adc.ring(storage);        // uses the board's `adc.conv` route
for (;;) {
    std::span<const std::uint16_t> half = stream.take();   // blocks
    process(half);
    if (stream.missed()) { /* a half was overwritten before you took it */ }
}
```

This exists only where the board assigned a DMA route *and* the routed
controller can do circular transfers with half-transfer events. Otherwise the
method is constrained away — a compile error, never a link error or a runtime
surprise. See [the DMA streams design](../design/dma-streams.md) for the
teardown ordering it encodes.

---

## What alloy does not do, and why

- **No volts, no degrees.** Counts only. The reference and the transfer
  function are board and datasheet facts.
- **No hardware trigger selection** on most parts. The field is curated, its
  *encoding* is not, and a curated field with an uncurated encoding is a magic
  number wearing an accessor.
- **No pin muxing for analog inputs on the RP2040.** GPIO26–29 reach the
  converter only with their digital pad disabled, which is not an alternate
  function; prepare the pad yourself. The temperature input needs no pin.
- **No injected group, scan or watchdog where the register data does not
  support them.** In every case the method is absent rather than failing at
  run time.

If you need something in that list, the [escape hatch](escape-hatch.md)
describes the layers below this facade and what each of them can still reach.
