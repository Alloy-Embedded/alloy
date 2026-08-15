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

## The firmware — `examples/dma_uart`, SHIPPED AND IN THE CI MATRIX

An earlier revision of this sheet said no shipped example reached this driver
and carried its firmware inline. **That is fixed.** The gap was on the UART side,
not the DMA side: `alloy::uart`'s `write_dma()` is constrained on the driver
having `dma_tx_begin()` / `dma_tx_end()` / `tdr_addr()`, and the RP2040 PL011
driver had none of them, so both DMA examples took their honest fallback branch.
`src/alloy/hal/uart/raspberrypi_uart_pl011.hpp` now has all three (they set and
clear `UARTDMACR.TXDMAE`, curated from the pinned SVD, and hand back `&UARTDR`),
and the TX request id was already chip data (`uart0.dma_requests.tx` = 20).

So the path folds open **from `board.json` alone, with no preprocessor**:

| board | before | after |
|---|---|---|
| `raspberry_pi_pico` | `dma: not available on this board` | `dma via DMA` |
| `rp2040_zero` | `dma: not available on this board` | `dma via DMA` |

Both measured by building `examples/dma_uart` at the parent commit and at HEAD
and reading the strings out of the two ELFs.

**Two shipped examples now reach the engine on this family**, and both were
already in the `build` matrix in `.github/workflows/ci.yml` for both RP2040
boards — so **no emulation leg was added and none may be** (see above). One CI
step *was* added, and it is a compile-branch assertion rather than a witness:
because every DMA example folds by design, a broken hook would leave `dma_uart`
compiling, linking and silently printing the fallback forever, and the build
loop cannot see that. The step reads the linked ELF and fails if the fallback
string is present. It proves the branch was selected; it proves nothing about
bytes moving, which is what this sheet is for.

- **`examples/dma_uart`** — the sheet's main instrument. Claims
  `alloy::dma::channel<board::dma_t, 1>`, arms a completion callback, sends
  `dma via DMA\r\n` from RAM to `UARTDR` by DMA, then prints whether the
  interrupt fired. Three of the rows below are just "flash this and read the
  console".
- **`examples/dma_probe`** — its third branch does the same on channel 3 with a
  longer payload; its ADC-burst and PWM-waveform branches still print their
  fallback lines on this family, correctly, because neither of those drivers has
  DMA hooks here.

`dma_uart` claims channel 1 by hand, which on the Pico is the channel
`board.json` assigns to `debug_uart.rx`. Nothing else claims it in that firmware
so there is no conflict — but if you extend the firmware, claim through
`alloy::dma::claim(board::dma::debug_uart_tx)` (channel 2, request 20) instead
of hand-spelling an index.

Rows 5–8 still need small edits to that example; each says exactly what to
change. **None of it has ever been run.**

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

- **RUN**: `cd examples/dma_uart && alloy build --board raspberry_pi_pico &&
  alloy flash`, then `alloy monitor`.
- **LOOK FOR**: the banner `alloy dma_uart`, then the line `dma via DMA` — that
  second line is the one the DMA wrote; the banner came out of the CPU's own
  `write()`. Seeing the banner and not the second line is the interesting
  failure, and it is why the example prints both.
- **FALSIFIED BY**: `dma: FAIL` (`write_dma()` returned false — `wait()` saw an
  error bit, so the address or the width is wrong), **or** the banner followed
  by nothing (the channel never advanced: `TREQ_SEL` wrong, `UARTDMACR.TXDMAE`
  never set, or — the interesting one — `MULTI_CHAN_TRIGGER` is not in fact a
  start).

### 3. The completion interrupt reaches the NVIC

- **RUN**: same run as row 2 — the example arms the callback before the transfer
  and prints the result after it.
- **LOOK FOR**: `dma irq: fired`.
- **FALSIFIED BY**: `dma irq: NOT fired` while row 2 still passed. That isolates
  three things the host double can only check against itself: the `INTE0` bit,
  the vector number (`DMA_IRQ_0` = 11), and the `irq_line_of` ↔ `INTS0`
  line-index coupling — the driver puts every channel on line 0 (`kIrqLine`),
  which is a *policy*, not silicon.

### 4. THE DREQ NEGATIVE CONTROL — the highest-value row on this sheet

- **RUN**: rebuild row 2 with `TREQ_SEL` deliberately off by one. The request id
  is chip data, so change it in ONE place and let the generator carry it: in
  `chips/raspberrypi/rp2040.yaml`, `uart0.dma_requests.tx: 20` → `21`
  (`UART0_RX`), rebuild, re-flash. Changing it there rather than in the firmware
  is the point — it proves the id that the board actually emits is the id the
  transfer obeys.
- **LOOK FOR**: `dma: FAIL`, or the banner with no second line.
- **FALSIFIED BY**: it still prints `dma via DMA`. That would mean the transfer
  is not request-paced at all, and row 2 proved nothing about the request — it
  would have proved only that a channel can copy bytes as fast as the bus
  allows, into a FIFO that overflows.
- **WHY IT MATTERS BEYOND THIS FAMILY**: this is the first and only place in the
  whole DMA design where a request id is ever tested. Claim it as an advance.

### 5. Configuration does not start the channel — the INFERRED claim

- **RUN**: a variant of `dma_uart` that calls the engine's two halves apart:
  `alloy::hal::dma_impl<board::dma_t>::setup<1>(...)`, then **a visible interval**
  (blink the LED ten times), then `::start<1>()`, with a recognisable payload.
- **LOOK FOR**: nothing on the wire until after the delay, then the whole
  payload at once.
- **FALSIFIED BY**: bytes appearing during the delay. That means writing
  `CH_AL1_CTRL` started the channel, i.e. the curation's inference about which
  registers trigger is wrong in the dangerous direction — and every `setup()`
  in this driver is starting a channel while programming it.

### 6. `stop()` really terminates, and the buffer is safe afterwards

- **RUN**: a variant of `dma_uart` using `write_dma_begin()` instead of
  `write_dma()` so the wait is yours: a payload of a few kB in RAM (visible
  seconds at 115200 baud), then `chan.stop()` mid-flight, then **overwrite the
  source buffer with a different pattern** and print what still arrives.
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

- **RUN**: `examples/dma_probe` (channel 3) and `examples/dma_uart` (channel 1)
  are already two different channels through the same engine, but they are two
  separate flashings. For the real test, extend `dma_uart` to claim a second
  channel, arm completion callbacks on both, and run one transfer on each.
- **LOOK FOR**: both callbacks firing, each exactly once.
- **FALSIFIED BY**: one callback firing twice, or one never firing — the
  shared-line guard (`INTS0` bit test) or the per-channel `INTR` clear is wrong,
  and the host double's version of that check agrees with the driver by
  construction.

## What executing this sheet does NOT license

- **Nothing about rings.** `alloy::dma::ring` does not exist on this family and
  this sheet does not test it. Both capability flags are false, for independent
  reasons: there is no half-transfer event anywhere in this IP, and a single
  channel halts at the end of its count with nothing to re-arm it. That refusal
  is a *compile* fact, measured rather than asserted — a project that calls
  `uart.rx_ring(buf)` or `adc.ring(samples, 4)` on a Pico fails to build with
  *`nested requirement 'ring_capable<…>' is not satisfied`*. See
  `docs/design/dma-streams.md` §3.4 for the named successor (a one-channel
  software ping-pong through `CH_AL2_WRITE_ADDR_TRIG`) and why it was deferred.
- **Nothing about peripheral→memory in either direction.** Every row above is
  memory→peripheral, because that is the only direction any shipped firmware
  reaches on this family: the PL011 driver has TX DMA hooks and deliberately no
  RX ones (that header says why — the ring is missing, and the frame-gap event
  the RX stream needs stands on a request flavour the pinned SVD does not name),
  and the RP2040 ADC driver has no burst hooks at all. `setup()`'s
  `periph_to_mem` branch — the read/write address swap and `INCR_WRITE` instead
  of `INCR_READ` — is witnessed only by the host double.
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
