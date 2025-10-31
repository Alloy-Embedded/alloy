# Exemplo Prático: Como o LSP Funciona com ALLOY_GENERATED_NAMESPACE

Este documento mostra visualmente como a IDE consegue resolver `ALLOY_GENERATED_NAMESPACE` e oferecer features completas.

## Setup Inicial

### 1. Configure o MCU no CMake

```cmake
# cmake/boards/bluepill.cmake
set(ALLOY_BOARD "bluepill")
set(ALLOY_MCU "STM32F103C8")
set(ALLOY_ARCH "cortex-m3")
```

### 2. Gere compile_commands.json

```bash
cmake -S . -B build -DALLOY_BOARD=bluepill
```

Isso gera `build/compile_commands.json` com:

```json
{
  "file": "src/hal/st/stm32f1/gpio.cpp",
  "command": "g++ -DALLOY_GENERATED_NAMESPACE=alloy::generated::stm32f103c8 \
              -I/path/to/src/generated/st/stm32f1/stm32f103c8 \
              ... src/hal/st/stm32f1/gpio.cpp"
}
```

## Como o LSP Resolve

### Exemplo 1: Hover sobre ALLOY_GENERATED_NAMESPACE

**Código em `gpio.hpp`:**
```cpp
using GpioPort = ALLOY_GENERATED_NAMESPACE::gpio::Registers;
//               ^^^^^^^^^^^^^^^^^^^^^^^^^^^
//               Hover aqui
```

**O que o LSP faz:**

1. **Lê `compile_commands.json`** → Encontra `-DALLOY_GENERATED_NAMESPACE=alloy::generated::stm32f103c8`
2. **Expande a macro** → `using GpioPort = alloy::generated::stm32f103c8::gpio::Registers;`
3. **Mostra no hover:**
   ```
   Macro ALLOY_GENERATED_NAMESPACE
   → alloy::generated::stm32f103c8
   ```

### Exemplo 2: Go to Definition

**Código:**
```cpp
using GpioPort = ALLOY_GENERATED_NAMESPACE::gpio::Registers;
//                                          ^^^^
//                                          Ctrl+Click aqui
```

**O que o LSP faz:**

1. **Expande a macro** → `alloy::generated::stm32f103c8::gpio::Registers`
2. **Busca nos include paths** → Encontra `src/generated/st/stm32f1/stm32f103c8/peripherals.hpp`
3. **Abre o arquivo** na linha:
   ```cpp
   namespace gpio {
       struct Registers {  // ← Cursor pula para aqui
           volatile uint32_t CRL;
           volatile uint32_t CRH;
           // ...
       };
   }
   ```

### Exemplo 3: Autocomplete

**Digitando:**
```cpp
void configure_pin() {
    ALLOY_GENERATED_NAMESPACE::traits::|
    //                                 ↑ cursor aqui, pressiona Ctrl+Space
}
```

**O que o LSP oferece:**
```
🔍 Autocomplete suggestions:
   • flash_size_kb
   • ram_size_kb
   • has_gpio
   • num_gpio_ports
   • max_gpio_pins
   • has_usart1
   • has_usart2
   • has_usart3
   ...
```

**Depois de selecionar:**
```cpp
void configure_pin() {
    constexpr auto pins = ALLOY_GENERATED_NAMESPACE::traits::max_gpio_pins;
    //                                                       ^^^^^^^^^^^^^^
    // Hover mostra: constexpr uint32_t = 112
}
```

### Exemplo 4: Error Checking em Tempo Real

**Código inválido:**
```cpp
// Tentando usar pino 150 (máximo é 112)
GpioPin<150> led;
```

**O que o LSP mostra (sublinhado vermelho):**
```
❌ Static assertion failed
   "GPIO pin not available on this MCU - check your pin number"

   Note: expression evaluates to '150 < 112'
```

Você vê o erro **enquanto digita**, não precisa compilar!

### Exemplo 5: Semantic Highlighting

O LSP destaca diferentes elementos com cores:

```cpp
template<uint8_t PIN>
// ↑ keyword (roxo)   ↑ type (azul)  ↑ template parameter (ciano)

class GpioPin {
// ↑ keyword        ↑ class name (amarelo)

    static_assert(PIN < ALLOY_GENERATED_NAMESPACE::traits::max_gpio_pins,
    // ↑ keyword       ↑ parameter      ↑ namespace (verde)    ↑ constant
                  "GPIO pin not available");
                  // ↑ string (laranja)
};
```

## Fluxo Completo: Do CMake ao LSP

```
┌─────────────────────────────────────────────────────────────┐
│ 1. Usuário edita CMakeLists.txt                            │
│    set(ALLOY_MCU "STM32F103C8")                             │
└──────────────────────┬──────────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────────┐
│ 2. CMake configura                                          │
│    - Detecta: vendor=st, family=stm32f1, mcu=stm32f103c8   │
│    - Define: ALLOY_GENERATED_NAMESPACE=...stm32f103c8       │
│    - Adiciona include: src/generated/st/stm32f1/stm32f103c8│
└──────────────────────┬──────────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────────┐
│ 3. CMake gera compile_commands.json                         │
│    {                                                         │
│      "file": "src/hal/st/stm32f1/gpio.cpp",                │
│      "command": "g++ -DALLOY_GENERATED_NAMESPACE=...        │
│                      -I.../stm32f103c8 ..."                 │
│    }                                                         │
└──────────────────────┬──────────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────────┐
│ 4. IDE (VS Code) detecta compile_commands.json             │
│    - Extensão C/C++ ou clangd lê o arquivo                 │
│    - Configura IntelliSense automaticamente                 │
└──────────────────────┬──────────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────────┐
│ 5. LSP indexa o código                                      │
│    - Expande todas as macros                                │
│    - Resolve todos os includes                              │
│    - Analisa todas as definições                            │
│    - Constrói índice de símbolos                            │
└──────────────────────┬──────────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────────┐
│ 6. Usuário edita código                                     │
│    ✅ Autocomplete funciona                                 │
│    ✅ Go to definition funciona                             │
│    ✅ Error checking em tempo real                          │
│    ✅ Hover mostra tipos e valores                          │
└─────────────────────────────────────────────────────────────┘
```

## Teste Prático

Abra `src/hal/st/stm32f1/gpio.hpp` no VS Code e teste:

### ✅ Teste 1: Hover
- Posicione o mouse sobre `ALLOY_GENERATED_NAMESPACE` (linha 22)
- Deve mostrar: `alloy::generated::stm32f103c8`

### ✅ Teste 2: Go to Definition
- `Ctrl+Click` em `gpio::Registers` (linha 22)
- Deve abrir `src/generated/st/stm32f1/stm32f103c8/peripherals.hpp`
- Deve pular para a definição de `struct Registers`

### ✅ Teste 3: Autocomplete
- Digite `ALLOY_GENERATED_NAMESPACE::traits::`
- Deve mostrar lista de membros: `max_gpio_pins`, `has_usart1`, etc.

### ✅ Teste 4: Error Checking
- Adicione temporariamente: `GpioPin<200> test;`
- Deve mostrar erro sublinhado vermelho
- Hover mostra: "GPIO pin not available on this MCU"

### ✅ Teste 5: Find All References
- `Ctrl+Click` com botão direito em `max_gpio_pins`
- Escolha "Find All References"
- Deve mostrar todos os lugares onde é usado

## Troubleshooting Visual

### ❌ Problema: ALLOY_GENERATED_NAMESPACE sublinhado vermelho

**Sintoma:**
```cpp
using GpioPort = ALLOY_GENERATED_NAMESPACE::gpio::Registers;
                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^
                 ❌ identifier is undefined
```

**Causa:** LSP não encontrou o `compile_commands.json`

**Solução:**
1. Verifique: `ls build/compile_commands.json` existe?
2. Se não, rode: `cmake -S . -B build -DALLOY_BOARD=bluepill`
3. Recarregue VS Code: `Ctrl+Shift+P` → "Developer: Reload Window"

### ❌ Problema: Autocomplete vazio

**Sintoma:**
```cpp
ALLOY_GENERATED_NAMESPACE::|
                          ↑ nenhuma sugestão aparece
```

**Causa:** IntelliSense não indexou os arquivos gerados

**Solução:**
1. Verifique: `ls src/generated/st/stm32f1/stm32f103c8/peripherals.hpp`
2. Se não existir, gere: `cd tools/codegen && python3 generator.py ...`
3. Reconfigure: `cmake -S . -B build`
4. Reset IntelliSense: `Ctrl+Shift+P` → "C/C++: Reset IntelliSense Database"

### ❌ Problema: Go to Definition não funciona

**Sintoma:** `Ctrl+Click` não faz nada ou abre arquivo errado

**Causa:** Include paths no `compile_commands.json` estão incorretos

**Solução:**
1. Verifique no `compile_commands.json`:
   ```bash
   cat build/compile_commands.json | grep -A5 gpio.cpp
   ```
2. Deve ter: `-I/full/path/to/src/generated/st/stm32f1/stm32f103c8`
3. Se não tiver, o CMake não configurou corretamente
4. Reconfigure com MCU: `cmake -S . -B build -DALLOY_MCU=STM32F103C8`

## Resumo

✅ **O LSP funciona perfeitamente** quando:
1. `compile_commands.json` existe e está atualizado
2. Arquivos gerados existem no local correto
3. IDE está configurada para usar o `compile_commands.json`

✅ **Features disponíveis:**
- Autocomplete em tempo real
- Go to definition/declaration
- Find all references
- Hover para ver tipos e valores
- Error checking em tempo real
- Semantic highlighting
- Code navigation

✅ **Performance:**
- Indexação inicial: ~5-10 segundos
- Updates incrementais: instantâneos
- Zero impacto na compilação

O sistema funciona **exatamente como se** `ALLOY_GENERATED_NAMESPACE` fosse um namespace normal, porque o LSP expande a macro antes de processar!
