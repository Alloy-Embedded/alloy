## 1. OpenSpec Baseline

- [x] 1.1 Add runtime-validation and runtime-tooling deltas for foundational hardware closure
- [x] 1.2 Add runtime-release-discipline delta for silicon-backed foundational claims

## 2. STM32G0 Hardware Closure

- [x] 2.1 Recover the board if needed and stabilize the supported flash path
- [x] 2.2 Run the declared `stm32g0` validation bundle on hardware
- [x] 2.3 Record results in `tests/hardware/stm32g0/CHECKLIST.md`
- [x] 2.4 Convert any hardware-only failures into host-MMIO or emulation coverage where possible

## 3. STM32F4 Hardware Closure

- [ ] 3.1 Stabilize the supported flash/monitor path for `nucleo_f401re`
- [ ] 3.2 Run the declared `stm32f4` validation bundle on hardware
- [ ] 3.3 Record results in `tests/hardware/stm32f4/CHECKLIST.md`
- [x] 3.4 Convert any hardware-only failures into host-MMIO or emulation coverage where possible

## 4. Tooling Hardening For Real Boards

- [x] 4.1 Add supported recovery flows to `alloyctl` where the repo can guarantee them
- [x] 4.2 Document recovery/debug expectations in public board tooling docs
- [x] 4.3 Keep SAME70 and STM32 board flows aligned enough to be teachable as one product story

## 5. Release Claims

- [ ] 5.1 Update support matrix and release manifest with real hardware evidence
- [ ] 5.2 Downgrade any board claim that is not actually supported by current evidence

## 6. Validation

- [x] 6.1 Verify documented hardware flows still pass automated tooling checks where possible
- [x] 6.2 Validate the change with `openspec validate close-foundational-hardware-validation --strict`
