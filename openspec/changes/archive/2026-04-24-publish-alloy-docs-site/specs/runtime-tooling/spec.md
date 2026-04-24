## ADDED Requirements

### Requirement: Public Docs Site Shall Be Built And Publishable From The Repo

The repo SHALL carry a committed, reproducible configuration for a public docs site that renders
the existing user-facing markdown documentation without requiring external tooling outside the
declared toolchain.

#### Scenario: Contributor builds the docs site locally

- **WHEN** a contributor runs the documented docs-site build command
- **THEN** a static site is produced from the committed `mkdocs.yml` configuration and the
  `docs/**.md` sources, with no external fetches beyond the declared python requirements
- **AND** the build fails loudly on broken internal links or missing nav entries

### Requirement: Docs Site Navigation Shall Separate User Guide From Internals

The published docs site SHALL expose a navigation structure that clearly separates the
user-facing guide from internal design notes and release discipline material.

#### Scenario: New adopter browses the docs site

- **WHEN** a first-time visitor lands on the docs site
- **THEN** the navigation surfaces the quickstart, board tooling, cookbook, and support matrix
  as the primary user guide entries
- **AND** internal material (architecture, release discipline, runtime device boundary, runtime
  cleanup audit) is reachable under a clearly labeled secondary section, not on the landing
  surface

### Requirement: Docs Site Shall Be Covered By A Release Gate

The docs site SHALL be covered by a release gate that fails loudly when the site cannot be
built from the current repo state.

#### Scenario: Release discipline validates the docs site

- **WHEN** the release discipline gate set is exercised for a candidate release
- **THEN** the docs-site gate runs the documented build command with strict link checking
- **AND** any broken link, missing nav entry, or build failure fails the gate and the release
