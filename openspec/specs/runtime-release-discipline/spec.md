# runtime-release-discipline Specification

## Purpose
Runtime release discipline defines what `alloy` must publish and validate before making support or
compatibility claims.

Releases need explicit support tiers, declared `alloy-devices` compatibility, documented migration
notes for breaking public changes, and enforced validation gates that keep those claims honest.
## Requirements
### Requirement: Releases Shall Publish Explicit Support Tiers

`alloy` releases SHALL publish explicit support tiers for boards and major peripheral classes.

#### Scenario: User checks whether a board is production-supported
- **WHEN** a user consults active support docs
- **THEN** they can tell whether a board is foundational, experimental, deprecated, or equivalent
- **AND** the support claim matches the documented validation gate

### Requirement: Releases Shall Declare Runtime Contract Compatibility

Each `alloy` release SHALL declare the compatible `alloy-devices` contract version or range.

#### Scenario: User upgrades `alloy`
- **WHEN** a user upgrades `alloy`
- **THEN** they can determine which published `alloy-devices` contract is expected
- **AND** compatibility drift is not left implicit

### Requirement: Breaking Changes Shall Include Migration Notes

Breaking changes affecting the public runtime story SHALL include migration notes.

#### Scenario: Public HAL shape changes
- **WHEN** a public HAL configuration or entry path changes incompatibly
- **THEN** the release documents the migration
- **AND** at least one canonical example is updated to the new shape
