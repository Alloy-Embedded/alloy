# What is proven, and how

Every other page in this documentation tells you how to use something. This one tells you
whether to believe it.

alloy's house rule is that **a claim carries its evidence**. That rule is easy to state and
tedious to keep, and it is kept in the places where it matters most — driver headers name what
they cannot prove, robot files name their own vacuous spots, `alloy chip-status` closes every run
by reminding you what it is not. This page collects all of it in one table so you do not have to
go looking, capability by capability, before you commit a product to it.

!!! danger "Read this before anything else on this page"
    **Almost nothing in this framework has been run on physical silicon by this project.**
    There is no hardware CI runner. There is one first-party silicon result — the
    [message bus](../guide/bus.md) on a SAM E70 Xplained, with bench numbers — and two driver
    quirks that were *found* on that same board and are marked `SILICON-FOUND` in the headers
    that own them. Everything else in these tables is a laptop or an emulator talking.

    What that means if you are shipping a product: the register **sequences** here have been
    exercised hard, and by machines, on every push. What has not been exercised is the **contact
    with the part** — analog behaviour, timing against a real clock, errata, DMA request routing,
    and every claim a datasheet makes that no model implements. Budget a bring-up. The point of
    this page is to tell you where that budget should go, so you spend your scarce hardware time
    on the rows that are weak rather than re-checking the rows a machine already re-checks for
    you on every commit.

---

## The five words

Used precisely, everywhere below and everywhere else in these docs.

| Word | What it means | What it does **not** mean |
| --- | --- | --- |
| **Renode-proven (blocking)** | A CI leg boots real driver code on an emulated die and asserts on real UART output. A red run fails the build. | It is not silicon. It is exactly as good as Renode's model of that peripheral, and several legs name their own blind spots. |
| **Renode-proven (experimental)** | The same, but the leg is `continue-on-error` while a new board+robot pair earns trust. | A red run here does **not** fail the build. Read it as "someone watched it pass", not "it cannot regress unnoticed". |
| **Host-tested** | The real driver code runs over a memory-backed register double on a laptop. Proves sequence, ordering and bookkeeping. | It proves nothing about whether the register map is right. A driver that writes the wrong address passes, because the double accepts whatever it is given. |
| **Compiles only** | The branch was selected at compile time and linked into an image. | Nothing has observed it do anything. |
| **Unwitnessed** | A named claim that nothing available can reach — no model, no double, no leg. | It is not known to be **false**. It is not known. The distinction matters: an unwitnessed claim is a place to spend bench time, not a bug report. |

---

## What actually runs, today

Measured on this tree, not recalled:

| | |
| --- | --- |
| Host assertions | **640 of 640 passing**, across 47 `tests/test_*.cpp` files plus 22 more in `libs/*/tests/`. `alloy test --framework`, 0.45 s. |
| Emulation robots | **17 distinct robots**, run as **32 board+robot pairs** — **24 blocking**, 8 experimental. |
| Plus, outside that matrix | 5 more oracle legs: the bootloader end to end on `nucleo_g071rb`, `nucleo_f722ze` and `same70_xplained`; the provisioning oracle; the signed-image + key-rotation oracle. And two product legs (`product_demo`, one per product of a family). |
| Compile breadth | Every example built for **8 boards** on every push, plus the network examples on the two boards with a MAC. |
| Silicon runs | **One**, and it is not in CI. See [What has ever touched hardware](#what-has-ever-touched-hardware). |

---

## Core peripherals

The **Witness** column is the strongest evidence that exists, and the board it ran on. Where a
leg has a known blind spot, the blind spot is in the last column, because a leg you over-read is
worse than no leg.

| Capability | Witness | What it asserts | What it cannot reach |
| --- | --- | --- | --- |
| **GPIO** out (`led.on/off/toggle`) | Renode, blocking — indirectly, through every leg that boots (`firmware_boots` on `nucleo_g071rb`, `nucleo_g0b1re`, `nucleo_f722ze`, `same70_xplained`) | The port is configured and driven without faulting. | No leg reads a pin back. Drive strength, speed, open-drain — unwitnessed. |
| **Pin interrupts** (`on_active`, `take_edge`) | Renode, blocking (`pin_irq`, `nucleo_g071rb`) | Port select, trigger selection and delivery to the right NVIC vector — each verified falsifiable by breaking it in turn. | The model leaves `IMR1` unimplemented and delivers whether or not the driver unmasks; and it latches both edge directions in one pending register where the G0 splits them. So "unmasks correctly" and "clears the right pending half" are open until silicon. |
| **UART** polled read/write | Renode, blocking (`uart_echo_roundtrip`, `nucleo_g071rb`; `uart_frame_surface` on `nucleo_g071rb` and `nucleo_f722ze`) | An injected line comes back out the TX path, through the real driver, at the configured baud. | Baud *accuracy* against a real clock. The emulated clock is not the target's. |
| **UART** interrupt RX (`on_receive`) | Renode, blocking (`uart_frame_surface`) and first-party silicon via the bus bridge (below) | The ISR path delivers every byte where the polled path drops them. | — |
| **I²C** blocking + `write_async` | Renode, blocking (`i2c_read`, `nucleo_g071rb`) against a `DummyI2CSlave` at 0x08, run both polled and interrupt-driven | Addressing, direction, NBYTES, STOP, and the interrupt path moving every byte. | Clock stretching, arbitration loss, a device that NACKs mid-transfer. A zero-length probe is not implemented by the model, so `probe()`'s real shape is unwitnessed. |
| **SPI** blocking + `transfer_async` | Renode, blocking (`spi_read`, `nucleo_g071rb`); experimental on `nucleo_g0b1re` and `nucleo_f722ze` | Mode, byte order and full-duplex exchange against a `DummySPISlave`; the async path clocking the remainder from the ISR. | The Microchip SPI has no interrupt path at all (no curated IER/IDR/IMR fields), so `transfer_async` does not exist there — a compile error, not a silent fallback. |
| **ADC** `read()` | Renode, blocking (`adc_read`, `nucleo_g071rb`) | *The right channel.* Distinct voltages to distinct channels (1650 mV on ch3, 3300 mV on ch4) and the converted counts asserted (2048, 4095) — converting the wrong channel prints the wrong number, verified. | Two vacuous spots, stated in the leg: the model does not implement the channel-ready flag, so that poll passes unconditionally through a shim; and the calibration bit is a tagged no-op, so the self-clear poll passes vacuously. Analog accuracy, sample time and reference behaviour: unwitnessed everywhere. |
| **ADC** analog watchdog | Renode, blocking (`adc_watchdog`, `nucleo_g0b1re`) | The window comparator trips on a fed voltage outside the configured band, and rearms. | — |
| **ADC** streaming (`ring`, `read_burst`) | See [Streaming without the CPU](#streaming-without-the-cpu) | | |
| **PWM** (`set_duty`, `stream_duty`) | **Compiles only** | Nothing. | Everything. The generated Renode platform does not instantiate a timer at all — *checked, not assumed*. The prescaler arithmetic, duty-step counts and centre-aligned halving are host-tested; nothing has observed a waveform. |
| **Bridge** (three-phase complementary) | **Compiles only**, on two families | Nothing. Dead-time encoding, the CR2 allowlist, the trigger refusal and the torn-frame witness are host-tested with negative controls. | **No `alloy::bridge` driver has switched a transistor, on any board.** Dead time, break input and main-output-enable are unmodelled. On the F7 the carrier frequency is additionally unverified — see [PWM](../guide/pwm.md#what-has-actually-been-proven-and-what-has-not) before you power a stage. |
| **Timers** — `tick`, `encoder` | **Host-tested** (`tests/test_tick.cpp`, `tests/test_encoder.cpp`) | Timebase arithmetic and count/delta bookkeeping. | **alloy's emitter maps no Renode model to any ST timer IP**, so no generated platform instantiates one and nothing timer-shaped has an emulation witness on any board. Renode itself is not the wall here: 1.16.1 ships `Timers.STM32_Timer` and its own stock `stm32g0.repl` wires ten of them. That model drives NVIC lines and never a pin, so it could witness an update interrupt and never a waveform — but the update interrupt is reachable work, not an impossibility. |
| **CAN** | **Host-tested** (`tests/test_can.cpp`) | Filter/accept logic and frame packing. | No FDCAN model exists. Nothing has put a frame on a bus. |
| **DAC** | **Compiles only** (`examples/dac` builds) | Nothing. | No model, no host test. |
| **RTC** | **Compiles only** (`examples/rtc` builds) | Nothing. | No model, no host test. Calendar arithmetic across a real LSE is unwitnessed. |
| **Watchdog** (IWDG) | **Compiles only** — a Renode model of the IP exists, but no leg asserts on it | Nothing. | Whether a missed feed actually resets the part. |
| **Window watchdog** (WWDG) | **Host-tested** (`tests/test_wwdt.cpp`) | The window arithmetic, against the manual's formula. | The driver header says it outright: *no board was on hand, Renode ships no WWDG model for any STM32, so nothing here has been observed to reset anything.* |
| **CRC** unit | **Host-tested** (`tests/test_crc.cpp`) | That the block's output is bit-for-bit the CRC-32 every alloy on-flash format already uses — checked against the software implementation. | The header is explicit: *not silicon-witnessed*, and Renode's platform carries a bare address tag for the CRC region that reads back zero, so an emulated run would "verify" every image as having checksum `0xFFFFFFFF`. **Do not add an emulation leg here** — it would be a false green. |
| **Device UID** | **Host-tested** (`tests/test_uid.cpp`) | Read shape and byte order. | No model. |
| **Flash / NVM / FS** | Mixed — see [Firmware update](../guide/firmware-update.md#what-this-is-proven-against) | The bootloader legs exercise everything above the flash driver on three boards. | **None of the three flash drivers has run on silicon.** alloy emits no STM32G0 flash-controller model — the only thing available is Renode's **F4** controller, which its own stock G0 platform maps onto the G0 base address despite the different register layout, and alloy's emitter marks that entry "deliberately absent" rather than assert it — so the G0 driver's own register sequences go unchecked; the F7 legs use Renode's controller; the SAME70 legs use a model **this project wrote**, which cannot falsify a mistake it shares with the driver. |

---

## Streaming without the CPU

DMA is the one area where the answer differs on every engine, so it has its own tables — a
per-board availability matrix and a per-capability witness table — in
**[Streaming data without the CPU](../guide/dma.md#how-well-proven-and-what-is-still-owed)**.
The one-paragraph summary, so you know whether to go and read it:

- `uart.write_dma` and `adc.ring` on the STM32 G0 are **Renode-proven by blocking legs**, the
  ring on two boards, with negative controls verified red.
- `uart.rx_ring` and `spi.transfer_dma` are **Renode-proven blocking** on `nucleo_g071rb` and
  experimental on three and two more boards respectively.
- `i2c.read_dma` / `write_dma` are **host-tested only**, and their emulation leg asserts a
  *bounded refusal* rather than a transfer, because Renode's model of that IP exposes no DMA
  request output. No leg anywhere witnesses an I²C DMA transfer.
- The **SAM E70 ring compiles, links, and has never been run**, against a descriptor layout that
  is curated in no file in either repository. This is the one honest gap in the framework that
  does **not** fail loudly, and the DMA guide leads with it.
- **RP2040 DMA has no emulation and must not get any** — Renode reads 0 from unmapped addresses,
  so a leg would report a completed transfer over a buffer of zeros. Its behavioural witness is
  a host mutation ledger plus the
  [on-hardware checklist](../guide/rp2040-dma-hardware-checklist.md), **written and never
  executed**.
- `pwm.stream_duty` and `alloy::async::ring_waiter`: compiles-only and host-tested respectively.

---

## Runtime and concurrency

| Capability | Witness | What it asserts | What it cannot reach |
| --- | --- | --- | --- |
| **Coroutine executor** (`task`, `executor`, `delay`) | Renode, blocking (`async_runs` on `nucleo_g071rb`, `nucleo_f722ze`, `same70_xplained`) | Two concurrent tasks are actually *scheduled* — across **two vendors**, which is what turns "the runtime works" from a claim about ST silicon into a claim about the runtime. | Timing. Stack high-water on a real part under real interrupt load. |
| **Peripheral awaitables** — `spi_master`, `i2c_master`, `dma_waiter` | Renode, blocking (`async_io`, `nucleo_g071rb`); experimental on `nucleo_f767zi` | A task **suspended and was resumed by the peripheral interrupt** — the leg prints how many times the executor resumed it, and the same I/O done without suspending prints a different number and fails. | Nothing has run on physical silicon. |
| **`uart_reader` awaitable** | **Host-tested only** | Bookkeeping against a `mock_uart`. | No example uses it and no leg exercises it; it has never been observed running on an ISA. |
| **`ring_waiter`** | **Host-tested only** (two cases in `tests/test_dma_ring.cpp`) | A parked task wakes once per boundary; a slow consumer resynchronizes. | All of it, on a target. |
| **Interrupt plumbing** — `attach`, `set_priority`, `critical_section` | Renode, blocking (`concurrency_probe`, `nucleo_g071rb`) | Shared-vector chaining, the handler pool, and that a masked level really is masked while a more-urgent line still fires. | Latency figures. Every timing number quoted about the executor is emulation arithmetic, not measured on a part. |
| **Cooperative scheduler** (`sched`) | **Host-tested** (`tests/test_sched.cpp`), plus `examples/services` builds everywhere | Ordering and deadline bookkeeping. | No leg. |
| **DSP / control / filters / FSM / units** | **Host-tested** (`tests/test_dsp.cpp`, `test_control.cpp`, `test_fsm.cpp`, `test_units.cpp`) | Arithmetic, against fixtures. | These are pure logic; a host test is close to the strongest witness they can have. Numerical behaviour on a specific FPU is not covered. |

---

## Shipping machinery

| Capability | Witness | What it asserts | What it cannot reach |
| --- | --- | --- | --- |
| **Crash reports** | Renode, blocking (`crash_report` on `nucleo_g071rb` and `nucleo_f722ze`) | The full loop across **five real boots of one machine**: the handler stores the stacked frame in `.noinit`, resets through `SYSRESETREQ`, and the next boot reports a real `pc` — with a consecutive counter that a stale record could not produce. | **Organic fault entry is silicon-only and unwitnessed.** The firmware's own trigger is a software-pended fault; Renode never vectors a real wild jump (it kills the core with the vector table unread). The NVM persistence branch is likewise unwitnessed in emulation. |
| **Bootloader** — verify → jump → recover | Renode, blocking (`bootloader_boots` on `nucleo_g071rb`, `nucleo_f722ze`, `same70_xplained`) | A packed image is verified, jumped into, and a bad slot is recovered from. | See the flash row above: on the G0 the flash driver's own register sequences are unchecked, and the SAME70 leg runs against a self-authored model. |
| **Signed images** | Renode, blocking (`signed_boot`) | Three machines, and **the two rejections are the claim**: a tampered payload whose CRCs were repaired is refused, and a perfectly valid signature from another key is refused. | Key storage. Nothing here defends a device whose flash an attacker can rewrite. |
| **Factory provisioning** | Renode, blocking (`provision`) | A real product firmware, booted out of slot A through the real bootloader, finds and parses the identity record the host tool wrote — so the linker address, the encoder and the parser are the same three facts. | That the page survives a real field update on real flash. |
| **Products** (`--product`) | Renode, blocking (`product_demo`, one leg per product) | The firmware names its product **and its control strategy's own arithmetic ran** — one strategy cannot produce the other's output, so a stale build tree fails here. | — |
| **RDP / WRP** (`alloy secure`) | Renode, experimental — the **option-byte register interface** on the F7 model, driven by the same write list `apply` sends to openocd | A locked register ignores writes, the key sequence unlocks it, the planner's poke list programs the expected value, relocking holds. | **The protection semantics themselves are unwitnessed**: actual readout blocking, the mass erase, RDP2 permanence, WRP refusing an erase, persistence across power cycles. Renode's F7 model has no protection *behaviour* at all, and its G0 has no flash controller. This gap cannot be closed by emulation, only by a board you are willing to lose. |
| **Network stack** (lwIP, sockets, HTTP) | **Compiles only** — `build-net` builds four examples for `same70_xplained` and `nucleo_f767zi` (two MAC families) | Nothing beyond the link. | No emulation leg, no packet ever sent. `tests/test_net.cpp` covers the host-side seams only. |

---

## Libraries

| Library | Witness | Notes |
| --- | --- | --- |
| **`bus`** (pub/sub + wire + bridge) | Renode, experimental (`bus_bridge`, `nucleo_g071rb`) **and first-party silicon** | The leg's peer is an *independent* frame implementation in monitor Python — its own bitwise CRC-32, its own encoder — so a firmware that merely echoed frames could not pass. The pong carries an incrementing count only the service's state can produce. This is also the one library with bench numbers from a real part. |
| **`modbus`** (RTU client + server) | Renode, blocking (`modbus_rtu`, `nucleo_g071rb`); experimental on `nucleo_g0b1re`, `nucleo_f722ze`, `nucleo_f767zi` | The robot plays a byte-level Modbus master. Six host test files cover the sans-IO core, framer, PDU, CRC, client and server. |
| **Sensor / display / RTC drivers** (`bh1750`, `bme280`, `ds3231`, `mpu6050`, `sht31`, `ssd1306`) | **Host-tested** against testkit doubles | Register sequences and error paths, including NACK. No part has answered any of them. |
| **Control plane** (`meter`, `ntc`, `param`, `pll`, `protect`) | **Host-tested** | Pure arithmetic and state, which is most of what they are. Nothing has closed a loop on a motor. |

---

## A route is witnessed by halves

This is the nuance that costs the most if you meet it for the first time on a bench, and it is
**measured**, not inferred. It applies to every DMA route on every board.

> **Emulation proves which channel moved the bytes. Only silicon proves that the right peripheral
> asked for them.**

A DMA route has two halves — a channel (or stream) index, and a *request id* that says which
peripheral's ready-signal paces that channel. Under emulation the first half is load-bearing and
the second is not:

- On the **STM32F7 stream engine** this was measured by adversarial control. Move the model's
  request wire by one stream and the leg goes red — the index is checked end to end. Delete the
  channel-select write from the driver entirely and the leg stays **green** — the request id is
  never checked.
- On the **STM32G0** Renode models no request multiplexer at all, and the emulated platform's
  wiring and the firmware's route both descend from the *same* `board.json` statement. Two
  copies of one assumption cannot falsify each other.
- On the **SAM E70** it is worse by construction: the pinned USART model exposes no DMA-request
  output, so there is no wire for any model to consume, and the generated DMA model never
  consults the request id at all.

**What to do about it.** On the first bring-up of a new board's DMA assignment, verify the
peripheral is really the source: feed a distinctive pattern and check that it is what arrives, or
perturb the request id in the chip data and confirm the transfer *stops*. In this entire
framework that check exists in exactly one place — row 4 of the
[RP2040 DMA hardware checklist](../guide/rp2040-dma-hardware-checklist.md) — and that place has
never been executed.

---

## What has ever touched hardware

Four categories, kept apart on purpose, because "we ran it" and "somebody else ran it" are
different sentences.

**1. First-party silicon — one result.** `examples/bus_bridge` and `libs/bus` on a SAM E70
Xplained. Round-trip 3.4 ms at 115200 and 1.9 ms at 230400; sustained ~290/s and ~400/s;
20 messages sent back to back where polled RX delivers 7 and interrupt RX delivers 20. The
numbers and what moves them are in [The message bus](../guide/bus.md#what-it-costs-on-a-real-wire-and-how-to-make-it-faster).
Note what it is *not*: that run uses no DMA path of any kind.

**2. Silicon-found driver quirks — two, both SAM E70, both marked in the header that owns them.**

- The XDMAC's memory-side master reads SRAM but **not** embedded flash: a `.rodata` source raised
  a read bus error on a real SAM E70 Xplained. (The ST engine reads flash fine, so portable code
  that works on a G0 can break here for this reason alone.)
- On the Microchip SPI in Variable PS mode, a TDR write with the "no peripheral" chip-select
  encoding **suppresses SCK entirely** — so every TDR write selects NPCS0 even when the real
  chip-select is a caller-driven GPIO.

Both are the good kind of evidence: they were discovered by a part disagreeing with a reading,
which is exactly what silicon is for.

**3. Inherited hardware claims — attributed, not first-party.** Several clock profiles in the
device database carry hardware provenance from a **predecessor codebase** ("old-alloy"): the G0's
`pll_64mhz` records an order hardware-verified there and validated on two Nucleos; the SAM E70's
`plla_150mhz` records a hardware-validated bring-up; the ESP32's boot defaults record a
hardware-validated blink calibration. One profile carries `"silicon_validated": true` as data.
These are real claims by real people about real boards — and they are **not** this project's
runs. Treat them as you would a well-sourced third-party report.

**4. The landing page's board table.** [Home](../index.md#supported-boards) has a column of
`silicon` pills. Those are **maintainer self-reports** from bring-up, not automated and not
re-checked, and the page says so directly above the table. Where that column and a feature page
disagree, the feature page wins — and several of them disagree flatly. See
[the unwitnessed list](#claims-this-page-cannot-settle) below.

---

## Where no model exists at all

Emulation can only be as broad as Renode's peripheral coverage, and it is worth knowing the shape
of that before you plan a validation strategy. `alloy chip-status <chip>` answers it per part;
measured on this tree:

| Chip | Curated | With drivers | **With a Renode model** |
| --- | --- | --- | --- |
| `st/stm32g0b1re` | 49 of 65 | 43 | **23** |
| `st/stm32g071rb` | 17 of 17 | 15 | **13** |
| `microchip/atsame70q21` | 16 of 16 | 13 | **3** |
| `raspberrypi/rp2040` | 10 of 10 | 4 | **1** |
| `espressif/esp32` | 9 of 9 | 4 | **0** |

Read the last column as the ceiling on what emulation can ever assert for that family. On the
SAM E70 only the flash controller, one USART and the DMA controller are modelled — and two of
those three are models **this project generated**. On the ESP32 nothing is modelled, which is
also why `alloy emulate` refuses those two boards rather than booting them into fiction.

On the ST side the gap has a shape worth naming: **a generated platform instantiates no ST timer
of any kind**, and none of CRC, DAC, RTC, LPUART, the device UID, FDCAN or the G0 flash
controller either. That is precisely the set a motor-control application leans on hardest, and it
is why the PWM, bridge, tick and encoder rows above say what they say.

That column is `emit/renode.py`'s table, and the distinction matters if you are deciding where to
spend effort. For CRC, DAC, LPUART and the UID there is genuinely nothing to instantiate —
Renode's own `stm32g0.repl` carries CRC and DAC as bare address `Tag`s, which read back zero, and
carries no LPUART or UID at all. For the **timers, the RTC and FDCAN** Renode does ship a model
and its own stock platform wires each one up; alloy's emitter declines to emit them. Those three
are a *decision*, re-openable by someone who wants a leg, rather than a limit of the emulator —
though for the timers the available model drives interrupt lines and never a pin, so reopening it
would buy an update-interrupt assertion and still no waveform. The G0 flash interface is a fourth
case and the most instructive: Renode's stock `stm32g0.repl` points its **F4** flash-controller
model at the G0's base address, a different register layout, and alloy's emitter marks that entry
"deliberately absent" rather than emit a model it does not believe.

---

## Claims this page cannot settle

Marked **unwitnessed** rather than deleted or left standing, per the rule at the top. Each is a
real disagreement inside this repository that needs a person with a board, not an editor.

!!! warning "Unwitnessed — the landing page's `silicon` column versus the feature pages"
    [Home](../index.md#supported-boards) reports, as maintainer self-reports, PWM and flash
    working on silicon on the STM32G0, and I²C/ADC/DMA on the SAM E70. Five feature pages state
    the opposite in their own voice:
    [PWM](../guide/pwm.md#what-has-actually-been-proven-and-what-has-not) ("never run on
    silicon… no `alloy::bridge` driver has switched a transistor, on any board"),
    [firmware update](../guide/firmware-update.md) ("none of the three flash drivers has run on
    silicon"), [async](../guide/async.md) ("none of it has run on physical silicon"),
    [crash reports](../guide/crash-reports.md) (organic fault entry: "silicon-only,
    unwitnessed") and [security](../guide/security.md) ("not witnessed by alloy on silicon").

    Both cannot be true of the same driver. The most likely reconciliation is that the pills
    describe a **predecessor codebase's** bring-up on those boards — the same lineage the clock
    profiles credit explicitly — and that the current drivers have not been back. That is a
    guess, and this page does not act on guesses. **Until a maintainer settles it, treat the
    feature pages as authoritative**, because they are the narrower and more recent claim.

!!! warning "Unwitnessed — `emulation.md`'s two loose ends"
    [Emulation](../guide/emulation.md) says PWM is "verified on silicon instead", which the PWM
    page contradicts in the same words as above. It also says hardware-validated families are
    "marked as such in [Boards]", and [Boards](../guide/boards.md) marks nothing of the kind —
    its only proof statement is a pointer at the project README. Both are inherited sentences
    that no longer describe the tree; both are flagged rather than rewritten, because deciding
    what they *should* say is the same maintainer decision as the one above.

!!! warning "Unwitnessed — three board `dma:` assignments that no driver can consume"
    `same70_xplained` assigns DMA channels for SPI and I²C; `raspberry_pi_pico` assigns one for
    the ADC. In each case the peripheral driver has no DMA entry points, so the assignment is
    inert — the facade folds and the image says so. Board validation accepts them because it
    checks the statement against the **chip's** routing data and never against the **driver's**
    hooks. Whether that should be a validation error is a design call, not a documentation one.
    Until then, an assignment in a board file is not by itself evidence that a capability exists;
    [the DMA availability table](../guide/dma.md#what-each-board-gives-you) is.

---

## Checking any of this yourself

None of the above needs to be taken on trust — that is rather the point.

```bash
alloy chip-status st/stm32g0b1re     # per-peripheral: curated / driver / Renode model / role
alloy test --framework               # the whole host suite, on your laptop
alloy emulate --board nucleo_g071rb  # boot your own firmware on the emulated die
```

Each robot under `tests/emulation/` opens with a documentation block written to be read: it says
what the leg asserts, which negative controls were run against it, and — the part that matters
here — which of its assertions are vacuous and why. `tests/emulation/adc_stream.robot` and
`tests/emulation/dma_uart.robot` are the two worth reading first, because they are the most
careful about the difference.

The CI workflow is the other primary source: `.github/workflows/ci.yml` carries the board+robot
matrix, the `experimental` flag on each pair, and long comments explaining why particular legs
exist in the form they do.

And when you quote a result of your own — especially a **red** one — stamp the tree it came from.
[Testing](../guide/testing.md#say-which-tree-you-measured) explains why that is not pedantry: two
sessions in this project once independently reported, in good faith, that every example failed to
build at a given commit. The commit was fine.
