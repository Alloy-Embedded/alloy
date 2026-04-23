# Downstream CMake Consumption

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

## Validation

The repo validates this flow with a dedicated downstream smoke project under `tests/downstream_package/`.