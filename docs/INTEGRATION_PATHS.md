# MicroCore Integration Paths

This document defines supported integration paths for framework consumers.

## 1. `add_subdirectory` (Monorepo/local source)

Use this when your application repository includes MicroCore as a source dependency.

```cmake
add_subdirectory(external/microcore)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE microcore::hal microcore::board::nucleo_f401re)
```

## 2. `FetchContent` (Source at configure time)

Use this when you want CMake to fetch MicroCore sources during configure.

```cmake
include(FetchContent)

FetchContent_Declare(
  microcore
  GIT_REPOSITORY https://github.com/your-org/microcore.git
  GIT_TAG main
)
FetchContent_MakeAvailable(microcore)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE microcore::hal microcore::board::nucleo_f401re)
```

## 3. `find_package` (Installed SDK)

Use this for release consumption without source-tree coupling.

### Build/install SDK (consumer profile)

```bash
cmake -B build-sdk \
  -DMICROCORE_BOARD=host \
  -DMICROCORE_PLATFORM=linux \
  -DMICROCORE_BUILD_TESTS=OFF \
  -DMICROCORE_DELIVERY_PROFILE=consumer \
  -DCMAKE_INSTALL_PREFIX=/opt/microcore-sdk

cmake --build build-sdk
cmake --install build-sdk
```

### Consume SDK

```cmake
find_package(microcore CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE microcore::hal microcore::rtos microcore::board::host)
```

### Configure consumer project

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/opt/microcore-sdk
cmake --build build
```

## Profile Contract

- `MICROCORE_DELIVERY_PROFILE=consumer`:
  - targets SDK usage
  - skips Python/codegen runtime dependency checks
  - consumes pre-generated board/codegen artifacts
  - runtime package excludes codegen tooling internals
- `MICROCORE_DELIVERY_PROFILE=contributor` (default):
  - full development profile for framework contributors
  - keeps codegen/development validation workflow enabled
  - can install codegen boundary assets (`MICROCORE_INSTALL_CODEGEN_TOOLING=ON`)

## Codegen Compatibility Metadata

Installed SDK exports these paths through `microcoreConfig.cmake`:

- `MICROCORE_BOARD_ARTIFACT_CONTRACT_MANIFEST`
- `MICROCORE_CODEGEN_CONTRACT_FILE` (contributor installs)
- `MICROCORE_CODEGEN_VALIDATION_SCRIPT` (contributor installs)
