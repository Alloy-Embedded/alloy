# Migration Example - Before and After

Este documento mostra exemplos concretos de código ANTES e DEPOIS da consolidação arquitetural, para facilitar o entendimento das mudanças.

## 1. Directory Structure

### ANTES - Estrutura Confusa

```
src/hal/
├── platform/              # ❓ Onde vai código hand-written?
│   ├── st/
│   │   ├── stm32f4/
│   │   │   ├── clock_platform.hpp
│   │   │   └── gpio.hpp
│   │   └── stm32f7/
│   │       ├── clock_platform.hpp
│   │       └── gpio.hpp
│   └── same70/
│       ├── clock_platform.hpp
│       └── gpio.hpp
└── vendors/               # ❓ Ou vai aqui?
    ├── st/
    │   ├── stm32f4/
    │   │   ├── registers/  # Gerado
    │   │   └── bitfields/  # Gerado
    │   └── stm32f7/
    │       ├── registers/
    │       └── bitfields/
    └── arm/
        └── same70/
            ├── registers/
            └── bitfields/
```

### DEPOIS - Estrutura Clara

```
src/hal/vendors/           # ✅ Único lugar para código de plataforma
├── st/
│   ├── common/            # ✅ Código compartilhado STM32
│   │   ├── cortex_m_common.hpp
│   │   └── stm32_clock_common.hpp
│   ├── stm32f4/
│   │   ├── generated/     # ✅ MARCADOR: Auto-gerado
│   │   │   ├── .generated # Arquivo marcador
│   │   │   ├── registers/
│   │   │   └── bitfields/
│   │   ├── clock_platform.hpp  # ✅ Hand-written aqui
│   │   └── gpio.hpp
│   └── stm32f7/
│       ├── generated/
│       ├── clock_platform.hpp
│       └── gpio.hpp
└── arm/
    └── same70/
        ├── generated/
        ├── clock_platform.hpp
        └── gpio.hpp
```

---

## 2. Board Code - Removing #ifdef

### ANTES - Board com #ifdef (RUIM)

```cpp
// boards/nucleo_f401re/board.cpp - PROBLEMÁTICO

#include "board.hpp"

#ifdef STM32F4
    #include "hal/platform/st/stm32f4/clock_platform.hpp"
    #include "hal/platform/st/stm32f4/gpio.hpp"
#endif

#ifdef SAME70
    #include "hal/platform/same70/clock_platform.hpp"
    #include "hal/platform/same70/gpio.hpp"
#endif

namespace board {

void init() {
    #ifdef STM32F4
        // STM32F4-specific code
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  // Enable GPIOA clock

        // Configure PA5 as output (LED)
        GPIOA->MODER &= ~GPIO_MODER_MODER5_Msk;
        GPIOA->MODER |= (0x1 << GPIO_MODER_MODER5_Pos);  // Output mode

        // Configure system clock to 84 MHz
        stm32f4::Clock::init();
    #endif

    #ifdef SAME70
        // SAME70-specific code
        PMC->PMC_PCER0 = (1 << ID_PIOA);  // Enable PIOA clock

        // Configure PA10 as output (LED)
        PIOA->PIO_PER = (1 << 10);   // Enable PIO
        PIOA->PIO_OER = (1 << 10);   // Output enable

        // Configure system clock to 300 MHz
        same70::Clock::init();
    #endif
}

void led_on() {
    #ifdef STM32F4
        GPIOA->BSRR = (1 << 5);  // Set PA5
    #endif

    #ifdef SAME70
        PIOA->PIO_SODR = (1 << 10);  // Set PA10
    #endif
}

void led_off() {
    #ifdef STM32F4
        GPIOA->BSRR = (1 << (5 + 16));  // Reset PA5
    #endif

    #ifdef SAME70
        PIOA->PIO_CODR = (1 << 10);  // Clear PA10
    #endif
}

} // namespace board
```

**Problemas:**
- ❌ Código não é portável (cheio de #ifdef)
- ❌ Acesso direto a registradores (RCC, GPIOA, PMC, PIOA)
- ❌ Adicionar nova plataforma requer editar board.cpp
- ❌ Não segue filosofia de abstrações compile-time

### DEPOIS - Board Genérico (BOM)

```cpp
// boards/nucleo_f401re/board_config.hpp - NOVO ARQUIVO

#pragma once

#include "hal/vendors/st/stm32f4/clock_platform.hpp"
#include "hal/vendors/st/stm32f4/gpio.hpp"
#include "hal/systick.hpp"
#include "hal/core/types.hpp"

namespace board {

// ============================================================================
// Platform Type Aliases
// ============================================================================

// Clock: 84 MHz system clock
using ClockPlatform = stm32f4::Clock<84'000'000>;

// GPIO platform
using GpioPlatform = stm32f4::GPIO;

// SysTick timer
using BoardSysTick = SysTick<ClockPlatform::system_clock_hz>;

// ============================================================================
// Pin Definitions
// ============================================================================

using LedPin = GpioPin<GpioPlatform, GPIOA, 5>;      // PA5 - User LED
using ButtonPin = GpioPin<GpioPlatform, GPIOC, 13>;  // PC13 - User Button

// ============================================================================
// Validation
// ============================================================================

static_assert(ClockPlatform::system_clock_hz == 84'000'000);
static_assert(BoardSysTick::tick_period_ms == 1);

} // namespace board
```

```cpp
// boards/nucleo_f401re/board.cpp - 100% GENÉRICO!

#include "board.hpp"
#include "board_config.hpp"

namespace board {

void init() {
    // ✅ Generic code - uses policy types
    ClockPlatform::init();

    // ✅ Configure LED using type alias
    LedPin::configure(OutputMode::PushPull, Speed::Low);

    // ✅ Initialize timing
    BoardSysTick::init();
}

void led_on() {
    // ✅ Generic - works with ANY GpioPlatform
    LedPin::set();
}

void led_off() {
    // ✅ Generic
    LedPin::clear();
}

void led_toggle() {
    // ✅ Generic
    LedPin::toggle();
}

} // namespace board
```

**Benefícios:**
- ✅ Código 100% genérico, sem #ifdef
- ✅ Todas dependências de plataforma em board_config.hpp
- ✅ Adicionar nova plataforma = só editar board_config.hpp
- ✅ Segue filosofia de abstrações compile-time

---

## 3. Naming Standardization

### ANTES - Nomes Misturados

```cpp
// Arquivo: src/hal/gpio.hpp

/**
 * @file gpio.hpp
 * @brief CoreZero GPIO HAL        // ❌ "CoreZero"
 */

#ifndef COREZERO_HAL_GPIO_HPP      // ❌ "COREZERO_"
#define COREZERO_HAL_GPIO_HPP

#define COREZERO_VERSION_MAJOR 0   // ❌ "COREZERO_"

namespace alloy::hal::gpio {       // ✅ Namespace já estava correto
    // ...
}

#endif // COREZERO_HAL_GPIO_HPP
```

```cmake
# CMakeLists.txt
project(alloy VERSION 0.1.0)       # ✅ CMake já usa "alloy"

# But README says "CoreZero Framework"  # ❌ Inconsistência
```

### DEPOIS - Nome Único "Alloy"

```cpp
// Arquivo: src/hal/gpio.hpp

/**
 * @file gpio.hpp
 * @brief Alloy Framework - GPIO HAL  // ✅ "Alloy"
 */

#ifndef ALLOY_HAL_GPIO_HPP           // ✅ "ALLOY_"
#define ALLOY_HAL_GPIO_HPP

#define ALLOY_VERSION_MAJOR 1        // ✅ "ALLOY_"

namespace alloy::hal::gpio {         // ✅ Consistente
    // ...
}

#endif // ALLOY_HAL_GPIO_HPP
```

```markdown
# README.md

# Alloy Framework                    ✅ Consistente

Alloy is a modern C++20 framework... ✅ Consistente
```

---

## 4. Code Generation

### ANTES - Múltiplos Geradores (Duplicação 95%)

```bash
tools/codegen/
├── generate_stm32f4_registers.py   # 380 linhas
├── generate_stm32f7_registers.py   # 380 linhas (95% igual!)
├── generate_stm32g0_registers.py   # 380 linhas (95% igual!)
├── generate_same70_registers.py    # 420 linhas (90% igual!)
└── ...
```

```python
# generate_stm32f4_registers.py - DUPLICADO

def generate_registers(svd_file):
    """Generate STM32F4 registers."""
    # 380 linhas de código quase idêntico a outros geradores
    svd = parse_svd(svd_file)
    for peripheral in svd.peripherals:
        output = render_template("register_stm32f4.j2", peripheral)
        write_file(f"stm32f4/registers/{peripheral.name}.hpp", output)
```

```python
# generate_stm32f7_registers.py - 95% IDÊNTICO!

def generate_registers(svd_file):
    """Generate STM32F7 registers."""
    # Código quase idêntico, só muda "stm32f4" → "stm32f7"
    svd = parse_svd(svd_file)
    for peripheral in svd.peripherals:
        output = render_template("register_stm32f7.j2", peripheral)
        write_file(f"stm32f7/registers/{peripheral.name}.hpp", output)
```

### DEPOIS - Gerador Único

```bash
tools/codegen/
├── codegen.py              # Único ponto de entrada
├── core/
│   ├── svd_parser.py
│   ├── generator_base.py
│   └── output_writer.py
├── generators/             # Plugins especializados
│   ├── register_generator.py
│   ├── bitfield_generator.py
│   └── peripheral_generator.py
└── templates/              # Templates consolidados
    ├── register.hpp.j2     # Funciona para TODAS plataformas
    └── bitfield.hpp.j2
```

```python
# codegen.py - Único ponto de entrada

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--svd", required=True)
    parser.add_argument("--vendor", required=True)
    parser.add_argument("--family", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    # Parse SVD
    svd_data = SVDParser(args.svd).parse()

    # Generate using plugins
    RegisterGenerator().generate(svd_data, args.output, args.vendor, args.family)
    BitfieldGenerator().generate(svd_data, args.output, args.vendor, args.family)
    PeripheralGenerator().generate(svd_data, args.output, args.vendor, args.family)

    print(f"✅ Generated code for {args.vendor}/{args.family}")
```

```bash
# Uso - TODAS plataformas

# STM32F4
python codegen.py --svd data/stm32f4.svd --vendor st --family stm32f4 \
    --output src/hal/vendors/st/stm32f4/generated/

# STM32F7
python codegen.py --svd data/stm32f7.svd --vendor st --family stm32f7 \
    --output src/hal/vendors/st/stm32f7/generated/

# SAME70
python codegen.py --svd data/same70.svd --vendor arm --family same70 \
    --output src/hal/vendors/arm/same70/generated/
```

---

## 5. CMake Build System

### ANTES - GLOB Anti-Pattern

```cmake
# src/hal/CMakeLists.txt - PROBLEMÁTICO

# ❌ GLOB é anti-pattern do CMake
file(GLOB_RECURSE HAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE VENDOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/vendors/${VENDOR}/*.cpp")

# Problemas:
# - CMake não detecta arquivos novos (deve re-rodar cmake)
# - Lento (scans filesystem todo build)
# - Inclui arquivos indesejados
# - Sem controle de ordem de link

add_library(alloy_hal STATIC ${HAL_SOURCES} ${VENDOR_SOURCES})
```

### DEPOIS - Listas Explícitas

```cmake
# src/hal/CMakeLists.txt - CORRETO

# ✅ Listas explícitas de sources
set(HAL_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/core/result.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interrupt.cpp
)

set(HAL_SOURCES_STM32F4
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/st/stm32f4/clock_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/st/stm32f4/gpio_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/st/stm32f4/uart_platform.cpp
)

set(HAL_SOURCES_STM32F7
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/st/stm32f7/clock_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/st/stm32f7/gpio_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/st/stm32f7/uart_platform.cpp
)

# Seleção condicional
if(MCU_FAMILY STREQUAL "stm32f4")
    set(HAL_PLATFORM_SOURCES ${HAL_SOURCES_STM32F4})
elseif(MCU_FAMILY STREQUAL "stm32f7")
    set(HAL_PLATFORM_SOURCES ${HAL_SOURCES_STM32F7})
endif()

add_library(alloy_hal STATIC
    ${HAL_CORE_SOURCES}
    ${HAL_PLATFORM_SOURCES}
)

# ✅ Validação de sources órfãos
include(cmake/validate_sources.cmake)
```

**Benefícios:**
- ✅ Builds incrementais rápidos
- ✅ Detecta arquivos novos (explícitos na lista)
- ✅ Controle total sobre ordem de link
- ✅ Melhor review em PRs (vê quais arquivos adicionados)

---

## 6. API Standardization with Concepts

### ANTES - APIs Inconsistentes

```cpp
// STM32F4 Clock - sem template
namespace stm32f4 {
    struct Clock {
        static constexpr u32 system_clock_hz = 84000000;  // Hardcoded
        static void init();  // ✓ Nome correto
    };
}

// STM32F7 Clock - com template, nome diferente
namespace stm32f7 {
    template <u32 CLOCK_HZ>
    struct Clock {
        static constexpr u32 frequency = CLOCK_HZ;  // ❌ Nome diferente!
        static void configure();  // ❌ Nome de método diferente!
    };
}

// SAME70 Clock - retorna Result
namespace same70 {
    struct Clock {
        static u32 get_frequency();  // ❌ Runtime, não compile-time!
        static Result<void, Error> initialize();  // ❌ Nome diferente!
    };
}

// ❌ Impossível escrever código genérico que funcione em todas plataformas
```

### DEPOIS - APIs Padronizadas com Concepts

```cpp
// src/hal/core/concepts.hpp - Define interface

template <typename T>
concept ClockPlatform = requires {
    // Deve ter frequência compile-time
    { T::system_clock_hz } -> std::convertible_to<u32>;

    // Deve ter função de inicialização
    { T::init() } -> std::same_as<void>;
};
```

```cpp
// TODAS plataformas seguem mesma interface

// STM32F4
namespace stm32f4 {
    template <u32 CLOCK_HZ>
    struct Clock {
        static constexpr u32 system_clock_hz = CLOCK_HZ;  // ✅ Padronizado
        static void init() { /* ... */ }                   // ✅ Padronizado
    };
}
static_assert(ClockPlatform<stm32f4::Clock<84'000'000>>);  // ✅ Valida

// STM32F7
namespace stm32f7 {
    template <u32 CLOCK_HZ>
    struct Clock {
        static constexpr u32 system_clock_hz = CLOCK_HZ;  // ✅ Padronizado
        static void init() { /* ... */ }                   // ✅ Padronizado
    };
}
static_assert(ClockPlatform<stm32f7::Clock<216'000'000>>);  // ✅ Valida

// SAME70
namespace same70 {
    template <u32 CLOCK_HZ>
    struct Clock {
        static constexpr u32 system_clock_hz = CLOCK_HZ;  // ✅ Padronizado
        static void init() { /* ... */ }                   // ✅ Padronizado
    };
}
static_assert(ClockPlatform<same70::Clock<300'000'000>>);  // ✅ Valida
```

```cpp
// ✅ AGORA código genérico funciona!

template <ClockPlatform Clock>
void system_init() {
    Clock::init();

    // Log frequency (compile-time constant)
    constexpr u32 freq_mhz = Clock::system_clock_hz / 1'000'000;
    printf("System clock: %lu MHz\n", freq_mhz);
}

// Funciona com QUALQUER plataforma que satisfaça ClockPlatform
system_init<stm32f4::Clock<84'000'000>>();
system_init<stm32f7::Clock<216'000'000>>();
system_init<same70::Clock<300'000'000>>();
```

---

## Summary of Changes

| Aspecto | ANTES | DEPOIS | Benefício |
|---------|-------|--------|-----------|
| **Diretórios** | Dual: `platform/` + `vendors/` | Único: `vendors/` com `/generated/` | Clareza, single source of truth |
| **Board Code** | Cheio de `#ifdef` | 100% genérico | Portabilidade verdadeira |
| **Naming** | CoreZero + Alloy | Apenas Alloy | Consistência |
| **Code Gen** | 10+ scripts duplicados | 1 gerador unificado | 36% menos código, manutenção fácil |
| **CMake** | `file(GLOB ...)` | Listas explícitas | Builds rápidos, detecção de arquivos |
| **APIs** | Inconsistentes | Padronizadas com concepts | Programação genérica, validação compile-time |

## Next Steps

1. **Revisar este exemplo** - Entender impacto das mudanças
2. **Ler specs detalhados** - Em `specs/*/spec.md`
3. **Começar implementação** - Seguir `tasks.md` fase por fase
4. **Validar cada fase** - Usar gates de validação do README.md
