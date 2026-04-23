## ADDED Requirements

### Requirement: Peripheral Tier Promotions Shall Update Release Metadata

Any promotion or downgrade of a peripheral-class support tier SHALL update the machine-readable and
human-readable release metadata in the same change.

#### Scenario: Promoting DMA-adjacent HAL classes

- **WHEN** a peripheral support tier changes
- **THEN** both the support matrix and release manifest are updated together
- **AND** the recorded validation evidence matches the new tier
