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
  **Estimate: ~80 lines + the Renode witness.**
- **Rings**: XDMAC has no circular bit; a ring is two linked view-0
  descriptors ping-ponging the halves, with the block-end interrupt as the
  half event. 16 bytes of RAM per descriptor, descriptors must live in
  DMA-visible RAM. `cursor()` degrades: `CUBC` gives position *within the
  current half* and which descriptor is live must be tracked in the ISR —
  workable, but the Modbus `readable()` path is more code than on ST.
  **Estimate: 150–250 lines. Phase 5, after the IRQ.**

### 3.4 RP2040 — curation from zero

Everything is missing: no `dma` peripheral entry in `rp2040.yaml` (measured —
only the two IRQ names exist), no register file, no driver.

- Data: 12 channels × (`READ_ADDR/WRITE_ADDR/TRANS_COUNT/CTRL_TRIG` + alias
  sets), `TREQ_SEL` (the DREQ table is per-peripheral, from RP2040 datasheet
  §2.5.3 — it lands as `dma_requests` on each peripheral, same key as G0
  because the meaning matches: chip-wide id, any channel). **Estimate: 1–2
  days curation.**
- Driver `rp_dma_v1.hpp`: one-shot is simple; rings use the address-wrap
  `RING_SIZE/RING_SEL` bits (power-of-two buffers only — `ring_storage` on
  this family `static_assert`s the size) or two chained channels. Half events
  come from `IRQ0` per-channel with the wrap giving full-buffer granularity —
  honest limitation: **half events on RP2040 v1 come from chaining two
  channels, or do not exist; the design admits `ring` shipping poll-only
  (`cursor()`/`readable()` work fine off `TRANS_COUNT`) with events deferred.**
  **Estimate: 200–300 lines.**
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
(shipped). Facade teardown order for rings: peripheral request off first
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
| SAME70 | XDMAC modelled | poll-mode transfers proven; completion-IRQ leg is the phase-5a witness; descriptor ping-pong fidelity unknown | partial |
| RP2040 | **no model** | nothing under Renode. Witness = host unit tests against register doubles (the `tests/doubles.hpp` pattern) + an on-hardware checklist in the PR | honest gap — a green CI on RP2040 DMA proves compilation and register-sequence intent, **not** behavior |

Rule inherited from the fault-report work: a leg asserts what it measures and
the robot file says what that is; no leg claims "DMA works" — it claims "these
bytes crossed, this IRQ fired, in this order".

## 6. Phase plan — each phase ends demonstrable, maintainer's cases first

| Phase | Lands | Demonstrable end-state |
|---|---|---|
| **1** | `route` emission + board `dma:` assignments; `ring<T>` + `ring_storage`; dma_v1 half events; `adc.ring()` | anchor 2.1 running under Renode on `nucleo_g0b1re`/`g071rb`: control loop consuming stable halves, half/full IRQ order asserted |
| **2** | `uart.rx_ring()` + IDLE wake + `cursor()/readable()/consume()`; route-claiming `alloy::dma::claim(route)` for the shipped TX path | anchor 2.2: Modbus RTU frames received with no per-byte ISR under the existing `modbus_rtu.robot`; anchor 2.3 async TX |
| **3** | alloy-devices: `dma_routes` rename [DONE phase 1] + shipped-chip regen; `st/dma_v2` curation; `st_dma_v2.hpp`; F7 board assignments | **[DONE]** anchors 2.2/2.3 on `nucleo_f767zi` (and f722ze) under Renode; same portable main.cpp, one config line changed — with two driver-side findings the promise turned out to need: `st_usart_v3` had no RX-DMA/IDLE hooks (now inherits the witnessed shared body) and `write_dma` sourced its request only from chip-wide `Inst::dmareq_tx`, which cannot exist on a stream engine (now falls back to the binder's matched-route request) |
| **4** | pair claim + `spi.transfer_dma()`; `i2c` one-shot DMA read/write | anchor 2.4 on G0 + F7 legs; i2c_read leg gains a DMA variant |
| **5** | SAME70: completion IRQ (5a), then linked-list rings (5b) | dma_uart leg on same70_xplained asserts the IRQ instead of printing "not available"; then anchor 2.1 on SAME70 |
| **6** | RP2040: curation from zero + `rp_dma_v1` + poll-mode ring | host-double tests green; hardware checklist executed on the owned Pico |

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
