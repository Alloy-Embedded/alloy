# IDE Setup Guide

Este guia explica como configurar sua IDE para funcionar perfeitamente com o Alloy Framework, especialmente com macros CMake como `ALLOY_GENERATED_NAMESPACE`.

## O Problema

O HAL usa macros definidas pelo CMake:
```cpp
#define ALLOY_GENERATED_NAMESPACE alloy::generated::stm32f103c8
```

A IDE precisa saber o valor dessas macros para:
- ✅ Autocomplete
- ✅ Go to definition
- ✅ Error checking
- ✅ Hover documentation

## Solução: compile_commands.json

O CMake gera um arquivo `compile_commands.json` que contém todas as definições de compilação, incluindo macros e include paths.

### Passo 1: Gerar compile_commands.json

O projeto já está configurado para gerar automaticamente:

```cmake
# CMakeLists.txt (linha 4)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

Gere o arquivo rodando CMake:

```bash
cd /path/to/corezero
cmake -S . -B build
```

Isso cria: `build/compile_commands.json`

### Passo 2: Configurar VS Code

#### Opção A: Extensão C/C++ da Microsoft (Recomendado)

1. **Instale a extensão**:
   - [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
   - [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)

2. **Configure no `.vscode/settings.json`** (já criado):
   ```json
   {
       "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
       "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
   }
   ```

3. **Selecione o kit do CMake**:
   - Pressione `Ctrl+Shift+P` → "CMake: Select a Kit"
   - Escolha o compilador (ex: GCC, Clang)

4. **Configure o MCU alvo**:
   - Edite `CMakeLists.txt` ou `cmake/boards/YOUR_BOARD.cmake`
   - Defina: `set(ALLOY_MCU "STM32F103C8")`
   - Reconfigure: `Ctrl+Shift+P` → "CMake: Configure"

5. **Recarregue a janela**:
   - `Ctrl+Shift+P` → "Developer: Reload Window"

#### Opção B: Extensão Clangd (Alternativa)

1. **Instale a extensão**:
   - [clangd extension](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd)
   - **Desabilite** a extensão C/C++ (conflita)

2. **Configure no `.vscode/settings.json`** (já criado):
   ```json
   {
       "clangd.arguments": [
           "--compile-commands-dir=${workspaceFolder}/build",
           "--background-index",
           "--clang-tidy"
       ]
   }
   ```

3. **Arquivo `.clangd`** (já criado na raiz do projeto)

4. **Reconfigure o CMake** para gerar `compile_commands.json`

### Passo 3: Verificar que Funciona

Abra `src/hal/st/stm32f1/gpio.hpp`:

```cpp
using GpioPort = ALLOY_GENERATED_NAMESPACE::gpio::Registers;
//              ^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Hover aqui - deve mostrar: alloy::generated::stm32f103c8
```

Se funcionar, você verá:
- ✅ `ALLOY_GENERATED_NAMESPACE` resolve para o namespace correto
- ✅ `Ctrl+Click` em `gpio::Registers` vai para o `peripherals.hpp`
- ✅ Autocomplete funciona ao digitar `ALLOY_GENERATED_NAMESPACE::`
- ✅ Nenhum erro de "identifier undefined"

## Configuração Multi-MCU

Se você trabalha com vários MCUs, pode criar **build directories separados**:

```bash
# Build para STM32F103C8
cmake -S . -B build-stm32f103c8 -DALLOY_MCU=STM32F103C8

# Build para nRF52840
cmake -S . -B build-nrf52840 -DALLOY_MCU=nRF52840

# Build para ESP32
cmake -S . -B build-esp32 -DALLOY_MCU=ESP32
```

Depois, atualize `.vscode/settings.json`:

```json
{
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build-stm32f103c8/compile_commands.json"
}
```

Ou crie múltiplos **workspaces do VS Code** (Arquivo → Adicionar Pasta ao Workspace).

## Troubleshooting

### ❌ "Identifier ALLOY_GENERATED_NAMESPACE is undefined"

**Causa**: IDE não encontrou `compile_commands.json`

**Solução**:
```bash
# 1. Verifique que o arquivo existe
ls build/compile_commands.json

# 2. Se não existir, gere-o
cmake -S . -B build

# 3. Reconfigure a IDE
# VS Code: Ctrl+Shift+P → "Developer: Reload Window"
```

### ❌ "Go to definition" não funciona

**Causa**: Caminho do `compile_commands.json` está errado

**Solução**: Verifique `.vscode/settings.json`:
```json
{
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

### ❌ Autocomplete não funciona

**Causa 1**: CMake não configurou com MCU alvo

**Solução**:
```bash
# Configure com MCU específico
cmake -S . -B build -DALLOY_BOARD=bluepill

# Ou defina diretamente o MCU
cmake -S . -B build -DALLOY_MCU=STM32F103C8
```

**Causa 2**: IntelliSense está usando configuração antiga

**Solução**:
```
Ctrl+Shift+P → "C/C++: Reset IntelliSense Database"
```

### ❌ Clangd mostra muitos erros

**Causa**: Clangd não está usando `compile_commands.json`

**Solução**: Verifique arquivo `.clangd` na raiz:
```yaml
CompileFlags:
  CompilationDatabase: build
```

## Como o Sistema Funciona

### 1. CMake Define Macros

```cmake
# src/hal/CMakeLists.txt
target_compile_definitions(alloy_hal PUBLIC
    ALLOY_GENERATED_NAMESPACE=alloy::generated::${MCU_LOWER}
)
```

### 2. CMake Exporta para compile_commands.json

```json
[
  {
    "directory": "/path/to/corezero/build",
    "command": "g++ -DALLOY_GENERATED_NAMESPACE=alloy::generated::stm32f103c8 ...",
    "file": "/path/to/corezero/src/hal/st/stm32f1/gpio.cpp"
  }
]
```

### 3. IDE Lê compile_commands.json

A extensão C/C++ ou clangd lê o arquivo e configura o IntelliSense:
- Sabe que `ALLOY_GENERATED_NAMESPACE` = `alloy::generated::stm32f103c8`
- Sabe os include paths para encontrar `peripherals.hpp`
- Sabe as flags de compilação

### 4. IDE Oferece Features Completas

- **Autocomplete**: `ALLOY_GENERATED_NAMESPACE::traits::|` → mostra `max_gpio_pins`
- **Go to Definition**: `Ctrl+Click` em `gpio::Registers` → abre `peripherals.hpp`
- **Hover**: Mouse sobre variável → mostra tipo completo
- **Error Checking**: Valida `static_assert` em tempo real

## Resumo

✅ **Sempre gere `compile_commands.json`** rodando `cmake -S . -B build`
✅ **Configure a IDE** para usar o arquivo (`.vscode/settings.json`)
✅ **Recarregue a IDE** após mudar o MCU alvo
✅ **Use CMake Tools** no VS Code para facilitar

Com isso, a IDE funciona perfeitamente mesmo com macros complexas do CMake!
