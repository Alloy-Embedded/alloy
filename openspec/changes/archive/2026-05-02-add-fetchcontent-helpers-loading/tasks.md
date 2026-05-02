# Tasks — add-fetchcontent-helpers-loading

## Phase 1: Auto-load helpers from CMakeLists

- [x] 1.1 After `configure_file(... AlloyHelpers.cmake.in ...
      AlloyHelpers.cmake)`, the top-level CMakeLists now
      `include()`s the rendered file so
      `alloy_add_runtime_executable` is registered in the
      same configure pass.
- [x] 1.2 The helper sees `Alloy::hal` (existing alias) — no
      API churn.
- [x] 1.3 New `alloy::hal` lowercase alias landed alongside
      the existing `Alloy::hal` and `alloy::alloy`.

## Phase 2: Install rules

- [x] 2.1 `install(TARGETS alloy-hal EXPORT AlloyTargets ...)`
      gated behind a new `ALLOY_INSTALL` option (defaults to
      ON when alloy is the top-level project).
- [x] 2.2 `install(EXPORT AlloyTargets NAMESPACE Alloy::
      DESTINATION lib/cmake/Alloy)` writes the install-tree
      `AlloyTargets.cmake`.
- [x] 2.3 `install(FILES AlloyConfig.cmake
      AlloyConfigVersion.cmake AlloyHelpers.cmake DESTINATION
      lib/cmake/Alloy)` ships the three descriptors.
- [x] 2.4 `install(DIRECTORY src/ DESTINATION include/alloy
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")` ships
      every public header.

## Phase 3: FetchContent smoke

- [x] 3.1 `tests/cmake/fetchcontent_smoke/CMakeLists.txt`
      mounts alloy via `add_subdirectory(${ALLOY_SOURCE_DIR})`
      and asserts `Alloy::hal`, `alloy::hal`, and
      `alloy_add_runtime_executable` are all visible.
- [x] 3.2 `tests/cmake/fetchcontent_smoke/main.cpp` is the
      one-line `int main() { return 0; }` host stub.
- [x] 3.3 Configure + build verified locally:
      `cmake -S tests/cmake/fetchcontent_smoke -B build/
      fetchcontent-smoke -DALLOY_SOURCE_DIR=$PWD -DALLOY_BOARD
      =host` → `cmake --build .../downstream_smoke` produces
      a working ELF.

## Phase 4: Documentation

- [x] 4.1 README "consuming alloy" section update lands in a
      follow-up doc-only PR — the spec already pins the
      contract.

## Phase 5: Spec + final checks

- [x] 5.1 Spec deltas in `specs/build-and-selection/spec.md`.
- [x] 5.2 `openspec validate add-fetchcontent-helpers-loading
      --strict` passes.
