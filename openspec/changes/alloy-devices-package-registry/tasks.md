# Tasks: alloy-devices Package Registry

Depends on: `open-codegen-pipeline` (phases 3–5 require published packages).
Phases 1–2 are host-only (CMake infrastructure, no hardware).

## 1. CMake infrastructure

- [x] 1.1 Create `cmake/alloy_devices_version.cmake`: pin `ALLOY_DEVICES_VERSION` +
      `ALLOY_DEVICES_BASE_URL`. Make both cache variables so CI can override.
- [x] 1.2 Extend `cmake/alloy_devices.cmake`: before probing filesystem paths, check
      `ALLOY_DEVICE_CACHE_DIR`. If device package absent, download
      `<vendor>-<family>-<version>.tar.gz` from the release URL via `file(DOWNLOAD)`.
- [x] 1.3 Implement cache dir resolution order:
      1. `ALLOY_DEVICE_CACHE_DIR` CMake variable
      2. `ALLOY_DEVICE_CACHE` environment variable
      3. `~/.alloy/devices/` (platform home dir via CMake)
      4. `${CMAKE_BINARY_DIR}/_alloy_devices_cache` (build-local fallback)
- [x] 1.4 Add offline mode: if `ALLOY_OFFLINE=ON`, disable download and fail
      with a clear error if the device package is not in cache.
- [ ] 1.5 Extend `AlloyDeviceConfig.cmake` in alloy-devices: support component syntax
      `find_package(AlloyDevices REQUIRED COMPONENTS stm32g071rb stm32h743zit6)`.
      Register each device component as a CMake imported target.

## 2. Package format and checksums

- [x] 2.1 Define the tar.gz layout for device packages (see proposal).
      Write `scripts/pack_device.py`: takes a family directory and produces the
      versioned tar.gz with deterministic file ordering.
- [x] 2.2 Define `checksums.json` format:
      `{ "st-stm32g0-1.0.0.tar.gz": "sha256:abc...", ... }`
      Consumed by CMake to verify downloads before unpacking.
- [x] 2.3 Implement CMake SHA256 verification using `file(DOWNLOAD ... EXPECTED_HASH)`.
      On mismatch: delete partial download, emit fatal error with remediation steps.

## 3. GitHub Release publishing

- [ ] 3.1 Write GitHub Actions workflow `alloy-devices/.github/workflows/publish.yml`:
      trigger on push to main; regen changed families; publish new release assets.
- [ ] 3.2 Write `scripts/regen_changed.py`: compares SVD source hashes in
      `artifact-manifest.json` against previous run; reruns pipeline only for changed families.
- [ ] 3.3 Implement release asset upload via `gh release create` in CI.
      Tag format: `alloy-devices-v<ALLOY_DEVICES_VERSION>`.
- [ ] 3.4 Add `checksums.json` as a release asset; include SHA256 of every family
      tar.gz for that release.

## 4. Developer tooling

- [x] 4.1 Add `alloy device prefetch [--family <f>] [--all] [--output <dir>]` to
      alloyctl.py / alloy-cli: downloads packages to a specified cache dir.
      Useful for air-gapped environments and CI caching.
- [x] 4.2 Add `alloy device list [--available] [--cached]`: lists devices available
      in the registry and which are already downloaded locally.
- [x] 4.3 Add `alloy device info <device_id>`: prints IR coverage report, alloy-codegen
      version, SVD source URL, applied patches count, alloy-devices version.

## 5. Migration and compatibility

- [x] 5.1 Keep `ALLOY_DEVICES_ROOT` pointing to a local checkout working (Mode B).
      If set, skip all FetchContent logic. Document as the offline / monorepo path.
- [ ] 5.2 Add a CI step to the alloy repo that runs a clean configure with no local
      alloy-devices clone and verifies FetchContent download succeeds.
- [ ] 5.3 Update `docs/QUICKSTART.md`: remove manual alloy-devices clone step.
      The one-liner becomes: `git clone alloy && cmake -DALLOY_BOARD=<board> -B build`.
- [x] 5.4 Add `alloy doctor` check: `alloy-devices reachable or locally present`.
      Distinguish between "no internet" (warn + hint prefetch) vs "missing + no internet" (fail).

## 6. Documentation

- [x] 6.1 `docs/DEVICE_PACKAGES.md`: how FetchContent download works, cache dirs,
      offline mode, version pinning, how to add a custom device package.
- [x] 6.2 Update `docs/CMAKE_CONSUMPTION.md`: mention automatic device resolution.
- [ ] 6.3 Update `docs/QUICKSTART.md`: new zero-step setup flow.
