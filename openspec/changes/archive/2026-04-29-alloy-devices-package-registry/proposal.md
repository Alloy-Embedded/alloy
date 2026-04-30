# Proposal: alloy-devices as a Package Registry

## Status
`open` — strategic priority #2. Depends on: `open-codegen-pipeline`.

## Problem

The alloy-devices repo contains only `artifact-manifest.json` files. The
generated C++ headers are not in the local checkout. A downstream project
that does `cmake -DALLOY_BOARD=nucleo_g071rb` will fail unless someone has
manually placed the generated headers at `ALLOY_DEVICES_ROOT` via a
separate, undocumented step.

This is the primary adoption barrier. A developer who wants to evaluate alloy:
1. Clones the alloy repo.
2. Clones alloy-devices (if they can find it).
3. Realizes the headers are missing.
4. Gives up.

For 5 000+ MCU targets this must become fully automatic.

## Proposed Solution

### Two delivery modes

**Mode A — FetchContent (recommended for new projects)**

CMake automatically downloads the device package for the selected board:

```cmake
# In CMakeLists.txt — nothing extra needed; alloy handles it
cmake_minimum_required(VERSION 3.25)
project(my_firmware LANGUAGES CXX C ASM)
add_subdirectory(alloy)                 # or FetchContent_Declare(alloy ...)
set(ALLOY_BOARD nucleo_g071rb)
add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE alloy::alloy)
```

Internally, `cmake/alloy_devices.cmake` performs:
```cmake
# resolve board → (vendor, family, device)
# check if headers already present in cache
if(NOT EXISTS "${ALLOY_DEVICE_CACHE_DIR}/${vendor}/${family}/generated")
    FetchContent_Declare(alloy_device_${family}
        URL "https://github.com/alloy-rs/alloy-devices/releases/download/\
             v${ALLOY_DEVICES_VERSION}/${vendor}-${family}.tar.gz"
        URL_HASH "SHA256=${manifest_sha256}"
    )
    FetchContent_MakeAvailable(alloy_device_${family})
endif()
```

The download is cached in `~/.alloy/devices/` (configurable via
`ALLOY_DEVICE_CACHE_DIR`). Subsequent builds are instant.

**Mode B — Checked-in (monorepo / offline)**

Teams that cannot download at configure time set:
```cmake
set(ALLOY_DEVICES_ROOT "/path/to/alloy-devices-checkout" CACHE PATH "")
```
This is the current behavior — fully preserved.

### Package format

Each release asset: `<vendor>-<family>-<version>.tar.gz`

```
st-stm32g0-1.0.0.tar.gz
└── st/stm32g0/
    ├── artifact-manifest.json
    └── generated/
        └── runtime/
            ├── types.hpp
            └── devices/
                ├── stm32g071rb/
                └── stm32g0b1re/
```

### Release cadence

A GitHub Actions workflow in alloy-devices publishes a new release when:
1. A new device is added.
2. A patch changes the generated output (detected by hash comparison).
3. The alloy runtime requests a breaking IR schema change.

### Version pinning

```cmake
# cmake/alloy_devices_version.cmake (in alloy repo)
set(ALLOY_DEVICES_VERSION "1.0.0" CACHE STRING "alloy-devices release version")
set(ALLOY_DEVICES_CHECKSUMS_URL
    "https://github.com/alloy-rs/alloy-devices/releases/download/\
     v${ALLOY_DEVICES_VERSION}/checksums.json")
```

`checksums.json` maps `<vendor>-<family>.tar.gz` → SHA256. CMake fetches this
file once and uses it to verify every device package download.

### Offline / air-gapped support

```bash
# Pre-download all packages for offline use
alloy device prefetch --all --output /shared/alloy-cache
# On air-gapped machine:
cmake -DALLOY_DEVICE_CACHE_DIR=/shared/alloy-cache ...
```

### `find_package(AlloyDevices)` mode

The existing `alloy-devices/cmake/AlloyDeviceConfig.cmake` is extended to
support component-based discovery:

```cmake
find_package(AlloyDevices 1.0 REQUIRED COMPONENTS stm32g071rb stm32h743zit6)
```

This is the integration point for IDE plugins and the alloy SDK manager.

## Non-goals

- This spec does not define the package content (that is `open-codegen-pipeline`).
- This spec does not define the board manifest format (that is `board-manifest-declarative`).
- This spec does not define the alloy-cli SDK commands (that is `alloy-cli-distribution`).
