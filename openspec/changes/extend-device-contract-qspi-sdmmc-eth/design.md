# Design: Extend Device Contract — QSPI, SDMMC, ETH

## Context

`device/runtime.hpp` is the single file that exposes alloy-devices
generated artifacts to the rest of the runtime. Every peripheral class
follows the same pattern:

```
ALLOY_DEVICE_<CLASS>_SEMANTICS_AVAILABLE
  → template alias <Class>SemanticTraits<Id>
  → span kinder   k<Class>SemanticPeripherals
  → const-ref alias <class>_semantic_peripheral_ids
```

Twelve classes exist today. Three more (QSPI, SDMMC, ETH) are
published by alloy-devices but never wired. The HAL proposals for
those three peripheral classes reference the traits as a given and
therefore fail to compile today.

## Goals

1. Wire QSPI, SDMMC, ETH traits into `runtime.hpp` using exactly
   the pattern of the twelve existing classes.
2. Keep the change mechanical and reviewable in < 30 min. No
   architecture decisions; this is bookkeeping.
3. Ship three `static_assert`-only compile tests confirming the
   binding is correct on representative boards before landing.

## Non-Goals

- Implementing any HAL method. The handle implementations are in
  `extend-qspi/sdmmc/eth-coverage`.
- Modifying alloy-devices. The generated artifacts already exist.
- Wiring USB. `add-usb-hal` is backend-static and explicitly does
  not need descriptor-driven semantic traits.

## Key Decisions

### Decision 1: Mirror the exact pattern of existing classes

The twelve existing classes in `runtime.hpp` use the same three-line
block (template alias, `constexpr auto` span, `constexpr const auto&`
direct-ref). The new three blocks are identical in structure. No new
mechanism.

Uniformity outweighs any savings from a helper macro — the file is
already repetitive by design (each class is gated and named
independently so `grep` on `QspiSemanticTraits` finds exactly one
definition).

### Decision 2: Compile tests over `device/runtime.hpp` directly

The compile tests instantiate `device::QspiSemanticTraits<Id>` (the
outer-namespace alias that `runtime.hpp` exports) rather than
`selected::runtime_driver_contract::QspiSemanticTraits<Id>`. This
checks the full forwarding chain — if the binding has a typo the
test fails.

Board choice:
- QSPI → `nucleo_f401re` (STM32F4 has QSPI; G0 and SAME70 do not
  in the current foundational set).
- SDMMC → `same70_xplained` (HSMCI; F4 SDIO is a different class
  and is not in the foundational set).
- ETH → `same70_xplained` (GMAC; no other foundational board has
  Ethernet).

### Decision 3: No macro for the repeated pattern

A helper macro (`ALLOY_WIRE_SEMANTIC_CLASS(Qspi, QSPI)`) would DRY
the six-line block but would make the file harder to grep and diff.
The file is generated-artifact glue, not application logic; clarity
over DRY.

## Dependency Graph

```
extend-device-contract-qspi-sdmmc-eth  ← this change
    ├── extend-qspi-coverage    (blocked on QspiSemanticTraits)
    ├── extend-sdmmc-coverage   (blocked on SdmmcSemanticTraits)
    └── extend-eth-coverage     (blocked on EthSemanticTraits)
```

All three downstream changes can land in any order once this lands.

## Risks and Mitigations

**Risk**: alloy-devices does not actually publish QSPI/SDMMC/ETH
traits for any currently-foundational board.
**Mitigation**: the compile tests run against `nucleo_f401re` (QSPI)
and `same70_xplained` (SDMMC, ETH) which are in the current 5-board
foundational matrix. If the guard macro is `0` for all boards the
test fails fast and the issue is upstream in alloy-devices.

**Risk**: the macro names (`ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE` etc.)
differ from what alloy-devices actually emits.
**Mitigation**: the compile test files `#include "device/runtime.hpp"`
and immediately `static_assert(ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE)`
(or `static_assert(!...)` if intentionally absent for a board).
Either way the name mismatch is caught at compile time, not by reading
generated headers.
