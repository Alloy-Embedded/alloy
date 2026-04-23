## ADDED Requirements

### Requirement: Public HAL Support Claims Shall Follow Validation Evidence

When the repo claims that a public HAL class is foundational, that claim SHALL follow the current
validation evidence rather than API existence alone.

#### Scenario: User checks whether SPI is foundational

- **WHEN** a public HAL class is listed as foundational in support docs
- **THEN** the class is backed by the declared validation ladder and canonical example coverage
- **AND** the support claim is downgraded if those proofs are missing

### Requirement: CAN Shall Require More Than Bring-Up Before Promotion

The public CAN path SHALL prove deterministic traffic behavior before it is promoted beyond an
experimental claim.

#### Scenario: CAN promotion

- **WHEN** the repo considers CAN for promotion
- **THEN** the validation includes deterministic loopback or equivalent message traffic proof
- **AND** not only board boot and peripheral configure
