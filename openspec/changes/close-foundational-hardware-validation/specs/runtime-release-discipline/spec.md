## ADDED Requirements

### Requirement: Foundational Board Claims Shall Be Silicon-Backed

Release support claims for foundational boards SHALL be backed by current silicon evidence and not
only host or emulation gates.

#### Scenario: Publishing a foundational board in the support matrix

- **WHEN** a board is declared foundational in release docs
- **THEN** its claim is backed by the declared automated gates plus maintained hardware evidence
- **AND** the claim is downgraded if that evidence is missing or stale
