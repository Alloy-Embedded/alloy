## Context

The runtime migration is already reflected in most of `alloy`:

- hot-path peripheral code consumes `device/runtime.hpp`
- board bring-up consumes typed driver semantics and typed runtime refs
- zero-overhead checks are written against the typed runtime boundary

The main inconsistency left is startup.

Today the selected config still includes:

- `generated/runtime/**` for the runtime contract
- `generated/devices/<device>/startup_descriptors.hpp` for startup descriptors

That means the import layer is not actually runtime-only yet, even though the rest of the runtime
is.

Once `alloy-codegen` removes the legacy generated C++ surface from the public publication, `alloy`
must stop depending on that last header family.

## Goals

- make the selected device boundary runtime-only
- consume startup through the typed runtime contract
- preserve generated startup implementation sources needed for linking board firmware
- clean up the remaining "runtime-lite" naming where it now obscures the actual architecture
- remove test/build glue that still assumes legacy generated device headers

## Non-Goals

- rewriting startup execution semantics from scratch
- moving startup implementation units out of `generated/devices/<device>/` in the same change
- renaming every historical internal helper if that adds noise without architectural value

## Decision 1: The Selected Import Layer Is Runtime-Only

The selected device import layer SHALL include only:

- `generated/runtime/types.hpp`
- `generated/runtime/devices/<device>/*`
- generated startup implementation sources used for linking

It SHALL NOT include legacy generated descriptor headers from `generated/devices/<device>/`
for runtime behavior.

## Decision 2: Startup Uses A Typed Runtime Startup Contract

`alloy::device::startup` SHALL consume a typed runtime startup contract published under:

- `generated/runtime/devices/<device>/startup.hpp`

This contract SHALL replace:

- `generated/devices/<device>/startup_descriptors.hpp`

The public startup wrapper in `src/device/startup.hpp` SHALL become a thin alias over the runtime
startup contract, matching what `device/runtime.hpp` and `device/system_clock.hpp` already do.

## Decision 3: Startup Source Files Remain Link Inputs

The generated implementation files:

- `startup.cpp`
- `startup_vectors.cpp`

MAY remain board build inputs outside `generated/runtime/**` if that is the simplest stable layout.

This change is about the *descriptor boundary*, not the physical location of implementation units.

## Decision 4: Internal Naming Should Match The New Reality

Where practical, `alloy` SHOULD stop presenting "runtime-lite" as if it were a temporary or
secondary contract.

Expected direction:

- `ALLOY_DEVICE_RUNTIME_AVAILABLE` is the only selected-import availability macro
- internal namespaces/helpers use `runtime` as the canonical name

This rename MAY be staged inside the change if it stays mechanical and low-risk. If some internal
names remain temporarily, they SHALL NOT imply support for a second published C++ contract.

## Decision 5: Tests And Overrides Must Stop Including Legacy Generated Headers

The following layers SHALL be updated to consume the typed runtime startup contract:

- selected config template
- host MMIO selected-config overrides
- startup/runtime compile tests
- validation harnesses that currently include `startup_descriptors.hpp`

## Migration Order

1. Define the runtime-only import boundary in OpenSpec
2. Switch selected config and `device/startup.hpp` to runtime startup contract
3. Update host MMIO overrides and startup-oriented tests
4. Remove legacy startup include assumptions from build glue
5. Rename remaining runtime-lite-facing names where it reduces confusion
