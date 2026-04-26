# public-hal-api Spec Delta: DAC Coverage Extension

## ADDED Requirements

### Requirement: DAC HAL SHALL Expose Per-Channel Addressing Where Published

The `alloy::hal::dac::handle<P>` MUST expose
`enable_channel(std::uint8_t)`,
`disable_channel(std::uint8_t)`,
`channel_ready(std::uint8_t) -> bool`, and
`write_channel(std::uint8_t, std::uint32_t)` whenever
`kChannelEnablePattern.valid`. Out-of-range channel ids MUST
return `core::ErrorCode::InvalidArgument`.

#### Scenario: STM32G0 DAC1 channel 2 writes independently of channel 1

- **WHEN** an application calls `dac.write_channel(1, 0x100)` and
  then `dac.write_channel(2, 0x200)` on DAC1 of `nucleo_g071rb`
- **THEN** both calls succeed and channels 1 / 2 hold their
  respective values

### Requirement: DAC HAL SHALL Expose Hardware Trigger / Prescaler / Software Reset Per Capability

The HAL MUST expose
`set_hardware_trigger(std::uint8_t source, TriggerEdge edge)`
(gated on `kHasHardwareTrigger`),
`set_prescaler(std::uint16_t)` (gated on `kPrescalerField.valid`),
and `software_reset()` (gated on `kSoftwareResetField.valid`).
`TriggerEdge` covers `Disabled / Rising / Falling / Both`.

#### Scenario: STM32G0 DAC1 triggers on TIM3 update edge

- **WHEN** an application calls
  `dac.set_hardware_trigger(/*TIM3_TRGO source idx*/ 4u,
  TriggerEdge::Rising)` on DAC1 of `nucleo_g071rb`
- **THEN** subsequent TIM3 update events trigger a DAC conversion
  without further `write` calls

### Requirement: DAC HAL SHALL Expose Typed Status / Interrupt Setters And IRQ Number List

The HAL MUST expose:

- `enum class InterruptKind { TransferComplete, Underrun,
  DmaComplete }`.
- `enable_interrupt(InterruptKind)` /
  `disable_interrupt(InterruptKind)` — per-kind gated.
- `underrun() -> bool`, `underrun_channel(std::uint8_t) -> bool`,
  `clear_underrun()`.
- `set_kernel_clock_source(KernelClockSource)` — gated on
  `kKernelClockSelectorField.valid`.
- `irq_numbers() -> std::span<const std::uint32_t>` returning
  `kIrqNumbers`.

#### Scenario: Underrun is observable per channel

- **WHEN** the application falls behind on writing channel 2 while
  the hardware trigger keeps firing
- **THEN** `dac.underrun_channel(2)` returns `true`
- **AND** `dac.clear_underrun()` returns `Ok` and the next
  `underrun()` call returns `false`

### Requirement: Async DAC Adapter SHALL Add wait_for(InterruptKind)

The runtime `async::dac` namespace MUST expose
`wait_for<P>(handle, InterruptKind kind)` so a coroutine can
`co_await` a TransferComplete / Underrun / DmaComplete event.

#### Scenario: Coroutine wakes on DMA-complete event

- **WHEN** a task awaits
  `async::dac::wait_for<DAC1>(dac, InterruptKind::DmaComplete)`
  while a DMA-driven sample stream is in flight
- **THEN** the task resumes when the DMA TC interrupt fires and
  the awaiter returns `Ok`
