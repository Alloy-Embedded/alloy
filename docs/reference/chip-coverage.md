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

## Baseline — STM32G0B1RE, 2026-08-09

Taken at `alloy` 1568006 with `alloy-devices` 9949429 (declared 0.3.0,
digest `sha256:ab31e6be…`), the commit that graduated the hand-written
`stm32g0b1.yaml` (23 peripherals) into the builder-generated `stm32g0b1re.yaml`.

**37 of 65 peripherals curated, 31 with drivers**, 23 with a Renode model,
13 reachable from a board role (`nucleo_g0b1re`). By IP: 18 curated IPs, 13 with drivers.

The jump from 23 to 65 peripherals is not new support — it is the **gap becoming visible**.
Every peripheral the silicon has now appears, and the ones alloy cannot yet touch say so.

### Curated, with a driver (31)

`adc1`, `dac1`, `dma1`, `dma2`, `exti`, `fdcan1`, `fdcan2`, `flash`,
`gpioa`–`gpiof`, `i2c1`–`i2c3`, `iwdg`, `rtc`, `spi1`–`spi3`, `tim2`, `tim3`, `tim4`,
`usart1`–`usart6`.

### Curated, no driver (6)

`adc1_common`, `dmamux1`, `fdcanram1`, `fdcanram2`, `pwr`, `rcc`.

These are the register-only blocks: they exist so other code can address them (the clock
program writes `rcc`, the FDCAN driver addresses its message RAM), and a standalone driver
would have nothing to drive.

### Uncurated — the 28-peripheral gap (43 % of the die)

`cec`, `comp1`–`comp3`, `crc`, `crs`, `dbgmcu`, `lptim1`, `lptim2`, `lpuart1`, `lpuart2`,
`syscfg`, `tamp`, `tim1`, `tim6`, `tim7`, `tim14`–`tim17`, `ucpd1`, `ucpd2`, `uid`, `usb`,
`usbram`, `vrefbuf`, `vrefintcal`, `wwdg`.

They are admitted by the database with their base address and clock gate, and marked
`uncurated: true`: codegen emits no descriptor, so nothing can accidentally depend on them.
This list *is* the work queue.

### Curated but unmodelled in Renode (14)

`adc1_common`, `dac1`, `dmamux1`, `fdcan1`, `fdcan2`, `fdcanram1`, `fdcanram2`, `flash`, `pwr`,
`rcc`, `rtc`, `tim2`, `tim3`, `tim4`.

A second, quieter gap: these have data and (mostly) drivers, but no emulation can falsify them.
The G0 flash controller is unmodelled on purpose — its legs pass on direct memory writes.
