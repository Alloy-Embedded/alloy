## ADDED Requirements

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
