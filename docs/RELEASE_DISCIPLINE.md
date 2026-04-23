# Runtime Release Discipline

This document defines the production-readiness contract for the active runtime path.

## Purpose

`alloy` only makes release claims that match validated runtime behavior.

That means each release must publish:

- explicit support tiers
- explicit `alloy-devices` compatibility
- explicit release gates
- explicit migration discipline for breaking changes

## Compatibility Policy

The compatibility source of truth is [docs/RELEASE_MANIFEST.json](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RELEASE_MANIFEST.json).

Current policy:

- `alloy` releases are validated against a pinned `alloy-devices` commit
- updating the pin is a deliberate compatibility change, not an incidental workflow edit
- CI workflow pins and the release manifest must stay aligned

## Release Tiers

### Foundational

A foundational board or peripheral class is part of the active release contract.

It must be backed by the declared release gates, not just compile success.

### Representative

A representative path has real runtime support and targeted validation, but it is not claimed as equally covered as the foundational set.

### Experimental

An experimental path may build or partially validate, but it is not yet part of the release promise.

### Deprecated

A deprecated path remains documented for migration only and should not receive new support claims.

## Release Gates

The active foundational release ladder is:

- runtime-device-boundary script
- release-discipline script
- descriptor contract smoke on foundational boards
- host-MMIO validation on foundational boards
- Renode runtime validation on foundational families
- SAME70 zero-overhead gate for any zero-overhead release claim
- required example/build coverage published in the release manifest

## Breaking Change Discipline

Breaking changes in any of the following require migration notes:

- public HAL shape
- build and board-selection model
- runtime/device contract expectations
- board migration path

At least one canonical example must be updated in the same change.

## Support Matrix Ownership

The active support matrix lives in [docs/SUPPORT_MATRIX.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/SUPPORT_MATRIX.md).

Human-readable docs and machine-readable release data must move together:

- update the support matrix
- update the release manifest
- keep CI and release workflow pins aligned