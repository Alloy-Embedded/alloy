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

### Requirement: Public HAL Shall Expose Ergonomic Selected-Device Aliases

The public HAL SHALL expose concise selected-device aliases for normal user code while preserving
the canonical generated ids behind the runtime boundary.

#### Scenario: User opens a GPIO pin through the public HAL

- **WHEN** a user writes normal application code against the public HAL
- **THEN** they can name pins and peripherals through concise public aliases such as
  `alloy::dev::pin::PA5` and `alloy::dev::periph::USART2`
- **AND** they do not need to spell `alloy::device::PinId::*` or
  `alloy::device::PeripheralId::*` as the taught first-choice syntax

### Requirement: Public HAL Shall Prefer Role-First Route Syntax

The public HAL SHALL prefer concise, role-first route syntax for UART, SPI, I2C, and similar
peripherals rather than teaching the raw connector kernel as normal usage.

#### Scenario: User connects a UART through the public HAL

- **WHEN** a user defines a UART route on the public HAL surface
- **THEN** they can express it through a route type such as `uart::route<...>` with role aliases
  like `tx<...>` and `rx<...>`
- **AND** the runtime keeps `connection::connector<...>` as an internal or expert-level kernel
  rather than the primary taught API shape

### Requirement: Public HAL Shall Preserve Expert Signal Control For Ambiguous Cases

The ergonomic public layer SHALL preserve an explicit expert escape hatch when a role/pin choice is
not uniquely resolvable from the published semantic traits.

#### Scenario: User configures a route with an ambiguous signal role

- **WHEN** a selected pin and peripheral support more than one plausible signal mapping for the
  same public role
- **THEN** the compile-time path reports the ambiguity and directs the user to the published
  connector guidance exposed by `alloyctl explain`
- **AND** the user can still spell an explicit expert-level signal choice through a concise public
  expert alias or equivalent canonical override

### Requirement: Public HAL Shall Avoid Teaching Structural Wrapper Noise

The public HAL SHALL avoid teaching structural wrapper types that only expose internal runtime
shape without adding user-facing hardware meaning.

#### Scenario: User reads a GPIO example

- **WHEN** a user reads an official raw HAL example for GPIO
- **THEN** the example prefers user-facing aliases instead of teaching
  `device::pin<>` and `gpio::pin_handle<...>` as the normal first syntax
- **AND** the underlying runtime still resolves to the same descriptor-driven implementation

