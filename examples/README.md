# Alloy Framework - SAME70 Examples

Este diretório contém exemplos para validar diferentes funcionalidades da placa SAME70 Xplained Ultra.

## Ordem de Teste Recomendada

### 1. GPIO Blink (blink_led)
**Objetivo**: Validar GPIO e Clock básico
- Usa board abstraction layer
- Apenas GPIO toggle com busy-wait
- SEM SysTick

**Build**:
```bash
make same70-blink-generic-build
```

**Flash**:
```bash
./scripts/flash_blink_led.sh
```

**Comportamento esperado**:
- LED pisca continuamente
- Timing não é preciso (busy-wait)

---

### 2. Clock Test (same70_clock_test)
**Objetivo**: Validar diferentes configurações de clock
- Testa RC 12MHz, RC 4MHz, Crystal, PLL
- Usa HAL direto (não usa board abstraction)
- SEM SysTick

**Build**:
```bash
cd examples/same70_clock_test
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBOARD=same70_xplained \
      -DCMAKE_TOOLCHAIN_FILE=../../../cmake/toolchains/arm-none-eabi.cmake \
      ..
make
```

**Flash**:
```bash
bossac --port=/dev/cu.usbmodem* --erase --write --verify --boot=1 --reset build/clock_test.bin
```

**Comportamento esperado**:
- 5 blinks rápidos = Clock OK
- Blink contínuo = Loop principal

---

### 3. SysTick Test (same70_systick_test)
**Objetivo**: Validar SysTick timer e delays precisos
- Testa Clock + SysTick integration
- Usa delay_ms() / delay_us()
- Usa HAL direto (não usa board abstraction)

**Build**:
```bash
cd examples/same70_systick_test
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBOARD=same70_xplained \
      -DCMAKE_TOOLCHAIN_FILE=../../../cmake/toolchains/arm-none-eabi.cmake \
      ..
make
```

**Flash**:
```bash
bossac --port=/dev/cu.usbmodem* --erase --write --verify --boot=1 --reset build/systick_test.bin
```

**Comportamento esperado**:
- 2 blinks = Startup
- 3 blinks médios = Clock OK
- 1 blink com busy-wait = Teste antes do SysTick
- 3 blinks rápidos = SysTick delay_ms() funcionando
- 5 blinks médios = Delays precisos OK
- Blink 1Hz preciso = Loop principal (250ms ON, 750ms OFF)

---

## Estrutura dos Exemplos

### Board Abstraction (blink_led)
```cpp
#include "same70_xplained/board.hpp"

int main() {
    board::init(board::ClockPreset::Clock12MHz);

    while (true) {
        board::led::toggle();
        busy_delay();  // Não usa SysTick
    }
}
```

### HAL Direto (clock_test, systick_test)
```cpp
#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/systick.hpp"

int main() {
    // Configurar clock
    Clock::initialize(config);

    // Configurar GPIO manualmente
    Clock::enablePeripheralClock(id::PIOC);

    // Inicializar SysTick (apenas no systick_test)
    SystemTick::init();

    // Usar delays precisos
    delay_ms(500);
}
```

## Notas

1. **blink_led**: Exemplo mais simples, usa board abstraction, SEM timing preciso
2. **same70_clock_test**: Valida diferentes configurações de clock
3. **same70_systick_test**: Valida timing preciso com SysTick + delay_ms()

## Troubleshooting

### LED não pisca
- Verificar se a placa está alimentada
- Verificar se o código foi gravado corretamente
- Tentar reset manual

### Timing impreciso no blink_led
- Normal! Este exemplo usa busy-wait, não SysTick
- Use `same70_systick_test` para timing preciso

### SysTick não funciona
- Verificar se Clock foi inicializado primeiro
- Verificar se SysTick_Handler está linkado corretamente
- Usar debugger para verificar se `millis_counter_` está incrementando
