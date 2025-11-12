# Plano de Modernização - Startup e Board Abstraction

## 🎯 Objetivos

1. **Remover classes antigas** de interrupt/systick
2. **Modernizar startup** para C++23 com flexibilidade
3. **Melhorar geração** de startup por MCU  
4. **Adicionar hooks** de inicialização
5. **Criar board abstraction** layer
6. **Exemplo limpo** usando board abstraction

---

## 📋 Fase 1: Modernizar Startup ARM (C++23)

### Estrutura Nova

```
src/hal/vendors/arm/cortex_m7/
├── startup.hpp          # Template moderno de startup
├── vector_table.hpp     # Vector table genérica
└── init_hooks.hpp       # Hooks de inicialização
```

### Features C++23

- `constexpr` vector tables
- `consteval` para validação compile-time
- Ranges para inicialização
- Concepts para type safety
- NRVO otimizations

---

## 📋 Fase 2: Board Abstraction Layer

### Estrutura

```
boards/
├── same70_xplained/
│   ├── board_config.hpp     # Clock, pins, periféricos
│   ├── board_init.cpp       # Inicialização do board
│   └── board.hpp            # API pública do board
```

### API do Board

```cpp
namespace board {
    // Clock configuration
    void init_clocks();      // 300MHz setup
    
    // GPIO pre-configurado
    inline auto& led_green = /* ... */;
    inline auto& button0 = /* ... */;
    
    // Periféricos pre-configurados  
    inline auto& console_uart = /* ... */;
    inline auto& debug_spi = /* ... */;
}
```

---

## 📋 Fase 3: Exemplo Limpo

### Antes

```cpp
int main() {
    // Setup manual de tudo
    configure_clocks();
    configure_led();
    configure_systick();
    configure_interrupts();
    
    while(1) { /* ... */ }
}
```

### Depois

```cpp
#include "boards/same70_xplained/board.hpp"

int main() {
    board::init();  // Tudo configurado!
    
    while(1) {
        board::led_green.toggle();
        board::delay_ms(500);
    }
}
```

---

## 🚀 Implementação

### Passo 1: ARM Cortex-M Modern Startup

Criar template genérico que funciona para M0/M3/M4/M7:
- Vector table flexível
- Init hooks customizáveis  
- C++23 constexpr/consteval
- Zero overhead

### Passo 2: SAME70 Board Config

Implementar para SAME70 Xplained:
- Clock 300MHz setup
- LED/Button mapping
- UART console setup
- SysTick 1ms config

### Passo 3: Update Example

Simplificar exemplo LED para usar board abstraction.

---

## 📊 Prioridades

1. ✅ **Alta**: ARM Cortex-M startup moderno
2. ✅ **Alta**: Board abstraction para SAME70
3. ✅ **Média**: Exemplo usando board
4. ⏭️ **Baixa**: Portar para outras boards (futuro)

