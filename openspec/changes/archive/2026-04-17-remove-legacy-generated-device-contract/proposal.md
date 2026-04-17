## Why

`alloy` already consumes the typed runtime device contract for the hot path, but it still carries
the last pieces of the legacy generated contract:

- `generated/devices/<device>/startup_descriptors.hpp` is still imported by the selected device layer
- some tests and build glue still assume the legacy startup include path
- "runtime-lite" remains part of internal naming even though the typed runtime contract is now the
  default path

If `alloy-codegen` removes the legacy generated C++ surface from publication, `alloy` must finish
the cut:

- startup must move to the typed runtime contract
- the selected import layer must become runtime-only
- the codebase should stop talking about a second "lite" contract as if a heavier public contract
  still existed

## What Changes

- **BREAKING** make the selected device import layer consume only the typed runtime contract
- **BREAKING** replace legacy startup descriptor includes with a typed runtime startup contract
- **MODIFIED** keep startup source consumption (`startup.cpp`, `startup_vectors.cpp`) where needed
  for board builds, but stop consuming legacy startup descriptor headers
- **MODIFIED** rename internal "runtime-lite" usage to simply "runtime" where that no longer causes
  churn or ambiguity
- **MODIFIED** remove tests, host overrides, and build glue that still include legacy generated
  device headers

## Impact

- Affected specs:
  - `runtime-device-boundary`
  - `startup-runtime`
  - `migration-cleanup`
- Affected code:
  - `cmake/alloy_devices.cmake`
  - `cmake/templates/selected_config.hpp.in`
  - `src/device/startup.hpp`
- `src/hal/detail/runtime_ops.hpp` and related internal naming
  - host MMIO selected-config overrides and startup validation tests
- Breaking impact:
  - `alloy` no longer supports consuming legacy generated C++ headers from `alloy-devices`
