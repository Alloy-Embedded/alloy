# Building Portable Applications with Alloy

Este guia explica como escrever aplicações que funcionam em **qualquer MCU** sem mudanças no código.

## Conceito

### ❌ Problema: Código Específico do Vendor

```cpp
// Código NÃO portável - preso ao STM32
#include "hal/st/stm32f1/gpio.hpp"

using namespace alloy::hal::st::stm32f1;
GpioPin<45> led;  // PC13 específico do BluePill
```

**Problemas:**
- ❌ Include path específico do vendor
- ❌ Namespace específico da família
- ❌ Pino hardcoded
- ❌ Não funciona em nRF, ESP32, RP2040, etc.

### ✅ Solução: Headers Genéricos

```cpp
// Código PORTÁVEL - funciona em qualquer MCU!
#include "alloy/hal/gpio.hpp"

using namespace alloy::hal;
GpioPin<LED_PIN> led;  // LED_PIN definido pelo CMake
```

**Benefícios:**
- ✅ Include path genérico
- ✅ Namespace portável
- ✅ Pino configurável
- ✅ Funciona em STM32, nRF, ESP32, RP2040, etc!

## Como Funciona

### 1. Aplicação Usa Headers Genéricos

```cpp
#include "alloy/hal/gpio.hpp"  // ← Generic
#include "alloy/hal/uart.hpp"  // ← Generic
```

### 2. CMake Configura Redirecionamento

```cmake
# Detecta MCU: STM32F103C8
detect_mcu_info(${ALLOY_MCU} vendor family mcu)
# → vendor=st, family=stm32f1, mcu=stm32f103c8

# Define redirecionamentos
target_compile_definitions(alloy_hal PUBLIC
    ALLOY_HAL_IMPL_GPIO="hal/st/stm32f1/gpio.hpp"
    ALLOY_HAL_IMPL_UART="hal/st/stm32f1/uart.hpp"
    ALLOY_HAL_VENDOR_NAMESPACE=alloy::hal::st::stm32f1
)
```

### 3. Header Genérico Redireciona

```cpp
// include/alloy/hal/gpio.hpp
#include ALLOY_HAL_IMPL_GPIO  // Expande para "hal/st/stm32f1/gpio.hpp"

namespace alloy::hal {
    using namespace ALLOY_HAL_VENDOR_NAMESPACE;  // alloy::hal::st::stm32f1
}
```

### 4. Compilador Usa Implementação Correta

```
alloy/hal/gpio.hpp → hal/st/stm32f1/gpio.hpp → alloy::hal::st::stm32f1::GpioPin
                  ↓
              alloy::hal::GpioPin (através de using)
```

## Exemplo Completo: Portable Blinky

### Código da Aplicação (PORTÁVEL)

```cpp
// examples/portable_blinky/main.cpp
#include "alloy/hal/gpio.hpp"
#include "alloy/hal/uart.hpp"

using namespace alloy::hal;

#ifndef LED_PIN
#define LED_PIN 13  // Default
#endif

int main() {
    GpioPin<LED_PIN> led;
    led.configure(PinMode::Output);

    while (true) {
        led.toggle();
        delay_ms(500);
    }
}
```

### Configuração CMake (POR BOARD)

```cmake
# examples/portable_blinky/CMakeLists.txt

# Board-specific configuration
if(ALLOY_MCU MATCHES "STM32F103")
    set(LED_PIN 45)  # PC13 BluePill
elseif(ALLOY_MCU MATCHES "nRF52")
    set(LED_PIN 13)  # P0.13 nRF52-DK
elseif(ALLOY_MCU MATCHES "RP2040")
    set(LED_PIN 25)  # GPIO25 Pico
endif()

add_executable(portable_blinky main.cpp)
target_link_libraries(portable_blinky alloy_hal)
target_compile_definitions(portable_blinky PRIVATE LED_PIN=${LED_PIN})
```

### Compilando para Diferentes MCUs

```bash
# Para STM32F103C8 (BluePill)
cmake -S . -B build-stm32 -DALLOY_BOARD=bluepill
cmake --build build-stm32
# LED pisca em PC13

# Para nRF52840
cmake -S . -B build-nrf -DALLOY_MCU=nRF52840
cmake --build build-nrf
# LED pisca em P0.13

# Para RP2040 (Raspberry Pi Pico)
cmake -S . -B build-pico -DALLOY_MCU=RP2040
cmake --build build-pico
# LED pisca em GPIO25

# Para ESP32
cmake -S . -B build-esp32 -DALLOY_MCU=ESP32
cmake --build build-esp32
# LED pisca em GPIO2
```

**Código da aplicação:** ZERO mudanças! 🎉

## Headers Genéricos Disponíveis

### GPIO
```cpp
#include "alloy/hal/gpio.hpp"

GpioPin<PIN_NUM> pin;
pin.configure(PinMode::Output);
pin.set_high();
pin.set_low();
pin.toggle();
bool state = pin.read();
```

### UART
```cpp
#include "alloy/hal/uart.hpp"

UartDevice<UsartId::USART1> serial;
serial.configure({baud_rates::Baud115200});
serial.write_byte('H');
auto byte = serial.read_byte();
```

### Futuros (em desenvolvimento)
```cpp
#include "alloy/hal/spi.hpp"
#include "alloy/hal/i2c.hpp"
#include "alloy/hal/adc.hpp"
#include "alloy/hal/pwm.hpp"
#include "alloy/hal/timer.hpp"
```

## Namespace Strategy

### Implementações Vendor-Specific

Cada vendor/família tem seu próprio namespace:

```cpp
namespace alloy::hal::st::stm32f1 {
    template<uint8_t PIN> class GpioPin { /*...*/ };
}

namespace alloy::hal::nordic::nrf52 {
    template<uint8_t PIN> class GpioPin { /*...*/ };
}

namespace alloy::hal::espressif::esp32 {
    template<uint8_t PIN> class GpioPin { /*...*/ };
}
```

### Header Genérico Une Tudo

```cpp
// include/alloy/hal/gpio.hpp
namespace alloy::hal {
    using namespace ALLOY_HAL_VENDOR_NAMESPACE;
    // Expande para alloy::hal::st::stm32f1 (ou qual vendor estiver ativo)
}
```

### Aplicação Usa Namespace Genérico

```cpp
using namespace alloy::hal;
GpioPin<LED_PIN> led;  // Resolve para vendor correto automaticamente!
```

## Validação em Compile-Time

A validação funciona mesmo com headers genéricos:

```cpp
#include "alloy/hal/gpio.hpp"

using namespace alloy::hal;

// Para STM32F103C8 (max 112 pinos)
GpioPin<45> led;   // ✅ OK (PC13 existe)
GpioPin<150> err;  // ❌ ERRO: "GPIO pin not available on this MCU"
```

O `static_assert` na implementação vendor-specific ainda funciona!

## Board Configuration Files

Crie arquivos de configuração para cada board:

```cmake
# cmake/boards/bluepill.cmake
set(ALLOY_BOARD "bluepill")
set(ALLOY_MCU "STM32F103C8")
set(ALLOY_ARCH "cortex-m3")

# Board-specific constants
set(LED_PIN 45)        # PC13
set(BUTTON_PIN 0)      # PA0
set(UART_DEBUG USART1) # Debug UART
```

```cmake
# cmake/boards/pico.cmake
set(ALLOY_BOARD "pico")
set(ALLOY_MCU "RP2040")
set(ALLOY_ARCH "cortex-m0plus")

set(LED_PIN 25)        # GPIO25 (onboard)
set(BUTTON_PIN 15)     # GPIO15
```

Depois use:
```bash
cmake -S . -B build -DALLOY_BOARD=bluepill
# ou
cmake -S . -B build -DALLOY_BOARD=pico
```

## Best Practices

### ✅ DO: Use Headers Genéricos

```cpp
#include "alloy/hal/gpio.hpp"
#include "alloy/hal/uart.hpp"
```

### ❌ DON'T: Include Vendor-Specific

```cpp
// NÃO faça isso em código de aplicação!
#include "hal/st/stm32f1/gpio.hpp"
```

### ✅ DO: Use Namespace Genérico

```cpp
using namespace alloy::hal;
GpioPin<LED_PIN> led;
```

### ❌ DON'T: Use Namespace Vendor

```cpp
// NÃO faça isso!
using namespace alloy::hal::st::stm32f1;
```

### ✅ DO: Configure Pins via CMake

```cmake
target_compile_definitions(my_app PRIVATE
    LED_PIN=45
    BUTTON_PIN=0
)
```

```cpp
GpioPin<LED_PIN> led;
GpioPin<BUTTON_PIN> button;
```

### ❌ DON'T: Hardcode Pins

```cpp
// NÃO faça isso!
GpioPin<45> led;  // Específico do BluePill
```

### ✅ DO: Use Conditional Features

```cpp
#ifdef HAS_UART
    UartDevice<UsartId::USART1> serial;
    // ...
#endif
```

### ✅ DO: Document Board Requirements

```cpp
/**
 * Required definitions from CMake:
 * - LED_PIN: GPIO pin for status LED
 * - BUTTON_PIN: GPIO pin for user button
 * - HAS_UART (optional): Enable UART debug output
 */
```

## Migration Guide

### Migrando Código Existente

**Antes (vendor-specific):**
```cpp
#include "hal/st/stm32f1/gpio.hpp"
#include "hal/st/stm32f1/uart.hpp"

using namespace alloy::hal::st::stm32f1;

GpioPin<45> led;  // PC13
UartDevice<UsartId::USART1> serial;
```

**Depois (portable):**
```cpp
#include "alloy/hal/gpio.hpp"
#include "alloy/hal/uart.hpp"

using namespace alloy::hal;

GpioPin<LED_PIN> led;
UartDevice<UsartId::USART1> serial;
```

**CMakeLists.txt:**
```cmake
target_compile_definitions(my_app PRIVATE LED_PIN=45)
```

## FAQ

### P: E se eu QUISER usar implementação vendor-specific?

R: Você pode! Basta incluir diretamente:
```cpp
#include "hal/st/stm32f1/gpio.hpp"
using namespace alloy::hal::st::stm32f1;
```

Mas perde a portabilidade.

### P: O header genérico tem overhead?

R: **Zero overhead**. É apenas um wrapper de namespace. O compilador inline tudo.

### P: Funciona com template metaprogramming?

R: Sim! Os tipos são exatamente os mesmos:
```cpp
#include "alloy/hal/gpio.hpp"
static_assert(std::is_same_v<
    alloy::hal::GpioPin<13>,
    alloy::hal::st::stm32f1::GpioPin<13>
>);  // TRUE quando compilando para STM32F1
```

### P: Como sei qual vendor está ativo?

R: Use macros do CMake:
```cpp
#ifdef ALLOY_VENDOR_ST
    // Código específico ST
#endif

#ifdef ALLOY_VENDOR_NORDIC
    // Código específico Nordic
#endif
```

### P: Posso misturar genérico e específico?

R: Sim, mas não recomendado:
```cpp
#include "alloy/hal/gpio.hpp"           // Genérico
#include "hal/st/stm32f1/advanced.hpp"  // Específico (features exclusivas ST)

alloy::hal::GpioPin<LED_PIN> led;                    // Genérico
alloy::hal::st::stm32f1::AdvancedTimer timer;  // Específico
```

Use apenas se a feature realmente não existir em outros vendors.

## Summary

✅ **Use headers genéricos**: `#include "alloy/hal/gpio.hpp"`
✅ **Configure pins no CMake**: `LED_PIN=45`
✅ **Use namespace portável**: `using namespace alloy::hal;`
✅ **Teste em múltiplos MCUs**: Garante verdadeira portabilidade
✅ **Zero overhead**: Inline, zero custo em runtime

Resultado: **Código que funciona em qualquer MCU com ZERO mudanças!** 🚀
