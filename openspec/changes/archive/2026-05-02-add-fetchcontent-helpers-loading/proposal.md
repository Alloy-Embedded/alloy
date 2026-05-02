# Auto-Load AlloyHelpers Under add_subdirectory + Install Path

## Why

The HAL already builds a complete CMake package descriptor —
`AlloyConfig.cmake`, `AlloyConfigVersion.cmake`, and a generated
`AlloyHelpers.cmake` in `${CMAKE_BINARY_DIR}/generated/cmake/`.
That works for downstream projects that point
`-Dalloy_DIR=<build>/generated/cmake` and call
`find_package(alloy CONFIG)`.

It does **not** work for the most common consumption path the
ecosystem assumes — `FetchContent_Declare(alloy ...)` followed
by `FetchContent_MakeAvailable(alloy)` (which falls through to
`add_subdirectory()`).  When alloy is consumed that way the
top-level `CMakeLists.txt` runs verbatim, so `Alloy::hal` exists
but `alloy_add_runtime_executable` is never `include`-d.  The
`alloy-cli`'s scaffold needs that helper to be loaded
*automatically* the moment alloy is added as a subproject.

A second gap: the configure-time package descriptor never gets
`install`-ed, so projects that DO `cmake --install` an alloy
build can't `find_package(alloy CONFIG)` from a system prefix.

## What Changes

### Auto-load helpers in the top-level build

- After `configure_file(... AlloyHelpers.cmake.in ... AlloyHelpers.cmake)`,
  the top-level CMakeLists SHALL `include()` the rendered file
  so `alloy_add_runtime_executable` is available regardless of
  the consumption path (`add_subdirectory` / FetchContent /
  find_package).
- The alias `alloy::alloy` already exists; the helper now relies
  on `Alloy::hal` (canonical) and we add `alloy::hal` (lowercase
  alias) so consumer scripts don't have to think about case.

### Install rules

- `install(TARGETS alloy-hal EXPORT AlloyTargets ...)` exports
  the HAL target.
- `install(EXPORT AlloyTargets ... NAMESPACE Alloy::)` writes
  the install-tree `AlloyTargets.cmake`.
- `install(FILES AlloyConfig.cmake AlloyConfigVersion.cmake
  AlloyHelpers.cmake DESTINATION lib/cmake/Alloy)` ships the
  three descriptors.
- `install(DIRECTORY src/ DESTINATION include/alloy)` and
  similar headers go into `<prefix>/include/alloy/...`.
- The board metadata + linker scripts continue to live next to
  the source (we don't try to redistribute them at install
  time; that's a follow-up).

### FetchContent smoke test

- New `tests/cmake/fetchcontent_smoke/` directory with a tiny
  downstream `CMakeLists.txt` that consumes alloy via
  `add_subdirectory(${ALLOY_SOURCE_DIR})` and uses
  `alloy_add_runtime_executable(...)`.
- `ctest -L alloy-fetchcontent` runs it as a configure-only
  check (no toolchain required) so CI catches regressions in
  the helper-loading path.

## Impact

- `alloy-cli`'s scaffold can FetchContent_Declare(alloy ...)
  and immediately call `alloy_add_runtime_executable(target
  SOURCES ...)` — no extra ceremony.
- External consumers who `cmake --install` alloy now get a
  working `find_package(alloy CONFIG)` from `<prefix>/lib/cmake
  /Alloy/`.
- The build-tree path keeps working unchanged for in-tree
  development.

## What this DOES NOT do

- Does not publish alloy on a package manager (vcpkg / Conan)
  — that's a release-track concern.
- Does not redistribute board metadata or linker scripts at
  install time — too risky right now (paths are
  source-relative).
- Does not change the on-target ABI; this is purely a CMake
  surface change.
