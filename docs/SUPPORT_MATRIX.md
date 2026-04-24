# Alloy Support Matrix

This file publishes the active support tiers for boards and peripheral classes.

Machine-readable source of truth:

- [docs/RELEASE_MANIFEST.json](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_MANIFEST.json)

## Tier Meanings

- `foundational`: included in the active release-validation ladder and eligible for release claims
- `representative`: supported on the runtime path with targeted validation, but not claimed as equally covered as the foundational set
- `experimental`: buildable or partially validated, but not part of the active release contract
- `deprecated`: still documented for migration, not expanded further, and scheduled for removal

## Board Tiers

| Board | Tier | Evidence for claim | Release notes |
| --- | --- | --- | --- |
| `same70_xplained` | `foundational` | descriptor smoke, host-MMIO, Renode runtime validation, SAME70 zero-overhead gate, canonical examples | Primary Microchip release board |
| `nucleo_g071rb` | `foundational` | descriptor smoke, host-MMIO, Renode runtime validation, canonical examples | Primary STM32G0 release board |
| `nucleo_f401re` | `foundational` | descriptor smoke, host-MMIO, Renode runtime validation, canonical examples | Primary STM32F4 release board |
| `nucleo_g0b1re` | `experimental` | embedded build coverage for `blink` only | Not part of the foundational release ladder |

Notes:

- `host` is a validation target, not a shipping board tier
- additional board packages may exist in `boards/`, but they do not inherit a release claim until they appear here and in the release manifest

## Peripheral Class Tiers

| Peripheral class | Tier | Current evidence |
| --- | --- | --- |
| `runtime-device-boundary` | `foundational` | boundary script + descriptor contract smoke |
| `clock-reset-startup` | `foundational` | descriptor smoke + host-MMIO + Renode runtime validation |
| `gpio` | `foundational` | descriptor smoke + host-MMIO + Renode runtime validation |
| `uart` | `foundational` | descriptor smoke + host-MMIO + Renode runtime validation |
| `time` | `foundational` | descriptor smoke + canonical `time_probe` coverage |
| `interrupt-event` | `foundational` | descriptor smoke + host-MMIO IRQ bridging coverage |
| `dma` | `foundational` | descriptor smoke + host-MMIO DMA coverage + canonical `dma_probe` |
| `i2c` | `representative` | descriptor smoke + SAME70 hardware spot-check (`i2c_scan`) |
| `spi` | `representative` | descriptor smoke + SAME70 hardware spot-check (`spi_probe`) |
| `timer` | `representative` | descriptor smoke + SAME70 hardware spot-check (`timer_pwm_probe`) + STM32F4/G0 hardware spot-check |
| `pwm` | `representative` | descriptor smoke + SAME70 zero-overhead gate + SAME70 hardware spot-check (`timer_pwm_probe`) |
| `rtc` | `representative` | descriptor smoke + host-MMIO SAME70 path + SAME70/STM32F4/G0 hardware spot-check (`rtc_probe`) |
| `watchdog` | `representative` | descriptor smoke + host-MMIO SAME70 path + SAME70/STM32F4/G0 hardware spot-check (`watchdog_probe`) |
| `adc` | `representative` | descriptor smoke + SAME70/STM32F4/G0 hardware spot-check (`analog_probe`) |
| `dac` | `representative` | descriptor smoke + SAME70 hardware spot-check (`analog_probe` DAC-active banner) |
| `low-power` | `representative` | descriptor smoke + isolated SAME70 host-MMIO wakeup path |
| `can` | `experimental` | descriptor smoke + SAME70 bring-up spot-check (`can_probe` boot/loop banner); no deterministic traffic assertion yet |

## Maintenance Rules

- support claims must follow validation reality, not implementation intent
- any tier change must update both this file and [docs/RELEASE_MANIFEST.json](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_MANIFEST.json)
- a board or peripheral class cannot move to `foundational` unless the declared release gates are already part of CI or an explicitly documented release runbook
- if a release gate is removed or weakened, the affected support claim must be downgraded in the same change