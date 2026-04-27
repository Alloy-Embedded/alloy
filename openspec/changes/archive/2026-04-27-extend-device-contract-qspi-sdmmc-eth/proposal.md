# Extend Device Contract: Wire QSPI, SDMMC, and ETH Semantic Traits

## Why

`alloy-devices` already emits `QspiSemanticTraits<P>`,
`SdmmcSemanticTraits<P>`, and `EthSemanticTraits<P>` for every
device whose descriptor declares those peripherals. Alloy's
`device/runtime.hpp` consumes twelve peripheral classes today
(GPIO, UART, SPI, I2C, DMA, ADC, DAC, CAN, RTC, Watchdog, Timer,
PWM) following a uniform pattern:

```cpp
#if ALLOY_DEVICE_<CLASS>_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using <Class>SemanticTraits = selected::runtime_driver_contract::<Class>SemanticTraits<Id>;
inline constexpr auto <class>_semantic_peripherals =
    std::span{selected::runtime_driver_contract::k<Class>SemanticPeripherals};
#endif
```

QSPI, SDMMC, and ETH follow the same codegen shape but have no
entries in `runtime.hpp`. The three pending HAL proposals
(`extend-qspi-coverage`, `extend-sdmmc-coverage`,
`extend-eth-coverage`) each open with "alloy-devices publishes
`<Class>SemanticTraits<P>`" as a given — meaning they cannot compile
today because the alloy side never exposes the traits.

This change wires the three missing peripheral classes into the
device contract, using exactly the same guards and accessors the
other twelve use. No alloy-devices changes required — the generated
artifacts are already there.

## What Changes

### `src/device/runtime.hpp` — three new semantic trait bindings

Following the existing pattern for ADC/DAC/CAN/RTC/Watchdog/Timer/PWM:

**QSPI** (gated on `ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE`):

```cpp
#if ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using QspiSemanticTraits = selected::runtime_driver_contract::QspiSemanticTraits<Id>;

inline constexpr auto qspi_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kQspiSemanticPeripherals};
inline constexpr const auto& qspi_semantic_peripheral_ids =
    selected::runtime_driver_contract::kQspiSemanticPeripherals;
#endif
```

**SDMMC** (gated on `ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE`):

```cpp
#if ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using SdmmcSemanticTraits = selected::runtime_driver_contract::SdmmcSemanticTraits<Id>;

inline constexpr auto sdmmc_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kSdmmcSemanticPeripherals};
inline constexpr const auto& sdmmc_semantic_peripheral_ids =
    selected::runtime_driver_contract::kSdmmcSemanticPeripherals;
#endif
```

**ETH** (gated on `ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE`):

```cpp
#if ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using EthSemanticTraits = selected::runtime_driver_contract::EthSemanticTraits<Id>;

inline constexpr auto eth_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kEthSemanticPeripherals};
inline constexpr const auto& eth_semantic_peripheral_ids =
    selected::runtime_driver_contract::kEthSemanticPeripherals;
#endif
```

The same three alias + span blocks are mirrored in the outer
`alloy::device` namespace (below the `runtime::` namespace, where
the existing twelve classes already appear).

### `src/device/selected_config.hpp` awareness

The three new `ALLOY_DEVICE_*_SEMANTICS_AVAILABLE` macros follow the
same convention as `ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE` etc. —
they are defined (to `1`) by the generated
`alloy/device/selected_config.hpp` when the target device declares
the respective peripheral class. Alloy itself needs no change to
`selected_config.hpp`; it reads the macro and conditionally includes
the generated driver semantics header via:

```
generated/runtime/devices/driver_semantics/qspi.hpp
generated/runtime/devices/driver_semantics/sdmmc.hpp
generated/runtime/devices/driver_semantics/ethernet.hpp
```

These files are already emitted by alloy-devices for every board
whose descriptor declares the peripheral class.

### Compile tests

Three new files under `tests/compile_tests/`:

- `test_qspi_device_contract.cpp` — confirms
  `ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE` is truthy for
  `nucleo_f401re` (only F4 declares QSPI among the foundational
  set); instantiates `QspiSemanticTraits<F4_QSPI_ID>` and checks
  a representative field (`kInstructionField.valid == true`).
- `test_sdmmc_device_contract.cpp` — same pattern for
  `same70_xplained` HSMCI; checks `kBusWidthField.valid`.
- `test_eth_device_contract.cpp` — same for `same70_xplained`
  GMAC; checks `kFullDuplexField.valid` and `kSpeedField.valid`.

Each test is a `static_assert`-only TU — zero runtime binary cost,
confirmed green in CI without hardware.

## What Does NOT Change

- The generated artifacts in alloy-devices are **not modified**.
  This change is pure alloy-side plumbing.
- USB is intentionally excluded: the `add-usb-hal` proposal
  explicitly states that the USB device controller does not require
  descriptor-driven semantic traits — the USB backend is
  statically configured per controller, not driven by the device
  contract.
- The twelve existing semantic trait bindings (GPIO, UART, SPI,
  I2C, DMA, ADC, DAC, CAN, RTC, Watchdog, Timer, PWM) are
  unchanged. This change is purely additive.
- No HAL implementation lands here — the three new peripheral
  handles (`src/hal/qspi/qspi.hpp`, `src/hal/sdmmc/sdmmc.hpp`,
  `src/hal/ethernet/ethernet_port.hpp`) are delivered by
  `extend-qspi-coverage`, `extend-sdmmc-coverage`, and
  `extend-eth-coverage` respectively. This change is the
  prerequisite that makes those proposals buildable.

## Out of Scope (Follow-Up Changes)

- **OctoSPI / HSPI (STM32H7).** A separate `kOctoSpiSemanticPeripherals`
  binding follows once an H7 board enters the foundational set.
  Tracked as `add-octospi-device-binding`.
- **eMMC / SDIO.** The SDMMC binding covers SD cards and eMMC in
  native mode; SDIO (Wi-Fi / BT modules) requires a separate
  descriptor class. Tracked as `add-sdio-device-binding`.
- **GMII / RGMII (1 Gbit/s).** The ETH binding covers 10/100 Mbit/s
  MII / RMII; gigabit descriptors are a follow-up once a GbE-capable
  device enters the matrix. Tracked as `add-eth-gbe-device-binding`.
- **ESP32 / RP2040 / AVR-DA QSPI / SDMMC / ETH parity.** Each
  requires an alloy-devices schema addition; tracked in the
  `expand-chip-coverage` family.

## Alternatives Considered

**Land the HAL and the device contract binding in one change.**
Rejected — the three HAL proposals (`extend-qspi/sdmmc/eth-coverage`)
are already written and complete. Merging this binding work into one
of them creates a large, hard-to-review change; it also creates an
ordering dependency between the three HAL proposals. Landing the
device contract binding first is a small, reviewable prerequisite
that unblocks all three HAL changes independently.

**Generate a single `kAllSemanticPeripherals` catch-all table.**
Rejected — the per-class tables let the HAL discover peripherals
without scanning the full peripheral list (which is potentially
hundreds of entries on large devices). The existing twelve classes
all use the per-class table; uniformity is preferred over a
monolithic accessor.
