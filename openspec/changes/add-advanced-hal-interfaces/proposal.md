# Add Advanced HAL Interfaces (ADC, DMA, PWM, Timer, Clock)

## Why

Para criar aplicações embedded completas, precisamos de interfaces para:
- **ADC**: Leitura de sensores analógicos (temperatura, tensão, corrente)
- **PWM**: Controle de motores, LEDs, servos
- **Timer**: Medição de tempo, eventos periódicos, contadores
- **DMA**: Transferências de dados eficientes sem CPU
- **Clock**: Configuração de frequência do MCU, seleção de cristal interno/externo

Essas são interfaces fundamentais que toda aplicação embedded precisa.

## What Changes

### New Capabilities

1. **ADC Interface** (`hal-adc`)
   - Single channel and multi-channel reading
   - Continuous and single-shot modes
   - DMA-capable for efficient sampling
   - Reference voltage configuration

2. **PWM Interface** (`hal-pwm`)
   - Frequency and duty cycle control
   - Multiple channels support
   - Complementary outputs (for motor control)
   - Dead-time insertion

3. **Timer Interface** (`hal-timer`)
   - One-shot and periodic timers
   - Input capture for frequency measurement
   - Output compare for precise timing
   - Counter mode

4. **DMA Interface** (`hal-dma`)
   - Memory-to-memory transfers
   - Peripheral-to-memory transfers
   - Circular buffer mode
   - Transfer complete callbacks

5. **Clock Interface** (`hal-clock`)
   - PLL configuration
   - Internal/external oscillator selection
   - Peripheral clock enable/disable
   - Clock frequency queries

### Affected Specs

- New: `specs/hal-adc/spec.md`
- New: `specs/hal-pwm/spec.md`
- New: `specs/hal-timer/spec.md`
- New: `specs/hal-dma/spec.md`
- New: `specs/hal-clock/spec.md`

### Affected Code

- New: `src/hal/interface/adc.hpp`
- New: `src/hal/interface/pwm.hpp`
- New: `src/hal/interface/timer.hpp`
- New: `src/hal/interface/dma.hpp`
- New: `src/hal/interface/clock.hpp`
- Modified: `src/core/error.hpp` (new error codes)

## Impact

### Benefits

- ✅ Complete set of essential embedded interfaces
- ✅ Type-safe, modern C++ design
- ✅ Platform-agnostic (works on any MCU)
- ✅ Enables complex applications (motor control, data acquisition)
- ✅ DMA support for efficient transfers
- ✅ Flexible clock configuration

### Risks

- ⚠️ Clock configuration is MCU-specific (need abstraction)
- ⚠️ DMA channels vary greatly between vendors
- ⚠️ Timer features differ across architectures

### Mitigation

- Use concepts to define minimum interface
- Vendor-specific extensions for advanced features
- Clear documentation of limitations

## Dependencies

### Required

- Existing GPIO interface (for PWM output pins)
- Existing error handling system
- C++20 compiler

### Blocks

- Motor control drivers
- Sensor data acquisition systems
- Audio output
- Precise timing applications

## Alternatives Considered

### Alternative 1: Bare Metal Register Access

❌ **Rejected**: Not portable, error-prone, no type safety

### Alternative 2: C-Style HAL

❌ **Rejected**: No compile-time checks, manual memory management

### Alternative 3: Current Approach (C++20 Concepts)

✅ **Selected**: Type-safe, zero-overhead, portable

## Open Questions

1. **Clock Configuration Complexity**
   - How to abstract PLL configuration across vendors?
   - **Decision**: Provide high-level API (set_frequency) + low-level API (vendor-specific)

2. **DMA Channel Management**
   - How to allocate DMA channels?
   - **Decision**: Static allocation at compile-time

3. **Timer Prescaler Values**
   - How to calculate prescaler for desired frequency?
   - **Decision**: Provide helper functions, hide complexity

## Success Criteria

- [ ] All 5 interfaces compile cleanly
- [ ] Concepts enforce interface contracts
- [ ] Error codes cover all failure modes
- [ ] Documentation with usage examples
- [ ] Integrates with existing HAL architecture
- [ ] Zero-overhead compared to direct register access

## Timeline

**Estimated effort**: 2-3 days

1. Day 1: ADC, PWM interfaces
2. Day 2: Timer, DMA interfaces
3. Day 3: Clock interface, documentation
