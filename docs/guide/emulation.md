# Emulation

alloy can run your firmware on an emulated MCU, with no board attached:

```console
$ alloy emulate --board nucleo_g071rb
platform: /…/.alloy/build-tree/nucleo_g071rb/out/nucleo_g071rb.repl
script:   /…/.alloy/build-tree/nucleo_g071rb/out/nucleo_g071rb.resc
uart:     sysbus.usart2
alloy uart_echo ready
```

This is not a simulator of alloy's own making — it is [Renode](https://renode.io) driving the
real instruction set and real peripheral models. What alloy contributes is that **the emulated
machine is generated from the same chip data the firmware is compiled against**, so the memory
map, the clock tree inputs and the UART instance cannot drift from the target.

## Setting it up

Put `renode` on your `PATH`, or drop an install under `~/.alloy/tools/` — the same place `alloy
setup` keeps cross-toolchains — and the CLI finds it:

```
~/.alloy/tools/renode-1.16.1/Renode.app        # macOS
~/.alloy/tools/renode-1.16.1/renode            # Linux / portable
```

`--emit-only` writes the `.repl` platform and `.resc` script and stops, which is what you want
when you are driving Renode yourself or debugging a platform:

```console
$ alloy emulate --emit-only --board same70_xplained
```

!!! tip "Pin your Renode"
    alloy's CI pins **v1.16.1** rather than tracking the nightly build. A nightly once changed a
    peripheral model's constructor signature and broke a working platform; a pinned version means
    a green run locally still means something in CI, and a Renode upgrade is a deliberate,
    reviewable change.

## What emulation is used for here

An emulator that only answers "did it boot" is a compile check with extra steps. alloy uses it as
a **behaviour oracle**: every leg below asserts on real UART output produced by real driver code,
and every one of them is a *blocking* CI gate.

| What is proven | Boards |
| --- | --- |
| Boot to a banner | `nucleo_g071rb`, `nucleo_g0b1re`, `nucleo_f722ze`, `same70_xplained` |
| UART echo round-trip (RX path → loop → TX path) | `nucleo_g071rb` |
| Two concurrent coroutines on the heap-less executor | `nucleo_g071rb`, `nucleo_f722ze`, `same70_xplained` |
| I²C driver conformance against a slave on the bus | `nucleo_g071rb` |
| SPI driver conformance | `nucleo_g071rb` |
| ADC conversion of the *right* channel | `nucleo_g071rb` |
| UART transmit driven by DMA | `nucleo_g071rb` |
| Bootloader verify → jump → recover | `nucleo_g071rb`, `nucleo_f722ze`, `same70_xplained` |
| The full [update lifecycle](firmware-update.md) + signed-image oracle | `nucleo_g071rb` (SAME70 runs the install/confirm half) |

Two of those deserve a note. The async legs cover **two vendors**, which is what turns "the
coroutine runtime works" from a claim about ST silicon into a claim about the runtime. And the
driver-conformance legs are designed so they cannot pass by coincidence: the ADC leg feeds
*distinct voltages to distinct channels* (1650 mV on ch3, 3300 mV on ch4) and asserts the
converted counts (`adc ch3: 2048`, `adc ch4: 4095` at the 3.3 V reference) — converting the
wrong channel prints the wrong counts (an unfed channel prints 0, verified), and so does wrong
conversion math.

!!! warning "Not every leg is equally strong"
    The I²C, SPI, ADC and DMA legs run against **Renode's own** models, so they can genuinely
    contradict a misreading of the reference manual. (The ADC and DMA legs originally ran against
    models this project wrote; both were replaced by Renode's native models.) The SAME70-flash leg
    still runs against a model *this project wrote* — a self-authored model cannot falsify a
    mistake it shares with the driver. Treat that one as a consistency check, not independent
    proof.

    The ADC leg carries two vacuous spots, stated so nobody over-reads it: Renode's `STM32G0_ADC`
    does not implement `ISR.CCRDY` (the flag real silicon raises after a `CHSELR` update — in the
    1.16.1 source, bit 13 is reserved), so the generated `.resc` hooks the bit into every ISR
    read and the driver's CCRDY poll passes **unconditionally**; and `ADCAL` is a tagged no-op
    that never reads back 1, so the calibration self-clear poll also passes vacuously. Everything
    else the leg asserts — `ADEN`→`ADRDY`, `ADSTART`→`EOC`, the DR read clearing `EOC`, channel
    routing, and the 12-bit conversion arithmetic — is the native model's behaviour.

    The **pin-interrupt** leg sits between the two: it runs against Renode's own
    `STM32WBA_EXTI`, standing in for the G0's identically-laid-out block — so the port select,
    the trigger selection and the delivery to the right NVIC vector are all genuinely
    falsifiable (verified by breaking each in turn). Two things it *cannot* check: this model
    leaves `IMR1` unimplemented and delivers the interrupt whether or not the driver unmasks
    the line, and it latches **both** edge directions in `FPR1` where the G0 splits them across
    `RPR1`/`FPR1`. So "the driver unmasks correctly" and "the driver clears the right pending
    half" are open until silicon.

## Writing a test

Tests are [Robot Framework](https://robotframework.org) files under `tests/emulation/`. The
pattern is always the same: include the generated script, attach a terminal tester to the UART,
run, and assert on lines.

```robotframework title="tests/emulation/adc_read.robot"
*** Test Cases ***
ADC Driver Converts The Right Channels
    Execute Command           include @${RESC}
    Execute Command           sysbus.adc SetDefaultValue 1650 3
    Execute Command           sysbus.adc SetDefaultValue 3300 4
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy adc_read    timeout=30
    Wait For Line On Uart     adc ch3: 2048    timeout=30
    Wait For Line On Uart     adc ch4: 4095    timeout=30
```

`${RESC}` and `${UART}` are passed in by the caller — they come straight from what `alloy
emulate` printed, so a test never re-derives a fact the board data already carries.

Run one locally — the three values are the three lines `alloy emulate --emit-only` printed:

```bash
renode-test --variable RESC:/abs/path/nucleo_g071rb.resc --variable UART:sysbus.usart2 tests/emulation/adc_read.robot
```

`RESC` must be **absolute**: `renode-test` resolves `include @…` relative to the `.robot` file,
not to your working directory.

!!! warning "`renode-test` and paths with spaces"
    Its wrapper expands the working directory unquoted, so it fails in a directory whose path
    contains a space. Run it from a space-free directory (CI does; a checkout under
    `~/My Projects/` will not).

## When Renode does not model the part

The emitter follows the same honesty rule as the rest of the framework: **an unmodelled
peripheral gets no mapping, never a guessed one.** `alloy emulate` refuses a board it cannot map
rather than producing a platform that boots into fiction.

Where a model is genuinely missing but the behaviour is worth proving, alloy ships its own —
today only the SAME70 EEFC flash controller, an inline Python peripheral emitted into the
platform file, written against the reference manual. (The STM32G0 ADC and DMA controllers used
to be in that list; both now use Renode's native models, which is strictly better — a model this
project wrote cannot falsify a mistake it shares with the driver.)

Known gaps, so you do not go looking:

- **RP2040** — this Renode ships no RP2040 peripheral models at all, so those boards are not
  emulated. Not an alloy gap; it needs a newer Renode.
  **And do not repair it half way.** alloy *can* already emit an rp2040 platform — `cortex-m0`
  and `UART.PL011` are generic types Renode has — and Renode reads 0 from unmapped addresses.
  A DMA leg on such a platform would read the RP2040 DMA controller's `CTRL` as 0 (`BUSY` low,
  no error bits) and its `TRANS_COUNT` as 0, and report a **completed transfer over a
  fully-written buffer**, having moved nothing. That is a false green, not weak evidence.
  What stands in for the missing model there is a host double
  (`tests/test_rp_dma_v1_latch.cpp`, which proves register-sequence *intent* and nothing about
  behaviour) plus an on-hardware sheet:
  [RP2040 DMA — the on-hardware checklist](rp2040-dma-hardware-checklist.md), **written and not
  yet executed**.
- **STM32G0 flash** — there is no G0 flash-controller model, and the emitter deliberately does not
  invent one. The G0 bootloader legs therefore exercise everything *above* the flash driver while
  its own register sequences go unchecked. (The F7 legs use Renode's controller; the SAME70 legs
  use a model written here.)
- **PWM** — nothing observable comes out over a UART, so there is no meaningful assertion to
  make. Verified on silicon instead.
- **I²C bus scan** — a zero-length write (a valid probe on real silicon) is not implemented by
  Renode's I²C model, so the conformance test does a one-byte write plus read instead.

## The limit of the claim

Emulation proves that driver logic drives a *faithful model* of the peripheral. It does not prove
timing against real silicon, analog behaviour, or anything a datasheet erratum covers. The
framework's hardware-validated families are marked as such in [Boards](boards.md); everything
proven only in emulation says so where it is documented — including the three flash drivers
behind [firmware update](firmware-update.md).

That boundary is the point. Emulation makes the *logic* a gate that runs on every commit, so the
scarce resource — time with real hardware — is spent on the things only real hardware can answer.
