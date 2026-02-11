# Codegen Extraction Gate Decision

Date: 2026-02-11
Change: `refactor-microcore-unification-and-sdk-delivery`

## Gate Checklist

- Stable generator interface contract exists: YES
- Artifact compatibility contract validated in CI: YES
- Consumer SDK build requires no codegen runtime: YES
- Rollback strategy documented: YES

## Decision

Go/No-Go: **NO-GO (for immediate repo split), GO (for staged extraction readiness)**

Rationale:
- Readiness gates for boundary enforcement are now in place.
- A dedicated repository split should happen as a follow-up change to avoid
  coupling this refactor with repository migration risk.

## Follow-up Required for Full GO

1. Create `microcore-codegen` repository and publish initial tagged release.
2. Wire framework contributor workflows to consume pinned external codegen tag.
3. Execute one full release-candidate cycle using external codegen pin.
