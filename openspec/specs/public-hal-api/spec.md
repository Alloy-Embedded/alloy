# public-hal-api Specification

## Purpose
TBD - created by archiving change consume-runtime-lite-device-contract. Update Purpose after archive.
## Requirements
### Requirement: Public HAL Shall Hide Compile-Time Runtime-Lite Machinery

The public HAL SHALL remain one coherent API per peripheral class even when the implementation
uses template specializations and compile-time route packs from the runtime-lite contract.

#### Scenario: User opens SPI through the public HAL

- **WHEN** application code opens an SPI peripheral through the normal public HAL
- **THEN** the call site remains on the single public API path
- **AND** the compile-time machinery stays behind that API instead of becoming a second public
  usage tier

