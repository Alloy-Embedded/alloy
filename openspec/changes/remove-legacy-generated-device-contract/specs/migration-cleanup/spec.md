## ADDED Requirements

### Requirement: Legacy generated C++ header usage is removed from Alloy

`alloy` MUST remove remaining dependence on legacy generated C++ headers after the runtime contract
becomes the sole supported published contract.

#### Scenario: Host validation overrides use runtime startup contract
- **WHEN** host MMIO or other selected-config overrides are generated
- **THEN** they include the typed runtime startup contract
- **AND** they do not include `startup_descriptors.hpp`

#### Scenario: Internal naming no longer implies a second public contract
- **WHEN** docs, selected import macros, or internal helper namespaces refer to the generated
  device contract
- **THEN** they describe it as the runtime contract
- **AND** they do not imply support for a second heavier published C++ contract
