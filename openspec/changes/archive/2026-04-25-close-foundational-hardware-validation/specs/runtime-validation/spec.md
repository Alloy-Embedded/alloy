## ADDED Requirements

### Requirement: Foundational Boards Shall Carry Recent Hardware Evidence

Boards claimed as foundational SHALL carry explicit and recent hardware validation evidence in the
repo.

#### Scenario: Checking a foundational board claim

- **WHEN** a board is listed as foundational in active support docs
- **THEN** the repo contains a maintained hardware checklist or equivalent run record for that board
- **AND** the evidence is not left implicit in chat history or private notes

### Requirement: Hardware Findings Shall Feed Faster Validation Layers

When hardware real-silicon validation discovers a runtime bug, the repo SHALL add a faster
validation layer for that behavior where practical.

#### Scenario: Hardware-only runtime failure

- **WHEN** a bug is first discovered on a foundational board
- **THEN** the fix also adds host-MMIO, emulation, or scripted validation coverage where possible
- **AND** the checklist records the silicon result
