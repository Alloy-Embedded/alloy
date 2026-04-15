## ADDED Requirements

### Requirement: Boards Shall Be Declarative

Boards SHALL declare local hardware choices and bring-up policy on top of shared runtime
primitives.

#### Scenario: Board LED declaration
- **WHEN** a board exposes an LED resource
- **THEN** it declares the selected pin and polarity
- **AND** it does not embed raw register initialization logic in the board header

### Requirement: Board Initialization Shall Use Runtime Bring-Up

`board::init()` SHALL orchestrate runtime bring-up using descriptor-driven runtime services.

#### Scenario: Foundational board initialization
- **WHEN** a foundational board initializes
- **THEN** clocks, startup services, connector setup, and default resources are brought up through
  the runtime path
- **AND** not through board-local raw register sequences

### Requirement: Canonical Examples Shall Follow the Official Runtime Path

Examples intended for users SHALL use the same public runtime path that production code is
expected to use.

#### Scenario: UART logger example
- **WHEN** the UART logger example is built
- **THEN** it configures UART through the public runtime API
- **AND** it does not bypass the runtime with raw register access
