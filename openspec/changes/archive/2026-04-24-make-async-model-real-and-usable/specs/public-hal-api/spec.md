## ADDED Requirements

### Requirement: Completion-Driven Paths Shall Stay On The Public HAL Surface

Interrupt-driven and DMA-driven completion paths SHALL stay on the same public HAL surface rather
than opening a separate family-private async API tier.

#### Scenario: User waits on UART DMA completion

- **WHEN** a user follows the supported completion-driven path for a public HAL class
- **THEN** they remain on the documented public HAL path plus runtime completion primitives
- **AND** they do not switch into a separate vendor-specific async API family

### Requirement: Public HAL Shall Ship One Canonical Completion+Timeout Example

The public HAL SHALL ship at least one canonical example under `examples/` that demonstrates the
recommended completion-token-plus-timeout pattern on a foundational board.

The example SHALL use only public HAL headers plus the runtime async-model headers.

#### Scenario: User copies the canonical example

- **WHEN** a user builds and flashes the canonical completion+timeout example on a foundational
  board
- **THEN** the example uses only public HAL and runtime headers
- **AND** the example reports success, timeout, and completion as distinguishable outcomes over
  the board's debug UART
- **AND** no code under `src/arch`, `src/device`, or other internal namespaces is touched by the
  example

### Requirement: Canonical Completion Example Shall Be Declared In Foundational Coverage

The canonical completion+timeout example SHALL be declared as part of the foundational example
coverage for at least one foundational board in `docs/RELEASE_MANIFEST.json`.

#### Scenario: Release validation checks foundational coverage

- **WHEN** the release checklist validates foundational example coverage for that board
- **THEN** the canonical completion+timeout example appears in the required example list
- **AND** a release with a broken example cannot be claimed as foundational for that board
