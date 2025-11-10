# Board Abstraction Layer - Design Document

## Objetivo

Criar uma camada de abstração de board padronizada que permita:
- Exemplos genéricos que funcionam em qualquer placa
- Código portável sem referências específicas ao MCU
- Interface consistente entre todas as boards
- Facilitar adição de novas placas

## Problema Atual

1. Exemplos incluem diretamente HAL específico da plataforma
2. Código duplicado entre boards similares (same70_xplained vs atmel_same70_xpld)
3. Startup code duplicado por board (deveria ser por MCU)
4. Sem padrão consistente de nomenclatura (Board:: vs board::)
5. Exemplos amarrados a hardware específico (nomes de pinos, periféricos)

## Arquitetura Proposta

### 1. Estrutura de Diretórios

```
boards/
├── common/                          # Código compartilhado
│   ├── board_interface.hpp          # Interface padrão que todas boards implementam
│   └── startup/                     # Startup code por família de MCU
│       ├── atsame70/
│       │   └── startup.cpp
│       ├── atsamd21/
│       │   └── startup.cpp
│       └── rp2040/
│           └── startup.cpp
│
├── same70_xplained/                 # Board específica
│   ├── board.hpp                    # Interface pública (inclui board_config.hpp)
│   ├── board_config.hpp             # Configuração e implementação específica
│   ├── ATSAME70Q21.ld              # Linker script
│   └── README.md                    # Documentação
│
├── arduino_zero/
│   ├── board.hpp
│   ├── board_config.hpp
│   ├── ATSAMD21G18.ld
│   └── README.md
│
└── waveshare_rp2040_zero/
    ├── board.hpp
    ├── board_config.hpp
    ├── RP2040.ld
    └── README.md
```

### 2. Interface Padrão (board_interface.hpp)

Todas as boards devem implementar esta interface mínima:

```cpp
namespace board {
    // ========================================================================
    // OBRIGATÓRIO - Todas boards devem implementar
    // ========================================================================

    /// Initialize board (clock, peripherals, etc)
    void init();

    /// Delay functions
    void delay_ms(uint32_t ms);
    void delay_us(uint32_t us);

    /// Board identification
    constexpr const char* name();
    constexpr const char* mcu();
    constexpr uint32_t clock_frequency_hz();

    // ========================================================================
    // OPCIONAL - Baseado na disponibilidade do hardware
    // ========================================================================

    namespace led {
        void on();           // LED principal ON
        void off();          // LED principal OFF
        void toggle();       // LED principal toggle
        // LEDs adicionais: led1, led2, etc (se disponível)
    }

    namespace button {
        bool read();         // Botão principal
        // Botões adicionais: button1, button2, etc (se disponível)
    }

    namespace uart {
        // UART padrão (geralmente UART0 ou console)
        void write(const char* data);
        // UARTs adicionais se disponível
    }
}
```

### 3. Implementação de Board (board.hpp)

Cada board terá um `board.hpp` que:
1. Inclui `board_interface.hpp`
2. Inclui `board_config.hpp` (específico da board)
3. Implementa a interface padrão

```cpp
// boards/same70_xplained/board.hpp
#pragma once

#include "boards/common/board_interface.hpp"
#include "boards/same70_xplained/board_config.hpp"

// Re-exporta namespace board:: para uso do usuário
using namespace alloy::boards::same70_xplained::board;
```

### 4. Configuração de Board (board_config.hpp)

Contém toda implementação específica:

```cpp
// boards/same70_xplained/board_config.hpp
#pragma once

#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/systick.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::boards::same70_xplained {
namespace board {

// ========================================================================
// Board Identification
// ========================================================================

inline constexpr const char* name() { return "SAME70 Xplained Ultra"; }
inline constexpr const char* mcu() { return "ATSAME70Q21B"; }
inline constexpr uint32_t clock_frequency_hz() { return 300'000'000; }

// ========================================================================
// Clock Configuration Options
// ========================================================================

enum class ClockPreset {
    Clock300MHz,  // Maximum performance
    Clock150MHz,  // Balanced
    Clock120MHz,  // USB compatible
    Clock48MHz,   // Low power
    Clock12MHz    // Ultra-low power
};

// ========================================================================
// Internal Definitions
// ========================================================================

namespace detail {
    using namespace alloy::hal::same70;
    using namespace alloy::generated::atsame70q21b;

    // LED pins (active-low)
    using Led0Pin = GpioPin<PIOC_BASE, 8>;
    using Led1Pin = GpioPin<PIOC_BASE, 9>;

    // Button pins (active-low, internal pull-up)
    using Button0Pin = GpioPin<PIOA_BASE, 11>;
    using Button1Pin = GpioPin<PIOC_BASE, 2>;

    // Global instances
    inline Led0Pin led0_instance;
    inline Led1Pin led1_instance;
    inline Button0Pin button0_instance;
    inline Button1Pin button1_instance;
}

// ========================================================================
// Board Initialization
// ========================================================================

inline void init(ClockPreset preset = ClockPreset::Clock300MHz) {
    using namespace detail;

    // Configure clock based on preset
    ClockConfig clock_config;
    switch (preset) {
        case ClockPreset::Clock300MHz:
            clock_config = {
                .main_source = MainClockSource::ExternalCrystal,
                .crystal_freq_hz = 12000000,
                .plla = {24, 1},  // 12MHz * 25 / 1 = 300MHz
                .mck_source = MasterClockSource::PLLAClock,
                .mck_prescaler = MasterClockPrescaler::DIV_2  // 150MHz MCK
            };
            break;
        // ... outros presets
    }

    Clock::initialize(clock_config);
    SystemTick::init();

    // Enable peripheral clocks for GPIOs
    volatile uint32_t* PMC_PCER0 = (volatile uint32_t*)0x400E0610;
    *PMC_PCER0 = (1u << 10)  // PIOA
               | (1u << 11)  // PIOB
               | (1u << 12)  // PIOC
               | (1u << 13)  // PIOD
               | (1u << 14); // PIOE

    // Configure LEDs (active-low output)
    led0_instance.setDirection(PinDirection::Output);
    led0_instance.set();  // OFF
    led1_instance.setDirection(PinDirection::Output);
    led1_instance.set();  // OFF

    // Configure buttons (input with pull-up)
    button0_instance.setDirection(PinDirection::Input);
    button0_instance.setPull(PinPull::PullUp);
    button1_instance.setDirection(PinDirection::Input);
    button1_instance.setPull(PinPull::PullUp);
}

// ========================================================================
// Delay Functions
// ========================================================================

inline void delay_ms(uint32_t ms) {
    alloy::hal::same70::delay_ms(ms);
}

inline void delay_us(uint32_t us) {
    alloy::hal::same70::delay_us(us);
}

// ========================================================================
// LED Interface
// ========================================================================

namespace led {
    inline void on() { detail::led0_instance.clear(); }    // active-low
    inline void off() { detail::led0_instance.set(); }     // active-low
    inline void toggle() { detail::led0_instance.toggle(); }

    // Additional LEDs
    namespace led1 {
        inline void on() { detail::led1_instance.clear(); }
        inline void off() { detail::led1_instance.set(); }
        inline void toggle() { detail::led1_instance.toggle(); }
    }
}

// ========================================================================
// Button Interface
// ========================================================================

namespace button {
    inline bool read() {
        return !detail::button0_instance.read();  // active-low
    }

    namespace button1 {
        inline bool read() {
            return !detail::button1_instance.read();  // active-low
        }
    }
}

} // namespace board
} // namespace alloy::boards::same70_xplained
```

### 5. Uso em Exemplos

Com essa arquitetura, exemplos ficam genéricos:

```cpp
// examples/blink_led/main.cpp
#include BOARD_HEADER  // Macro definida pelo CMake

int main() {
    // Initialize board with default settings
    board::init();

    while (true) {
        board::led::on();
        board::delay_ms(500);
        board::led::off();
        board::delay_ms(500);
    }
}
```

### 6. Configuração CMake

```cmake
# Top-level CMakeLists.txt

# User selects board
set(ALLOY_BOARD "same70_xplained" CACHE STRING "Target board")

# Map board to header
if(ALLOY_BOARD STREQUAL "same70_xplained")
    set(BOARD_HEADER_PATH "boards/same70_xplained/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/boards/common/startup/atsame70/startup.cpp")
elseif(ALLOY_BOARD STREQUAL "arduino_zero")
    set(BOARD_HEADER_PATH "boards/arduino_zero/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/boards/common/startup/atsamd21/startup.cpp")
endif()

# Define macro for examples
add_compile_definitions(BOARD_HEADER="${BOARD_HEADER_PATH}")

# Add startup code automatically
if(CMAKE_CROSSCOMPILING)
    target_sources(${PROJECT_NAME} PRIVATE ${STARTUP_SOURCE})
endif()
```

### 7. Estrutura de Build

```bash
# Build para SAME70
mkdir build-same70
cd build-same70
cmake .. -DALLOY_BOARD=same70_xplained -DCMAKE_BUILD_TYPE=Release
make blink_led

# Flash
make flash-blink_led

# Build para Arduino Zero
mkdir build-arduino
cd build-arduino
cmake .. -DALLOY_BOARD=arduino_zero -DCMAKE_BUILD_TYPE=Release
make blink_led

# Mesmo código funciona em ambos!
```

## Benefícios

1. **Portabilidade**: Exemplos funcionam em qualquer board sem modificação
2. **Manutenibilidade**: Código compartilhado em `boards/common/`
3. **Consistência**: Interface padronizada entre boards
4. **Simplicidade**: Usuário não precisa conhecer detalhes do hardware
5. **Flexibilidade**: Ainda permite acesso direto ao HAL quando necessário
6. **Documentação**: Cada board tem README com especificações

## Migração

### Fase 1: Criar infraestrutura
- [ ] Criar `boards/common/board_interface.hpp`
- [ ] Mover startup code para `boards/common/startup/`
- [ ] Criar templates de documentação

### Fase 2: Refatorar boards existentes
- [ ] Refatorar `same70_xplained` para novo padrão
- [ ] Refatorar `arduino_zero` para novo padrão
- [ ] Consolidar `atmel_same70_xpld` → `same70_xplained`

### Fase 3: Atualizar exemplos
- [ ] Criar exemplos genéricos (blink, uart_hello, systick_test)
- [ ] Atualizar CMakeLists.txt com `BOARD_HEADER` macro
- [ ] Deprecar exemplos board-specific

### Fase 4: Documentação
- [ ] Guia de criação de boards
- [ ] Documentar interface padrão
- [ ] Exemplos de uso

## Exemplos de Boards Genéricos

### Exemplo 1: blink_led
```cpp
#include BOARD_HEADER

int main() {
    board::init();
    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
```

### Exemplo 2: systick_test
```cpp
#include BOARD_HEADER

int main() {
    board::init();

    // Startup sequence - 3 fast blinks
    for (int i = 0; i < 3; i++) {
        board::led::on();
        board::delay_ms(100);
        board::led::off();
        board::delay_ms(100);
    }

    // Main loop - 1Hz blink
    while (true) {
        board::led::on();
        board::delay_ms(250);
        board::led::off();
        board::delay_ms(750);
    }
}
```

### Exemplo 3: button_led
```cpp
#include BOARD_HEADER

int main() {
    board::init();

    while (true) {
        if (board::button::read()) {
            board::led::on();
        } else {
            board::led::off();
        }
    }
}
```

## Estrutura de Exemplo Final

```
examples/
├── blink_led/              # Genérico - funciona em qualquer board
│   ├── main.cpp
│   ├── CMakeLists.txt
│   └── README.md
├── systick_test/           # Genérico - testa SysTick + LED
│   ├── main.cpp
│   ├── CMakeLists.txt
│   └── README.md
├── button_led/             # Genérico - botão controla LED
│   ├── main.cpp
│   ├── CMakeLists.txt
│   └── README.md
└── uart_hello/             # Genérico - hello via UART
    ├── main.cpp
    ├── CMakeLists.txt
    └── README.md
```

## Notas de Implementação

1. **Startup Code**: Compartilhado por família de MCU, não por board
2. **Linker Script**: Permanece específico por MCU/board
3. **BOARD_HEADER**: Macro CMake que aponta para board.hpp correto
4. **Namespaces**: Padronizar em `board::` (lowercase)
5. **Documentação**: Cada board deve ter README completo

## Referências

- modm board abstraction: https://modm.io/
- ARM CMSIS: https://arm-software.github.io/CMSIS_5/
- Arduino board definitions: https://www.arduino.cc/
