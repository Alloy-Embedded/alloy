## ADDED Requirements

### Requirement: Releases Shall Ship A Repo-Root Changelog

The repo SHALL carry a `CHANGELOG.md` at the repo root that follows a stable, machine-readable
section convention (Keep A Changelog or equivalent), and SHALL contain a section for the
version currently declared in the build system.

#### Scenario: User inspects what landed in a release

- **WHEN** a user opens `CHANGELOG.md` at the repo root
- **THEN** the file contains a section for the version declared in `CMakeLists.txt`
- **AND** the section enumerates the user-visible additions, changes, and fixes included in
  that version

#### Scenario: Release-discipline gate validates the changelog

- **WHEN** the release-discipline gate set runs on a candidate release
- **THEN** a dedicated changelog gate asserts that `CHANGELOG.md` exists and contains a
  matching section for the declared version
- **AND** the gate fails the release if the file is missing or the version section is absent

### Requirement: Releases Shall Publish Reviewable Release Notes In The Repo

Each non-prerelease tag cut from `alloy` SHALL have a corresponding `docs/RELEASE_NOTES_vX.Y.Z.md`
file checked into the repo that documents the scope, validation evidence, and known gaps of that
release.

#### Scenario: Reviewer reads the release body before the tag is pushed

- **WHEN** a reviewer opens the pull request that prepares a release cut
- **THEN** the diff includes a `docs/RELEASE_NOTES_vX.Y.Z.md` file matching the declared version
- **AND** the file documents what ships, what is still representative or experimental, and the
  `alloy-devices` compatibility pin for that release
