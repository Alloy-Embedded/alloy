## ADDED Requirements

### Requirement: The Async Model Shall Prove One Canonical Completion Path

The runtime async model SHALL prove at least one canonical completion-driven driver path in the
official examples and validation ladder.

#### Scenario: User follows the recommended async path

- **WHEN** a user follows the official async/event documentation
- **THEN** they can use one validated completion-driven driver path with timeout support
- **AND** they do not need handwritten interrupt glue as the normal story

### Requirement: Low Power Coordination Shall Be Observable In Validation

Low-power coordination with time and wake-capable events SHALL be backed by observable validation.

#### Scenario: Supported wake-capable wait path

- **WHEN** the repo claims a supported low-power + wake interaction
- **THEN** that interaction is backed by host, emulation, or hardware evidence
- **AND** not only by API presence
