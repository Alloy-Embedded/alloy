# Streaming data without the CPU

A control loop that has to fetch each ADC sample by hand is a control loop that
spends its budget on load/store instructions. A Modbus slave that takes an
interrupt per received byte is a slave whose worst-case latency is set by the
baud rate, not by the work. DMA is how you get those two costs off the CPU —
and this page is how you reach it in alloy without typing a single DMAMUX id,
CHSEL value or DREQ number.

It is also honest about the other half. DMA is the part of this framework where
the gap between *this compiles* and *this has been observed working* is widest,
and the gap is different on every engine. The second half of the page is a
per-board table of what you get, and a per-capability table of what has actually
been witnessed and by what. If you are putting this in a product, read the
second half first.

---

## The one idea

**A DMA route is a fact the board states and the generator validates. A stream
is an object you own. The peripheral facade is where the two meet.**

Everything below follows from that.

You write the route once, in `board.json`, as a role signal and a channel:

```json
"dma": {
  "adc.conv":      { "controller": "dma1", "channel": 1 },
  "debug_uart.rx": { "controller": "dma1", "channel": 2 },
  "debug_uart.tx": { "controller": "dma1", "channel": 3 },
  "spi.rx":        { "controller": "dma1", "channel": 4 },
  "spi.tx":        { "controller": "dma1", "channel": 5 },
  "i2c.rx":        { "controller": "dma1", "channel": 6 }
}
```

That is `boards/nucleo_g071rb/board.json` verbatim. The key is
`role.signal` — the role is one of the [role names](portable-code.md) your
board already wires, the signal is what that peripheral moves. The value names
a controller and a channel, and **the request id is not in there**: which
hardware request line serves `usart2 rx` is the *chip's* fact, and alloy-devices
already knows it.

The generator turns each line into a constant, next to the role bindings in
your generated `board.hpp`:

```cpp
inline constexpr alloy::dma::route<alloy::dev::dma1_t, 1, /*request=*/5>  adc_conv{};        // serves adc conv
inline constexpr alloy::dma::route<alloy::dev::dma1_t, 2, /*request=*/52> debug_uart_rx{};   // serves usart2 rx
inline constexpr alloy::dma::route<alloy::dev::dma1_t, 3, /*request=*/53> debug_uart_tx{};   // serves usart2 tx
```

The request ids `5`, `52`, `53` were looked up, not typed. The same routes are
also attached to the role's bind type, which is what makes portable code work:
`board::adc` knows its own `adc.conv` route, so `adc.ring(storage)` needs no
argument naming a channel, and on a board that assigned nothing the method is
simply not there.

So the three spellings you will see are:

| Spelling | When |
| --- | --- |
| `adc.ring(storage)`, `uart.rx_ring(buf)`, `spi.transfer_dma(tx, rx)` | The normal one. The facade uses the board's route. |
| `alloy::dma::claim(board::dma::debug_uart_tx)` | One-shot transfers, where you hold the channel token yourself. |
| `alloy::dma::channel<board::dma_t, 1>::claim()` | The escape hatch: a board with a DMA driver but no assignment, or a channel you want for something alloy does not model. |

---

## Four working shapes

Each recipe below is code from a shipped example under `examples/`, which CI
builds for every board and (where noted) runs under emulation. Go run the
example rather than retyping the snippet.

### 1. An ADC ring under a control loop

The compressor-inverter case: conversions land in a buffer by themselves and
your loop takes *hardware-stable halves* — the half being filled is never
visible to you.

Board line:

```json
"adc.conv": { "controller": "dma1", "channel": 1 }
```

Code, from `examples/adc_stream/src/main.cpp`:

```cpp
#include <alloy/board.hpp>
#include <alloy/dma.hpp>

// 256 samples = 128 per half. Static storage is the lifetime the stream
// needs, and the reference-taking API pushes toward it.
alloy::dma::ring_storage<std::uint16_t, 256> samples;
constexpr std::uint8_t kChannel = 3;

int main() {
    board::init();
    auto adc = board::adc::open();
    auto ring = adc.ring(samples, kChannel);

    for (;;) {
        std::span<const std::uint16_t> half = ring.take();  // blocks until stable
        process(half);
        if (ring.missed() != 0) { /* you fell behind — a half was overwritten */ }
    }
}
```

`take()` hands you `N/2` samples that the engine has finished writing and will
not touch again until it comes back around. `missed()` counts every stable half
you never collected — overrun is counted, never silent.

!!! warning "`take()` spins forever if a boundary can never arrive"
    That is deliberate: a ring whose peripheral was never started hangs
    honestly rather than returning an unstable half. In a superloop, poll
    `pending()` with a spin budget instead. Both shipped ring examples do
    exactly that, and `examples/adc_stream` prints `STUCK` and fails its
    emulation leg in seconds rather than wedging the runner.

### 2. A UART RX ring, with no per-byte interrupt

The Modbus case. Bytes go from the receive register into your buffer by DMA;
the only interrupt in the path is the UART's IDLE event — the first quiet bit
time after a byte, i.e. the frame gap.

Board line:

```json
"debug_uart.rx": { "controller": "dma1", "channel": 2 }
```

Code, from `examples/modbus_rtu_server/src/main.cpp`:

```cpp
// 256 bytes = the RTU spec's largest ADU.
alloy::dma::ring_storage<std::uint8_t, 256> rxbuf;

auto ring = uart.rx_ring(rxbuf);
for (;;) {
    alloy::sleep_until_event();                                  // IDLE on silicon
    const std::span<const std::uint8_t> bytes = ring.readable();
    if (!bytes.empty() && server.on_bytes(bytes)) {
        ring.consume(bytes.size());
    }
}
```

`readable()` returns everything written since your last `consume()`, as one
contiguous span — at a buffer wrap you get the tail run first and the rest on
the next call. The write position is computed from the engine's live
remaining-count register, so nothing runs per byte.

### 3. Transmit without blocking

Two spellings, depending on whether you have a scheduler.

Blocking-free one-shot, from `examples/dma_uart/src/main.cpp`:

```cpp
auto chan = alloy::dma::channel<board::dma_t, 1>::claim();
// Registered BEFORE the transfer: the channel config register may only be
// written while the channel is disabled, so the interrupt enables are folded
// in at setup.
chan.on_complete(+[](void* flag) {
    *static_cast<volatile bool*>(flag) = true;
}, const_cast<bool*>(&g_dma_irq_fired));

std::uint8_t msg[] = "dma via DMA\r\n";      // in RAM for the memory-side read
uart.write_dma(chan, {msg, sizeof(msg) - 1});
```

Or awaited from a coroutine, from `examples/async_io/src/main.cpp` — the route
spelling, with the channel number nowhere in the file:

```cpp
auto chan = alloy::dma::claim(board::dma::debug_uart_tx);
dma_waiter<decltype(chan)> w{chan};          // constructed BEFORE the transfer
co_await w.run([&] { uart.write_dma_begin(chan, line); });
uart.write_dma_end(chan);
```

### 4. SPI full duplex — the pair

A duplex exchange is one operation with two channels, and every interesting
property of it belongs to the pair rather than to either channel: both are
claimed in one interrupts-masked section so an overlapping claimant traps
deterministically, and the receive side is armed and the peripheral's receive
request raised **before** the transmit side starts — because on a duplex bus the
first byte comes back while the first byte is still going out.

Board lines:

```json
"spi.rx": { "controller": "dma1", "channel": 4 },
"spi.tx": { "controller": "dma1", "channel": 5 }
```

Code, from `examples/spi_read/src/main.cpp`:

```cpp
const std::uint8_t out[] = {0xC0, 0xFF, 0xEE, 0x77};
std::uint8_t in[4] = {};
if (!spi.transfer_dma(out, in)) {
    // bounded refusal, not a hang
}
```

Nothing there names a channel, a DMAMUX id or a CHSEL value; the routes ride
the binder. `transfer_dma` claims the pair, runs it, waits for both halves —
a duplex that half-failed did not succeed — and releases both channels when it
returns.

The I2C one-shot is the same idea with an explicitly held token, from
`examples/i2c_read/src/main.cpp`:

```cpp
auto rx = alloy::dma::claim(typename B::rx_route{});
std::uint8_t in[3] = {};
if (bus.read_dma(rx, kAddr, in)) { /* … */ }
```

---

## Consuming a ring: two disciplines

One type, two contracts, and they are for different jobs.

| | `take()` / `missed()` / `pending()` | `cursor()` / `readable()` / `consume()` |
| --- | --- | --- |
| Shape | Half-buffer batches | Byte stream |
| Audience | Control loops (ADC) | Framed protocols (UART RX) |
| Granularity | `N/2` items at a time | Any number of items |
| Overrun | **Checked.** Every uncollected half counts in `missed()`. | **Not checked at item granularity.** A writer that laps an idle reader silently overwrites unread items; `missed()` is still your overrun evidence. |
| Blocking | `take()` blocks; `pending()` is the non-blocking probe | Never blocks; an empty span means nothing new |

Both are **thread context only**. `on_boundary()` and (on a UART ring)
`on_idle()` run in interrupt context, and the alloy IRQ contract applies there:
set a flag or wake a task, nothing else.

Lifetime is RAII. Constructing the ring claims the channel and starts the
hardware; destroying it stops the hardware, detaches the events and releases the
claim, in that order, so no ISR can fire into a dead object and no new claimant
can grab a channel that is still moving data. The peripheral side of teardown
belongs to the facade wrapper and runs first — which is why
`uart::rx_stream` exists as a wrapper at all, rather than handing you a bare
ring.

`ring_storage<T, N>` refuses `N` that is odd, below 2, or above 65535 at compile
time. The raw-`std::span` overloads accept the same buffer without the type
telling you its lifetime, and trap at run time on the same conditions — that is
the sharp edge, offered deliberately for callers who have their own storage.

---

## When your board assigns nothing

The method is **not there**. That is the whole design: a capability the board or
the silicon does not have is a compile error naming what you asked for, never a
link error and never a runtime surprise.

Portable code asks first. The spelling matters more than it looks:

```cpp
[&uart]<class AdcBind>(AdcBind*) {
    if constexpr (requires(decltype(AdcBind::open()) h) { h.ring(samples, kChannel); }) {
        auto adc = AdcBind::open();
        auto ring = adc.ring(samples, kChannel);
        // …
    } else {
        uart.write("adc ring: not available on this board\r\n");
    }
}(static_cast<board::adc*>(nullptr));
```

!!! danger "The generic lambda is load-bearing — do not simplify it away"
    A discarded `if constexpr` branch **outside a template** is still
    name-looked-up and type-checked, so the naive spelling hard-errors on
    exactly the board it exists to skip. Wrapping the probe in a generic lambda
    that takes the bind type as a pointer parameter makes the names dependent,
    which is what lets the branch be discarded. Probe through the *handle's*
    aliases (`typename S::rx_route`) rather than through
    `board::dma::spi_rx`: a missing namespace-scope constant is a hard error,
    not a substitution failure, so it will not fold.

## When the assignment is illegal

The generator refuses at build time and tells you the legal alternatives. These
are real outputs of `alloy board-validate --file …` on modified copies of the
shipped boards:

Two signals on one channel:

```console
error: dma.spi.tx: dma 'spi.tx': dma1 channel 3 already serves 'debug_uart.tx' — one channel moves one stream  (try: 5, 7)
```

A channel that does not exist:

```console
error: dma.spi.tx: dma 'spi.tx': dma1 has channels 1..7, not 9  (try: 5, 7)
```

A signal the peripheral does not have:

```console
error: dma.spi.miso: dma 'spi.miso': the chip states no DMA request for spi1 'miso'  (try: spi.rx, spi.tx)
```

A signal the chip routes nowhere at all — this is the STM32F7 ADC:

```console
error: dma.adc.conv: dma 'adc.conv': the chip states no DMA request for adc1 'conv' — adc1 advertises no DMA requests at all  (try: debug_uart.rx, debug_uart.tx, spi.rx, spi.tx)
```

The wrong key for the engine — free routers take `channel`, ST's stream engines
take `stream`:

```console
error: dma.debug_uart.tx: dma 'debug_uart.tx': usart3 'tx' rides a stream engine: the key is 'stream' (0-based, ST stream numbering), not 'channel' (dma_v1's 1-based) — legal: {dma1, stream 3}, {dma1, stream 4}  (try: {dma1, stream 3}, {dma1, stream 4})
```

A stream that cannot reach that peripheral:

```console
error: dma.debug_uart.tx: dma 'debug_uart.tx': {dma1, stream 2} does not reach usart3 'tx' — the chip's dma_routes allow only: {dma1, stream 3}, {dma1, stream 4}  (try: {dma1, stream 3}, {dma1, stream 4})
```

And when a capability is missing rather than a route, the compile error names
the capability. Asking for `adc.ring()` on a Raspberry Pi Pico:

```console
error: no matching function for call to 'alloy::adc::handle<…>::ring(alloy::dma::ring_storage<short unsigned int, 256>&, int)'
note: the required expression 'alloy::hal::adc_impl<Inst>::dma_burst_begin(uint8_t{})' is invalid
note: nested requirement 'ring_capable<typename Route::controller, ((unsigned int)Route::channel)>' is not satisfied
```

Two named reasons at once: that ADC driver has no DMA hooks, *and* that engine
cannot present a ring.

!!! warning "An assignment is a promise the driver may not be able to keep"
    Validation checks the board statement against the chip's routing data. It
    does not check that the peripheral *driver* has DMA entry points. Three
    assignments in the shipped boards are inert today for exactly that reason:
    `same70_xplained`'s `spi.rx`/`spi.tx` and `i2c.rx`/`i2c.tx` (the Microchip
    SPI and TWIHS drivers have no DMA hooks), and `raspberry_pi_pico`'s
    `adc.conv` (no ring on that engine, and no DMA hooks on that ADC driver).
    The build succeeds and the method folds away.

---

## What each engine can do

Four engine families ship. `ring_capable` — the concept that gates every ring —
asks `supports_ring`, and `supports_ring` is **not** `supports_circular`:

- `supports_circular` is a statement about *hardware*: this engine has a
  circular mode bit that reloads the count and wraps by itself.
- `supports_ring` is a statement about *this engine's code*: it can present
  `alloy::dma::ring`'s contract, by whatever means.

| Engine | Families | `supports_circular` | `supports_ring` | Note |
| --- | --- | --- | --- | --- |
| `st/dma_v1` | STM32 G0 (free router + DMAMUX) | yes | yes | The same fact spelled twice. |
| `st/dma_v2` | STM32 F4 / F7 (stream engine, CHSEL) | yes | yes | Same. |
| `microchip/xdmac_v1` | SAM E70 | **no** | **yes** | No circular bit exists in the IP at all. A ring is two linked view-0 descriptors ping-ponging the halves. `start_m2p_circular_u16` is correctly absent. |
| `raspberrypi/dma_v1` | RP2040 | no | no | A single channel halts at count zero and goes permanently empty; there is no honest degraded ring, so there is none. |

STM32 **G4** is a gap of a different kind: the chip data declares `dma1`/`dma2`
with no channel geometry and no DMAMUX companion, so the driver's `mux_t`
requirement is unsatisfiable and every DMA facade method is constrained away on
that family. 93 G4 parts are in `alloy chips` and none ships as a board; if you
clone one, expect the folded-away branch until the chip data grows those keys.

## What each board gives you

Rows are the boards that ship; the answers were measured by building the
matching example for each board and reading the linked image, not by reading
source. Find your board, then read across.

| Board | `adc.ring` | `uart.rx_ring` | `uart.write_dma` | `spi.transfer_dma` | `i2c.read_dma` | `adc.read_burst` | `pwm.stream_duty` |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `nucleo_g071rb` | yes | yes | yes | yes | yes | yes | yes |
| `nucleo_g0b1re` | yes | yes | yes | yes | yes | yes | yes |
| `nucleo_f722ze` | not routable | yes | yes | yes | no role | no hooks | no role |
| `nucleo_f767zi` | not routable | yes | yes | no role | no role | no hooks | no role |
| `same70_xplained` | **yes — never run** | no hooks | yes | no hooks | no hooks | yes | no role |
| `raspberry_pi_pico` | no ring | no hooks | yes | no role | no role | no hooks | no role |
| `rp2040_zero` | no role | no hooks | yes | no role | no role | no role | no role |
| `esp32_devkit` | no DMA | no DMA | no DMA | no DMA | no DMA | no DMA | no DMA |
| `esp_wrover_kit` | no DMA | no DMA | no DMA | no DMA | no DMA | no DMA | no DMA |

Every negative cell names *which* gate failed, because that is the difference
between four completely different fixes:

| Cell | What failed | What you do about it |
| --- | --- | --- |
| `no route` | The board file assigns no channel for this signal. | Add the line to `board.json`, or pass an explicit route / hand-claimed channel. |
| `not routable` | The **chip data** states no request for this signal, so no board file can legally assign it. | File it against alloy-devices, or use the escape hatch and take responsibility for the request id. |
| `no hooks` | The peripheral **driver** has no DMA entry points. | Write them, or use the polled path. The board may already assign a channel; it is inert. |
| `no ring` | The engine cannot present a ring (`ring_capable` is false). | This silicon will not do it. Use one-shots. |
| `no role` | The board declares no `spi` / `i2c` / `adc` role at all. | Wire the role in `board.json` first. |
| `no DMA` | The chip has no DMA controller in the data at all. | Nothing to reach. |

One call is missing from the table on purpose. `i2c.write_read_dma()` — a
repeated start under DMA — is a **deleted member** on every board. Calling it is
a compile error naming the function, and a `requires` probe on it is false. The
reason is that `CR1.TCIE` is deliberately uncurated in alloy-devices' ST I2C
data, so the write-to-read handoff has no reachable event; a method that could
only be implemented by spinning on an uncurated bit would be a promise the
facade cannot keep.

Two more per-board details the grid cannot hold:

- `nucleo_g071rb` assigns `i2c.rx` but not `i2c.tx` (the die offers nine
  assignable signals for seven channels), so `i2c.write_dma` on that board needs
  a hand-claimed channel. `nucleo_g0b1re` assigns both.
- `pwm.stream_duty` exists only where the timer driver has `dma_update_begin`
  *and* the timer instance publishes an update request — today the STM32 G0
  general-purpose 16-bit timer only.

---

## How well proven, and what is still owed

Three words, used precisely, for the rest of this page:

- **Renode-proven** — a CI leg runs it on an emulated die. Blocking legs gate
  the build; experimental legs are `continue-on-error` while a new board+robot
  pair earns trust.
- **Host-tested** — the real driver code runs over a memory-backed register
  double on a laptop. This proves sequence and bookkeeping. It proves nothing
  about whether the register map is right.
- **Compiles only** — the branch was selected at compile time and linked.
  Nothing has observed it move a byte.

And a footer on all of it, in the words `alloy chip-status` closes with:
**none of this is evidence from silicon.** No DMA stream in this framework —
no ring, no pair, no `rx_ring` — has ever run on a real part. (The one
first-party silicon result this project has is the
[bus bridge on a SAM E70](bus.md); it does not use DMA.)

### Read this one first

!!! danger "`adc.ring()` on `same70_xplained` compiles, links, and has never been run"
    Every other honest gap in this framework fails **loudly** — a compile error
    naming the missing capability. This one does not. The XDMAC ring builds
    clean: the linked image for `examples/adc_stream` on that board contains the
    stream type and the ping-pong descriptors in `.bss`, with no fallback
    branch. A reader who checks by compiling will conclude it works.

    What is unproven is the **descriptor layout itself**. What a view-0
    descriptor contains in memory — word order, and the bit packing of its own
    control word — is stated in **no file in either repository**. The register
    curation covers the registers that *point at* descriptors exhaustively, with
    datasheet provenance; the descriptor structure is not a register and is not
    there. The pinned vendor pack has every XDMAC register bit and zero
    descriptor-structure content.

    The host test (`tests/test_xdmac_v1_ring.cpp`) runs the real engine over a
    double that genuinely fetches and executes the descriptors. It proves
    linkage, ping-pong parity, cursor arithmetic across the half boundary and
    teardown — **against the driver's own reading of the layout**. It cannot
    prove the reading. If the reading is wrong, every test still passes and the
    first silicon run takes a bus error or writes to a wild address.

    The reading is corroborated by a second, independent implementation
    (Zephyr's SAM XDMAC driver declares the same three words in the same order
    and puts the control word's bits in the same positions). Two independent
    readings agreeing is evidence, not a citation — they can share a misreading
    of one figure.

    **Confirm the structure and the UBC bit positions against DS60001527,
    "Linked List Descriptor View 0", before shipping this to hardware.**
    Everything the confirmation touches is one struct and four named constants
    in `src/alloy/hal/dma/microchip_xdmac_v1_body.hpp`.

### The witness table

| Capability | Strongest witness | What a green run actually asserts | What only a board can close |
| --- | --- | --- | --- |
| `uart.write_dma` on STM32 G0 | Renode, blocking CI leg (`dma_uart`) | The bytes crossed from RAM to the USART on the programmed channel, and the completion IRQ fired. | Which peripheral asked — the request id is never witnessed. |
| `adc.ring` on STM32 G0 | Renode, blocking CI legs on `nucleo_g071rb` and `nucleo_g0b1re` (`adc_stream`) | In `take()` order: the half event arrived before the full event, the full event arrived, and the ring wrapped — with each half pinned to the millivolts fed to that channel, so neither routing nor data can pass by coincidence. Two negative controls verified red. | DMAMUX request routing (unmodelled). Once-per-boundary half events — the model re-raises the half flag per sample, so `missed()` is diagnostic under emulation, not evidence. Free-running continuous conversion (a tagged no-op paced by a shim). The event-attach order. |
| `uart.rx_ring` | Renode, blocking on `nucleo_g071rb`; experimental on `nucleo_g0b1re`, `nucleo_f722ze`, `nucleo_f767zi` (`modbus_rtu`) | Modbus RTU frames received into the ring with no per-byte ISR, read back by an independent master implementation. | The request id. And the IDLE wake itself: the pinned USART model does not implement IDLE at all, so under emulation the wakes are SysTick's. |
| `spi.transfer_dma` (the pair) | Renode, blocking on `nucleo_g071rb`; experimental on `nucleo_g0b1re`, `nucleo_f722ze` (`spi_read`) | The channel/stream **index**, the RX-before-TX arming order, and the receive-request enable — each with a negative control that goes red rather than hanging. | The request id, on either engine. |
| SAM E70 XDMAC completion IRQ | Renode, experimental CI leg (`dma_uart`) — against a model **this project had to generate**, because Renode 1.16.1 ships no Microchip DMA model of any kind | The bytes crossed and the XDMAC interrupt line fired. Two negative controls. | PERID routing, unwitnessable by construction: the pinned USART model exposes no DMA-request output, so no model can consume one. The model triggers the whole microblock on the enable write and never consults PERID. |
| `i2c.read_dma` / `i2c.write_dma` | Host-tested (`tests/test_i2c_dma.cpp`, 18 cases over a memory-backed double) | The emulation leg asserts a **bounded refusal** — it waits for the literal line `i2c dma: no transfer` — because Renode's model of this IP exposes no DMA request output at all, so nothing can ask the engine to move a byte. | That a single byte ever moves by DMA on this bus. No leg anywhere witnesses an I2C DMA transfer. |
| `adc.ring` on SAM E70 (XDMAC linked-list ring) | Host-tested — and the host test cannot prove the part that breaks | Linkage, ping-pong parity, cursor arithmetic across the half boundary, teardown — against the driver's own reading of the descriptor layout. | The descriptor layout itself. See the box above. |
| RP2040 one-shot DMA | **Nothing under emulation, and a leg here would be worse than absent** — Renode reads 0 from unmapped addresses, so a leg would report a finished transfer over a buffer of zeros | The CI gate reads the linked image and fails if the fallback branch is present. It asserts that the DMA path was *compiled in*, and nothing about bytes crossing. The behavioural witness is `tests/test_rp_dma_v1_latch.cpp`: 23 mutations, 20 go red, 3 are named as staying green. | Everything behavioural. See the [RP2040 DMA hardware checklist](rp2040-dma-hardware-checklist.md) — eight RUN / LOOK FOR / FALSIFIED BY rows, **written and never executed**. |
| `pwm.stream_duty` | Compiles only | Nothing. There is no emulation leg and no host test. | All of it. |
| `alloy::async::ring_waiter` | Host-tested (two cases in `tests/test_dma_ring.cpp`) | A parked task wakes once per boundary, and a slow consumer resynchronizes. | All of it, on a target. |

### A route is witnessed by halves

This is the one sentence to carry into a bring-up:

> **Emulation proves which channel moved the bytes. Only silicon proves that
> the right peripheral asked for them.**

It is measured, not assumed, and on three families with three different
strengths:

- On the **F7 stream engine** it was measured by adversarial control. Move the
  model's request wire by one stream and the leg goes red — so the stream index
  is load-bearing end to end. Delete the channel-select write from the driver
  and the leg stays **green** — so the request id is not witnessed at all.
- On the **G0** Renode models no DMAMUX whatsoever, and the platform wire and
  the firmware route both descend from the same `board.json` statement, so the
  request half cannot be witnessed even by coincidence.
- On the **SAM E70** it is worse by construction: the USART model exposes no
  DMA-request output, so there is no wire for any model to consume.

What that means for you: **the first time you bring up a new board's DMA
assignment, verify the peripheral is really the source.** Feed a distinctive
pattern and check that it is what arrives, or perturb the request id in the chip
data and confirm the transfer *stops*. The entire framework tests that in
exactly one place — row 4 of the RP2040 hardware checklist — and that place has
never been executed.

---

## Sharp edges

The things you would otherwise find at 2 a.m.:

!!! warning "Read before your first DMA bring-up"
    - **The SAM E70's memory-side master cannot read embedded flash.** A
      `.rodata` source raises a read bus error — found on real silicon. Copy the
      payload to RAM before any `write_dma` / `transfer_dma` on that part. The
      ST engines read flash fine, so portable code that works on a G0 can break
      on a SAM E70 for this reason alone.
    - **`ring::take()` spins forever if no boundary can arrive.** Wrap it in a
      bounded `pending()` poll, the way both shipped ring examples do.
    - **`ring_storage<T, N>` demands an even `N` in 2..65535** at compile time.
      The raw-span overloads trap at run time on the same conditions plus an
      empty buffer.
    - **A ring stored in a static, holding a span to a shorter-lived buffer, is
      the corruption scope cannot close.** Taking `ring_storage` by reference is
      the countermeasure, which is why that is the default spelling.
    - **`take()` / `readable()` / `consume()` are thread context only.**
      Callbacks run in interrupt context.
    - **Teardown order is peripheral request off, then channel stop.** The
      facade wrappers encode it in member-destruction order; a bare
      `alloy::dma::ring` does not get it for free.
    - **A board's `dma:` assignment is a promise the driver may not be able to
      keep.** Three shipped assignments are inert today.
    - **Register the completion callback before starting a transfer.** The
      channel's config register may only be written while the channel is
      disabled, so the interrupt enables are folded in at setup — a callback
      registered afterwards arms nothing.

---

## Where to go next

- **Run one.** `examples/adc_stream`, `examples/modbus_rtu_server`,
  `examples/spi_read`, `examples/i2c_read`, `examples/dma_uart`,
  `examples/dma_probe`, `examples/async_io` — every snippet on this page came
  from one of them, and CI builds all of them for every board.
- **Add a route to your own board** — [Adding a board](adding-a-board.md).
- **Await a boundary instead of polling one** — [Async](async.md), and
  `alloy::async::ring_waiter`.
- **See what the legs assert, in their own words** —
  [Emulation](emulation.md), and the design record this feature came from,
  [DMA streams](../design/dma-streams.md).
