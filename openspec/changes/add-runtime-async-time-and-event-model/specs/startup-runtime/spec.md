## ADDED Requirements

### Requirement: Startup Runtime Shall Provide Typed Interrupt Integration For Runtime Events

The startup/runtime layer SHALL expose typed interrupt entry points that the runtime event model
can bind to without recreating dynamic interrupt ownership outside the generated contract.

#### Scenario: Runtime event model consumes generated interrupt stubs
- **WHEN** the runtime attaches an interrupt-driven completion path
- **THEN** it binds through generated interrupt ids and startup stubs
- **AND** it does not depend on handwritten family-wide interrupt registries
