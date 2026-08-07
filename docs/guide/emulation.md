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
driver-conformance legs are designed so they cannot pass by coincidence: the ADC model returns
`1000 + channel`, so printing `adc: 1003` proves the driver selected channel 3 — a wrong channel
prints a different number.

!!! warning "Not every leg is equally strong"
    The I²C and SPI legs run against **Renode's own** slave models, so they can genuinely
    contradict a misreading of the reference manual. The ADC, DMA and SAME70-flash legs run against
    models *this project wrote* — a self-authored model cannot falsify a mistake it shares with the
    driver. Treat those as consistency checks, not independent proof.

## Writing a test

Tests are [Robot Framework](https://robotframework.org) files under `tests/emulation/`. The
pattern is always the same: include the generated script, attach a terminal tester to the UART,
run, and assert on lines.

```robotframework title="tests/emulation/adc_read.robot"
*** Test Cases ***
ADC Driver Converts A Channel
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy adc_read    timeout=30
    Wait For Line On Uart     adc: 1003    timeout=30
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

Where a model was genuinely missing but the behaviour was worth proving, alloy ships its own —
the STM32G0 ADC and DMA controllers, and the SAME70 EEFC flash controller, are inline Python
peripherals emitted into the platform file, written against the reference manuals.

Known gaps, so you do not go looking:

- **RP2040** — this Renode ships no RP2040 peripheral models at all, so those boards are not
  emulated. Not an alloy gap; it needs a newer Renode.
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
