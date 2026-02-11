## ADDED Requirements

### Requirement: Identity and Compatibility Regression Matrix

Testing infrastructure SHALL validate both canonical identifiers and migration compatibility aliases during the deprecation window.

#### Scenario: Canonical path regression coverage
- **WHEN** CI runs for framework changes
- **THEN** canonical namespace/build identifier paths SHALL be compiled and tested
- **AND** failures SHALL block merges

#### Scenario: Legacy alias compatibility coverage
- **WHEN** CI runs migration compatibility tests
- **THEN** legacy aliases SHALL be validated for supported paths
- **AND** warnings SHALL be reported without introducing runtime behavior drift

### Requirement: CLI End-to-End Build Matrix

Testing infrastructure SHALL execute CLI smoke tests across supported board/example combinations.

#### Scenario: Deterministic CLI build matrix
- **WHEN** `ucore` smoke tests run in CI
- **THEN** each supported board/example mapping SHALL resolve to the expected build target
- **AND** failures SHALL report board, example, and expected target mapping

#### Scenario: Non-interactive CLI mode for automation
- **WHEN** CI invokes `ucore` in non-interactive mode
- **THEN** commands SHALL complete without blocking prompts
- **AND** required user actions SHALL be reported as actionable errors

### Requirement: External Consumer Packaging Validation

Testing infrastructure SHALL validate the installed package from a clean external consumer workspace.

#### Scenario: `find_package` consumer validation
- **WHEN** CI runs packaging tests
- **THEN** an external sample project SHALL configure with `find_package(microcore CONFIG REQUIRED)`
- **AND** it SHALL build and link using exported public targets
