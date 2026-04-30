# Proposal: Open-Source the Codegen Pipeline

## Status
`open` — strategic priority #1

## Problem

`alloy-codegen` is a private tool. The `alloy-devices` repo contains only
`artifact-manifest.json` files — no generated C++ headers. Consequences:

- Every new MCU family requires maintainer intervention.
- Community contributors cannot add devices independently.
- Downstream projects cannot verify or audit the generated contract.
- The local build is broken without an out-of-band delivery of generated headers.
- Alloy cannot scale to 5 000+ MCUs without this being resolved first.

Embassy (Rust) solved this with `chiptool` (public) + `embassy-stm32` generated
from public SVD. Zephyr solved it with public DTS bindings. Alloy must do the same.

## Proposed Architecture

The pipeline splits into three independent, separately-publishable tools:

```
SVD / ATDF / YAML patches
         │
         ▼
  alloy-svd-ingest          ← Phase 1: parse vendor sources → Alloy IR
         │  alloy-ir.json (schema v1.2.0)
         ▼
  alloy-ir-validate         ← Phase 2: schema check + coverage report
         │  validation-report.json
         ▼
  alloy-cpp-emit            ← Phase 3: IR → generated/ C++ header tree
         │
         ▼
  alloy-devices/<vendor>/<family>/generated/
```

Each tool is a standalone Python package installable via pip.
The full pipeline is composed by `alloy-codegen`, a thin orchestrator.

### alloy-svd-ingest

Input:
- CMSIS SVD (ST, Nordic, NXP, Microchip ARM) at a URL or local path
- ATDF (Microchip AVR/PIC) at a URL or local path
- ESP-IDF register header scraper (Espressif, no standard SVD)
- YAML patch files (`patches/<vendor>/<family>.yaml`) for corrections and extensions

Output: `alloy-ir.json` — vendor-neutral intermediate representation:

```json
{
  "ir_schema_version": "1.2.0",
  "vendor": "st",
  "family": "stm32g0",
  "device": "stm32g071rb",
  "peripherals": [
    {
      "id": "USART2",
      "base_address": "0x40004400",
      "registers": [...],
      "fields": [...],
      "signals": [...],
      "irq_numbers": [28]
    }
  ],
  "pins": [...],
  "routes": [...],
  "clock_tree": {...},
  "dma_mux": {...}
}
```

### alloy-ir-validate

Input: `alloy-ir.json`
Output: `validation-report.json` with:
- Schema conformance errors
- Missing required fields (per HAL module contract)
- Coverage percentage per peripheral class
- Diff against previous version (breaking changes)

```json
{
  "device": "stm32g071rb",
  "coverage": {
    "uart": {"required": 18, "present": 16, "pct": 88},
    "spi":  {"required": 12, "present": 12, "pct": 100},
    "gpio": {"required": 9,  "present": 9,  "pct": 100},
    "usb":  {"required": 22, "present": 14, "pct": 63}
  },
  "errors": [],
  "warnings": ["uart.kKernelClockSelectorField missing — HAL method will return NotSupported"]
}
```

### alloy-cpp-emit

Input: `alloy-ir.json` (validated)
Output: The full `generated/runtime/` tree matching the current alloy-devices layout.

Templates (Jinja2) for each generated file:
- `types.hpp.j2` → `PeripheralId`, `PinId`, `RegisterId`, `FieldId`, `SignalId` enums
- `register_fields.hpp.j2` → `RegisterFieldTraits<Id>` with constexpr field refs
- `driver_semantics/uart.hpp.j2` → `UartSemanticTraits<PeripheralId>` full struct
- etc. (one template per generated file type)

Templates are checked into the public repo and auditable.

### YAML patch format

```yaml
# patches/st/stm32g0.yaml
version: "1.0"
target: "stm32g0"
patches:
  - op: add_field
    peripheral: USART2
    register: CR3
    field:
      name: UCESM
      bit_offset: 23
      bit_width: 1
      description: "USART Clock Enable in Stop mode"
  - op: set_irq
    peripheral: USART2
    irq_numbers: [28]
  - op: add_semantic
    peripheral: USART2
    key: kKernelClockSelectorField
    register: RCC_CCIPR
    field: USART2SEL
```

Patches are the community's escape hatch for vendor SVD errors and for adding
semantic annotations that are not present in raw SVD.

## Field Contract (IR → HAL requirement mapping)

Each HAL module publishes a JSON contract listing required and optional fields:

```json
// src/hal/uart/contract.json
{
  "module": "uart",
  "required_fields": ["kUeField", "kTeField", "kReField", "kBrrField", "kTxeField", "kRxneField"],
  "optional_fields": ["kKernelClockSelectorField", "kFifoModeField", "kLinEnField", "kHdselField"]
}
```

`alloy-ir-validate` reads these contracts and reports coverage.

## Migration path

1. Tag current alloy-devices layout as `artifact_layout_version: alloy-devices-v1`.
2. Regenerate all existing devices through the new open pipeline.
3. Verify generated output matches current checked-in headers (diff test).
4. Open-source all three tools and the YAML patches.
5. Document contribution guide: "How to add a new MCU family in 4 steps."

## Non-goals

- This spec does not change the generated C++ API surface (no breaking changes to alloy runtime).
- This spec does not cover the package registry (see `alloy-devices-package-registry`).
- alloy-codegen does not need to be fast; correctness and auditability are priorities.
