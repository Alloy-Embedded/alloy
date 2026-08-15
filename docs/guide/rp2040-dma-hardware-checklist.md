# RP2040 DMA — the on-hardware checklist

**Nothing below has been executed.** This sheet is the entire behavioural
evidence base for `alloy/hal/dma/raspberrypi_dma_v1.hpp`, and until somebody
runs it on a board, every claim in that driver is a transcription plus a
compile.

It is a file rather than a PR comment (which is what
`docs/design/dma-streams.md` §5 originally asked for) because a PR comment does
not survive the merge, and because the driver header, the host witness and the
design's §5 row all point here by name.

> The design's own sentence, verbatim and unsoftened: *a green CI on RP2040 DMA
> proves compilation and register-sequence intent, **not** behavior.*

## Why this family needs a sheet at all

The other three DMA engines in this tree are watched by something. G0 and F7
have Renode legs. SAM E70 has a generated model that proves its completion IRQ.
RP2040 has **nothing**: Renode 1.16.1 ships no RP2040 peripheral of any kind, no
board and no CPU platform for this part.

And the obvious repair is a trap, so it is written down in three places
including this one. alloy *can* already emit an rp2040 Renode platform —
`cortex-m0` and `UART.PL011` are generic types Renode has — and Renode reads 0
from unmapped addresses. A DMA leg on that platform would read `CTRL` as 0
(`BUSY` low, no error bits) and `TRANS_COUNT` as 0, and report a **completed
transfer over a fully-written buffer**, having moved nothing. That is a false
green. **Do not add the leg.**

So: the host double (`tests/test_rp_dma_v1_latch.cpp`), or this sheet.

## What the host double already covers, so you do not re-test it here

Fourteen cases run the real engine over a register double, and 23 single-line
mutations were applied to the driver to find out which of its claims are load
bearing — 20 went red. Those cases pin *which register gets which value in which
order*: that configuration never addresses a trigger register, that the start is
one store to `MULTI_CHAN_TRIGGER`, that chaining is disabled by self-reference,
that `stop()` aborts and polls, that the completion latch survives its own ISR,
and that the handler guards the same interrupt line the arm routed to.

**None of that is behaviour.** Three claims in particular are unreachable off
the board, and they are the reason this sheet exists:

| Unwitnessed claim | Why nothing off-target can reach it |
|---|---|
| **`CH_CTRL_TRIG` triggers** | Marked INFERRED in `registers/raspberrypi/dma_v1.yaml`: the vendor SVD annotates the *other three* view-`0xC` registers as trigger registers and does not annotate this one. The driver's whole configure-through-`CH_AL1_CTRL` discipline rests on it. A host double implements the trigger from the same reading the driver writes, so model and firmware agree by construction — this family's equivalent of XDMAC's uncited descriptor layout. |
| **The three DREQ ids** (`UART0_TX`=20, `UART0_RX`=21, `ADC`=36) | A wrong `TREQ_SEL` is a transfer paced by the wrong peripheral. §5 records the request id as unwitnessed on G0, unwitnessed on F7 (delete the `CHSEL` write and the leg stays green) and unwitnessable *by construction* on SAME70. **Row 4 below is the first and only place in this entire design where a DMA request id is ever tested.** |
| **`CHAN_ABORT` flushes anything** | The double models the poll-to-zero protocol, so the *loop* is real and countable — but whether in-flight transfers are actually drained by the time it reads zero is the SVD's claim, not the test's. |

## Kit, and the instrument this sheet is read through

- A Raspberry Pi Pico (`boards/raspberry_pi_pico`) or an RP2040-Zero
  (`boards/rp2040_zero`).
- **A USB-serial adapter, and this is not optional.** Both boards probe via
  `bootsel`/`uf2` with **no SWD**, so the only observation channels are the LED
  and `debug_uart` — and both board files say the UART needs an external
  adapter (Pico: header pins 1/2, GP0/GP1; Zero: castellated pads). A DMA
  checklist read through a blinking LED measures almost nothing. **Wire the
  adapter before starting.**
- `alloy flash` (uf2 to `/Volumes/RPI-RP2`) and `alloy monitor`.

State on the sheet, when you run it, **what you observed with**. A row that says
"OK" without naming its instrument is not a measurement.

## The firmware — WHICH DOES NOT EXIST YET, AND THAT IS A FINDING

**No shipped example reaches this driver.** `dma_uart` and `dma_probe` both
guard their DMA paths on `uart.write_dma(...)` / `adc.read_burst(...)`, and the
RP2040 PL011 UART driver has **no DMA hooks at all** (`dma_rx_begin`,
`dma_rx_end`, `rdr_addr`, `tdr_addr` — none present, though `UARTDMACR` and
`RTIM` are already curated) and the RP2040 ADC driver has no burst hooks. So on
a Pico today both examples take their honest fallback branch and print
"driver has no DMA hooks" — which is *correct*, and which also means flashing
them proves nothing about this engine.

Everything below therefore needs a purpose-built firmware. This one has been
**cross-compiled for both boards** with `arm-none-eabi-gcc 14.2.1` (3132 bytes
of text on the Pico, 3052 on the Zero) and forces a real instantiation of every
member of the engine; it has never been run.

```cpp
// alloy new rpdma --board raspberry_pi_pico ; then src/main.cpp:
#include <alloy/board.hpp>
#include <alloy/dma.hpp>
#include <cstdint>
#include <span>

namespace {
volatile bool g_fired = false;
alignas(4) std::uint8_t g_msg[] = "dma via DMA\r\n";
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("rp2040 dma checklist\r\n");

    // The board's own spelling: route -> claim -> engine. On the Pico
    // board::dma::debug_uart_tx is channel 2, request 20 (UART0_TX).
    auto tx = alloy::dma::claim(board::dma::debug_uart_tx);
    tx.on_complete(+[](void* f) { *static_cast<volatile bool*>(f) = true; },
                   const_cast<bool*>(&g_fired));
    // The UART's transmit data register, and its DMA request must be enabled on
    // the PERIPHERAL side too (UARTDMACR.TXDMAE) — the driver has no hook for
    // that yet, so set it by hand here, from the generated field accessors.
    tx.start_m2p_u8(std::span<const std::uint8_t>{g_msg, sizeof(g_msg) - 1},
                    /* uart0 base + UARTDR */ 0u,
                    alloy::dev::uart0_t::dmareq_tx);
    uart.write(tx.wait() ? "dma: OK\r\n" : "dma: FAIL\r\n");
    uart.write(g_fired ? "dma irq: fired\r\n" : "dma irq: NOT fired\r\n");
    tx.stop();
    for (;;) {
    }
}
```

Two blanks are deliberately left for whoever runs this, because filling them
from memory is exactly the mistake the curation discipline exists to prevent:
the `UARTDR` address must come from the generated `alloy::dev::uart0_t::base`
plus the IP header's offset, and `UARTDMACR.TXDMAE` must be set through the
generated field accessor. Neither is a driver feature yet.

## The rows

Each row is **RUN / LOOK FOR / FALSIFIED BY**. A row that cannot be falsified is
not a test.

### 1. Regression floor — run this FIRST

- **RUN**: `blink`, then `hello`, on the target board.
- **LOOK FOR**: the LED blinking; the banner on the serial console.
- **FALSIFIED BY**: silence. That means the new `dma` peripheral entry broke the
  `RESETS` gate list or the clock bring-up — *not* the driver. This is the only
  row that separates "the data change" from "the engine", which is why it goes
  first.

### 2. One-shot memory→peripheral actually moves bytes

- **RUN**: the firmware above.
- **LOOK FOR**: the line `dma via DMA` on the console, followed by `dma: OK`.
- **FALSIFIED BY**: `dma: FAIL` (`wait()` returned false, i.e. an error bit rose
  — the address or the width is wrong), **or** nothing after the banner (the
  channel never advanced: `TREQ_SEL` wrong, `UARTDMACR.TXDMAE` never set, or —
  the interesting one — `MULTI_CHAN_TRIGGER` is not in fact a start).

### 3. The completion interrupt reaches the NVIC

- **RUN**: same run as row 2.
- **LOOK FOR**: `dma irq: fired`.
- **FALSIFIED BY**: `dma irq: NOT fired` while row 2 still passed. That isolates
  three things the host double can only check against itself: the `INTE0` bit,
  the vector number (`DMA_IRQ_0` = 11), and the `irq_line_of` ↔ `INTS0`
  line-index coupling — the driver puts every channel on line 0 (`kIrqLine`),
  which is a *policy*, not silicon.

### 4. THE DREQ NEGATIVE CONTROL — the highest-value row on this sheet

- **RUN**: rebuild row 2 with `TREQ_SEL` deliberately off by one — pass
  `alloy::dev::uart0_t::dmareq_tx + 1` (21, `UART0_RX`) instead of 20 — and
  re-run.
- **LOOK FOR**: `dma: FAIL`, or no line at all.
- **FALSIFIED BY**: it still prints `dma via DMA`. That would mean the transfer
  is not request-paced at all, and row 2 proved nothing about the request — it
  would have proved only that a channel can copy bytes as fast as the bus
  allows, into a FIFO that overflows.
- **WHY IT MATTERS BEYOND THIS FAMILY**: this is the first and only place in the
  whole DMA design where a request id is ever tested. Claim it as an advance.

### 5. Configuration does not start the channel — the INFERRED claim

- **RUN**: a variant that calls `setup()` (via `start_m2p_u8`'s first half) and
  then **waits a visible interval before `start()`** — e.g. blink the LED ten
  times between them, with the payload a recognisable pattern.
- **LOOK FOR**: nothing on the wire until after the delay, then the whole
  payload at once.
- **FALSIFIED BY**: bytes appearing during the delay. That means writing
  `CH_AL1_CTRL` started the channel, i.e. the curation's inference about which
  registers trigger is wrong in the dangerous direction — and every `setup()`
  in this driver is starting a channel while programming it.

### 6. `stop()` really terminates, and the buffer is safe afterwards

- **RUN**: start a long transfer (a payload of a few kB at 115200 baud, so it
  takes visible seconds), then call `stop()` mid-flight, then **overwrite the
  source buffer with a different pattern** and print what arrives.
- **LOOK FOR**: output stopping promptly, and **none of the second pattern** on
  the wire.
- **FALSIFIED BY**: bytes from the second pattern appearing. That is the
  use-after-free the abort-and-poll sequence exists to close: `EN` cleared alone
  only PAUSES, and a channel still draining through the address and data FIFOs
  is still reading your buffer.

### 7. Teardown releases the channel for a second claimant

- **RUN**: destroy the claim (scope exit), then claim the SAME channel again and
  run a second transfer.
- **LOOK FOR**: the second transfer completing normally.
- **FALSIFIED BY**: a trap on the second claim (the release path is wrong), or
  the second transfer never starting (the abort left the channel in a state
  `setup()` does not recover from).

### 8. A second channel, and the shared line

- **RUN**: claim two channels, arm completion callbacks on both, run one
  transfer on each.
- **LOOK FOR**: both callbacks firing, each exactly once.
- **FALSIFIED BY**: one callback firing twice, or one never firing — the
  shared-line guard (`INTS0` bit test) or the per-channel `INTR` clear is wrong,
  and the host double's version of that check agrees with the driver by
  construction.

## What executing this sheet does NOT license

- **Nothing about rings.** `alloy::dma::ring` does not exist on this family and
  this sheet does not test it. Both capability flags are false, for independent
  reasons: there is no half-transfer event anywhere in this IP, and a single
  channel halts at the end of its count with nothing to re-arm it. See
  `docs/design/dma-streams.md` §3.4 for the named successor (a one-channel
  software ping-pong through `CH_AL2_WRITE_ADDR_TRIG`) and why it was deferred.
- **Nothing about the second interrupt line.** `DMA_IRQ_1` is curated and
  deliberately unbound; the driver puts every channel on line 0. Rows 3 and 8
  test line 0 only.
- **Nothing about the sniffer, the pacing timers, or the 42 uncurated DREQ ids.**
- **Nothing about timing.** No row here measures a rate, a latency or a
  throughput, and the chip data records `kernel_clock: ahb` with an explicit
  note that no timing may be derived from it.

## When you have run it

Record, in this file: the date, which board, **what you observed with**, and the
result of each row *with the falsifier you actually tried* — a row marked OK
whose negative control was never run is a row that has not been tested. Then
update the §5 RP2040 row and the phase-6 row in
`docs/design/dma-streams.md`, which today both say this sheet is owed.
