# Downstream CMake Consumption

For projects that own their board (the typical user-app case), use the custom-board path
in [CUSTOM_BOARDS.md](CUSTOM_BOARDS.md). That recipe consumes the runtime through
`add_subdirectory(${ALLOY_ROOT})` with `ALLOY_BOARD=custom`, so the board lives in the
user's repo and the alloy runtime stays untouched. `alloy new` wires this up automatically.

The package-style path documented below is for downstream projects that link against an
already-configured `alloy` build tree (e.g. CI integration tests, in-tree examples).

`alloy` publishes a supported build-tree package config for downstream consumers.

This is the supported package-style path:

1. configure an `alloy` build for the board or host profile you want
2. point the downstream project at that build tree's generated CMake package
3. use `find_package(Alloy CONFIG REQUIRED)` and `alloy_add_runtime_executable(...)`

## Important Scope

The package is build-tree scoped.

That means the package config describes one already-configured `alloy` build tree, including:

- selected board
- selected platform
- generated device contract import state
- linker script path when relevant

It is not a generic installed SDK that can switch boards after the fact.

## Step 1: Configure Alloy

Host-oriented downstream integration:

```bash
cmake --preset host-validation-debug
```

Board-oriented downstream integration example:

```bash
cmake --preset stm32g0-renode-debug
```

Both presets generate a package config under:

```text
build/presets/<preset>/generated/cmake
```

## Step 2: Use `find_package(Alloy)`

Example downstream `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.25)
project(my_alloy_app LANGUAGES CXX C ASM)

find_package(Alloy CONFIG REQUIRED
    PATHS "/absolute/path/to/alloy/build/presets/host-validation-debug/generated/cmake"
    NO_DEFAULT_PATH
)

alloy_add_runtime_executable(my_alloy_app
    SOURCES
        main.cpp
)
```

## Step 3: Write Application Code

Minimal host-safe smoke example:

```cpp
#include "core/result.hpp"

int main() {
    auto result = alloy::core::Result<int, int>(alloy::core::Ok(42));
    return result.is_ok() ? 0 : 1;
}
```

If the selected package build publishes board macros such as `BOARD_HEADER`, your downstream source may also use the same board-oriented include path as the in-repo examples.

## Exported Items

Imported target:

- `Alloy::hal`

Helper function:

- `alloy_add_runtime_executable(<name> SOURCES ... )`

Useful package variables:

- `Alloy_BOARD`
- `Alloy_PLATFORM`
- `Alloy_SOURCE_DIR`
- `Alloy_BINARY_DIR`
- `Alloy_BOARD_SOURCE_DIR`
- `Alloy_LINKER_SCRIPT`

## Automatic device resolution

CMake automatically downloads the required `alloy-devices` package on first
configure — no manual clone step needed.  See [DEVICE_PACKAGES.md](DEVICE_PACKAGES.md)
for the full resolution order (local checkout → cache → auto-download) and
how to use offline mode or a private package mirror.

```bash
# Default: auto-download to ~/.alloy/devices/ on first configure
cmake -DALLOY_BOARD=nucleo_g071rb -B build

# Pin to a local alloy-devices checkout (developer mode)
cmake -DALLOY_DEVICES_ROOT=../alloy-devices -DALLOY_BOARD=nucleo_g071rb -B build

# Offline — device package must already be in cache
cmake -DALLOY_OFFLINE=ON -DALLOY_BOARD=nucleo_g071rb -B build
```

## Validation

The repo validates this flow with a dedicated downstream smoke project under `tests/downstream_package/`.