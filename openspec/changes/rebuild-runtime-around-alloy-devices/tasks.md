## 1. OpenSpec and Baseline Cleanup

- [x] 1.1 Create the permanent OpenSpec baseline for `alloy`
- [x] 1.2 Audit the current runtime tree and mark files/directories as:
      - keep
      - rewrite
      - delete
- [x] 1.3 Document the new `alloy` vs `alloy-devices` boundary in repo docs
- [x] 1.4 Add CI checks that fail when new runtime code reaches into vendor-specific generated
      trees directly instead of going through the device import layer

### Gate R1: Runtime boundary reset
- [x] R1.1 `src/device` exists and is the only supported runtime entrypoint into selected device data
- [x] R1.2 new runtime code no longer depends on handwritten cross-vendor signal/pin enums

## 2. Build and Device Import Layer

- [x] 2.1 Add CMake integration for `alloy-devices`
- [x] 2.2 Add device-selection infrastructure that resolves board -> vendor/family/device/arch
- [x] 2.3 Introduce `src/device/import.hpp`
- [x] 2.4 Introduce `src/device/selected.hpp`
- [x] 2.5 Introduce the selected device import layer, later collapsed to runtime-lite only
- [x] 2.6 Introduce `src/device/traits.hpp`
- [x] 2.7 Add compile smoke tests proving the selected device contract can be included without
      direct vendor-path includes from boards or drivers

## 3. Connector and Claim Kernel

- [x] 3.1 Implement runtime consumption of connector candidates, groups, requirements, and
      operations
- [x] 3.2 Implement typed `connect()` over generated descriptors
- [x] 3.3 Implement resource claim primitives for pins, peripheral instances, DMA routes, and
      interrupt bindings
- [x] 3.4 Add compile-time rejection for invalid connector combinations
- [x] 3.5 Add foundational tests for package-aware connection selection

## 4. Single Public HAL API

- [x] 4.1 Define one public API shape for GPIO with config defaults
- [x] 4.2 Define one public API shape for UART with config defaults
- [x] 4.3 Define one public API shape for SPI with config defaults
- [x] 4.4 Define one public API shape for I2C with config defaults
- [ ] 4.5 Define one public API shape for DMA with config defaults
- [x] 4.6 Remove the public architecture split between `simple`, `expert`, and `fluent`
- [ ] 4.7 Replace legacy interface headers with the new unified API entrypoints
- [ ] 4.8 Update examples and docs to stop presenting multiple API tiers

### Gate R2: Single public API
- [ ] R2.1 only one public API concept exists for each foundational peripheral class
- [ ] R2.2 defaults and helper aliases do not form separate public API layers

## 5. Descriptor-Driven Drivers

- [x] 5.1 Rebuild GPIO on top of device descriptors and claims
- [x] 5.2 Rebuild UART on top of instances, capabilities, connectors, and clock/reset descriptors
- [x] 5.3 Rebuild SPI on top of the same model
- [x] 5.4 Rebuild I2C on top of the same model
- [ ] 5.5 Rebuild DMA on top of the same model
- [x] 5.6 Add compile coverage for foundational-family driver bring-up using generated descriptors

## 6. Startup Runtime and Architecture Layer

- [x] 6.1 Create `src/arch` runtime for foundational Cortex-M targets
- [x] 6.2 Move startup algorithm ownership into `alloy`
- [x] 6.3 Consume generated startup vectors from `alloy-devices`
- [x] 6.4 Replace handwritten family startup selection in CMake
- [x] 6.5 Add startup smoke tests for foundational targets

### Gate R4: Startup and build integration
- [ ] R4.1 startup data is consumed from `alloy-devices`, not handwritten in the runtime repo
- [ ] R4.2 device/build selection no longer depends on large manual family switch logic

## 7. Board and Example Rebuild

- [ ] 7.1 Rebuild foundational boards to be declarative and side-effect safe
- [ ] 7.2 Remove hardware initialization side effects from headers
- [ ] 7.3 Make `board::init()` call runtime bring-up paths instead of raw register code
- [ ] 7.4 Rebuild canonical examples:
      - blink
      - uart logger
      - spi
      - i2c
      - dma
- [ ] 7.5 Ensure examples use only the official runtime path

### Gate R3: Descriptor-driven bring-up
- [ ] R3.1 foundational boards initialize through descriptor-driven runtime code
- [ ] R3.2 canonical examples no longer bypass the library with raw registers

## 8. Legacy Removal

- [ ] 8.1 Remove or archive obsolete public vendor/runtime glue under `src/hal/vendors`
- [x] 8.2 Remove handwritten cross-vendor signal registries and enum cores that conflict with the
      descriptor model
- [ ] 8.3 Remove stale universal `hal/*.hpp` platform shims that point to the old architecture
- [ ] 8.4 Remove dead board/platform selection logic after the new build path is stable
- [ ] 8.5 Delete obsolete examples and docs that teach the wrong path

## 9. Foundational Runtime Completion

- [ ] 9.1 Close the new runtime path for `stm32g0`
- [ ] 9.2 Close the new runtime path for `stm32f4`
- [ ] 9.3 Close the new runtime path for `same70`
- [ ] 9.4 Verify that the runtime core remains vendor-light while supporting all three
- [ ] 9.5 Document vendor-4 admission criteria based on the new runtime architecture

### Gate R5: Foundational runtime completeness
- [ ] R5.1 foundational boards on `stm32g0`, `stm32f4`, and `same70` build on the new path
- [ ] R5.2 the runtime no longer requires a new public API shape to add another vendor

## 10. Validation

- [x] 10.1 Run `openspec validate rebuild-runtime-around-alloy-devices --strict`
- [x] 10.2 Add compile and smoke tests for the new device-import and driver layers
- [ ] 10.3 Update docs so the public story matches the rebuilt runtime
