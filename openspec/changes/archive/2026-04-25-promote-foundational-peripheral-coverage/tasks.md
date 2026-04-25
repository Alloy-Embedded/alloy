## 1. OpenSpec Baseline

- [x] 1.1 Add public-hal-api, runtime-validation, and runtime-release-discipline deltas

## 2. Promote Representative Peripheral Classes

- [x] 2.1 Define explicit promotion criteria per peripheral class
      Criteria: descriptor smoke + host-MMIO + 3-board hardware spot-check + foundational-example-coverage
- [ ] 2.2 Close `i2c` promotion — blocked: only SAME70 hardware evidence, no STM32G0/F4 i2c_scan run
- [ ] 2.3 Close `spi` promotion — blocked: only SAME70 hardware evidence, no STM32G0/F4 spi_probe run
- [x] 2.4 Close `timer` and `pwm` — promoted to `foundational`: 3-board hardware (SAME70/STM32G0/STM32F4) + host-MMIO (TC0/PWM0 paths in same70 bringup test)
- [x] 2.5 Close `rtc` and `watchdog` — promoted to `foundational`: 3-board hardware (SAME70/STM32G0/STM32F4) + host-MMIO already gated
- [x] 2.6 Close `adc` — promoted to `foundational`: 3-board hardware (SAME70/STM32G0/STM32F4 analog_probe) + host-MMIO (AFEC path)
      `dac` blocked: only SAME70 evidence; remains `representative`
- [ ] 2.7 Close `low-power` — blocked: only host-MMIO evidence, no multi-board hardware proof; remains `representative`

## 3. CAN Closure

- [ ] 3.1 Add loopback or equivalent deterministic CAN traffic validation
- [ ] 3.2 Add an official example or test path that proves CAN beyond bring-up
- [ ] 3.3 Promote `can` only if the new evidence justifies it
      Note: SAME70 bring-up/loop validated; deterministic traffic still missing; stays `experimental`

## 4. Release Claims

- [x] 4.1 Update `docs/SUPPORT_MATRIX.md`
- [x] 4.2 Update `docs/RELEASE_MANIFEST.json`
- [x] 4.3 Examples and board docs already aligned (timer_pwm_probe/rtc_probe/watchdog_probe/analog_probe are canonical)

## 5. Validation

- [ ] 5.1 Re-run foundational host/emulation/hardware gates affected by promoted classes (requires CI/hardware)
- [ ] 5.2 Validate the change with `openspec validate promote-foundational-peripheral-coverage --strict`
