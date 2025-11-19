# Add Multi-Vendor Clock Implementations and Board Support

## Why

Para validar nossa abstração HAL e permitir desenvolvimento real em múltiplas plataformas, precisamos:

1. **Implementações reais de clock** para diversos vendors:
   - STM32F103 (ARM Cortex-M3) - MCU muito popular, 72MHz max
   - ESP32 (Xtensa LX6) - Dual-core, WiFi/BT, até 240MHz
   - STM32F407 (ARM Cortex-M4F) - High performance, 168MHz max
   - ATSAMD21 (ARM Cortex-M0+) - Arduino Zero/MKR, 48MHz max
   - RP2040 (ARM Cortex-M0+) - Raspberry Pi Pico, dual-core, 133MHz

2. **Board support completo** para testar na prática:
   - Linker scripts (.ld) específicos para cada MCU
   - Startup code (reset handler, vector table)
   - Clock initialization
   - GPIO básico para blinkar LED
   - Build system integrado no CMake

3. **Objetivo**: Compilar e rodar blink LED em 5 placas diferentes para validar a base da lib

## What Changes

### New Implementations

#### 1. STM32F103 (Blue Pill / STM32F1 Discovery)
- **Clock**: 8MHz HSE → PLL×9 → 72MHz
- **Files**:
  - `src/hal/st/stm32f1/clock.hpp` - Clock implementation
  - `boards/stm32f103c8/board.hpp` - Board config
  - `boards/stm32f103c8/STM32F103C8.ld` - Linker script
  - `boards/stm32f103c8/startup.cpp` - Startup code
  - `examples/blink_stm32f103/main.cpp` - Blink example

#### 2. ESP32 (ESP32 DevKit)
- **Clock**: 40MHz XTAL → PLL → 160/240MHz CPU
- **Files**:
  - `src/hal/espressif/esp32/clock.hpp` - Clock implementation
  - `boards/esp32_devkit/board.hpp` - Board config
  - `boards/esp32_devkit/esp32.ld` - Linker script
  - `boards/esp32_devkit/startup.cpp` - Startup code (ESP-IDF minimal)
  - `examples/blink_esp32/main.cpp` - Blink example

#### 3. STM32F407 (STM32F4 Discovery)
- **Clock**: 8MHz HSE → PLL → 168MHz
- **Files**:
  - `src/hal/st/stm32f4/clock.hpp` - Clock implementation
  - `boards/stm32f407vg/board.hpp` - Board config
  - `boards/stm32f407vg/STM32F407VG.ld` - Linker script
  - `boards/stm32f407vg/startup.cpp` - Startup code
  - `examples/blink_stm32f407/main.cpp` - Blink example

#### 4. ATSAMD21 (Arduino Zero)
- **Clock**: 32kHz XOSC32K → DFLL48M → 48MHz
- **Files**:
  - `src/hal/microchip/samd21/clock.hpp` - Clock implementation
  - `boards/arduino_zero/board.hpp` - Board config
  - `boards/arduino_zero/ATSAMD21G18.ld` - Linker script
  - `boards/arduino_zero/startup.cpp` - Startup code
  - `examples/blink_arduino_zero/main.cpp` - Blink example

#### 5. RP2040 (Raspberry Pi Pico)
- **Clock**: 12MHz XOSC → PLL → 125-133MHz
- **Files**:
  - `src/hal/raspberry/rp2040/clock.hpp` - Clock implementation
  - `boards/pico/board.hpp` - Board config
  - `boards/pico/rp2040.ld` - Linker script (stage2 boot)
  - `boards/pico/startup.cpp` - Startup code
  - `examples/blink_pico/main.cpp` - Blink example

### Affected Specs

- New: `specs/clock-stm32f1/spec.md`
- New: `specs/clock-esp32/spec.md`
- New: `specs/clock-stm32f4/spec.md`
- New: `specs/clock-samd21/spec.md`
- New: `specs/clock-rp2040/spec.md`
- New: `specs/board-support/spec.md`

### Affected Code

- New: Clock implementations for 5 vendors (using hal-clock interface)
- New: 5 board definitions with linker scripts and startup code
- New: 5 blink examples (one per board)
- Modified: `CMakeLists.txt` - add board selection
- Modified: `boards/CMakeLists.txt` - build system for boards

## Impact

### Benefits

- ✅ Validação prática da abstração HAL em hardware real
- ✅ Suporte a 5 plataformas populares (ARM Cortex-M0+, M3, M4F, Xtensa)
- ✅ Exemplos funcionais de como usar a lib
- ✅ Base para adicionar mais vendors no futuro
- ✅ Linker scripts e startup code reutilizáveis
- ✅ Build system robusto com seleção de board

### Risks

- ⚠️ ESP32 requer ESP-IDF SDK (dependência externa grande)
- ⚠️ Linker scripts são específicos para cada MCU (flash/RAM layout)
- ⚠️ Startup code precisa inicializar .data, .bss, chamadas de construtores
- ⚠️ RP2040 requer second-stage bootloader

### Mitigation

- ESP32: Usar configuração mínima do ESP-IDF, ou implementar do zero
- Linker scripts: Templates bem documentados, fáceis de adaptar
- Startup code: Seguir padrão CMSIS, bem estabelecido
- RP2040: Usar boot2 do pico-sdk ou implementar próprio

## Dependencies

### Required

- Existing clock interface (`src/hal/interface/clock.hpp`)
- Existing GPIO interface (para blinkar LED)
- CMake 3.25+
- Toolchains:
  - `arm-none-eabi-gcc` (para ARM Cortex-M)
  - `xtensa-esp32-elf-gcc` (para ESP32)

### Blocks

- Drivers mais complexos (UART, I2C, SPI) que dependem de clock configurado
- Aplicações reais em embedded
- Validação de performance e power consumption

## Alternatives Considered

### Alternative 1: Usar HALs dos vendors (STM32Cube, ESP-IDF, etc)

❌ **Rejected**: Perde controle, dependências pesadas, não portável

### Alternative 2: Implementar apenas 1 ou 2 MCUs

❌ **Rejected**: Não valida suficientemente a abstração multi-vendor

### Alternative 3: Abordagem Atual (5 MCUs, do zero)

✅ **Selected**: Valida abstração, controle total, exemplos práticos

## Open Questions

1. **ESP32 SDK Dependency**
   - Implementar do zero ou usar ESP-IDF mínimo?
   - **Decision**: Usar ESP-IDF mínimo para clock/GPIO, evitar componentes pesados

2. **Linker Script Templates**
   - Como tornar linker scripts reutilizáveis?
   - **Decision**: Templates com variáveis (FLASH_SIZE, RAM_SIZE), gerar via CMake

3. **Board Selection**
   - Como selecionar board no build?
   - **Decision**: CMake option `-DALLOY_BOARD=stm32f103c8`, define macros

4. **RP2040 Boot2**
   - Implementar second-stage bootloader ou usar do pico-sdk?
   - **Decision**: Copiar boot2 mínimo do pico-sdk (bem testado)

## Success Criteria

- [x] Clock implementations compilam para todos 5 MCUs
- [x] Linker scripts alocam memória corretamente
- [x] Startup code inicializa .data/.bss e chama main()
- [x] Blink LED funciona em todas 5 placas
- [x] Build system permite selecionar board facilmente
- [x] Documentação de como compilar e flash cada board
- [x] Zero warnings no build
- [x] Código portável (mesma aplicação roda em qualquer board)

## Timeline

**Estimated effort**: 5-7 days

1. Day 1-2: STM32F103 + STM32F407 (ARM Cortex-M similar)
2. Day 3: ATSAMD21 (ARM Cortex-M0+)
3. Day 4: RP2040 (Cortex-M0+ com boot2)
4. Day 5: ESP32 (arquitetura diferente)
5. Day 6-7: Testing, docs, polimento
