# Expand Chip Coverage — Open Codegen + 20+ MCU Families

## Why

Alloy v0.1.0 supports six MCU families (STM32F4, STM32G0, SAME70, ESP32, ESP32-C3,
RP2040). modm supports 300+, Zephyr 500+. Coverage is the primary reason embedded
developers do not adopt a HAL — the framework may be architecturally superior but
unusable if their chip is not there.

The current `alloy-devices` artifact (pinned at `fa0630fe`) is the correct architectural
answer: peripheral topology, register layouts, clocks, DMA bindings, and IRQ routing are
generated externally and consumed by the core. The problem is that this generator is not
public. Without a public code-generation tool, the community cannot contribute new chips
and the project will never reach the coverage needed for broad adoption.

This change does three things:

1. **Opens the `alloy-devices` codegen** — publish the spec and tooling so contributors
   can generate device artifacts for any chip from vendor SVD / header files.
2. **Adds 20+ MCU families** via batch-generated artifacts — covering the most popular
   ARM Cortex-M and RISC-V families in use today.
3. **Adds nRF52 support** — the dominant BLE platform, missing from every C++ HAL.

## What Changes

### Public codegen spec (`tools/alloy-codegen/`)

A documented Python tool (`alloy-codegen`) that ingests:
- CMSIS SVD files (vendor-provided, most ARM Cortex-M chips)
- ATDF files (Microchip)
- ESP-IDF register headers (Espressif)
- Hand-authored YAML overlays (for semantic data SVD does not encode: DMA request IDs,
  pin alternate functions, clock domains)

Output is an `alloy-devices` artifact: a directory of C++ headers following the existing
contract (`alloy/device/semantics.hpp`, `alloy/device/descriptors.hpp`,
`alloy/device/bindings.hpp`). The tool is MIT-licensed and lives in this repo.

A `CONTRIBUTING_DEVICES.md` guide describes the full pipeline: acquire SVD → run
`alloy-codegen generate` → run `alloy validate` smoke checks → open PR.

### New MCU families (first batch)

Priority determined by Hackster/embedded-survey popularity and existing SVD availability.
Each family gets: `cmake/platforms/<family>.cmake`, toolchain reference, one reference
board bring-up, and at least one compile-review preset.

| Family | Chips | Notes |
|--------|-------|-------|
| STM32H7 | H723, H743, H750 | Cortex-M7, most popular high-perf STM32 |
| STM32L4 | L432, L452, L476 | Ultra-low-power, very common in IoT |
| STM32U5 | U575, U585 | ARM TrustZone, newest STM32 series |
| STM32WB | WB55 | BLE + Cortex-M4/M0+ dual core |
| nRF52840 | nRF52840 | Nordic Semiconductor BLE flagship |
| nRF52833 | nRF52833 | nRF52 mid-range |
| nRF9160 | nRF9160 | LTE-M / NB-IoT, cellular IoT |
| RP2350 | RP2350 | Raspberry Pi Pico 2 (Cortex-M33 + RISC-V) |
| GD32F450 | GD32F450 | Chinese STM32-compatible, widely deployed |
| CH32V307 | CH32V307 | WCH RISC-V, growing embedded community |
| SAMD21 | SAMD21G18 | Microchip Cortex-M0+, Arduino MKR base |
| SAMD51 | SAMD51J20 | Microchip Cortex-M4F, Adafruit Feather M4 |
| LPC55S69 | LPC55S69 | NXP Cortex-M33 with TrustZone |
| MIMXRT1062 | RT1062 | NXP i.MX RT, Teensy 4.x |
| PY32F003 | PY32F003 | Puya cheapest ARM MCU on market |
| ESP32-S3 | ESP32-S3 | Dual Xtensa LX7, AI/ML focus |
| ESP32-H2 | ESP32-H2 | RISC-V, IEEE 802.15.4 (Thread/Zigbee) |
| K22F | MK22F512 | NXP Kinetis, industrial IoT |
| CY8C6347 | PSoC 6 | Infineon dual-core M0+/M4 |
| TM4C123 | TM4C123GH6PM | Texas Instruments Tiva-C, educational |

### Reference board additions (one per new family)

One board per new family with `board.hpp` and a working blink example as baseline
validation. Hardware validation is deferred to a follow-up; compile-review is the
acceptance gate for the initial batch.

### `alloyctl doctor` extension

`alloy doctor` gains a `--check devices` subcommand that queries the pinned
`alloy-devices` artifact version and lists which families are available locally vs.
available upstream.

## What Does NOT Change

- The existing `alloy-devices` contract schema — new families add artifacts; they do not
  change the contract headers already consumed by the HAL.
- The six existing foundational boards — no regression on validated platforms.
- The `alloy-codegen` tool is **additive** to this repo; it does not replace the
  hand-authored board files for existing platforms.

## Alternatives Considered

**Adopt Zephyr devicetree directly:** Zephyr's DT is well-established but its schema is
entangled with Zephyr RTOS semantics (chosen drivers, Kconfig). Adopting it would couple
Alloy to Zephyr concepts it explicitly avoids (runtime polymorphism, Kconfig, C headers).

**Hand-author all 20 families:** Accurate but unscalable — the SAME70 device artifact
took significant engineering time. Codegen is the only path to broad coverage.

**Use CMSIS-SVD directly at runtime:** SVD is XML; parsing it at runtime adds a host
tooling dependency and is not embeddable. Code generation keeps the MCU firmware
zero-dependency.
