# Chip coverage

How much of a chip's silicon does alloy actually support? Until there was a verb for it, the
answer was an impression. `alloy chip-status` makes it a number — and, more importantly, makes
the number **derived**, so it moves on its own when the work lands and cannot be talked up.

```console
$ alloy chip-status st/stm32g0b1re
$ alloy chip-status st/stm32g0b1re --json      # same facts, for tooling
```

## What each column means

| Column | Question | Where the answer comes from |
| --- | --- | --- |
| `REG` | is there curated register data? | the chip yaml binds an `ip`, and `alloy-devices` ships `registers/<vendor>/<ip>.yaml` for it — the same gate codegen applies before it emits a descriptor |
| `DRV` | is there a HAL driver? | `src/alloy/hal/<class>/<vendor>_<ip>.hpp` exists — the exact path the chip codegen `#include`s when it is there |
| `REN` | is there a Renode model? | the IP is named in one of `emit/renode.py`'s model tables |
| `ROLE` | is it reachable from a board? | a board using this chip binds the instance to a role — or, for a GPIO port, names one of its pins |

Nothing is hand-maintained. There is no checklist file to update, so the scoreboard cannot
quietly drift from the tree it describes. Run it inside a project and your own `boards/` are
searched too.

## What "100%" counts — and what it does not

The denominator is **peripheral instances of one part**, as the chip yaml lists them. On the
G0B1RE that means `usart1`…`usart6` count six times while sharing one IP, one register file and
one driver — so the per-IP figures are printed next to the headline. Six instances of a curated
IP is *one* curation job done, not six.

Three things the scoreboard deliberately does **not** claim:

- **`REG` is about register data, not about a working peripheral.** An instance can be curated,
  have a driver, and be entirely unproven.
- **`DRV` is about a header existing for the IP.** It says nothing about how much of the
  peripheral that driver covers, and nothing about it being tested.
- **`REN` means a model is known for the IP** — not that this board's platform instantiates it
  (that also depends on the board's roles), and not that any emulation leg exercises it.

And none of the four is evidence from silicon. This scoreboard is the **floor** of the coverage
push, not its ceiling.

## A reading — STM32G0B1RE

!!! warning "This section is a snapshot, and snapshots are the thing this page exists to abolish"
    The numbers below were read out of `alloy chip-status st/stm32g0b1re --json` on `alloy`
    `2abbb92` with `alloy-devices` `9f838ed` (declared 0.3.0, digest
    `sha256:928acb9d…`, 520 files). They are here to give the shape of a real answer, not to be
    the answer. **Run the verb.** A previous version of this section sat here for a week and went
    stale by twelve peripherals and twelve drivers — which is exactly the failure mode the verb
    was written to end.

**49 of 65 peripherals curated, 43 with drivers**, 23 with a Renode model, 18 reachable from a
board role (`nucleo_g0b1re`). By IP: 27 curated IPs, 22 with drivers.

The denominator being 65 rather than the 23 an early hand-written chip file listed is not new
support — it is the **gap becoming visible**. Every peripheral the silicon has now appears, and
the ones alloy cannot yet touch say so.

### Curated, no driver (6)

`adc1_common`, `dmamux1`, `fdcanram1`, `fdcanram2`, `pwr`, `rcc`.

These are the register-only blocks: they exist so other code can address them (the clock
program writes `rcc`, the FDCAN driver addresses its message RAM), and a standalone driver
would have nothing to drive. This is the one list here that has not moved in months, which is why
it is still written out.

### Uncurated — the 16-peripheral gap (25 % of the die)

`cec`, `comp1`–`comp3`, `crs`, `dbgmcu`, `lptim1`, `lptim2`, `syscfg`, `tamp`, `ucpd1`, `ucpd2`,
`usb`, `usbram`, `vrefbuf`, `vrefintcal`.

They are admitted by the database with their base address and clock gate, and marked
`uncurated: true`: codegen emits no descriptor, so nothing can accidentally depend on them.
This list *is* the work queue — and it is the fastest-moving list on this page. Twelve names that
were on it a week ago (the CRC unit, both LPUARTs, the whole `tim1`/`tim6`/`tim7`/`tim14`–`tim17`
set, the device UID and the window watchdog) are now curated **and** have drivers.

### Curated but unmodelled in Renode — 26 of the 49

The second, quieter gap: data, mostly drivers, and no emulation that can falsify them. It is
large and it has a shape — the entire ST timer family, plus CRC, DAC, RTC, both LPUARTs, the UID,
FDCAN and the G0 flash controller. That set is most of what a motor-control application touches,
which is why [What is proven, and how](proof.md) marks those capabilities host-tested or
compiles-only rather than emulated.

The G0 flash controller is unmodelled **on purpose** — its legs pass on direct memory writes, and
inventing a model would make them pass for the wrong reason.
