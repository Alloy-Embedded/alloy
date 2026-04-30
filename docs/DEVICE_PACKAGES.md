# Device Packages

alloy-devices provides generated C++ headers for each supported MCU family.
Packages are delivered in two modes — no manual step required for most users.

## Resolution order

When CMake configures an alloy project, device headers are resolved as follows:

| Priority | Source | How to activate |
|----------|--------|-----------------|
| 1 (highest) | **Mode B** — local alloy-devices checkout | `ALLOY_DEVICES_ROOT=/path/to/alloy-devices` |
| 2 | **Mode A** — package cache | automatic; cache populated on first configure |
| 3 | Download | auto-download from GitHub Releases on first configure |

## Mode A — automatic download (default)

On first `cmake -B build`, CMake checks the cache directory for the required
family package.  If absent and the network is reachable, it downloads and
extracts the tar.gz automatically:

```
-- alloy-devices: Mode A — package cache at /Users/you/.alloy/devices
-- alloy-devices: downloading st-stm32g0-1.0.0.tar.gz from ...
-- alloy-devices: extracting st-stm32g0-1.0.0.tar.gz -> /Users/you/.alloy/devices
-- alloy-devices: st/stm32g0 ready at /Users/you/.alloy/devices/st/stm32g0
```

Subsequent configures reuse the cached package with no network activity.

### Cache directory resolution

The cache directory is resolved in this order:

1. `ALLOY_DEVICE_CACHE_DIR` CMake variable (highest priority)
2. `ALLOY_DEVICE_CACHE` environment variable
3. `~/.alloy/devices/` (platform home directory)
4. `${CMAKE_BINARY_DIR}/_alloy_devices_cache` (build-local fallback)

Override for CI or shared NFS caches:

```bash
cmake -DALLOY_DEVICE_CACHE_DIR=/nfs/shared/alloy-devices-cache -B build
```

### SHA256 verification

CMake downloads `checksums.json` from the same release tag before fetching the
package tar.gz.  If the SHA256 does not match, the partial download is deleted
and configuration fails with a clear error.

## Mode B — local checkout (developers / CI / offline)

Point CMake at a local alloy-devices clone.  Mode B is tried first and takes
precedence over any cache.

```bash
cmake -DALLOY_DEVICES_ROOT=/path/to/alloy-devices -B build
```

This is the standard developer workflow when working on alloy-devices itself or
applying custom SVD patches.

## Offline mode

Set `ALLOY_OFFLINE=ON` to disable all network downloads.  CMake fails with a
clear error if the required package is not already in the cache or pointed to
via `ALLOY_DEVICES_ROOT`.

```bash
cmake -DALLOY_OFFLINE=ON -B build
```

Prefetch packages before going offline:

```bash
python scripts/alloyctl.py device prefetch --family stm32g0
# or prefetch everything:
python scripts/alloyctl.py device prefetch --all
```

## CLI commands

### `alloy device prefetch`

Download device packages to the local cache without running a full build.

```bash
# Download one family
python scripts/alloyctl.py device prefetch --family stm32g0

# Download all families listed in the registry
python scripts/alloyctl.py device prefetch --all

# Download to a custom directory
python scripts/alloyctl.py device prefetch --family same70 --output /path/to/cache
```

### `alloy device list`

List all devices available in the registry and their local cache status.

```bash
python scripts/alloyctl.py device list
python scripts/alloyctl.py device list --vendor st
python scripts/alloyctl.py device list --cached        # only locally present
python scripts/alloyctl.py device list --available     # only not yet cached
python scripts/alloyctl.py device list --json          # machine-readable
```

### `alloy device info`

Show version, SVD source, and applied patches for a specific device.

```bash
python scripts/alloyctl.py device info stm32g071rb
```

## Package format

Each device family is distributed as a single tar.gz:

```
<vendor>-<family>-<version>.tar.gz
├── <vendor>/<family>/
│   ├── artifact-manifest.json      # version, SVD source, patches applied
│   ├── generated/
│   │   ├── runtime/
│   │   │   ├── types.hpp
│   │   │   └── devices/<device>/
│   │   │       ├── pins.hpp
│   │   │       ├── registers.hpp
│   │   │       ├── register_fields.hpp
│   │   │       ├── driver_semantics/
│   │   │       │   ├── gpio.hpp
│   │   │       │   ├── uart.hpp
│   │   │       │   └── ...
│   │   │       └── ...
│   │   └── devices/<device>/
│   │       ├── startup.cpp
│   │       └── startup_vectors.cpp
│   └── metadata/
```

## Version pinning

Pin the package version in CI to ensure reproducible builds:

```bash
cmake -DALLOY_DEVICES_VERSION=1.2.0 -B build
```

The default version is set in `cmake/alloy_devices_version.cmake` and updated
with each alloy release.

## Building device packages (contributors)

Use `scripts/pack_device.py` to create a package from an alloy-devices checkout:

```bash
python scripts/pack_device.py \
    --family-dir ../alloy-devices/st/stm32g0 \
    --vendor st --family stm32g0 \
    --version 1.0.0 \
    --output dist/ \
    --update-checksums dist/checksums.json
```

## Custom / private device packages

Host your own packages on any HTTP server and point CMake at it:

```cmake
# In your project CMakeLists.txt (before add_subdirectory(alloy)):
set(ALLOY_DEVICES_BASE_URL "https://my-server.internal/alloy-devices/releases/v1.0.0")
set(ALLOY_DEVICES_VERSION  "1.0.0")
```

CMake will download from your server and apply the same SHA256 verification.
