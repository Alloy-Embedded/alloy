# public-hal-api Specification

## Purpose
The public HAL is the single, coherent API surface that application code and board helpers use
to configure and operate peripherals.

The runtime exposes one primary API per peripheral class — UART, SPI, I2C, DMA, timer, PWM,
ADC, DAC, RTC, CAN, and watchdog — backed by generated device descriptors from `alloy-devices`.
Convenience wrappers and board helpers are thin shims over this API, not separate tiers.

Users should not need family-private glue, raw register sequences, or vendor-specific headers
to configure any foundational peripheral. The public API is the only supported path.
## Requirements
### Requirement: Public HAL Shall Hide Compile-Time Runtime-Lite Machinery

The public HAL SHALL remain one coherent API per peripheral class even when the implementation
uses template specializations and compile-time route packs from the runtime contract.

#### Scenario: User opens SPI through the public HAL

- **WHEN** application code opens an SPI peripheral through the normal public HAL
- **THEN** the call site remains on the single public API path
- **AND** the compile-time machinery stays behind that API instead of becoming a second public
  usage tier

### Requirement: The Public HAL Shall Expose One Primary API Per Peripheral Class

The public runtime SHALL expose one primary API per peripheral class.

Defaults, helper aliases, and convenience wrappers MAY exist, but they SHALL be thin helpers over
the same underlying API and SHALL NOT be presented as separate API families.

#### Scenario: UART configuration with defaults
- **WHEN** a user configures a UART
- **THEN** they use one primary UART API with a configuration object that provides defaults
- **AND** they do not need to choose between `simple`, `expert`, or `fluent` tiers

### Requirement: Public HAL APIs Shall Be Descriptor-Driven

The public HAL SHALL configure peripherals from generated device descriptors and runtime behavior,
not from handwritten vendor-specific public contracts.

#### Scenario: SPI open on foundational family
- **WHEN** the user opens an SPI peripheral on any foundational family
- **THEN** the same public API shape is used
- **AND** vendor-specific differences are resolved by descriptors and runtime internals

### Requirement: Resource Ownership Shall Be Explicit in the Runtime Model

The runtime SHALL implement explicit ownership or claim semantics for pins, peripheral instances,
DMA routes, and interrupt bindings.

#### Scenario: Double-use prevention
- **WHEN** two configurations attempt to claim the same incompatible resource
- **THEN** the runtime rejects the second configuration through compile-time or runtime validation

### Requirement: Blocking And Event-Driven Use Shall Share The Same Public HAL Shape

The public HAL SHALL keep one primary API shape even when a peripheral supports blocking,
event-driven, and async-backed operation.

#### Scenario: UART used in blocking and event-driven modes
- **WHEN** a UART is used first in blocking mode and later with event/completion handling
- **THEN** both uses remain on the same primary public HAL surface
- **AND** the user does not switch to a second family-specific API for event support

