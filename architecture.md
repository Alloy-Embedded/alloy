# Alloy: Arquitetura Técnica Detalhada

**"The modern C++20 framework for bare-metal embedded systems"**

## 1. Visão Geral

Alloy é um framework C++20 para sistemas embarcados bare-metal, projetado para ser:
- **Moderno**: Uso de C++20 Concepts, Ranges, constexpr/consteval
- **Eficiente**: Zero overhead na HAL, otimizações em compile-time
- **Testável**: Arquitetura em camadas com injeção de dependências
- **Pragmático**: CMake puro, implementação do zero, sem vendor SDKs
- **Compatível**: Headers tradicionais (não C++ Modules), funciona com GCC 11+

## 2. Estrutura de Diretórios

```
alloy/
├── cmake/                      # Scripts CMake reutilizáveis
│   ├── toolchains/            # Toolchain files (arm-none-eabi, host, etc)
│   ├── boards/                # Definições de boards (*.cmake)
│   ├── modules.cmake          # Helpers para C++20 modules
│   └── codegen.cmake          # Integração com gerador de código
│
├── src/                       # Código fonte do framework
│   ├── core/                  # Primitivos fundamentais
│   │   ├── types.hpp         # Tipos básicos, traits
│   │   ├── concepts.hpp      # Concepts reutilizáveis
│   │   └── units.hpp         # Type-safe units (frequency, voltage, etc)
│   │
│   ├── hal/                   # Hardware Abstraction Layer
│   │   ├── interface/         # Interfaces abstratas (concepts)
│   │   │   ├── gpio.hpp
│   │   │   ├── uart.hpp
│   │   │   ├── i2c.hpp
│   │   │   ├── spi.hpp
│   │   │   └── timer.hpp
│   │   │
│   │   ├── rp2040/           # Implementação para RP2040
│   │   │   ├── gpio.hpp      # Interface pública
│   │   │   ├── gpio.cpp      # Implementação
│   │   │   ├── uart.hpp
│   │   │   ├── uart.cpp
│   │   │   └── ...
│   │   │
│   │   ├── stm32f4/          # Implementação para STM32F4
│   │   │   ├── gpio.hpp
│   │   │   ├── gpio.cpp
│   │   │   └── ...
│   │   │
│   │   └── host/             # Mock para testes (simula hardware)
│   │       ├── gpio.hpp
│   │       ├── gpio.cpp
│   │       └── ...
│   │
│   ├── drivers/              # Drivers para periféricos externos
│   │   ├── display/
│   │   │   ├── ssd1306.hpp   # OLED I2C
│   │   │   └── ssd1306.cpp
│   │   ├── sensor/
│   │   │   ├── bme280.hpp    # Temp/Humidity I2C
│   │   │   └── bme280.cpp
│   │   └── ...
│   │
│   └── platform/             # Código gerado e específico de plataforma
│       └── (código gerado)    # Registradores, startup, linker scripts
│
├── tools/                    # Ferramentas internas (não expostas ao usuário)
│   ├── codegen/              # Gerador de código
│   │   ├── generator.py      # Script principal
│   │   ├── svd_parser.py     # Parser de arquivos SVD
│   │   ├── templates/        # Templates Jinja2 para geração
│   │   │   ├── startup.cpp.j2
│   │   │   ├── registers.hpp.j2
│   │   │   └── linker.ld.j2
│   │   └── database/         # Database de MCUs
│   │       ├── rp2040.json
│   │       ├── stm32f4xx.json
│   │       └── svd/          # Arquivos SVD originais
│   │           └── ...
│   │
│   └── cli/                  # CLI helper (futuro - Phase 2)
│       └── alloy_cli.py
│
├── examples/                 # Exemplos de uso
│   ├── blinky/
│   ├── uart_echo/
│   ├── i2c_sensor/
│   └── ...
│
├── tests/                    # Testes automatizados
│   ├── unit/                 # Testes unitários (host)
│   ├── integration/          # Testes de integração (hardware)
│   └── mocks/                # HAL mocks para testes
│
└── docs/                     # Documentação
    ├── getting_started.md
    ├── api_reference/
    └── tutorials/

```

## 3. Arquitetura em Camadas

```
┌─────────────────────────────────────────┐
│      User Application Layer            │  ← Código do usuário
│  (main.cpp, business logic)            │
└─────────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────┐
│      Drivers Layer                      │  ← Drivers agnósticos
│  (SSD1306, BME280, SD Card, etc)       │     à plataforma
└─────────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────┐
│      HAL Interface Layer                │  ← Concepts e interfaces
│  (GPIO, UART, I2C, SPI concepts)       │     abstratas
└─────────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────┐
│      HAL Implementation Layer           │  ← Implementações
│  (rp2040/, stm32f4/, host/)            │     específicas
└─────────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────┐
│      Platform Layer (Generated)         │  ← Código gerado
│  (registers, startup, linker scripts)   │     automaticamente
└─────────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────┐
│      Hardware                           │  ← MCU físico
└─────────────────────────────────────────┘
```

### Princípios de Design

1. **Dependency Inversion**: Camadas superiores dependem de abstrações (concepts), não de implementações concretas
2. **Zero Cost Abstraction**: Templates e `constexpr` garantem que não há overhead em runtime
3. **Compile-time Configuration**: Tudo que pode ser resolvido em compile-time, será
4. **Mockable by Design**: Todas as HALs podem ser substituídas por mocks para testes

## 4. Sistema de Headers e Namespaces

### Estrutura de um Header de Interface

```cpp
// src/hal/interface/gpio.hpp
#pragma once

#include "core/types.hpp"
#include "core/concepts.hpp"

namespace alloy::hal {

// Concept que define o que é um GPIO pin
template<typename T>
concept GpioPin = requires(T pin) {
    { pin.set_high() } -> std::same_as<void>;
    { pin.set_low() } -> std::same_as<void>;
    { pin.toggle() } -> std::same_as<void>;
    { pin.read() } -> std::same_as<bool>;
    { pin.configure(PinMode{}) } -> std::same_as<void>;
};

// Enum seguro para modos de operação
enum class PinMode : uint8_t {
    Input,
    Output,
    InputPullUp,
    InputPullDown,
    Alternate,
    Analog
};

// Interface base (opcional, para type erasure se necessário)
class IGpioPin {
public:
    virtual ~IGpioPin() = default;
    virtual void set_high() = 0;
    virtual void set_low() = 0;
    virtual void toggle() = 0;
    virtual bool read() const = 0;
    virtual void configure(PinMode mode) = 0;
};

} // namespace alloy::hal
```

### Implementação Específica (RP2040)

```cpp
// src/hal/rp2040/gpio.hpp
#pragma once

#include "hal/interface/gpio.hpp"
#include <cstdint>

namespace alloy::hal::rp2040 {

template<uint32_t PIN_NUM>
class GpioPin {
    static_assert(PIN_NUM < 30, "RP2040 has only 30 GPIO pins");

public:
    constexpr GpioPin() = default;

    void initialize() {
        // Configurar GPIO como saída/entrada diretamente nos registradores
        // (sem usar Pico SDK - implementação própria)
        configure_gpio_function(PIN_NUM, GPIO_FUNC_SIO);
    }

    void set_high() {
        // Acesso direto aos registradores do RP2040
        sio_hw->gpio_set = (1u << PIN_NUM);
    }

    void set_low() {
        sio_hw->gpio_clr = (1u << PIN_NUM);
    }

    void toggle() {
        sio_hw->gpio_togl = (1u << PIN_NUM);
    }

    bool read() const {
        return (sio_hw->gpio_in & (1u << PIN_NUM)) != 0;
    }

    void configure(PinMode mode) {
        switch (mode) {
            case PinMode::Output:
                // Acesso direto aos registradores SIO
                sio_hw->gpio_oe_set = (1u << PIN_NUM);
                break;
            case PinMode::Input:
                sio_hw->gpio_oe_clr = (1u << PIN_NUM);
                // Desabilitar pulls via PAD control
                padsbank0_hw->io[PIN_NUM] &= ~(PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_PDE_BITS);
                break;
            case PinMode::InputPullUp:
                sio_hw->gpio_oe_clr = (1u << PIN_NUM);
                padsbank0_hw->io[PIN_NUM] |= PADS_BANK0_GPIO0_PUE_BITS;
                padsbank0_hw->io[PIN_NUM] &= ~PADS_BANK0_GPIO0_PDE_BITS;
                break;
            // ... outros modos
        }
    }

private:
    static constexpr uint32_t pin_number = PIN_NUM;

    // Helpers para acesso aos registradores (implementados no .cpp)
    static void configure_gpio_function(uint32_t pin, uint32_t func);
};

// Validação em compile-time de que implementa o concept
static_assert(GpioPin<GpioPin<25>>);

} // namespace alloy::hal::rp2040
```

### Mock para Testes (Host)

```cpp
// src/hal/host/gpio.hpp
#pragma once

#include "hal/interface/gpio.hpp"
#include <iostream>

namespace alloy::hal::host {

template<uint32_t PIN_NUM>
class GpioPin {
public:
    void set_high() {
        state_ = true;
        std::cout << "[GPIO Mock] Pin " << PIN_NUM << " set HIGH\n";
    }

    void set_low() {
        state_ = false;
        std::cout << "[GPIO Mock] Pin " << PIN_NUM << " set LOW\n";
    }

    void toggle() {
        state_ = !state_;
        std::cout << "[GPIO Mock] Pin " << PIN_NUM << " toggled to "
                  << (state_ ? "HIGH" : "LOW") << "\n";
    }

    bool read() const { return state_; }

    void configure(PinMode mode) {
        mode_ = mode;
        std::cout << "[GPIO Mock] Pin " << PIN_NUM << " configured\n";
    }

private:
    bool state_ = false;
    PinMode mode_ = PinMode::Input;
};

} // namespace alloy::hal::host
```

## 5. Integração com CMSIS

### O que é CMSIS?

**CMSIS** (Cortex Microcontroller Software Interface Standard) é um padrão da ARM que fornece:
- Definições de registradores de periféricos
- Funções de acesso ao core (NVIC, SysTick, etc)
- Estruturas padronizadas para todos os MCUs ARM Cortex

### Como Usaremos CMSIS

```cpp
// Para STM32F4, incluímos o CMSIS header oficial
#include "stm32f4xx.h"  // Fornecido pela ST/ARM

// Exemplo de uso na implementação STM32F4 GPIO:
namespace alloy::hal::stm32f4 {

template<uint32_t PIN>
class GpioPin {
public:
    void set_high() {
        // GPIOA é definido pelo CMSIS header como:
        // #define GPIOA ((GPIO_TypeDef *) GPIOA_BASE)

        constexpr auto port = get_port_from_pin(PIN);
        constexpr auto pin_mask = get_pin_mask(PIN);

        // Usar registrador BSRR (Bit Set/Reset Register)
        // Definido em CMSIS como parte de GPIO_TypeDef
        port->BSRR = pin_mask;  // Set bit
    }

    void set_low() {
        constexpr auto port = get_port_from_pin(PIN);
        constexpr auto pin_mask = get_pin_mask(PIN);

        port->BSRR = (pin_mask << 16);  // Reset bit
    }

private:
    static constexpr GPIO_TypeDef* get_port_from_pin(uint32_t pin) {
        // PA0-PA15: pins 0-15
        // PB0-PB15: pins 16-31
        // etc...
        if (pin < 16) return GPIOA;
        if (pin < 32) return GPIOB;
        if (pin < 48) return GPIOC;
        // ...
    }

    static constexpr uint32_t get_pin_mask(uint32_t pin) {
        return 1u << (pin % 16);
    }
};

} // namespace alloy::hal::stm32f4
```

### Estrutura de CMSIS Headers

```
external/cmsis/
├── CMSIS/Core/Include/          # Core ARM headers
│   ├── core_cm0plus.h          # Cortex-M0+ (RP2040)
│   ├── core_cm4.h              # Cortex-M4 (STM32F4)
│   └── ...
│
├── Device/ST/STM32F4xx/Include/ # Device specific (STM32)
│   ├── stm32f4xx.h             # Main device header
│   ├── stm32f446xx.h           # Specific to F446
│   └── system_stm32f4xx.h      # System configuration
│
└── Device/Raspberry/RP2040/     # Device specific (RP2040)
    ├── rp2040.h                # Registradores do RP2040
    └── ...
```

### Vantagens de Usar CMSIS

1. **Padrão da indústria**: Todos os fabricantes ARM suportam
2. **Bem documentado**: Cada registrador tem documentação oficial
3. **Tipos corretos**: Structs com tipos e offsets corretos
4. **Constantemente atualizado**: Fabricantes mantêm os headers
5. **Zero overhead**: São apenas definições, sem código executável

### Nossa Camada sobre CMSIS

```cpp
// Nossa HAL abstrai CMSIS com uma API consistente

// Usuário escreve (independente de plataforma):
#include "hal/gpio.hpp"
using namespace alloy::hal;

auto led = make_gpio_pin<LED_PIN, PinMode::Output>();
led.set_high();

// Internamente, para STM32:
// - usa CMSIS para acessar GPIOA->BSRR
// - nossa implementação garante API consistente

// Internamente, para RP2040:
// - usa estruturas do rp2040.h (similar ao CMSIS)
// - mesma API para o usuário!
```

## 6. Sistema de Boards

### Board Definition File (CMake)

```cmake
# cmake/boards/rp_pico.cmake

# Identificação da placa
set(ALLOY_BOARD_NAME "Raspberry Pi Pico" CACHE STRING "Board name")
set(ALLOY_MCU "RP2040" CACHE STRING "MCU model")
set(ALLOY_ARCH "cortex-m0plus" CACHE STRING "CPU architecture")

# Configurações de clock
set(ALLOY_CLOCK_FREQ_HZ 125000000 CACHE STRING "System clock frequency")

# Periféricos disponíveis
set(ALLOY_HAS_GPIO ON)
set(ALLOY_HAS_UART ON)
set(ALLOY_HAS_I2C ON)
set(ALLOY_HAS_SPI ON)
set(ALLOY_HAS_ADC ON)
set(ALLOY_HAS_PWM ON)

# Pinout especial (LED onboard, etc)
set(ALLOY_LED_PIN 25 CACHE STRING "Onboard LED pin")

# Memória
set(ALLOY_FLASH_SIZE "2M" CACHE STRING "Flash size")
set(ALLOY_RAM_SIZE "264K" CACHE STRING "RAM size")

# Toolchain específico
set(ALLOY_TOOLCHAIN "arm-none-eabi" CACHE STRING "Toolchain to use")

# Linker script (pode ser gerado ou customizado)
set(ALLOY_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../linker/rp2040.ld")

# SDK adicional (opcional)
option(ALLOY_USE_PICO_SDK "Use official Pico SDK for low-level operations" ON)
if(ALLOY_USE_PICO_SDK)
    set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../../external/pico-sdk"
        CACHE PATH "Path to Pico SDK")
endif()
```

### Uso no Projeto do Usuário

```cmake
# CMakeLists.txt do projeto do usuário
cmake_minimum_required(VERSION 3.25)

# Definir a placa ANTES de incluir o framework
set(ALLOY_BOARD "rp_pico")

# Incluir o framework
add_subdirectory(external/alloy)

project(my_robot CXX)

# Criar executável
add_executable(my_robot
    src/main.cpp
)

# Linkar com o framework
target_link_libraries(my_robot PRIVATE
    alloy::hal::gpio
    alloy::hal::uart
    alloy::drivers::ssd1306
)

# O CMake do Alloy já configurou tudo:
# - Compilador correto
# - Flags de otimização
# - Linker script
# - Código gerado (se necessário)
```

## 6. Sistema de Geração de Código

### Fluxo de Geração

```
User defines:                 CMake detects:              Generator runs:
ALLOY_MCU="STM32F446RE"   → Is code generated?    →   tools/codegen/generator.py
                             → No? Invoke generator!

                                                      ↓

Database lookup:              Template processing:        Output:
database/stm32f4xx.json   →   templates/*.j2         →   build/generated/
                                                          ├── startup.cpp
                                                          ├── registers.hpp
                                                          ├── vectors.cpp
                                                          └── stm32f446re.ld
```

### CMake Integration

```cmake
# cmake/codegen.cmake

function(alloy_generate_platform_code)
    set(MCU_DATABASE "${ALLOY_ROOT}/tools/codegen/database/${ALLOY_MCU_FAMILY}.json")
    set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/${ALLOY_MCU}")

    # Arquivo marcador para saber se já geramos
    set(GENERATION_MARKER "${GENERATED_DIR}/.generated")

    # Se a configuração mudou ou não existe, regenerar
    if(NOT EXISTS ${GENERATION_MARKER} OR
       ${MCU_DATABASE} IS_NEWER_THAN ${GENERATION_MARKER})

        message(STATUS "Generating platform code for ${ALLOY_MCU}...")

        execute_process(
            COMMAND ${Python3_EXECUTABLE}
                    ${ALLOY_ROOT}/tools/codegen/generator.py
                    --mcu ${ALLOY_MCU}
                    --database ${MCU_DATABASE}
                    --output ${GENERATED_DIR}
            RESULT_VARIABLE CODEGEN_RESULT
        )

        if(NOT CODEGEN_RESULT EQUAL 0)
            message(FATAL_ERROR "Code generation failed for ${ALLOY_MCU}")
        endif()

        # Marcar como gerado
        file(TOUCH ${GENERATION_MARKER})
    else()
        message(STATUS "Platform code for ${ALLOY_MCU} already generated")
    endif()

    # Exportar diretório gerado para uso no build
    set(ALLOY_GENERATED_DIR ${GENERATED_DIR} PARENT_SCOPE)
endfunction()
```

### Database Format (JSON)

```json
{
  "mcu": "STM32F446RE",
  "family": "STM32F4",
  "core": "ARM Cortex-M4",
  "fpu": "fpv4-sp-d16",
  "flash_size": "512K",
  "ram_size": "128K",
  "clock": {
    "max_freq_hz": 180000000,
    "hse_freq_hz": 8000000
  },
  "peripherals": {
    "GPIO": {
      "ports": ["A", "B", "C", "D", "E", "H"],
      "pins_per_port": 16,
      "base_addresses": {
        "GPIOA": "0x40020000",
        "GPIOB": "0x40020400",
        "GPIOC": "0x40020800"
      }
    },
    "UART": {
      "instances": ["USART1", "USART2", "USART3", "UART4", "UART5", "USART6"],
      "base_addresses": {
        "USART1": "0x40011000",
        "USART2": "0x40004400"
      }
    }
  },
  "interrupts": {
    "vectors": [
      {"name": "Reset_Handler", "number": 1},
      {"name": "NMI_Handler", "number": 2},
      {"name": "USART1_IRQHandler", "number": 37}
    ]
  },
  "memory_map": {
    "flash": {"start": "0x08000000", "size": "512K"},
    "ram": {"start": "0x20000000", "size": "128K"}
  }
}
```

### Generator Script (Python)

```python
# tools/codegen/generator.py
import json
import argparse
from pathlib import Path
from jinja2 import Environment, FileSystemLoader

class CodeGenerator:
    def __init__(self, database_path: Path, output_dir: Path):
        self.database = self._load_database(database_path)
        self.output_dir = output_dir
        self.template_env = Environment(
            loader=FileSystemLoader('tools/codegen/templates')
        )

    def _load_database(self, path: Path) -> dict:
        with open(path) as f:
            return json.load(f)

    def generate_startup(self):
        """Gera o startup code (reset handler, vector table)"""
        template = self.template_env.get_template('startup.cpp.j2')
        output = template.render(
            mcu=self.database['mcu'],
            interrupts=self.database['interrupts'],
            memory=self.database['memory_map']
        )
        (self.output_dir / 'startup.cpp').write_text(output)

    def generate_registers(self):
        """Gera definições de registradores"""
        template = self.template_env.get_template('registers.hpp.j2')
        output = template.render(peripherals=self.database['peripherals'])
        (self.output_dir / 'registers.hpp').write_text(output)

    def generate_linker_script(self):
        """Gera o linker script"""
        template = self.template_env.get_template('linker.ld.j2')
        output = template.render(memory=self.database['memory_map'])
        (self.output_dir / f"{self.database['mcu'].lower()}.ld").write_text(output)

    def generate_all(self):
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.generate_startup()
        self.generate_registers()
        self.generate_linker_script()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--mcu', required=True)
    parser.add_argument('--database', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()

    generator = CodeGenerator(args.database, args.output)
    generator.generate_all()
    print(f"✓ Code generated for {args.mcu}")
```

## 7. API Design Patterns

### Pattern 1: Type-Safe Units

```cpp
// src/core/units.hpp
#pragma once

#include <cstdint>

namespace alloy::units {

// Strong types para frequências
template<typename Rep = uint32_t>
class Frequency {
public:
    constexpr explicit Frequency(Rep hz) : value_hz_(hz) {}

    constexpr Rep hertz() const { return value_hz_; }
    constexpr Rep kilohertz() const { return value_hz_ / 1000; }
    constexpr Rep megahertz() const { return value_hz_ / 1000000; }

    // Literais convenientes
    friend constexpr Frequency operator""_Hz(unsigned long long hz) {
        return Frequency(static_cast<Rep>(hz));
    }
    friend constexpr Frequency operator""_kHz(unsigned long long khz) {
        return Frequency(static_cast<Rep>(khz * 1000));
    }
    friend constexpr Frequency operator""_MHz(unsigned long long mhz) {
        return Frequency(static_cast<Rep>(mhz * 1000000));
    }

private:
    Rep value_hz_;
};

// Uso:
// auto baud_rate = 115200_Hz;
// uart.configure(baud_rate);

} // namespace alloy::units
```

### Pattern 2: Compile-Time Pin Configuration

```cpp
// src/hal/interface/gpio.hpp (continuação)

// Configuração de pino em compile-time
template<uint32_t PIN, PinMode MODE>
class ConfiguredGpioPin {
public:
    constexpr ConfiguredGpioPin() {
        static_assert(PIN < 30, "Invalid pin number");
        // Mode será aplicado na inicialização
    }

    void initialize() {
        underlying_pin_.configure(MODE);
    }

    // Métodos especializados baseado no MODE
    void set_high() requires (MODE == PinMode::Output) {
        underlying_pin_.set_high();
    }

    bool read() const requires (MODE == PinMode::Input ||
                               MODE == PinMode::InputPullUp) {
        return underlying_pin_.read();
    }

private:
    GpioPin<PIN> underlying_pin_;
};

// Uso:
// ConfiguredGpioPin<25, PinMode::Output> led;
// led.set_high();  // OK
// led.read();      // ERRO DE COMPILAÇÃO - pin é Output, não Input!
```

### Pattern 3: Callback Registry (sem dynamic allocation)

```cpp
// src/hal/interface/uart.hpp

#include <array>
#include <cassert>

// HAL UART com callbacks estáticos
template<typename UartImpl, size_t MAX_CALLBACKS = 4>
class UartWithCallbacks : public UartImpl {
public:
    using CallbackFn = void(*)(uint8_t data);

    void register_rx_callback(CallbackFn callback) {
        for (auto& cb : rx_callbacks_) {
            if (cb == nullptr) {
                cb = callback;
                return;
            }
        }
        // Buffer cheio - erro em compile-time com assert
        assert(false && "Callback buffer full");
    }

    void on_rx_interrupt() {
        uint8_t data = this->read_byte();
        for (auto cb : rx_callbacks_) {
            if (cb) cb(data);
        }
    }

private:
    std::array<CallbackFn, MAX_CALLBACKS> rx_callbacks_{};
};
```

## 8. Fluxo de Compilação Completo

```
1. User configures:
   CMakeLists.txt
   └─ set(ALLOY_BOARD "rp_pico")

2. CMake loads board:
   cmake/boards/rp_pico.cmake
   └─ Sets ALLOY_MCU = "RP2040"
   └─ Sets ALLOY_ARCH = "cortex-m0plus"

3. CMake checks if code generation needed:
   cmake/codegen.cmake
   └─ Runs tools/codegen/generator.py (if needed)
   └─ Generates code in build/generated/RP2040/

4. CMake configures toolchain:
   cmake/toolchains/arm-none-eabi.cmake
   └─ Sets compiler flags: -mcpu=cortex-m0plus -mthumb
   └─ Sets linker script: build/generated/RP2040/rp2040.ld

5. Build HAL modules:
   src/hal/rp2040/*.cpp
   └─ Compiles with C++20 modules
   └─ Creates alloy::hal::gpio, alloy::hal::uart, etc.

6. Build user application:
   user/src/main.cpp
   └─ import alloy.hal.gpio;
   └─ Links with HAL libraries

7. Link final binary:
   ├─ User code
   ├─ HAL implementations
   ├─ Generated startup code
   └─ Linker script

   → Outputs: firmware.elf, firmware.bin, firmware.hex
```

## 9. Exemplo de Aplicação Completa

### Projeto do Usuário

```cpp
// examples/blinky/main.cpp
#include "hal/gpio.hpp"
#include "platform/delay.hpp"  // Board-specific utilities

using namespace alloy::hal;

// LED_PIN é definido no board config (ex: 25 para RP Pico)
#ifndef LED_PIN
#define LED_PIN 25
#endif

int main() {
    // Criar pin configurado em compile-time
    ConfiguredGpioPin<LED_PIN, PinMode::Output> led;
    led.initialize();

    while (true) {
        led.toggle();
        alloy::platform::delay_ms(500);
    }
}
```

```cmake
# examples/blinky/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)

set(ALLOY_BOARD "rp_pico")
add_subdirectory(../../ alloy)

project(blinky CXX)

add_executable(blinky main.cpp)
target_link_libraries(blinky PRIVATE alloy::hal::gpio)

# Helper para gerar .uf2 para o Pico
alloy_generate_uf2(blinky)
```

### Build e Flash

```bash
# Build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Flash (usando CLI helper - futuro)
alloy flash --board rp_pico --file build/blinky.uf2

# Ou manualmente
cp build/blinky.uf2 /Volumes/RPI-RP2/
```

## 10. Estratégia de Testes

### Unit Tests (Host)

```cpp
// tests/unit/test_gpio.cpp
#include "hal/host/gpio.hpp"
#include <gtest/gtest.h>

using namespace alloy::hal::host;

TEST(GpioTest, ToggleChangesState) {
    GpioPin<25> pin;

    pin.set_low();
    EXPECT_FALSE(pin.read());

    pin.toggle();
    EXPECT_TRUE(pin.read());

    pin.toggle();
    EXPECT_FALSE(pin.read());
}
```

### Integration Tests (Hardware)

```cpp
// tests/integration/test_uart_echo.cpp
// Roda no hardware real, conectado a um test runner
#include "hal/uart.hpp"

void test_uart_echo() {
    auto uart = make_uart<0>(115200_Hz);
    uart.initialize();

    uart.write("PING");
    auto response = uart.read_until('\n', timeout_ms(1000));

    assert(response == "PONG");
}
```

## 11. Naming Conventions

### Arquivos e Diretórios
- **Headers:** `snake_case.hpp` (ex: `gpio_pin.hpp`, `uart_driver.hpp`)
- **Sources:** `snake_case.cpp` (ex: `gpio_pin.cpp`, `uart_driver.cpp`)
- **Diretórios:** `snake_case` (ex: `hal/`, `drivers/`, `stm32f4/`)

### Código C++
- **Namespaces:** `snake_case` (ex: `alloy::hal::`, `alloy::drivers::`)
- **Classes/Structs:** `PascalCase` (ex: `GpioPin`, `UartDriver`, `I2cMaster`)
- **Funções/Métodos:** `snake_case` (ex: `set_high()`, `read_byte()`, `configure()`)
- **Variáveis:** `snake_case` (ex: `led_pin`, `baud_rate`, `buffer_size`)
- **Constantes:** `UPPER_SNAKE_CASE` (ex: `MAX_BUFFER_SIZE`, `DEFAULT_TIMEOUT`)
- **Template Parameters:** `PascalCase` (ex: `template<typename PinImpl>`)
- **Enums:** `PascalCase` para tipo, `PascalCase` para valores (ex: `enum class PinMode { Input, Output }`)

### Variáveis CMake
- **Prefix:** `ALLOY_` para todas as variáveis públicas
- **Board:** `ALLOY_BOARD`, `ALLOY_MCU`, `ALLOY_ARCH`
- **Paths:** `ALLOY_ROOT`, `ALLOY_GENERATED_DIR`
- **Features:** `ALLOY_HAS_GPIO`, `ALLOY_USE_RTOS`

### Macros
- **Prefix:** `ALLOY_` para todas as macros públicas
- **Include guards:** `ALLOY_HAL_GPIO_HPP` (baseado no caminho do arquivo)
- **Feature flags:** `ALLOY_ENABLE_LOGGING`, `ALLOY_DEBUG_MODE`

## 12. Memory-Constrained Design

### 12.1 Target: MCUs com 8KB-16KB RAM

O Alloy é explicitamente projetado para funcionar bem em MCUs com memória RAM limitada, incluindo:
- **Renesas RL78:** 8KB-16KB RAM típico
- **STM32F030/F103 (variantes pequenas):** 4KB-10KB RAM
- **ATmega328P:** 2KB RAM

**Objetivo:** O overhead total do Alloy deve ser < 2KB em MCUs com 8KB-16KB RAM.

### 12.2 Zero-Cost Abstractions

```cpp
// ✅ Template com constexpr - otimizado em compile-time
template<uint8_t PIN>
class GpioPin {
public:
    constexpr void set_high() {
        // Compila para uma única instrução: PORTB |= (1 << PIN)
        port_register() |= (1 << PIN);
    }

private:
    static constexpr volatile uint8_t& port_register() {
        return *reinterpret_cast<volatile uint8_t*>(PORT_ADDRESS);
    }
};

// ❌ Virtual dispatch - adiciona vtable overhead
class GpioPin {
    virtual void set_high() = 0;  // Evitar
};
```

**Regras:**
- Preferir `template` + `constexpr` sobre polimorfismo runtime
- Virtual functions apenas quando realmente necessário
- Compiler deve inline agressivamente

### 12.3 Template Bloat Control

Templates mal usados podem explodir o tamanho do código.

```cpp
// ❌ Ruim: Código duplicado para cada tamanho
template<size_t SIZE>
class UartDriver {
    std::array<uint8_t, SIZE> buffer_;

    void complex_send_logic(const char* data) {
        // Código complexo duplicado para cada SIZE!
        // ...
    }
};

// ✅ Melhor: Extrair lógica para classe base
class UartDriverBase {
protected:
    void send_impl(uint8_t* buf, size_t size);  // Uma instância
};

template<size_t SIZE>
class UartDriver : private UartDriverBase {
    std::array<uint8_t, SIZE> buffer_;
public:
    void send(const char* data) {
        send_impl(buffer_.data(), SIZE);  // Chama base
    }
};
```

### 12.4 Compile-Time Configuration

Usuário escolhe quais periféricos incluir:

```cmake
# CMakeLists.txt do usuário
set(ALLOY_HAS_UART ON)
set(ALLOY_HAS_I2C OFF)    # Não usado = não compila
set(ALLOY_HAS_SPI OFF)
set(ALLOY_HAS_ADC OFF)
```

Gera `alloy_config.hpp`:
```cpp
#define ALLOY_HAS_UART 1
// I2C, SPI, ADC não definidos
```

Código usa conditional compilation:
```cpp
#ifdef ALLOY_HAS_UART
    namespace alloy::hal {
        class UartDriver { /* ... */ };
    }
#endif
```

**Benefício:** Linker elimina código não usado (dead code elimination).

### 12.5 Buffer Sizing Strategies

```cpp
// API: Tamanhos configuráveis com defaults sensatos
template<
    size_t RX_SIZE = 64,   // Default: 64 bytes
    size_t TX_SIZE = 64
>
class UartDriver {
    std::array<uint8_t, RX_SIZE> rx_buffer_;
    std::array<uint8_t, TX_SIZE> tx_buffer_;
};

// MCU com 8KB RAM:
UartDriver<32, 32> uart;   // 64 bytes total

// MCU com 128KB RAM:
UartDriver<512, 512> uart; // 1KB total
```

### 12.6 Stack Usage Guidelines

**Regras:**
- ❌ Nunca alocar grandes arrays na stack
- ✅ Buffers grandes devem ser `static` ou membros de classe
- ⚠️ Recursão proibida ou muito limitada
- 📊 Documentar stack usage de cada função

```cpp
// ❌ Perigo: 1KB na stack!
void process() {
    uint8_t buffer[1024];
    // ...
}

// ✅ Seguro
class Processor {
    std::array<uint8_t, 1024> buffer_;  // No objeto
public:
    void process() {
        // usa buffer_
    }
};
static Processor processor;  // Static allocation
```

### 12.7 Memory Budget por Módulo

Cada módulo deve documentar seu footprint:

| Módulo | RAM (bytes) | Flash (bytes) | Notas |
|--------|-------------|---------------|-------|
| `GpioPin<N>` | 0 | ~20 | Zero-size, inline |
| `UartDriver<64,64>` | 128 + ~16 | ~500 | Buffers + estado |
| `I2cMaster<128>` | 128 + ~32 | ~800 | Buffer + estado |
| `ErrorCode` | 0 | ~50 | Enum, zero runtime |
| `Result<T, Error>` | sizeof(T) + 4 | ~30 | Union, minimal |

### 12.8 Memory Analysis Tools

```cmake
# Target para análise de memória
add_custom_target(memory-report
    COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:my_app>
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:my_app>
    COMMAND python ${ALLOY_ROOT}/tools/analyze_memory.py $<TARGET_FILE:my_app>.map
)
```

Output esperado:
```
Memory Usage Report:
====================
Flash: 8432 / 65536 bytes (12.9%)
RAM:   1024 / 8192  bytes (12.5%)

Top RAM Consumers:
- uart_rx_buffer:     256 bytes
- i2c_buffer:         128 bytes
- main_stack:         512 bytes
- Alloy HAL overhead: 148 bytes
```

### 12.9 Compilation Flags para Minimal Builds

```cmake
option(ALLOY_MINIMAL_BUILD "Optimize for smallest footprint" OFF)

if(ALLOY_MINIMAL_BUILD)
    add_compile_options(
        -Os                    # Optimize for size
        -ffunction-sections    # Each function in own section
        -fdata-sections        # Each data in own section
        -flto                  # Link-time optimization
        -fno-rtti              # No RTTI (já não usamos)
        -fno-exceptions        # No exceptions (já não usamos)
    )
    add_link_options(
        -Wl,--gc-sections      # Remove unused sections
        -Wl,--print-memory-usage
    )
endif()
```

### 12.10 Validation: Static Assertions

```cpp
// Garantir que tipos são zero-overhead
static_assert(sizeof(GpioPin<0>) == 0 || sizeof(GpioPin<0>) <= 4);
static_assert(std::is_trivially_copyable_v<GpioPin<0>>);
static_assert(sizeof(Result<uint8_t, ErrorCode>) <= 8);

// Garantir alinhamento correto
static_assert(alignof(UartDriver<64, 64>) <= 8);
```

### 12.11 Memory Budget Targets

| MCU Class | RAM Total | Alloy Overhead | User Available | Targets |
|-----------|-----------|----------------|----------------|---------|
| Tiny      | 2-8 KB    | < 512 bytes    | 1.5-7.5 KB     | ATmega328P, RL78/G10 |
| Small     | 8-32 KB   | < 2 KB         | 6-30 KB        | RL78/G13, STM32F030, BluePill |
| Medium    | 32-128 KB | < 8 KB         | 24-120 KB      | STM32F103, RP2040 |
| Large     | 128+ KB   | < 16 KB        | 112+ KB        | STM32F4, ESP32 |

**Referência:** Ver **ADR-013** em decisions.md para detalhes completos sobre estratégias de low-memory support.

---

## 13. Próximos Passos na Implementação

### Immediate (Phase 0)

1. ⬜ Criar estrutura básica de diretórios
2. ⬜ CMake básico (sem modules, C++20 puro)
3. ⬜ GPIO interface (concepts) + implementação host mockada
4. ⬜ Exemplo blinky rodando em host com output simulado
5. ⬜ Sistema de error codes básico
6. ⬜ Google Test configurado

### Short-term (Phase 1)

1. ⬜ Gerador de código funcional (MVP)
2. ⬜ RP2040 HAL completo (GPIO, UART, I2C, SPI)
3. ⬜ STM32F4 HAL básico (GPIO, UART)
4. ⬜ Sistema de boards robusto
5. ⬜ Testes automatizados (CI)

### Medium-term (Phase 2)

1. ⬜ CLI tool (`alloy-cli`)
2. ⬜ STM32F4 suporte completo
3. ⬜ Drivers externos (10+ drivers)
4. ⬜ Documentação completa
5. ⬜ Website e branding

---

**Última atualização:** 2025-10-29
**Status:** Architecture v1.0 - Foundation phase
**Repositório:** `github.com/alloy-embedded/alloy`
