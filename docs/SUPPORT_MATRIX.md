# Alloy Support Matrix

This file publishes the active support tiers for boards and peripheral classes.

Machine-readable source of truth:

- [docs/RELEASE_MANIFEST.json](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_MANIFEST.json)

## Tier Meanings

- `foundational`: included in the active release-validation ladder and eligible for release claims
- `representative`: supported on the runtime path with targeted validation, but not claimed as equally covered as the foundational set
- `compile-only`: buildable through the runtime CMake path; no hardware validation claim. Useful as a CLI scaffolding target (`alloy new --board <name>`) while bring-up runbooks are written
- `experimental`: buildable or partially validated, but not part of the active release contract
- `deprecated`: still documented for migration, not expanded further, and scheduled for removal

## Board Tiers

| Board | Tier | Evidence for claim | Release notes |
| --- | --- | --- | --- |
| `same70_xplained` | `foundational` | descriptor smoke, host-MMIO, Renode runtime validation, SAME70 zero-overhead gate, canonical examples | Primary Microchip release board |
| `nucleo_g071rb` | `foundational` | descriptor smoke, host-MMIO, Renode runtime validation, canonical examples | Primary STM32G0 release board |
| `nucleo_f401re` | `foundational` | descriptor smoke, host-MMIO, Renode runtime validation, canonical examples | Primary STM32F4 release board |
| `nucleo_g0b1re` | `experimental` | embedded build coverage for `blink` only | Not part of the foundational release ladder |
| `esp32c3_devkitm` | `compile-only` | board files, descriptors, CMake presets, build CI; no hardware validation claim | Foundational compile-only target for `alloy new --mcu ESP32-C3`. See OpenSpec change `add-esp-toolchain-pins`. |
| `esp32s3_devkitc` | `compile-only` | board files, descriptors, CMake presets, build CI; no hardware validation claim | Foundational compile-only target for `alloy new --mcu ESP32-S3`. See OpenSpec change `add-esp-toolchain-pins`. |
| `esp32_devkit` | `compile-only` | bare-metal direct-boot startup, GPIO LED + UART0 raw HAL, esp32 LX6 descriptor consumed; single-core only (dual-core APP_CPU bring-up gated on `alloy-devices` publishing the secondary-core start facts). See OpenSpec change `add-esp32-classic-family`. |
| `esp_wrover_kit` | `compile-only` | shares the DevKitC v4 startup + LED/UART path; WROVER-specific peripherals (PSRAM, ILI9341 LCD, microSD, full RGB, FT2232HL JTAG) are follow-ups. |

Notes:

- `host` is a validation target, not a shipping board tier
- additional board packages may exist in `boards/`, but they do not inherit a release claim until they appear here and in the release manifest

## CLI Scaffolding Coverage

`alloy new` supports two project shapes:

- **Foundational board copy** (`alloy new --board <name>` or `alloy new --mcu <part>` when
  the catalog matches): copies an in-tree board into `<project>/board/`. Available for
  every entry in the table above plus the experimental boards still cadastered in
  [`tools/alloy-cli/src/alloy_cli/_boards.toml`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tools/alloy-cli/src/alloy_cli/_boards.toml).
- **Custom-board skeleton** (`alloy new --mcu <part>` without a catalog match): generates
  a board skeleton from the `alloy-devices` descriptor. Available for any vendor/family/
  device tuple that ships a descriptor in the active SDK; the foundational descriptor set
  is whichever `alloy-devices` ref is pinned for the active runtime release.

The scaffolder never invents a board: if no descriptor matches `--mcu`, it fails with a
pointer to `alloy-devices` so the missing facts can be added there first. See
[CUSTOM_BOARDS.md](CUSTOM_BOARDS.md) for the runtime contract every scaffolded project
relies on.

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
| `timer` | `foundational` | descriptor smoke + host-MMIO (TC0 path) + 3-board hardware spot-check: SAME70/STM32G0/STM32F4 `timer_pwm_probe` all pass |
| `pwm` | `foundational` | descriptor smoke + host-MMIO (PWM0 path) + zero-overhead gate + 3-board hardware spot-check: SAME70/STM32G0/STM32F4 `timer_pwm_probe` all pass |
| `rtc` | `foundational` | descriptor smoke + host-MMIO (RTC path) + 3-board hardware spot-check: SAME70/STM32G0/STM32F4 `rtc_probe` all pass |
| `watchdog` | `foundational` | descriptor smoke + host-MMIO (WDT path) + 3-board hardware spot-check: SAME70/STM32G0/STM32F4 `watchdog_probe` all pass |
| `adc` | `foundational` | descriptor smoke + host-MMIO (AFEC path) + 3-board hardware spot-check: SAME70/STM32G0/STM32F4 `analog_probe` all pass |
| `dac` | `representative` | descriptor smoke + SAME70 hardware spot-check (`analog_probe` DAC-active banner) |
| `low-power` | `representative` | descriptor smoke + isolated SAME70 host-MMIO wakeup path |
| `can` | `experimental` | descriptor smoke + SAME70 bring-up spot-check (`can_probe` boot/loop banner); no deterministic traffic assertion yet |
| `modbus` | `representative` | host loopback tests pass (634 assertions / 113 cases): PDU codec, RTU framing, RS-485 DE, variable registry, slave (FC01–17), master scheduler, discovery FC 0x65, TCP MBAP framing. UART stream adapter and hardware spot-check pending. See [MODBUS.md](MODBUS.md). |
| `ethernet` | `representative` | concept checks pass (MdioBus, EthernetMac, NetworkInterface, PhyDriver, TcpStream/ByteStream); Same70Mdio + Same70Gmac + KSZ8081 wired via EthernetInterface; lwIP host loopback test (TcpStream round-trip). SAME70 Xplained hardware spot-check pending (`ethernet_basic` + `modbus_tcp_slave`). See [NETWORK.md](NETWORK.md). |
| `wifi` | `scaffolded` | W5500Interface and EspAtInterface satisfy NetworkInterface concept (compile checks pass); full SPI/UART driver implementation is deferred to a follow-on change. |
| `tasks` | `representative` | host suite (25 cases / 135 assertions) covers pool, Task spawn, priority + FIFO, delay/yield_now/on(event)/until(predicate), CancellationToken propagation, Channel SPSC + drops, UartRxChannel + UartTxChannel kick callback. Cross-compiles for nucleo_g071rb at 5132 B text. Hardware ISR validation pending. See [TASKS.md](TASKS.md). |

## Maintenance Rules

- support claims must follow validation reality, not implementation intent
- any tier change must update both this file and [docs/RELEASE_MANIFEST.json](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_MANIFEST.json)
- a board or peripheral class cannot move to `foundational` unless the declared release gates are already part of CI or an explicitly documented release runbook
- if a release gate is removed or weakened, the affected support claim must be downgraded in the same change