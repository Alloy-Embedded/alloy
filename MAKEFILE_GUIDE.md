# Makefile Guide - CoreZero Framework

## Quick Start

```bash
# Ver todos os comandos disponíveis
make help

# Build e flash rápido para SAME70 (exemplo genérico recomendado!)
make same70-blink

# Build e test para host (desenvolvimento)
make all
```

## 📋 Comandos Disponíveis

### 🎯 SAME70 Xplained - Exemplos Genéricos (Recomendado)

Estes exemplos usam a **Board Abstraction Layer** e funcionam em qualquer placa!

```bash
# Build e flash do exemplo genérico blink_led
make same70-blink                    # Build + Flash (tudo em um comando!)
make same70-blink-generic-build      # Apenas build
make same70-blink-generic-flash      # Build + Flash

# Limpar build
make same70-clean
```

**Por que usar os exemplos genéricos?**
- ✅ Código portável - funciona em qualquer board
- ✅ Interface consistente
- ✅ Mais fácil de manter
- ✅ Sem código específico de hardware

### 📦 SAME70 Xplained - Exemplos Legacy

Exemplos antigos específicos para SAME70 (a serem migrados):

```bash
# Blink LED (legacy - board-specific)
make same70-build                    # Build
make same70-flash                    # Flash
make same70-rebuild                  # Clean + build

# Clock test
make same70-clock-build
make same70-clock-flash

# SysTick test
make same70-systick-build
make same70-systick-flash
```

### 🖥️ Host (Linux/macOS) - Desenvolvimento

Para desenvolvimento e testes sem hardware:

```bash
make all           # Clean + build + test
make build         # Build tudo
make test          # Rodar testes
make rebuild       # Clean + build
make quick         # Build rápido (sem clean/configure)
```

### 🔍 Qualidade de Código

```bash
make format        # Formatar código com clang-format
make format-check  # Verificar formatação
make lint          # Rodar clang-tidy
make check         # format-check + lint + test
make ci            # Pipeline completo (CI)
```

### 🧹 Limpeza

```bash
make clean         # Limpar build directory
make clean-all     # Limpar tudo (incluindo cache)
make same70-clean  # Limpar build SAME70
```

### ℹ️ Informação

```bash
make help          # Mostrar ajuda
make info          # Mostrar configuração e versões
```

## 🚀 Fluxo de Trabalho Típico

### Desenvolvimento com SAME70

```bash
# 1. Build e flash exemplo genérico
make same70-blink

# 2. LED deve piscar (500ms ON, 500ms OFF)

# 3. Modificar código em examples/blink_led/main.cpp

# 4. Rebuild e flash
make same70-blink

# 5. Testar novamente
```

### Desenvolvimento Host (sem hardware)

```bash
# 1. Build e test
make all

# 2. Modificar código

# 3. Quick rebuild
make quick

# 4. Rodar testes
make test
```

### CI/CD Pipeline

```bash
# Rodar pipeline completo
make ci

# Isso executa:
# - clean
# - build
# - format-check
# - lint
# - test
```

## 🔧 Configuração

### Variáveis de Ambiente

Você pode customizar o build editando variáveis no início do Makefile:

```makefile
BUILD_DIR := build-sanitizer    # Diretório de build para host
BOARD := host                    # Board padrão
JOBS := 8                        # Número de jobs paralelos
CC := clang                      # Compilador C
CXX := clang++                   # Compilador C++
```

### Toolchain ARM

O Makefile espera o toolchain ARM xPack em:

```
~/Library/xPacks/@xpack-dev-tools/arm-none-eabi-gcc/14.2.1-1.1.1/.content/bin
```

Se estiver em local diferente, edite a variável `XPACK_ARM_BASE` no Makefile.

**Instalar toolchain:**

```bash
./scripts/install-xpack-toolchain.sh
```

## 📝 Detalhes dos Comandos

### `make same70-blink`

**O que faz:**
1. Verifica se ARM toolchain está instalado
2. Configura CMake para SAME70 com board abstraction
3. Compila o exemplo `blink_led` genérico
4. Mostra uso de memória
5. Conecta via OpenOCD
6. Faz flash do binário
7. Reseta a placa

**Saída esperada:**
```
========================================
Building Generic Blink LED (SAME70)
========================================

This uses the board abstraction layer
Same code works on any supported board!

✓ ARM GCC found: arm-none-eabi-gcc (xPack GNU Arm Embedded GCC x86_64) 14.2.1
✓ CMake configured
✓ Building...
✨ Build complete!

Output files:
-rwxr-xr-x  1 user  staff   15K blink_led.elf
-rw-r--r--  1 user  staff   2.1K blink_led.hex
-rwxr-xr-x  1 user  staff   1.9K blink_led.bin

Memory usage:
   text    data     bss     dec     hex filename
   1856      12      96    1964     7ac blink_led

========================================
Flashing Generic Blink LED
========================================

Connecting to board...
✅ Flash complete!

Expected behavior:
  - LED blinks with 500ms ON, 500ms OFF pattern
  - Continues indefinitely

💡 This same code works on:
  - SAME70 Xplained Ultra
  - Arduino Zero
  - Waveshare RP2040 Zero
  - Any board with board abstraction support!
```

### `make same70-systick-flash`

Flash o exemplo de teste do SysTick com delays precisos.

**Comportamento esperado:**
- 3 blinks rápidos = System startup
- 5 blinks médios = Clock + SysTick inicializados
- 1Hz blink = Operação normal (100ms on, 400ms off)
- Blink extra longo a cada 10 segundos = Marcador de 10 segundos

### `make test`

Roda todos os testes unitários usando Catch2.

**Testes incluídos:**
- Core utilities
- HAL interfaces
- Logger
- RTOS primitives
- Scoped device management

### `make lint`

Roda clang-tidy em todo o código fonte.

**Arquivos verificados:**
- `src/**/*.cpp`
- `src/**/*.hpp`
- `tests/**/*.cpp`
- `tests/**/*.hpp`

**Arquivos excluídos:**
- `src/hal/vendors/*` (código gerado)
- Build artifacts
- Dependências externas

### `make format`

Formata todo o código usando clang-format.

**Estilo:** Definido em `.clang-format`

## ⚠️ Troubleshooting

### Erro: ARM toolchain not found

```bash
✗ ARM toolchain not found at /path/to/toolchain
  Run: ./scripts/install-xpack-toolchain.sh
```

**Solução:**
```bash
./scripts/install-xpack-toolchain.sh
```

### Erro: OpenOCD not found

```bash
✗ OpenOCD not found
  Install with: brew install openocd
```

**Solução:**
```bash
brew install openocd
```

### Erro: Flash failed

```bash
✗ Flash failed!
Check:
  - USB cable connected to EDBG port
  - Board powered on
  - OpenOCD installed: brew install openocd
```

**Checklist:**
1. ✅ Cabo USB conectado na porta EDBG (não na USB device)
2. ✅ Placa ligada (LED de power aceso)
3. ✅ OpenOCD instalado e no PATH
4. ✅ Drivers USB funcionando
5. ✅ Apenas uma placa conectada

**Testar conexão:**
```bash
openocd -f board/atmel_same70_xplained.cfg
```

Deve mostrar:
```
Info : Listening on port 3333 for gdb connections
Info : ATSAME70Q21 (512K flash)
```

### Erro: Board not detected

Algumas vezes o macOS não detecta a placa. Tente:

```bash
# Ver dispositivos USB
system_profiler SPUSBDataType | grep -A 10 "EDBG"

# Reiniciar placa
# 1. Desconectar USB
# 2. Pressionar RESET
# 3. Conectar USB
# 4. Tentar flash novamente
```

### Erro: Permission denied (macOS)

```bash
sudo chmod 666 /dev/cu.*
```

Ou adicione seu usuário ao grupo `dialout`:
```bash
sudo dseditgroup -o edit -a $USER -t user dialout
```

### Build lento

Se o build estiver lento, aumente o número de jobs:

```bash
# No Makefile, mude:
JOBS := 16  # ou número de CPUs disponíveis
```

Ou override na linha de comando:
```bash
make same70-blink JOBS=16
```

## 🎓 Exemplos de Uso

### Criar Novo Exemplo Genérico

1. Criar diretório:
```bash
mkdir examples/meu_exemplo
```

2. Criar `main.cpp`:
```cpp
#include BOARD_HEADER

int main() {
    board::init();

    // Seu código aqui

    return 0;
}
```

3. Criar `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.25)
project(meu_exemplo)

set(CMAKE_CXX_STANDARD 20)
set(SOURCES main.cpp)

if(CMAKE_CROSSCOMPILING AND DEFINED STARTUP_SOURCE)
    list(APPEND SOURCES ${STARTUP_SOURCE})
endif()

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${ALLOY_HAL_LIBRARY})
```

4. Adicionar no `CMakeLists.txt` principal:
```cmake
if(DEFINED BOARD_HEADER_PATH)
    add_subdirectory(examples/meu_exemplo)
endif()
```

5. Adicionar target no Makefile:
```makefile
same70-meu-exemplo-build: ## Build meu exemplo
	@PATH="$(ARM_TOOLCHAIN):$$PATH" cmake --build $(SAME70_BUILD_DIR) --target meu_exemplo -j$(JOBS)

same70-meu-exemplo-flash: same70-meu-exemplo-build ## Flash meu exemplo
	@openocd -f board/atmel_same70_xplained.cfg \
		-c "program $(SAME70_BUILD_DIR)/examples/meu_exemplo/meu_exemplo verify reset exit"
```

6. Build e flash:
```bash
make same70-meu-exemplo-flash
```

### Debug com GDB

1. Iniciar OpenOCD em um terminal:
```bash
openocd -f board/atmel_same70_xplained.cfg
```

2. Em outro terminal, iniciar GDB:
```bash
arm-none-eabi-gdb build-same70/examples/blink_led/blink_led

# No GDB:
(gdb) target remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
```

### Usar BOSSA ao invés de OpenOCD

Se preferir usar BOSSA:

```bash
# Colocar placa em modo bootloader
# 1. Pressionar ERASE
# 2. Pressionar RESET
# 3. Soltar RESET
# 4. Soltar ERASE

# Flash
bossac -e -w -v -b -R build-same70/examples/blink_led/blink_led.bin
```

## 📚 Referências

- [Board Abstraction Design](docs/BOARD_ABSTRACTION_DESIGN.md)
- [Board Abstraction Quick Start](docs/BOARD_ABSTRACTION_QUICK_START.md)
- [Generic Blink Example](examples/blink_led/README.md)
- [SAME70 Xplained Board](boards/same70_xplained/README.md)

## 🔗 Links Úteis

- [xPack ARM Toolchain](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack)
- [OpenOCD](https://openocd.org/)
- [BOSSA](https://github.com/shumatech/BOSSA)
- [SAME70 Xplained Ultra](https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/ATSAME70-XPLD)
