# Vendored: lwIP

Subset of [lwIP](https://savannah.nongnu.org/projects/lwip/) (lightweight TCP/IP
stack) — the NO_SYS / IPv4 / TCP path for alloy's `alloy::net` layer.

- **Version:** STABLE-2_2_1_RELEASE (lwIP 2.2.1)
- **Commit:** `77dcd25a72509eb83f72b033d219b1d40cd8eb95`
- **License:** BSD-3-Clause (see `COPYING`)
- **Vendored:** `src/core/`, `src/core/ipv4/`, `src/netif/`, `src/include/`.
  Skipped: `src/api/` (sequential/socket API — needs an RTOS) and `src/apps/`.

Framework-owned C, compiled **only** for boards+examples that pull the net stack
in (an example `#include <alloy/net/lwip...>` on an ethernet board — see
`build.py`), with warnings silenced (`-w`). All lwIP behaviour is configured by
the alloy port (`src/alloy/net/lwip/lwipopts.h`, `arch/cc.h`, `port.cpp`); the
per-device seam is `src/alloy/net/lwip.hpp`.

## Updating
Re-clone the tag and copy `src/{core,netif,include}` + `COPYING`, then bump the
version/commit above. Do not edit the sources — keep them a clean mirror.
