## Why

Project version already reads `0.1.0` in `CMakeLists.txt`, three foundational boards are
hardware-validated, the runtime async model is closed, tooling parity has landed, and the docs
site ships. There is no actual `v0.1.0` tag or release artifact yet — so from the outside the
project still reads as pre-release with no shipping cadence. A cut v0.1 release converts the
existing work into a public signal that `alloy` is real.

Today the release story is also missing two pieces:

- there is no `CHANGELOG.md` at the repo root, so nobody can tell what landed between any two
  points in git history without reading commit messages
- the release gate set does not enforce that a changelog entry exists for the declared
  `project(alloy VERSION X.Y.Z)`, so a release can quietly ship without one

## What Changes

- Add `CHANGELOG.md` at the repo root following the Keep A Changelog structure with a v0.1.0
  section covering the runtime async model, descriptor-driven device boundary, foundational
  board bring-up, tooling parity, and the docs site.
- Add `scripts/check_changelog_present.py` — a release gate that asserts `CHANGELOG.md` exists
  and contains a section for the version declared in `CMakeLists.txt`.
- Register the new `changelog-present` release gate in `docs/RELEASE_MANIFEST.json`.
- Add `docs/RELEASE_NOTES_v0.1.0.md` as the canonical release-body text for the GitHub release
  draft, so the release-discipline narrative is reviewable in git, not only in the GitHub UI.
- Extend the `runtime-release-discipline` spec to require: a changelog at the repo root, an
  entry matching the declared version, and release notes checked into the repo for every
  non-prerelease cut.

Out of scope: creating the git tag and pushing the release — that stays a manual operator step
documented in `docs/RELEASE_CHECKLIST.md`.

## Outcome

A future `git tag v0.1.0` followed by the existing release workflow produces a coherent release:
versioned, documented in CHANGELOG, with a release-body file that reviewers can read in the diff
before the tag is pushed. Release discipline fails CI for any later release that ships without a
matching changelog entry.

## Impact

- Affected specs:
  - `runtime-release-discipline`
- Affected code and docs:
  - `CHANGELOG.md` (new, repo root)
  - `docs/RELEASE_NOTES_v0.1.0.md` (new)
  - `scripts/check_changelog_present.py` (new)
  - `docs/RELEASE_MANIFEST.json` (new `changelog-present` gate)
  - `docs/RELEASE_CHECKLIST.md` (cross-reference to the new gate and release-notes file)
