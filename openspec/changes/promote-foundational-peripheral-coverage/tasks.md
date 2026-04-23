## 1. OpenSpec Baseline

- [ ] 1.1 Add public-hal-api, runtime-validation, and runtime-release-discipline deltas

## 2. Promote Representative Peripheral Classes

- [ ] 2.1 Define explicit promotion criteria per peripheral class
- [ ] 2.2 Close `i2c` promotion if current evidence supports it
- [ ] 2.3 Close `spi` promotion if current evidence supports it
- [ ] 2.4 Close `timer` and `pwm` promotion if current evidence supports them
- [ ] 2.5 Close `rtc` and `watchdog` promotion if current evidence supports them
- [ ] 2.6 Close `adc` and `dac` promotion if current evidence supports them
- [ ] 2.7 Close `low-power` promotion if current evidence supports it

## 3. CAN Closure

- [ ] 3.1 Add loopback or equivalent deterministic CAN traffic validation
- [ ] 3.2 Add an official example or test path that proves CAN beyond bring-up
- [ ] 3.3 Promote `can` only if the new evidence justifies it

## 4. Release Claims

- [ ] 4.1 Update `docs/SUPPORT_MATRIX.md`
- [ ] 4.2 Update `docs/RELEASE_MANIFEST.json`
- [ ] 4.3 Keep examples and board docs aligned with the promoted tiers

## 5. Validation

- [ ] 5.1 Re-run foundational host/emulation/hardware gates affected by promoted classes
- [ ] 5.2 Validate the change with `openspec validate promote-foundational-peripheral-coverage --strict`
