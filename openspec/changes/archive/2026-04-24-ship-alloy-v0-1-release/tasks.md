## 1. OpenSpec Baseline

- [x] 1.1 Add runtime-release-discipline delta requiring CHANGELOG + release notes + gate

## 2. Changelog

- [x] 2.1 Add `CHANGELOG.md` at the repo root with a v0.1.0 entry
- [x] 2.2 Add `scripts/check_changelog_present.py` that matches project version to a CHANGELOG section
- [x] 2.3 Register the new `changelog-present` gate in `docs/RELEASE_MANIFEST.json`

## 3. Release Notes

- [x] 3.1 Add `docs/RELEASE_NOTES_v0.1.0.md` as the canonical release-body text

## 4. Release Checklist

- [x] 4.1 Cross-reference `changelog-present` and the release-notes file in `docs/RELEASE_CHECKLIST.md`

## 5. Validation

- [x] 5.1 Run `scripts/check_changelog_present.py`
- [x] 5.2 Run `scripts/check_release_discipline.py`
- [x] 5.3 `openspec validate ship-alloy-v0-1-release --strict`
