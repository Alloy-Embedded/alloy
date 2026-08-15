# DMA streams — design

!!! note "Status: design, not shipped"
    This page is the decided design for DMA-fed peripherals — SPI, UART, I2C,
    ADC — across every ARM family alloy ships. Nothing below exists in the tree
    unless it says so explicitly. Sizes are labelled: **measured** means we ran
    it or counted it this week; **estimate** means engineering judgement an
    implementer should re-check on contact with the silicon or the data.

## 1. The decision, and the vocabulary

Three sentences, then the words they use:

> **A DMA route is a generated fact, a stream is an owned object, and the
> peripheral facade is the only place the two meet.** The board states which
> channel serves which role signal; the generator validates that statement
> against the silicon and emits a typed `route`; the facade turns a route plus
> a user buffer into a stream and claims the channel(s) through the existing
> `alloy::claim` machinery. User code never names a DMAMUX id, a CHSEL value,
> or a DREQ number.

This is the surface doctrine's question 0 applied to DMA: the database already
knows the request routing — three different ways on three silicon shapes — so
routing lives in data, and the C++ consumes it by name.

### The vocabulary

| Word | Meaning | Who produces it |
|---|---|---|
| **endpoint** | the peripheral half of a DMA hookup: data-register address + request id + item width + direction. | the facade + the instance descriptor (`Inst::dmareq_*`, `tdr_addr()` — both exist today) |
| **route** | `alloy::dma::route<Controller, Ch, Request>` — a typed constant saying "this board serves this signal with this channel". | **generated**, per board, validated against the chip's routing data |
| **transfer** | a bounded one-shot move with a completion. Exists today as `start_p2m_u16` / `start_m2p_u8` + poll / callback / awaitable. | `alloy::dma::channel` (shipped) |
| **ring** | a circular stream over a user buffer with half/full events and a read cursor. The ADC-into-control-loop shape and the UART-RX-console shape. | new: `alloy::dma::ring<T>` |
| **pair** | two channels claimed as one unit, started and stopped together. The SPI full-duplex shape. | new: claimed by `spi::transfer_dma` |

### Who claims what

Unchanged mechanism, one new row for the pair:

- A **stream claims its channel** at construction via
  `claim::sub_exclusive<Controller, Ch, personality::dma>` — exactly what
  `dma::channel::claim()` does today, same trap code, same fault-report story.
- A **pair claims both channels, RX first, TX second**, in one
  interrupts-masked section. Fixed order so two claimants of an overlapping
  pair produce a deterministic trap, not an order-dependent one.
- Stream destruction **stops the hardware and releases the claim**
  (`sub_release` — the scope that already has a disarm, built for EXTI lines).
  This is a deliberate departure from `dma::channel::claim()`'s
  one-token-per-firmware rule, and it is what makes the lifetime story in
  §4 honest instead of hopeful. The old `channel` API keeps its semantics;
  streams are the new surface.

### Where the route comes from — the three silicon shapes, one statement

| Family | Silicon shape | What constrains the board's choice |
|---|---|---|
| STM32 G0/G4 (dma_v1 + DMAMUX) | any channel serves any request; `dma_requests: {rx: 50}` is chip-wide | nothing but collision with other roles |
| STM32 F4/F7 (stream engine, CHSEL) | one signal reaches only the `dma_routes` triples; often exactly two alternatives | the triples — the generator refuses anything else, and the error lists the legal alternatives |
| RP2040 (DREQ, no router) | any of 12 channels; request = per-peripheral DREQ id | nothing but collision |

The board file states assignments once, in `board.json`:

```json
"dma": {
  "adc.conv":       {"controller": "dma1", "channel": 1},
  "debug_uart.rx":  {"controller": "dma1", "channel": 2},
  "debug_uart.tx":  {"controller": "dma1", "channel": 3}
}
```

and the generator emits, per role, a typed constant **and** attaches it to the
role's binder:

```cpp
// GENERATED — board::dma
namespace board::dma {
inline constexpr alloy::dma::route<alloy::dev::dma1_t, 1, /*request=*/5>  adc_conv{};
inline constexpr alloy::dma::route<alloy::dev::dma1_t, 2, /*request=*/50> debug_uart_rx{};
}
```

The request number is **never** in the board file — it is the chip's fact
(`dma_requests` on G0/G4/RP2040-DREQ, the matched `dma_routes` triple on
F4/F7). The board picks only what the board is entitled to pick: which channel.
On F4/F7 even that choice is checked: `{"controller": "dma2", "channel": 5}`
for a signal whose triples name only dma1 streams 1 and 3 is a **generation
error** that prints both legal options. Collisions — two roles, one channel —
are refused at generation exactly like two roles on one peripheral are today.

A board that assigns nothing still builds; the facade's stream methods are
gated (`requires`) on the role carrying a route, so `uart.rx_ring(buf)` on a
board with no assignment is a compile error naming the missing fact, and the
explicit-route overload `uart.rx_ring(some_route, buf)` always exists for
hand-wired projects.

**Rejected alternatives, for the record.** (1) *Automatic allocation at
compile time* — a constexpr allocator picking free channels. Rejected: the
assignment becomes an artifact of link order and feature flags, two builds of
one product can differ silently, and a fault report cannot name "the channel
the allocator felt like". Boards are where wiring facts live (the same answer
Zephyr's `spi_dt_spec` gives for per-device SPI config). (2) *Explicit-always* —
today's `channel<Inst, 1>::claim()` everywhere. Rejected as the default
because it makes the user restate a fact the database already knows, per
family, which is the exact failure the brief calls out; it survives as the
escape hatch.

## 2. The four anchor cases

These four are the acceptance test for the whole design. They are written
against boards alloy ships, and each names the phase (§6) that makes it real.

### 2.1 ADC ring under a control loop (the compressor-inverter case) — phase 1

```cpp
// One portable main.cpp: ADC samples stream into a ring while the control
// loop consumes stable halves. The CPU never touches the sampling path.
#include <alloy/board.hpp>
#include <alloy/dma.hpp>

// 256 samples = 128 per half. alignas(32) is part of ring_storage: free today,
// load-bearing the day an M7 board runs this with D-cache enabled (§4).
static alloy::dma::ring_storage<std::uint16_t, 256> samples;

int main() {
    board::init();
    auto adc = board::adc::open();

    // Claims the board-assigned channel, programs a circular p2m transfer of
    // 16-bit items, arms the half + full interrupts, starts the peripheral's
    // continuous conversion. Compile error if this board assigned no route or
    // this controller cannot do circular (today: SAME70 XDMAC — §3.3).
    auto ring = adc.ring(samples);

    for (;;) {
        // Blocks until the OTHER half is hardware-stable, then hands it over.
        // The half being filled is never visible to user code.
        std::span<const std::uint16_t> half = ring.take();
        control_step(half);              // 128 fresh samples, zero copies
        if (ring.missed() != 0) {        // consumer fell behind: counted, never silent
            board::led.toggle();
        }
    }
}
```

### 2.2 Modbus RTU RX that survives the CPU sleeping — phase 2

```cpp
static alloy::dma::ring_storage<std::uint8_t, 256> rxbuf;

int main() {
    board::init();
    auto bus  = board::rs485::open({.baud = 19'200, .parity = alloy::uart::parity::even});
    auto ring = bus.rx_ring(rxbuf);      // bytes land with the CPU asleep

    for (;;) {
        alloy::sleep_until_event();      // any interrupt wakes us; IDLE is armed by rx_ring
        // cursor() = items written since start, computed from the channel's
        // live remaining-count register — no ISR ran per byte, no byte was
        // copied. The Modbus t3.5 frame gap is detected by the UART IDLE
        // event waking us, and readable() yields everything since last read.
        std::span<const std::uint8_t> frame = ring.readable();
        if (!frame.empty() && modbus.on_bytes(frame)) {
            ring.consume(frame.size());
        }
    }
}
```

### 2.3 Telemetry TX that costs no cycles — phase 2

```cpp
// Existing surface: write_dma is shipped; the async form is the same transfer
// awaited instead of spun (alloy::async::dma_waiter, shipped, G0-proven).
auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
auto tx   = alloy::dma::claim(board::dma::debug_uart_tx);   // route -> channel token

alloy::async::dma_waiter w{tx};
co_await w.run([&] { uart.write_dma_begin(tx, log_line); });  // CPU free while it drains
uart.write_dma_end(tx);
```

### 2.4 SPI full-duplex transfer (the pair) — phase 4

```cpp
auto spi = board::sensor_spi::open({.clock_hz = 8'000'000});

std::uint8_t tx[8] = {0x9F};   // in RAM: the XDMAC cannot read flash (§4)
std::uint8_t rx[8] = {};

// Claims BOTH board-assigned channels as one unit (RX armed before TX —
// the order that cannot lose the first byte), runs both, waits both,
// releases both. One call, because half a duplex is not a thing.
bool ok = spi.transfer_dma(std::span{tx}, std::span{rx});
```

## 3. Per-family plan, with honest sizes

### 3.1 STM32 G0/G4 — dma_v1 + DMAMUX: mostly wiring

The driver, the completion IRQ, the ISR→poller latch, and the awaitable are
shipped and Renode-proven. What phase 1 adds:

- **Half-transfer events**: `htie`/`htif`/`chtif` are already in the generated
  `ip/st/dma_v1.hpp` (measured this week) — the driver grows
  `enable_half_irq`-style plumbing folded into `setup` the same way `tcie` is,
  plus a `p2m` circular starter (today only `m2p_circular` exists).
  **Estimate: ~60 lines driver, ~200 lines `ring<T>` (family-neutral), plus
  the facade methods.**
- **Board `dma:` assignments + route emission** in `emit/board.py`.
  **Estimate: ~150 lines Python + tests.** Curated G0/G4 boards get default
  assignments for `adc.conv`, `debug_uart.rx/tx`.
- ADC facade `ring()` on the shipped `dma_burst_begin/kick` sequencing,
  continuous-conversion variant.

### 3.2 STM32 F4/F7 — the big one: a new register file and a new driver

There is no stream-engine driver at all; `registers/st` has only
`dma_v1`/`dmamux_v1`. The work, in dependency order:

1. **Change the `dma_routes` shape now, before its first consumer.** Two
   changes, both cheap today and breaking later:
    - Rename the key `channel` → `stream` where the controller is a stream
      engine. F4/F7 "channel" already means *the CHSEL value* in ST's own
      documentation, the triples currently use it to mean *the stream*, and
      alloy's dma_v1 channels are 1-based while these are 0-based. One word
      meaning three things is how an off-by-one ships. L4 (a channel engine
      with CSELR) keeps `channel`.
    - **Regenerate the shipped F7 chips.** Measured this week:
      `stm32f767zi.yaml` and the other shipped-board chips carry **no**
      `dma_routes` — only chips regenerated after the builder change
      (`stm32f767bi`, `stm32f429zi`, …) have them. The data the driver needs
      does not exist yet for the boards that need it. Release-ordering applies
      (alloy-devices first, exact pin).
2. **Curate `st/dma_v2`**: `LISR/HISR/LIFCR/HIFCR` + eight
   `SxCR/SxNDTR/SxPAR/SxM0AR/SxM1AR/SxFCR` blocks (stride 0x18), `CHSEL`,
   `HTIE/TCIE/TEIE/DMEIE`, FIFO control. Also an `ip_map` entry so `dma1/dma2`
   stop being `uncurated` on F7. **Estimate: 1–2 days of careful curation +
   lints.**
3. **Write `st_dma_v2.hpp`** constrained on `ip::st::dma_v2` (no `mux_t`
   companion — that requirement is what keeps `st_dma_v1` from grabbing it).
   The genuinely different behavior: flags split across two registers with a
   non-linear per-stream layout; **EN=0 is a request, not a state** — teardown
   must poll EN until the FIFO drains; direct mode vs FIFO mode (v1: direct
   mode, matching dma_v1 semantics); per-stream NVIC lines (data, not
   grouping logic). **Estimate: 250–350 lines, the same order as the two
   shipped drivers.**
4. Facades and `ring<T>` come for free; board assignments for
   `nucleo_f722ze`/`nucleo_f767zi` are validated against the (regenerated)
   triples.

Honest scope note: **`adc1` on the F7 is uncurated** (measured), so anchor 2.1
cannot run on F7 until the F2/F4/F7 ADC IP is curated — a separate, unsized
item. Anchors 2.2/2.3 (UART) and 2.4 (SPI) have curated peripherals waiting.

### 3.3 SAME70 — completion IRQ now, rings later

- **Completion IRQ** (closes the `requires`-gate gap that makes `on_complete`
  ST-only): one NVIC line for the whole XDMAC; the ISR reads `GIS` once and
  dispatches to per-channel latches — the same latch contract as st_dma_v1,
  except `CIS` is clear-on-read so the latch is *mandatory*, not defensive.
  ~~**Estimate: ~80 lines + the Renode witness.**~~ **Landed, and the estimate
  was wrong in the expensive direction**: the driver was ~80 lines as written,
  but "the Renode witness" is not a wire — it is a GENERATED C# model
  (`emit/renode.py` `RENODE_XDMAC` / `XDMAC_CS`, ~250 lines with its emitter
  plumbing), the third one this project writes, because Renode ships no XDMAC
  and no Microchip DMA model at all. The leg is green (§5). One deliberate
  divergence from this bullet, argued in the body header rather than drifted
  into: the ISR is spelled PER CHANNEL — each armed channel attaches its own
  `complete_isr<Ch>`, reads `GIS`, and returns when the pending bit is not its
  own — rather than one handler dispatching from a runtime table. It costs one
  extra `GIS` read per attached channel and buys an attach/detach contract
  identical to both ST engines, no table, and a handler a host test can call
  for one channel.
- **Rings**: XDMAC has no circular bit; a ring is two linked view-0
  descriptors ping-ponging the halves, with the block-end interrupt as the
  half event. ~~16 bytes~~ **12 bytes** of RAM per descriptor — corrected
  while implementing: view 0 is THREE words (`MBR_NDA`, `MBR_UBC`, `MBR_TA`),
  and 16 bytes is the four-word view 1 that carries `SA` and `DA`
  separately. Descriptors must live in DMA-visible RAM (they are engine-owned
  `.bss` statics, per channel, so this is true by construction rather than by
  a caller's discipline). `cursor()` degrades: `CUBC` gives position *within
  the current half* and which descriptor is live is tracked in the ISR —
  `remaining()` becomes a synthesis of the two rather than a register read.
  **Landed at ~250 lines. THE DESCRIPTOR LAYOUT IS UNCURATED AND THE WHOLE
  PATH IS UNWITNESSED** — see the §5 SAME70 row and the warning at the top of
  `src/alloy/hal/dma/microchip_xdmac_v1_body.hpp`.
- **Capability**: the ring is gated on a `supports_ring` flag SEPARATE from
  `supports_circular`, because on this IP the two have different answers and
  folding them would make `start_m2p_circular_u16` — which needs two different
  transfer widths XDMAC does not have — visible and trapping.

### 3.4 RP2040 — curation from zero **[DONE, minus the ring — see below]**

Everything was missing: no `dma` peripheral entry in `rp2040.yaml` (measured —
only the two IRQ names existed), no register file, no driver. All three landed.

- Data: `registers/raspberrypi/dma_v1.yaml` + the chip's `dma` instance and its
  three `dma_requests`. §3.4's key prediction held — the DREQ lands as
  `dma_requests` on each peripheral, **same key as G0**, and the free-router
  branch of `dma_assignment_problems()` needed no third shape. One correction:
  the estimate said the DREQ table comes "from RP2040 datasheet §2.5.3", and it
  does not have to — all 45 ids are `enumeratedValues` on `CH0_CTRL_TRIG/
  TREQ_SEL` in the pinned vendor SVD, so nothing in the curation is
  datasheet-derived or hand-entered. The one thing the SVD does NOT state is
  that `CH_CTRL_TRIG` itself triggers; that is marked INFERRED in the data and
  is the driver's most consequential unverified claim.
- Driver **`raspberrypi_dma_v1.hpp` + `_body.hpp`**, not `rp_dma_v1.hpp`:
  codegen resolves `alloy/hal/<class>/<vendor>_<ip>.hpp` and the include is
  `.exists()`-guarded, so a driver at this section's original name would compile
  and simply never be included. Three things this silicon does that no other
  engine in the tree does: the register at each channel-view offset `0xC` is a
  **trigger**, so `setup()` configures through `CH_AL1_CTRL` and `start()` is a
  separate store to `MULTI_CHAN_TRIGGER`; `CHAIN_TO`'s reset value 0 means
  "trigger channel 0", so every control write names the channel's OWN number to
  disable chaining; and stopping is abort-and-poll (§4, fourth shape).
- **NO RING SHIPPED, AND THE PRE-AUTHORISATION IT WOULD HAVE USED IS HALF
  WRONG.** This section admitted `ring` shipping poll-only on the grounds that
  "`cursor()`/`readable()` work fine off `TRANS_COUNT`". Measured against the
  shipped `src/alloy/dma.hpp`: `readable()`/`consume()` do (they are pure
  `remaining()`), **`cursor()` does not** — it is `wraps_ * items_ + …` and
  `wraps_` is incremented only by the completion ISR, so with no events it
  silently under-reports one buffer per lap. And the deeper problem is not the
  events: a single RP2040 channel **HALTS** when `TRANS_COUNT` reaches zero,
  nothing re-arms it (`CHAIN_TO` naming its own channel is how chaining is
  DISABLED), so a one-channel poll-only ring does not degrade — it stops, and
  goes permanently empty. So the engine sets **`supports_ring = false`**
  alongside `supports_circular = false`, for INDEPENDENT reasons, and has no
  `enable_half_irq` at all (absent, not a no-op stub — a stub satisfies
  `ring_capable`, compiles, links, and hangs in `take()`). `alloy::dma::ring`
  is therefore **constrained away on this family**: a facade's `ring()` is a
  compile error naming the capability.
- **THE REFUSAL WAS THEN MEASURED, NOT ASSERTED** — a scratch project that calls
  `uart.rx_ring(buf)` and `adc.ring(samples, 4)` unconditionally on
  `raspberry_pi_pico` fails to compile, and the diagnostic names the capability
  in as many words: *`nested requirement 'ring_capable<typename
  Route::controller, Route::channel>' is not satisfied`*, on both facades. That
  is the promise this design makes about capability gates, honoured on the
  family where it matters most. Two mutations pin how much of it is load
  bearing:
  - Flip **`supports_ring` alone** to `true`: `ring_capable` stays **false** and
    `rx_ring()` stays refused. The concept has a second lock — it also requires
    `enable_half_irq` to *exist*, and this engine does not have it.
  - Flip the flag **and add two no-op `enable_half_irq`/`disable_half_irq`
    stubs** (three lines total): `ring_capable` becomes **true** for channels 0
    and 11, `alloy::dma::ring` instantiates, links — and `take()` would spin
    forever on a `pending_` no ISR ever sets. So "a stub satisfies the concept,
    compiles, and hangs" is not an argument, it is a measurement, and it is the
    concrete case for the third capability (`supports_ring_events`, with
    `take()`/`missed()`/`pending()`/`on_boundary()`/`cursor()` constrained on
    it) that shipping a poll-only ring on ANY future family will need. That is a
    change to shipped cross-family code and remains a maintainer decision;
    phase 6 changed **zero lines** of `src/alloy/dma.hpp`.
- **The named successor**: a full ring is reachable on ONE channel with a
  software ping-pong — the completion ISR re-points the write address at the
  other half through `CH_AL2_WRITE_ADDR_TRIG`, which is at view offset `0xC`, so
  that single store sets the address, reloads the count from the reload value
  `TRANS_COUNT` already holds (this part DOES have a reload; the scouts said it
  did not), and re-triggers. Deferred for a WITNESS reason, not a scope one:
  its failure modes — the DREQ stall window while the channel is halted, and the
  ISR re-arm deadline — are invisible to a host double, on the one family with
  no emulation at all. Shipping it here would put all of the risk exactly where
  nothing in this project can see it.
- **AND THE RING WOULD HAVE HAD NO USER HERE EVEN IF IT HAD SHIPPED** — measured
  afterwards, and it is the argument the deferral above was missing. Both ring
  facades are constrained away on this family for reasons that have nothing to
  do with the DMA engine:
  - `adc::stream` (anchor 2.1) exposes `take()`, `missed()`, `pending()` and
    `on_boundary()` and **nothing else** — every member of it is half-event
    driven. A poll-only ring would give anchor 2.1 not a degraded surface but an
    empty one. Separately `adc::stream_capable` also demands
    `dma_burst_begin/kick/end` + `dr_addr()`, and the RP2040 ADC driver has
    none of them.
  - `uart::rx_stream` (anchor 2.2) is the one whose primary discipline IS
    `readable()`/`consume()`, so it is the one a poll-only ring would have
    served — except that `rx_stream_capable` also demands `enable_idle_irq()`,
    the frame-gap wake, and the PL011 cannot honestly provide it. Its nearest
    analogue is `RTIM`, which asserts when the RX FIFO is NOT empty and the line
    then goes quiet; whether that can fire *while a DMA channel is draining the
    FIFO* depends on which request flavour the RP2040 wires to `TREQ_SEL` 21 —
    `UARTRXDMASREQ` (any non-empty FIFO, which keeps it empty and `RTIM` silent)
    or `UARTRXDMABREQ` (a burst at the `UARTIFLS` level, which leaves residue).
    The pinned SVD names **both** flavours in `UARTDMACR/DMAONERR`'s own text
    and picks neither. A hook built on the guess would arm and might never fire
    — a sleeper that never wakes, which is worse than the compile error.
  So the poll-only ring would have been mechanism with no caller, on the one
  family with no emulation. That does not make the deferral right on its own —
  it makes it cheap, and it names what has to be true before the successor is
  worth building: the RX request flavour has to be sourced, or the frame gap has
  to come from somewhere other than the UART.
- **WHAT DOES REACH THE ENGINE FROM A SHIPPED EXAMPLE**: the memory→peripheral
  one-shot, through `uart.write_dma()`. `src/alloy/hal/uart/raspberrypi_uart_pl011.hpp`
  gained `dma_tx_begin()` / `dma_tx_end()` / `tdr_addr()` (set/clear
  `UARTDMACR.TXDMAE`, curated from the same pinned SVD, and `&UARTDR`), and the
  TX request id was already chip data. `examples/dma_uart` and
  `examples/dma_probe` therefore fold their DMA branch open **from `board.json`
  alone, zero preprocessor**, on both RP2040 boards — measured before and after:
  `dma: not available on this board` → `dma via DMA`. Both were already in
  ci.yml's `build` matrix for both boards, so **no emulation leg was added and
  none may be**. One CI step *was* added and it is not a witness: because every
  DMA example folds by design, a broken hook would leave `dma_uart` compiling,
  linking and silently printing the fallback forever while the build loop stayed
  green, so the step reads the linked ELF and fails if the fallback string is
  there. Verified in both directions locally — renaming `dma_tx_begin` leaves the
  build green and turns the gate red. It asserts the BRANCH, never behaviour.
  There is
  deliberately **no RX hook** (see above), which is a named absence in that
  header, not an oversight.
- Also unresolved and NOT this phase's to fix: `ring_storage<T, N>` is a fixed
  `alignas(32)`, while `RING_SIZE` needs a power-of-two-BYTES buffer aligned to
  its own size — a cross-family decision, and moot while no ring exists here.
- No Renode model (§5): the witness is host-double tests plus hardware.

## 4. Safety mechanisms

**Lifetime.** A stream is the RAII owner of the hardware it started: its
destructor stops the channel (§ stop sequence below), *then* releases the
claim. Returning from a function whose local ring is still being filled stops
the DMA before the stack frame dies — the memory corruptor in the brief is
closed by scope, not by discipline. What scope cannot close: a stream stored
in a static holding a span to a shorter-lived buffer. `ring_storage<T, N>` is
the countermeasure — the ring APIs take it by reference, so the natural
spelling puts the buffer in static storage; a raw-span overload exists,
documented as the sharp edge it is. This is a doctrine rule, not a compiler
guarantee, and the doc says so.

**The stop sequence is per-engine behavior, stated once.** dma_v1: clear EN
(immediate). dma_v2: clear EN, then **poll EN until 0** — the FIFO drains
asynchronously and freeing the buffer before that poll completes is a
use-after-free with hardware as the reader. XDMAC: `GD`, then poll `GS`
(shipped). **RP2040 is a fourth shape**: clearing `EN` here only PAUSES — the
SVD says `BUSY` stays high if it was high — so terminating means writing the
channel's bit to `CHAN_ABORT` and then **polling `CHAN_ABORT` to all-zero**,
because until it reads zero the in-flight transfers are still draining through
the address and data FIFOs and "it is unsafe to restart the channel" (the SVD's
own words). Same use-after-free class as dma_v2's poll, and it lives inside the
engine's `stop()` because `ring`/`channel` teardown calls that and returns.
Facade teardown order for rings: peripheral request off first
(e.g. ADC `DMAEN`), then channel stop — a request that lands mid-teardown on
a disabled channel is how overrun flags get stuck.

**M7 D-cache (SAME70, F7).** Alloy startup never enables the D-cache today
(measured: stated and relied on in the shipped XDMAC driver), so there is no
coherence hole to fix — yet. The design pre-commits two things so enabling the
cache later is not an API break: `ring_storage` is `alignas(32)` and pads to a
32-byte multiple from day one (so a half never shares a cache line with the
other half or with neighbors), and the documented policy for cache-on is **an
MPU non-cacheable region for DMA buffers**, not per-transfer maintenance —
invalidate-on-take of a live ring is unwinnable in general and we decline to
pretend otherwise.

**Flash visibility.** The XDMAC cannot read embedded flash (silicon-measured,
documented in the driver). Today this traps as a bus error. The memory map is
generated data, so the m2p starters gain a debug assert comparing the source
address against the flash range on families whose data flags the limitation —
a named trap instead of a mystery bus error. ST families deliberately skip the
check: dma_v1 reads flash fine and .rodata sources are legitimate there.

**The latch rule is now a driver contract.** Any ISR that consumes a hardware
flag that pollers also read MUST latch it (st_dma_v1's `callback<Ch>::latched`
is the precedent; XDMAC's clear-on-read `CIS` makes it mandatory there). Rings
extend it: the half/full ISR records *which* half is stable in a two-slot
latch that `take()` consumes; a missed boundary increments `missed()` and
`take()` resynchronizes to the most recent stable half. Never silent.

**ISR discipline.** `on_half`/`on_complete` callbacks run in interrupt
context: set a flag, wake a task, start the next transfer — the same contract
`alloy::irq` states everywhere. `take()`/`readable()`/`consume()` are
thread-context only.

## 5. Witnessability under Renode 1.16.1, per family

Honest per-leg statements of what a green run actually proves:

| Family | Model | What the leg can assert | Status |
|---|---|---|---|
| G0 | native `DMA.STM32G0DMA` | one-shot m2p bytes on the UART, completion IRQ fired (proven leg, in CI); phase 1 adds: half-IRQ then full-IRQ ordering, ring wrap (pattern written twice), ADC ring data | one-shot **proven**; ring events **expected but unverified** until the phase-1 leg runs — the model's HTIF behavior has not been exercised by us |
| F4/F7 | stock `DMA.STM32DMA` **measured, then replaced** by a generated model (phase 3) | probe day found: per-request P2M through a `ReceiveDmaRequest` wire, immediate M2P, TCIF + per-stream NVIC all work on stock; **CIRC and HTIF are tagged no-ops** (circular p2m stalls after one buffer) and the class is sealed — so the platform emits `DMA_V2_CS` (emit/renode.py): stock behaviours + CIRC reload + HTIF, CHSEL routing by-construction unwitnessable (the G0 DMAMUX caveat) | anchors 2.2/2.3 **proven in emulation** on both F7 Nucleos (modbus ring + route-claimed TX, every G0 assertion unchanged), DMAR negative control red-not-hung. **A route is witnessed by halves**, measured by adversarial control: the STREAM index is load-bearing end-to-end (move the model's request wire by one stream and modbus fails red), the REQUEST id is NOT (delete the CHSEL write from the driver and the leg stays green) — so on F7 emulation proves the stream, and only silicon can prove the request |
| SAME70 | **a GENERATED model, because there is none at all** (phase 5a). Renode 1.16.1 ships no XDMAC and no Microchip DMA controller of any kind — measured three ways: every plausible type name fails to resolve, the complete `SAM*` type set in its Infrastructure assembly is 19 peripherals and none is a DMA engine, and Renode's own `sam_e70.repl` instantiates no DMA. This row used to say "XDMAC modelled" and that was never true. Neither cheaper tier can serve: an unmapped region is not a stub (Renode reads 0 from nothing, so `complete()` sees GS as "already finished" and the firmware reports success having moved no bytes), and `Python.PythonPeripheral` — the tier this platform's own flash controller uses — has no IRQ property, while the NVIC line is the whole assertion. So the platform emits `XDMAC_CS` (emit/renode.py) | **5a (completion IRQ): PROVEN IN EMULATION.** `dma_uart` on same70_xplained is green in 4.0–5.1 s — banner, `dma via DMA` (bytes crossed from RAM to the USART by the programmed channel), `dma irq: fired` (the XDMAC NVIC line fired and that channel's handler ran) — with the same robot and the same three assertions the G0 leg uses, plus the host witness (`test_xdmac_v1_latch.cpp`, every latch line proven by revert). Two negative controls isolate the interrupt from the transfer: delete the driver's single GIE arm, or the model's single `IRQ.Set`, and the bytes still cross while the leg fails at `dma irq: fired` in ~31 s — fail, not hang. **5b (rings): NO LEG, and it cannot honestly be given one.** The view-0 descriptor layout is curated in neither repo, so a model would encode the same unverified reading as the firmware and a green run would prove only that the two agree with each other; the generated model therefore REFUSES linked-list mode out loud (a warning, nothing transferred) rather than self-confirm. Its witness is `test_xdmac_v1_ring.cpp` (bookkeeping, ping-pong, cursor, teardown — each line proven by revert) plus a datasheet confirmation nobody has done | 5a **proven**; 5b an honest, host-only gap — and PERID is worse here than the G0/F7 "witnessed by halves" boundary: `UART.SAM_USART` exposes no DMA-request output, so there is no wire for any model to consume, the model triggers on the GE write and never consults PERID. The request half is unwitnessable **by construction**, not by coincidence |
| RP2040 | **no model, and no RP2040 peripheral of ANY kind** — re-measured for phase 6 via Renode's own type resolver: every plausible DMA/UART/ADC/GPIO/timer type name for this part fails to resolve, and there is no rp2040 board or CPU platform either. **A DMA leg here would be worse than absent.** alloy CAN already emit an rp2040 platform (`cortex-m0` and `UART.PL011` are generic types Renode has), and Renode reads 0 from unmapped addresses — so on that platform the engine would read `CTRL` as 0 (`BUSY` low, no error bits), read `TRANS_COUNT` as 0, and report a **finished transfer over a fully-written buffer**, having moved nothing. That is a false green, not weak evidence. The refusal is written into `raspberrypi_dma_v1_body.hpp` and `test_rp_dma_v1_latch.cpp` so nobody adds the leg as an obvious improvement | nothing under Renode. Witness = `tests/test_rp_dma_v1_latch.cpp` — 14 cases running the REAL engine over a hand-written double whose `INTR` is genuinely write-1-to-clear, whose `INTS0/INTS1` are DERIVED from `INTR & INTEn` (which is what makes the two-line dispatch checkable), and whose `CHAN_ABORT` actually drains with the poll count as a knob. **23 mutations, 20 red** — including two that fail as a HANG rather than an assertion (revert either completion-latch line and `channel::wait()` spins forever, which is the bug class the latch closes). **3 stayed green and are named** in both files: the error latch's two lines (defensive here, because the failure lives in `CTRL.AHB_ERROR`, which no read destroys and the ISR does not clear — mandatory on XDMAC, load-bearing on both ST engines) and setup's latch-reset ordering (this `clear_flags()` only writes, so unlike XDMAC's harvesting purge it cannot re-latch anything). Plus `docs/guide/rp2040-dma-hardware-checklist.md` — **WRITTEN, NOT EXECUTED**, and now with a SHIPPED instrument rather than an inline listing: `examples/dma_uart` and `examples/dma_probe` reach the engine on both RP2040 boards since the PL011 gained its TX-DMA hooks, and both are already in ci.yml's build matrix. Eight RUN / LOOK FOR / FALSIFIED BY rows; row 4 (perturb `uart0.dma_requests.tx` in chip data and expect the transfer to STOP working) is the first and only place in this design where a DMA request id is ever tested. Executing it needs a board and a USB-serial adapter, and neither is reachable from here | honest gap — a green CI on RP2040 DMA proves compilation and register-sequence intent, **not** behavior. Three specific claims are owed to the board and cannot be reached off it: that `CH_CTRL_TRIG` triggers (marked INFERRED in the curation — the SVD annotates the other three view-`0xC` registers and not this one, so the whole configure-through-`AL1` discipline rests on it), the three DREQ ids (unwitnessed here as on every other family), and that `CHAN_ABORT` flushes anything |

Rule inherited from the fault-report work: a leg asserts what it measures and
the robot file says what that is; no leg claims "DMA works" — it claims "these
bytes crossed, this IRQ fired, in this order".

## 6. Phase plan — each phase ends demonstrable, maintainer's cases first

| Phase | Lands | Demonstrable end-state |
|---|---|---|
| **1** | `route` emission + board `dma:` assignments; `ring<T>` + `ring_storage`; dma_v1 half events; `adc.ring()` | anchor 2.1 running under Renode on `nucleo_g0b1re`/`g071rb`: control loop consuming stable halves, half/full IRQ order asserted |
| **2** | `uart.rx_ring()` + IDLE wake + `cursor()/readable()/consume()`; route-claiming `alloy::dma::claim(route)` for the shipped TX path | anchor 2.2: Modbus RTU frames received with no per-byte ISR under the existing `modbus_rtu.robot`; anchor 2.3 async TX |
| **3** | alloy-devices: `dma_routes` rename [DONE phase 1] + shipped-chip regen; `st/dma_v2` curation; `st_dma_v2.hpp`; F7 board assignments | **[DONE]** anchors 2.2/2.3 on `nucleo_f767zi` (and f722ze) under Renode; same portable main.cpp, one config line changed — with two driver-side findings the promise turned out to need: `st_usart_v3` had no RX-DMA/IDLE hooks (now inherits the witnessed shared body) and `write_dma` sourced its request only from chip-wide `Inst::dmareq_tx`, which cannot exist on a stream engine (now falls back to the binder's matched-route request) |
| **4** | pair claim + `spi.transfer_dma()`; `i2c` one-shot DMA read/write | **[DONE]** anchor 2.4 green on `nucleo_g071rb`, `nucleo_g0b1re` (dma1 channel 4/5, free router) and `nucleo_f722ze` (dma2 streams 0/3, stream engine) — one portable `spi_read` and one robot across both engines, differing by nothing but `board.json` `dma:` lines. Stronger than phase 3 could claim: besides the channel/stream INDEX, the leg witnesses the **RX-before-TX arming order** of the pair above (arm TX first and every request edge lands on a channel that is not enabled yet — red) and `CR2.RXDMAEN` (red without it). Still unwitnessed, unchanged: the REQUEST id, on either engine. The **i2c one-shot** landed with host witnesses only, and its leg asserts a bounded refusal rather than a transfer: Renode 1.16.1's model of that IP exposes no DMA request output at all, so nothing can ask the engine to move a byte — see the two options recorded in `tests/emulation/i2c_read.robot`. `write_read_dma` (repeated start) is a **named absence**: a deleted member, because the polled path hands off on TC and `CR1.TCIE` is deliberately uncurated |
| **5** | SAME70: completion IRQ (5a), then linked-list rings (5b) | **FIRST HALF REACHED, SECOND HALF NOT — and the two failed differently, which is the useful part.** The end-state as drafted read "dma_uart leg on same70_xplained asserts the IRQ instead of printing 'not available'; then anchor 2.1 on SAME70", and it was drafted against a §5 row that wrongly said "XDMAC modelled". **5a IS DONE, leg and all**: the driver landed (612c4f4), the requires-gate flipped, and the leg is green in CI-shape — but only after a cost nobody had counted, a GENERATED Renode C# model, because Renode ships no XDMAC and no Microchip DMA model at all. A reader sizing 5a from the old table under-sized it by exactly that. **5b's ring landed with a host witness and can never honestly have a leg** (see §5: the descriptor layout is uncurated, so a model and the firmware would agree by construction; the generated model refuses linked-list mode rather than self-confirm). **And "then anchor 2.1 on SAME70" is NOT REACHED and is blocked twice over, independently**: it needs the ring leg that cannot exist, AND it needs an AFEC model — `RENODE_ADC` is ST-only, and re-measured for this phase, Renode 1.16.1 contains no AFEC type of any kind (the only `AFEC` substrings across every shipped assembly are hex-string false positives; `Analog.SAM4S_ADC` exists but is a different block and would need a compatibility probe first). **Anchor 2.1 on SAME70 is HOST-WITNESSED ONLY** — `tests/test_xdmac_v1_ring.cpp` drives the shipped `alloy::dma::ring` over a double that really fetches and executes the descriptors — and should be re-opened as its own scoped decision once the AFEC question is answered, not carried as a phase-5 leftover. |
| **6** | RP2040: curation from zero + `rp_dma_v1` + poll-mode ring | **CURATION, DRIVER AND A REACHABLE ONE-SHOT PATH DONE; THE POLL-MODE RING IS NOT, ON PURPOSE; THE CHECKLIST IS WRITTEN AND NOT EXECUTED.** The data landed from the pinned vendor SVD (and the DREQ table turned out to be IN that SVD, so nothing is datasheet-derived); the free-router legality expression needed no third branch, only a data-driven channel BASE (`channels: {first: 0}`) because this part numbers from 0 and the expression hardcoded 1 — it refused `channel: 0` and accepted a `channel: 12` that does not exist; the engine landed as `raspberrypi_dma_v1*.hpp` (NOT §3.4's `rp_dma_v1.hpp` — codegen would silently never include that) with 14 host cases and a 23-mutation ledger. **The ring did not land and should not have**: §3.4's pre-authorisation rested on "`cursor()`/`readable()` work fine off `TRANS_COUNT`", and `cursor()` does not (it needs `wraps_`, maintained only by the completion ISR) — and worse, a one-channel RP2040 ring HALTS at the end of its count and goes permanently empty rather than degrading. So both capability flags are false for independent reasons and `alloy::dma::ring` is constrained away here, which is a compile error naming the capability rather than a silent hang in `take()`. Shipping it would have needed a THIRD capability plus splitting `ring_capable` in shipped, adversarially-reviewed cross-family code — a maintainer decision, not a phase side effect. The named successor (one-channel software ping-pong through `CH_AL2_WRITE_ADDR_TRIG`) is sized in §3.4 with the reason it is deferred: its failure modes are the ones a host double cannot see, on the one family with no emulation. **The ring's absence was then re-examined and confirmed from the OTHER end**: both ring facades are constrained away on this family independently of the DMA engine — `adc::stream` exposes only half-event members (and the RP2040 ADC driver has no burst hooks at all), and `uart::rx_stream` needs a frame-gap event the PL011 cannot honestly provide, because whether `RTIM` can fire while DMA drains the FIFO turns on a request flavour the pinned SVD names both of and picks neither. A poll-only ring would have been mechanism with no caller. The refusal itself was then MEASURED rather than asserted: a project calling `uart.rx_ring(buf)` or `adc.ring(samples, 4)` on a Pico fails to compile with *`nested requirement 'ring_capable<…>' is not satisfied`* — the capability named, as promised. Two mutations bound how much of that is load bearing: flipping `supports_ring` alone leaves `ring_capable` FALSE (the concept also demands `enable_half_irq` exist), while the flag plus two no-op half-IRQ stubs — three lines — makes `ring_capable` TRUE and `alloy::dma::ring` instantiate and link, with `take()` spinning forever. So "a stub compiles and hangs" is now a measurement, and it is the concrete case for the third capability (`supports_ring_events`) that any future poll-only ring will need; phase 6 changed **zero lines** of `src/alloy/dma.hpp`. **What DID land instead is a real user for the one-shot path**: the PL011 gained `dma_tx_begin`/`dma_tx_end`/`tdr_addr` (from `UARTDMACR.TXDMAE`, newly curated), so `examples/dma_uart` and `examples/dma_probe` fold their DMA branch open from `board.json` alone on both RP2040 boards — measured `dma: not available on this board` → `dma via DMA`. Both were already in ci.yml's build matrix, so **no emulation leg exists or was faked** — the one CI step that WAS added asserts the compile BRANCH (a folded-back `dma_uart` still builds green, so the loop cannot see it), verified in both directions locally, and claims nothing about behaviour. **The hardware checklist is WRITTEN and has NOT been executed** — the board is not reachable from here, and both RP2040 boards observe only through an LED and a `debug_uart` that needs an external USB-serial adapter — but it now points at shipped, CI-built firmware instead of an inline listing |

Ordering rationale: phases 1–2 land the maintainer's two product cases on the
G0/G4-class boards they own, on the one family with proven DMA-IRQ machinery —
no new register file at risk. The F4/F7 driver (the biggest single item) comes
third, when the stream vocabulary it must implement is already exercised by
two shipping anchors. SAME70/RP2040 close the "every ARM family" promise.

## 7. Open questions — the maintainer's call

1. **Where may a project override a board's channel assignment?** `board.json`
   is a default and `alloy.toml` chooses — the products/overrides doctrine
   says only `roles.py` `project_fields` may be overridden. Should `dma:`
   assignments join `project_fields`, or is re-curating the board the intended
   path when a product needs a different channel split?
2. **Should stream destructors release the claim** (as designed, `sub_release`)
   **or hold it for firmware lifetime** like `dma::channel::claim()` does
   today? Release makes scoped streams and tests natural; hold makes "who has
   this channel" a one-time fact a fault report can trust. The design picks
   release — veto it and §4's lifetime story needs a different mechanism.
3. **RP2040 before or after SAME70 rings?** Phase 5b and 6 are ordered by
   "finish a family before opening one", but if a product needs the Pico
   sooner, 6 can jump 5b — nothing couples them.
4. **Is a duplex *ring* (continuous SPI streaming, e.g. a codec) needed by any
   planned product?** v1 ships one-shot duplex only; a duplex ring is a new
   completion topology (two rings in lockstep) and is deliberately unscoped
   here.
5. **F7 ADC curation:** pull it into phase 3 so anchor 2.1 runs on F7 too, or
   keep F7 scoped to comms DMA until a product asks? It is the one item in
   this plan with no size estimate.
6. **Confirm the `dma_routes` rename now.** It is a data-only change with no
   consumer (measured), but it triggers the alloy-devices release ordering
   (devices repo first, exact `==` pin, then repin + CI). Saying yes this week
   is cheap; saying yes after phase 3 starts is a migration.
