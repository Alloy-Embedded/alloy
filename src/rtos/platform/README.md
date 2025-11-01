# RTOS Platform Support

Platform-specific context switching implementations for Alloy RTOS.

## Supported Platforms

### ARM Cortex-M (STM32F1, STM32F4, SAMD21, RP2040)

**Files**: `arm_context.hpp`, `arm_context.cpp`, `arm_systick_integration.cpp`

**Architecture**: ARM Cortex-M3/M4/M0+
**Context Switch Mechanism**: PendSV exception (lowest priority)
**Context Size**: 64 bytes (16 registers × 4 bytes)
**Switch Time**: ~5-10µs @ 72MHz

**Features**:
- Hardware-assisted context switch
- PendSV for deferred context switching
- Automatic save/restore of r0-r3, r12, lr, pc, xPSR by hardware
- Manual save/restore of r4-r11 by software
- Uses CLZ instruction for O(1) priority search

**Stack Layout**:
```
High memory
┌─────────────┐
│    xPSR     │ ← Hardware pushed
│     PC      │
│     LR      │
│     R12     │
│     R3      │
│     R2      │
│     R1      │
│     R0      │
├─────────────┤
│     R11     │ ← Software pushed
│     R10     │
│     R9      │
│     R8      │
│     R7      │
│     R6      │
│     R5      │
│     R4      │ ← SP points here
└─────────────┘
Low memory
```

### Xtensa (ESP32)

**Files**: `xtensa_context.hpp`, `xtensa_context.cpp`, `xtensa_systick_integration.cpp`

**Architecture**: Xtensa LX6 (dual-core)
**Context Switch Mechanism**: Software interrupt + esp_timer
**Context Size**: ~160 bytes (40+ registers)
**Switch Time**: ~10-20µs @ 240MHz

**Features**:
- Windowed registers (a0-a15)
- Register window overflow/underflow handling
- Uses esp_timer for periodic tick (1ms)
- Integrates with ESP-IDF timer system

**Stack Layout**:
```
High memory
┌─────────────┐
│     PC      │ ← Exception frame
│     PS      │
│    SAR      │
│   a0-a15    │
├─────────────┤
│  EXCCAUSE   │
│  EXCVADDR   │
│    LBEG     │
│    LEND     │
│   LCOUNT    │
└─────────────┘
Low memory
```

**Notes**:
- ESP32 has more complex register architecture than ARM
- Requires proper handling of register windows
- Uses ESP-IDF's timer infrastructure
- Compatible with ESP-IDF v4.x+

## Implementation Notes

### Adding New Platform Support

To add a new platform:

1. Create `<platform>_context.hpp` with:
   - `init_task_stack()` - Initialize task stack frame
   - `start_first_task()` - Start scheduler
   - `trigger_context_switch()` - Request context switch

2. Create `<platform>_context.cpp` with implementations

3. Create `<platform>_systick_integration.cpp` for timer integration

4. Update `rtos.hpp` to include your platform:
   ```cpp
   #elif defined(YOUR_PLATFORM)
       #include "rtos/platform/your_platform_context.hpp"
   ```

### Platform Requirements

Each platform must provide:
- Stack initialization for new tasks
- Context save/restore mechanism
- Timer interrupt (1ms tick rate)
- Atomic operations for critical sections

### Performance Targets

- Context switch: <10µs on ARM, <20µs on Xtensa
- Interrupt latency: <5µs
- Memory footprint: <100 bytes per task context

## Testing

### ARM Cortex-M

Test on real hardware:
```bash
# Build for STM32F103
cmake -B build -DALLOY_BOARD=bluepill
cmake --build build --target rtos_blink

# Flash to board
openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg \
    -c "program build/examples/rtos_blink.elf verify reset exit"
```

### ESP32

Test with ESP-IDF:
```bash
# Build for ESP32
idf.py build

# Flash to board
idf.py -p /dev/ttyUSB0 flash monitor
```

## References

- **ARM Cortex-M RTOS Guide**: ARM Application Note 321
- **FreeRTOS Kernel**: Context switch implementation
- **Xtensa ISA**: Tensilica Instruction Set Architecture Reference
- **ESP-IDF Programming Guide**: Timer and interrupt handling
