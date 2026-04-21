## ADDED Requirements

### Requirement: Residual Family-Private Peripheral Glue Shall Not Remain In The Active Path

The migration SHALL delete, isolate, or clearly mark as temporary internal adaptation any
residual family-private glue for a foundational peripheral class once that class has a finished
public HAL path.

#### Scenario: Foundational SPI path is migrated
- **WHEN** the foundational SPI runtime path is complete
- **THEN** new production usage does not depend on family-private SPI glue
- **AND** any remaining family-private file is either unused in the active path or tracked for removal
