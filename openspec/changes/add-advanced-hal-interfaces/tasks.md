# Implementation Tasks

## 1. ADC Interface

- [x] 1.1 Create `src/hal/interface/adc.hpp`
- [x] 1.2 Define AdcConfig struct (resolution, reference voltage, sample time)
- [x] 1.3 Define AdcChannel concept
- [x] 1.4 Add read_single() for single conversion
- [x] 1.5 Add read_continuous() for continuous mode
- [x] 1.6 Add read_multi_channel() for scanning
- [x] 1.7 Add configure() for ADC parameters
- [x] 1.8 Add calibrate() for accuracy
- [x] 1.9 Add start_dma() and stop_dma() for DMA transfers

## 2. PWM Interface

- [x] 2.1 Create `src/hal/interface/pwm.hpp`
- [x] 2.2 Define PwmConfig struct (frequency, resolution)
- [x] 2.3 Define PwmChannel concept
- [x] 2.4 Add set_duty_cycle() (0-100% or absolute value)
- [x] 2.5 Add set_frequency()
- [x] 2.6 Add start() and stop() channel
- [x] 2.7 Add set_polarity() (normal/inverted)
- [x] 2.8 Add set_dead_time() for complementary outputs
- [x] 2.9 Add configure() for PWM parameters

## 3. Timer Interface

- [x] 3.1 Create `src/hal/interface/timer.hpp`
- [x] 3.2 Define TimerConfig struct (mode, prescaler, period)
- [x] 3.3 Define TimerMode enum (one-shot, periodic, PWM, input capture)
- [x] 3.4 Define Timer concept
- [x] 3.5 Add start() and stop()
- [x] 3.6 Add get_counter() and set_counter()
- [x] 3.7 Add set_period() for interval changes
- [x] 3.8 Add configure_capture() for input capture mode
- [x] 3.9 Add configure_compare() for output compare mode
- [x] 3.10 Add configure() for timer parameters

## 4. DMA Interface

- [x] 4.1 Create `src/hal/interface/dma.hpp`
- [x] 4.2 Define DmaConfig struct (direction, mode, priority)
- [x] 4.3 Define DmaDirection enum (mem-to-mem, mem-to-periph, periph-to-mem)
- [x] 4.4 Define DmaMode enum (normal, circular)
- [x] 4.5 Define DmaChannel concept
- [x] 4.6 Add configure() for DMA parameters
- [x] 4.7 Add start_transfer() for initiating transfer
- [x] 4.8 Add stop_transfer()
- [x] 4.9 Add get_remaining_count() for progress
- [x] 4.10 Add is_complete() and is_error()
- [x] 4.11 Add set_callback() for transfer complete notification

## 5. Clock Interface

- [x] 5.1 Create `src/hal/interface/clock.hpp`
- [x] 5.2 Define ClockSource enum (internal RC, external crystal, PLL)
- [x] 5.3 Define ClockConfig struct (source, PLL multiplier/divider)
- [x] 5.4 Define SystemClock concept
- [x] 5.5 Add configure_system_clock() for main clock setup
- [x] 5.6 Add get_system_frequency() to query current speed
- [x] 5.7 Add enable_peripheral_clock() for peripheral clocks
- [x] 5.8 Add disable_peripheral_clock()
- [x] 5.9 Add get_peripheral_frequency() for specific peripheral
- [x] 5.10 Add configure_pll() for advanced PLL setup
- [x] 5.11 Add set_flash_latency() for high-speed operation

## 6. Error Codes

- [x] 6.1 Add ADC-specific errors (calibration failed, overrun)
- [x] 6.2 Add DMA-specific errors (transfer error, alignment error)
- [x] 6.3 Add Clock-specific errors (PLL lock failed, invalid frequency)

## 7. Helper Functions

- [x] 7.1 Add ADC value conversion helpers (raw to voltage, raw to percentage)
- [x] 7.2 Add PWM duty cycle conversion (percentage to ticks)
- [x] 7.3 Add Timer frequency calculation helpers
- [x] 7.4 Add Clock prescaler calculation helpers

## 8. Documentation

- [ ] 8.1 Write usage examples for ADC (single, continuous, DMA)
- [ ] 8.2 Write usage examples for PWM (motor control, LED dimming)
- [ ] 8.3 Write usage examples for Timer (periodic callback, input capture)
- [ ] 8.4 Write usage examples for DMA (buffer transfer, circular mode)
- [ ] 8.5 Write usage examples for Clock (switch to external crystal, PLL setup)
- [ ] 8.6 Document performance characteristics
- [ ] 8.7 Document vendor-specific limitations
