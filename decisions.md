# Alloy: Decisões Técnicas (ADR - Architecture Decision Records)

Este documento registra as principais decisões técnicas do projeto Alloy, incluindo o contexto, as alternativas consideradas e a justificativa para cada escolha.

---

## ADR-001: Usar C++20 em vez de C++23

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Precisamos decidir qual versão do C++ usar como base para o framework. C++23 oferece features interessantes, mas C++20 já está mais maduro.

### Decisão

Usaremos **C++20** como padrão base do projeto.

### Alternativas Consideradas

1. **C++23**: Features mais modernas (std::print, ranges melhorados, etc)
2. **C++20**: Já tem ótimo suporte em GCC 11+, mais estável
3. **C++17**: Máxima compatibilidade, mas perde Concepts e Ranges

### Justificativa

- **Toolchain support**: GCC 11+ tem suporte sólido a C++20
- **arm-none-eabi-gcc**: GCC 11 já está disponível para ARM embedded
- **Estabilidade**: C++23 ainda tem muitas features experimentais
- **Suficientemente moderno**: C++20 tem Concepts, Ranges, consteval - suficiente para nossas necessidades
- **Evita bleeding edge**: Reduz risco de bugs de compilador

### Consequências

- ✅ Ampla compatibilidade com toolchains embarcados
- ✅ Features modernas suficientes (Concepts, Ranges)
- ❌ Não podemos usar std::print (mas podemos implementar similar)
- ❌ Ranges ainda não tem todas as melhorias do C++23

---

## ADR-002: Não usar C++ Modules

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

C++20 introduziu Modules como forma de substituir headers. Modules oferecem compilação mais rápida e melhor encapsulamento, mas ainda têm suporte experimental em muitos compiladores.

### Decisão

**NÃO** usaremos C++ Modules. Continuaremos com headers tradicionais (`.hpp`/`.h`).

### Alternativas Consideradas

1. **C++ Modules**: Compilação rápida, encapsulamento melhor
2. **Headers tradicionais**: Compatibilidade total, suporte universal
3. **Híbrido**: Modules onde disponível, fallback para headers

### Justificativa

- **Toolchain embedded**: arm-none-eabi-gcc ainda tem suporte experimental/incompleto a modules
- **CMake**: Suporte a modules em CMake ainda está evoluindo
- **IDE support**: VSCode/CLion ainda têm limitações com modules
- **Pragmatismo**: Headers funcionam perfeitamente, não há necessidade urgente de modules
- **Evitar complexidade**: Não queremos debuggar problemas de toolchain em vez de desenvolver o framework

### Consequências

- ✅ Compatibilidade total com todos os toolchains
- ✅ IDEs funcionam perfeitamente (IntelliSense, navegação)
- ✅ CMake simples e direto
- ❌ Compilação um pouco mais lenta (mas aceitável com ccache)
- ❌ Menos encapsulamento (precisamos de include guards)
- 📝 **Futuro**: Podemos adicionar modules quando o suporte estiver maduro (C++26?)

---

## ADR-003: Não usar Coroutines

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

C++20 introduziu coroutines para programação assíncrona. São úteis para operações I/O, mas têm overhead de memória e complexidade.

### Decisão

**NÃO** usaremos coroutines. Preferimos APIs síncronas com polling e callbacks leves quando necessário.

### Alternativas Consideradas

1. **Coroutines**: Código mais limpo para operações async (`co_await uart.read()`)
2. **Callbacks**: Padrão tradicional, zero overhead
3. **Polling**: Simples, direto, adequado para bare-metal

### Justificativa

- **Overhead de memória**: Coroutines precisam de stack frames, problemático em MCUs com pouca RAM
- **Complexidade**: Coroutines adicionam complexidade conceitual
- **Bare-metal first**: Em bare-metal, polling e interrupts são suficientes
- **MCUs pequenos**: Muitos targets têm apenas 64KB ou menos de RAM
- **Performance previsível**: Polling/callbacks têm timing mais determinístico

### Consequências

- ✅ Zero overhead de memória para async operations
- ✅ Código mais simples de entender e debugar
- ✅ Performance previsível
- ❌ Código assíncrono pode ficar verboso (callback hell)
- ❌ Usuários vindos de linguagens modernas podem estranhar
- 💡 **Solução**: Fornecer helpers e patterns para simplificar código com callbacks

### Exemplo de API

```cpp
// Em vez de (com coroutines):
auto data = co_await uart.read_async();

// Teremos:
uart.read(buffer, [](auto& data) {
    // callback quando dados chegarem
});

// Ou polling simples:
while (!uart.available()) {}
auto data = uart.read();
```

---

## ADR-004: Implementar HAL do Zero (Sem Vendor SDKs)

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Podemos usar SDKs oficiais dos fabricantes (Pico SDK, STM32 HAL) ou implementar tudo do zero.

### Decisão

Implementaremos toda a HAL **do zero**, sem dependências de vendor SDKs (exceto CMSIS headers).

### Alternativas Consideradas

1. **Usar vendor SDKs**: Rápido de implementar, já testado
2. **Wrapper sobre vendor SDKs**: Abstrai diferenças, mas ainda depende deles
3. **Implementação própria**: Controle total, mas mais trabalho

### Justificativa

**Contra vendor SDKs:**
- **Inconsistência**: Cada fabricante tem API diferente (Pico SDK vs STM32 HAL vs ESP-IDF)
- **Bloat**: SDKs geralmente incluem muita coisa desnecessária
- **Complexidade**: STM32 HAL, por exemplo, é extremamente complexo
- **Nossa proposta de valor**: Queremos API consistente e moderna entre plataformas

**A favor de implementação própria:**
- **Controle total**: Podemos otimizar para nosso caso de uso
- **API consistente**: Mesma interface para GPIO no RP2040 e STM32
- **Código limpo**: Sem legacy baggage dos SDKs
- **Aprendizado**: Entendemos profundamente o hardware
- **Tamanho**: Código final muito menor

### Consequências

- ✅ API perfeitamente consistente entre plataformas
- ✅ Código leve e otimizado
- ✅ Controle total sobre implementação
- ✅ Nenhuma "mágica" acontecendo nos bastidores
- ❌ Muito mais trabalho inicial
- ❌ Precisamos ler muitas datasheets
- ❌ Responsabilidade por bugs é nossa
- ⚠️ **Mitigação**: Começar com 2 plataformas (RP2040 + STM32F4), expandir gradualmente

### Exceção: CMSIS Headers

Usaremos **CMSIS headers oficiais** para definições de registradores em ARM Cortex-M:
- São apenas definições (structs, constantes)
- Padrão da indústria
- Zero overhead
- Bem documentado

---

## ADR-005: Usar CMSIS Headers Oficiais

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Precisamos acessar registradores de periféricos. Podemos gerar nossas próprias definições ou usar CMSIS.

### Decisão

Usaremos **CMSIS headers oficiais** da ARM/fabricantes para definições de registradores.

### Alternativas Consideradas

1. **CMSIS headers oficiais**: Padrão da indústria, mantidos pelos fabricantes
2. **Gerar nossas definições**: A partir de SVD files
3. **Definições inline**: Escrever nossas próprias structs manualmente

### Justificativa

- **Padrão da indústria**: Todos os fabricantes ARM fornecem CMSIS
- **Bem documentado**: Cada registrador tem doc oficial
- **Mantido**: Fabricantes atualizam quando há silicon revisions
- **Tipos corretos**: Structs com layout correto garantido
- **Zero overhead**: São apenas definições, não há código executável
- **Facilita porting**: Novo MCU? Só precisa do CMSIS header dele

### O que NÃO usaremos de CMSIS

- ❌ CMSIS-RTOS: Vamos bare-metal
- ❌ CMSIS-DSP: Podemos adicionar depois se necessário
- ❌ CMSIS-Driver: Implementaremos nossa própria HAL

### O que usaremos

- ✅ CMSIS-Core: Definições de registradores do core ARM
- ✅ Device headers: Definições específicas do MCU (ex: `stm32f446xx.h`)

### Consequências

- ✅ Não precisamos gerar/manter definições de registradores
- ✅ Compatibilidade com documentação oficial
- ✅ Fácil adicionar novos MCUs (só incluir o CMSIS header)
- ❌ Dependência externa (mas mínima, são só headers)
- ❌ Estilo de código não é totalmente nosso (mas aceitável)

---

## ADR-006: MVP no Host (Sem Hardware Inicialmente)

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Podemos começar implementando direto para hardware ou criar uma versão host-simulated primeiro.

### Decisão

O **MVP (Phase 0)** será desenvolvido para **host** (Linux/macOS/Windows) com HAL mockada, antes de implementar para hardware real.

### Alternativas Consideradas

1. **Host first**: Desenvolver mocks primeiro, validar arquitetura
2. **Hardware first**: Começar direto com RP2040 ou STM32
3. **Paralelo**: Desenvolver host e hardware juntos

### Justificativa

**Vantagens do host-first:**
- **Ciclo de desenvolvimento rápido**: Compile → run em segundos, sem flash
- **Debugging fácil**: GDB, valgrind, sanitizers funcionam perfeitamente
- **Validar arquitetura**: Garantir que concepts, interfaces e padrões funcionam
- **Testar testabilidade**: Provar que a arquitetura realmente permite testes
- **Sem hardware necessário**: Qualquer desenvolvedor pode contribuir
- **CI/CD simples**: Testes rodam em GitHub Actions sem hardware especial

**Desvantagens:**
- Não valida código real de hardware até mais tarde
- Pode descobrir problemas de hardware depois

### Mitigação de Riscos

- Implementação host deve ser **realista** (não trivial demais)
- Usar referências reais (datasheets) mesmo no mock
- Assim que possível, portar para hardware real (Phase 1)

### Consequências

- ✅ Desenvolvimento inicial muito mais rápido
- ✅ Fácil para novos contribuidores
- ✅ CI/CD trivial de configurar
- ✅ Prova que a arquitetura é testável
- ⚠️ Precisamos garantir que host mock é realista
- 📝 Phase 1 deve começar LOGO após Phase 0

---

## ADR-007: Bare-Metal First, RTOS Opcional Depois

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Muitos frameworks embarcados assumem uso de RTOS (FreeRTOS, Zephyr). Isso adiciona complexidade.

### Decisão

Começaremos com suporte **bare-metal only**. RTOS será adicionado depois como feature opcional.

### Alternativas Consideradas

1. **Bare-metal only**: Sem RTOS, apenas polling/interrupts
2. **RTOS first**: Assumir FreeRTOS desde o início
3. **RTOS obrigatório**: Framework depende de RTOS

### Justificativa

- **Simplicidade**: Bare-metal é mais simples de entender e debugar
- **Zero overhead**: Sem overhead de scheduler
- **Adequado para muitos casos**: Muitas aplicações embarcadas não precisam de RTOS
- **Foco**: Queremos validar a HAL primeiro, RTOS adiciona muitas variáveis
- **Portabilidade**: Bare-metal funciona em qualquer MCU

### Futuro: RTOS Opcional

Em Phases futuras, adicionaremos **suporte opcional** a:
- FreeRTOS
- Zephyr (talvez)

**Como funcionará:**
```cpp
// Bare-metal (default)
#include "hal/gpio.hpp"
auto led = make_gpio_pin<25, Output>();

// Com FreeRTOS (opcional, via compile flag)
#define ALLOY_USE_FREERTOS
#include "hal/gpio.hpp"
#include "rtos/task.hpp"

void led_task(void*) {
    auto led = make_gpio_pin<25, Output>();
    while(1) {
        led.toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

### Consequências

- ✅ Escopo inicial reduzido
- ✅ Código mais simples
- ✅ Funciona em qualquer MCU
- ✅ Usuários que não precisam de RTOS não pagam o overhead
- ⏭️ Precisaremos pensar em thread-safety quando adicionar RTOS
- 💡 API deve ser projetada pensando em futuro suporte a RTOS (mesmo que não implementado ainda)

---

## ADR-008: CMake Puro (Sem Ferramentas Customizadas para Usuário)

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

modm usa lbuild (ferramenta customizada). Podemos fazer similar ou usar CMake puro.

### Decisão

Usaremos **CMake 100% puro**. Qualquer ferramenta interna (codegen) será **invocada automaticamente pelo CMake**, invisível ao usuário.

### Alternativas Consideradas

1. **CMake puro**: Padrão da indústria, integração perfeita com IDEs
2. **Ferramenta customizada** (tipo lbuild): Mais flexível, mas barreira de adoção
3. **Outros build systems**: Meson, Bazel, etc

### Justificativa

**Por que CMake:**
- **Padrão da indústria**: Todo desenvolvedor C++ conhece
- **IDE support**: VSCode, CLion, Visual Studio funcionam perfeitamente
- **Ecossistema**: Ferramentas como ccache, clang-tidy, etc funcionam nativamente
- **Sem curva de aprendizado extra**: Já é CMake, não precisa aprender ferramenta nova
- **Transparência**: Usuário pode ver exatamente o que está acontecendo

**Contra ferramentas customizadas (lbuild-like):**
- Barreira de adoção
- Break IDE integration
- Mais uma coisa para aprender
- Dificulta debugging

### Geração de Código

Teremos um **code generator interno** (Python), mas:
- Será **invocado automaticamente pelo CMake**
- Usuário nunca precisa chamá-lo manualmente
- Transparente: código gerado fica em `build/generated/` (visível, navegável)

### Consequências

- ✅ Zero friction para adoção
- ✅ IDEs funcionam perfeitamente
- ✅ Ferramental C++ padrão funciona
- ✅ Fácil integrar em projetos existentes
- ⚠️ Precisamos fazer CMake files muito bem documentados
- 💡 Template repository com CMakeLists.txt exemplar

---

## ADR-009: Zero Dynamic Allocation na HAL

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Em sistemas embarcados, alocação dinâmica (`new`/`delete`) pode ser problemática (fragmentação, imprevisibilidade).

### Decisão

A **HAL não fará nenhuma alocação dinâmica**. Tudo será estático ou em stack.

### Alternativas Consideradas

1. **Zero allocation**: Apenas stack e static storage
2. **Allocation permitida**: Usar heap normalmente
3. **Custom allocators**: std::pmr, memory pools, etc

### Justificativa

- **Previsibilidade**: Uso de memória é conhecido em compile-time
- **Determinismo**: Sem variação de timing por alocações
- **Safety**: Sem fragmentação de heap
- **MCUs pequenos**: Muitos têm heap limitado ou inexistente
- **Best practice**: Maioria dos embedded coding standards proíbe heap

### Regras

**Na HAL (nossa implementação):**
- ❌ Não usar `new` / `delete`
- ❌ Não usar `malloc` / `free`
- ❌ Não usar `std::vector`, `std::string` (alocam dinamicamente)
- ✅ Usar `std::array` (tamanho fixo)
- ✅ Usar stack allocations
- ✅ Usar static storage

**Na aplicação do usuário:**
- ✅ Usuário PODE usar heap se quiser
- ✅ É escolha do usuário, não impomos
- 💡 Mas documentaremos best practices

### Exemplos

```cpp
// ❌ NÃO na HAL:
class UartDriver {
    std::vector<uint8_t> rx_buffer;  // Aloca dinamicamente!
};

// ✅ SIM na HAL:
template<size_t BUFFER_SIZE = 256>
class UartDriver {
    std::array<uint8_t, BUFFER_SIZE> rx_buffer;  // Tamanho fixo
};

// ✅ Usuário pode fazer:
std::vector<SensorReading> readings;  // OK na app
readings.push_back(sensor.read());
```

### Consequências

- ✅ Uso de memória previsível
- ✅ Zero fragmentação
- ✅ Segue best practices embedded
- ⚠️ Templates com tamanhos podem poluir namespace (mitigar com defaults sensatos)
- 💡 Documentar claramente que usuário pode usar heap se quiser

---

## ADR-010: Google Test para Unit Tests

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Precisamos de um framework de testes para unit tests (que rodarão no host).

### Decisão

Usaremos **Google Test (gtest)** como framework de testes.

### Alternativas Consideradas

1. **Google Test**: Padrão de facto em C++, muito features
2. **Catch2**: Header-only, mais leve
3. **doctest**: Ainda mais leve, compilation rápida
4. **Custom**: Escrever nosso próprio (não vale a pena)

### Justificativa

- **Industry standard**: Google Test é o mais usado
- **Features**: Mocking (gmock), parametrized tests, fixtures
- **IDE integration**: VSCode, CLion têm suporte nativo
- **CI integration**: Fácil integrar com GitHub Actions
- **Maturidade**: Extremamente maduro e testado
- **Documentação**: Extensa documentação e exemplos

### Consequências

- ✅ Framework robusto e completo
- ✅ Boa integração com ferramentas
- ✅ Comunidade grande (fácil achar ajuda)
- ❌ Dependência externa (mas via CMake FetchContent é trivial)
- ❌ Um pouco mais pesado que alternativas (mas roda só no host, ok)

---

## Resumo das Decisões

| # | Decisão | Status | Impacto |
|---|---------|--------|---------|
| 001 | C++20 (não C++23) | ✅ Aceito | Alto - define features disponíveis |
| 002 | Sem C++ Modules | ✅ Aceito | Alto - afeta organização de código |
| 003 | Sem Coroutines | ✅ Aceito | Médio - afeta API async |
| 004 | HAL do zero | ✅ Aceito | Alto - define escopo de trabalho |
| 005 | Usar CMSIS headers | ✅ Aceito | Médio - facilita implementação |
| 006 | MVP no host | ✅ Aceito | Alto - define Phase 0 |
| 007 | Bare-metal first | ✅ Aceito | Alto - simplifica escopo inicial |
| 008 | CMake puro | ✅ Aceito | Alto - define DX |
| 009 | Zero alloc na HAL | ✅ Aceito | Médio - afeta design da API |
| 010 | Google Test | ✅ Aceito | Baixo - só afeta testes |
| 011 | snake_case naming | ✅ Aceito | Médio - define estilo de código |
| 012 | Result<T, Error> | ✅ Aceito | Alto - afeta toda API de erros |
| 013 | Low-memory support (8KB) | ✅ Aceito | Crítico - define arquitetura |

---

## ADR-011: Naming Conventions (snake_case)

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Precisamos definir um padrão de nomenclatura consistente para todo o projeto.

### Decisão

Usaremos **snake_case** para código (funções, variáveis, arquivos) e **PascalCase** para tipos (classes, structs).

### Padrão Completo

**Arquivos:**
- Headers: `snake_case.hpp` (ex: `gpio_pin.hpp`)
- Sources: `snake_case.cpp` (ex: `uart_driver.cpp`)

**Código C++:**
- Namespaces: `snake_case` (ex: `alloy::hal::`)
- Classes/Structs: `PascalCase` (ex: `GpioPin`, `UartDriver`)
- Funções/Métodos: `snake_case` (ex: `set_high()`, `read_byte()`)
- Variáveis: `snake_case` (ex: `led_pin`, `baud_rate`)
- Constantes: `UPPER_SNAKE_CASE` (ex: `MAX_BUFFER_SIZE`)
- Template params: `PascalCase` (ex: `template<typename PinImpl>`)

**CMake:**
- Variáveis: `ALLOY_` prefix + `UPPER_SNAKE_CASE`
- Funções: `alloy_` prefix + `snake_case`

**Macros:**
- `ALLOY_` prefix + `UPPER_SNAKE_CASE`

### Justificativa

- **snake_case para funções**: Mais legível em embedded, comum em C++ moderno (STL usa)
- **PascalCase para tipos**: Distinção clara entre tipos e valores
- **Consistência com STL**: Funções snake_case como `std::vector::push_back()`
- **Facilita porting**: Muitos desenvolvedores embedded vêm de C (snake_case)

### Consequências

- ✅ Consistente com STL
- ✅ Legível e moderno
- ✅ Distinção clara entre tipos e valores
- ❌ Diferente de Google C++ Style Guide (mas Ok)

---

## ADR-012: Custom Error Codes (Não Exceptions)

**Data:** 2025-10-29
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Precisamos decidir como lidar com erros. Exceptions são problemáticas em embedded (overhead, stack unwinding).

### Decisão

Usaremos **custom error codes** com tipo `Result<T, Error>` inspirado em Rust.

### Design

```cpp
// src/core/error.hpp
namespace alloy::core {

enum class ErrorCode {
    Ok = 0,
    InvalidParameter,
    Timeout,
    Busy,
    NotSupported,
    HardwareError,
    // ... peripheral-specific codes
};

template<typename T>
class Result {
public:
    // Success case
    static Result ok(T value) {
        return Result(std::move(value));
    }

    // Error case
    static Result error(ErrorCode code) {
        return Result(code);
    }

    bool is_ok() const { return has_value_; }
    bool is_error() const { return !has_value_; }

    T& value() { return value_; }
    ErrorCode error() const { return error_; }

private:
    bool has_value_;
    union {
        T value_;
        ErrorCode error_;
    };
};

} // namespace alloy::core
```

### Uso

```cpp
// HAL API retorna Result
Result<uint8_t> uart_read_byte() {
    if (!uart_available()) {
        return Result<uint8_t>::error(ErrorCode::Timeout);
    }
    return Result<uint8_t>::ok(read_register());
}

// Usuário verifica
auto result = uart_read_byte();
if (result.is_ok()) {
    uint8_t data = result.value();
} else {
    // Handle error
    handle_error(result.error());
}
```

### Justificativa

- **Zero exceptions**: Sem overhead de exception handling
- **Explícito**: Forçam o usuário a lidar com erros
- **Zero cost**: Sem alocações, compile-time otimizado
- **Type-safe**: Compiler ajuda a garantir tratamento correto
- **Familiar**: Similar a Rust `Result<T, E>`

### Consequências

- ✅ Zero overhead
- ✅ Determinístico (sem stack unwinding)
- ✅ Type-safe
- ✅ Forçam tratamento de erros
- ❌ Mais verboso que exceptions (mas mais seguro)
- 💡 Usuário ainda pode usar exceptions na aplicação se quiser

---

## ADR-013: Support for Low-Memory MCUs (8KB-16KB RAM)

**Data:** 2025-10-30
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Muitos MCUs embarcados têm memória RAM extremamente limitada. Por exemplo:
- Renesas RL78: 8KB-16KB RAM típico
- STM32F103C6: 10KB RAM
- ATmega328P: 2KB RAM

Se o framework não for cuidadosamente projetado, pode consumir toda a RAM disponível apenas com overhead, impossibilitando aplicações reais.

### Decisão

O Alloy será **otimizado desde o início para MCUs com apenas 8KB de RAM**, garantindo que aplicações reais possam rodar mesmo em hardware muito limitado.

### Estratégias de Design

#### 1. Zero-Cost Abstractions (C++ Moderno)

```cpp
// ✅ Resolve em compile-time, zero runtime cost
template<uint8_t PIN>
class GpioPin {
    void set_high() {
        PORTB |= (1 << PIN);  // Inline, otimizado para constante
    }
};

// ❌ Virtual dispatch = overhead de vtable
class GpioPin {
    virtual void set_high() = 0;  // Evitar quando possível
};
```

**Regras:**
- ✅ Preferir `template` e `constexpr` sobre runtime dispatch
- ✅ Usar `consteval` para computação em compile-time (C++20)
- ⚠️ Virtual functions apenas quando absolutamente necessário
- ✅ Compiler deve conseguir inline agressivamente

#### 2. Template Bloat Control

Templates podem gerar múltiplas instâncias de código, aumentando uso de Flash e RAM.

```cpp
// ❌ Gera N instâncias diferentes
template<size_t BUFFER_SIZE>
class UartDriver {
    void send(const char* data);  // Corpo diferente para cada SIZE
};

// ✅ Implementação base sem template
class UartDriverBase {
protected:
    void send_impl(uint8_t* buffer, size_t size);  // Uma instância
};

template<size_t BUFFER_SIZE>
class UartDriver : UartDriverBase {
    std::array<uint8_t, BUFFER_SIZE> buffer_;
    void send(const char* data) {
        send_impl(buffer_.data(), buffer_.size());  // Chama base
    }
};
```

**Regras:**
- ⚠️ Evitar código complexo em templates (gera bloat)
- ✅ Extrair lógica não-dependente de template para classe base
- ✅ Usar type erasure quando apropriado
- ✅ Defaults sensatos para tamanhos (ex: `template<size_t SIZE = 64>`)

#### 3. Compile-Time Configuration

Usuário escolhe o que incluir, não pagando por features não usadas.

```cpp
// CMakeLists.txt
set(ALLOY_HAS_UART ON)
set(ALLOY_HAS_I2C OFF)   # Não usado? Não compila
set(ALLOY_HAS_SPI OFF)

// Gera alloy_config.hpp automaticamente
#define ALLOY_HAS_UART 1
// #undef ALLOY_HAS_I2C
// #undef ALLOY_HAS_SPI

// Código usa conditional compilation
#ifdef ALLOY_HAS_UART
namespace alloy::hal {
    class UartDriver { /*...*/ };
}
#endif
```

**Vantagens:**
- Usuário não paga (Flash/RAM) por periféricos não usados
- Linker pode eliminar código morto
- Builds menores e mais rápidos

#### 4. Static Buffers com Tamanhos Configuráveis

```cpp
// Usuário configura no CMakeLists.txt ou compile-time
template<size_t RX_SIZE = 64, size_t TX_SIZE = 64>
class UartDriver {
    std::array<uint8_t, RX_SIZE> rx_buffer_;
    std::array<uint8_t, TX_SIZE> tx_buffer_;

    // Para MCU com 8KB RAM:
    // UartDriver<32, 32> uart;  // 64 bytes total

    // Para MCU com 128KB RAM:
    // UartDriver<1024, 1024> uart;  // 2KB total
};
```

#### 5. Stack Usage Awareness

Em MCUs pequenos, stack overflow é um problema real.

**Regras:**
- ❌ Nunca alocar grandes arrays/structs na stack
- ✅ Grandes buffers devem ser `static` ou no heap (se disponível)
- ✅ Recursão deve ser evitada (ou muito limitada)
- ✅ Documentar stack usage esperado de funções críticas

```cpp
// ❌ Ruim para MCU pequeno
void process_data() {
    uint8_t buffer[1024];  // 1KB na stack!
    // ...
}

// ✅ Melhor
class Processor {
    std::array<uint8_t, 1024> buffer_;  // No objeto (static ou heap)
    void process_data() {
        // usa buffer_ ...
    }
};
```

#### 6. Linker Map Analysis

Forneceremos ferramentas para analisar uso de memória:

```bash
# Alloy deve gerar relatório de memória
cmake --build build --target memory-report

# Output exemplo:
# Memory Usage Report:
# ==================
# Flash: 8432 / 65536 bytes (12.9%)
# RAM:   1024 / 8192  bytes (12.5%)
#
# Top RAM consumers:
# - uart_rx_buffer: 256 bytes
# - i2c_tx_buffer:  128 bytes
# - main_stack:     512 bytes
```

#### 7. Examples para MCUs Pequenos

Criar exemplos específicos demonstrando uso eficiente:

```
examples/
├── blinky-8kb/           # Para MCUs com 8KB
│   ├── main.cpp          # Uso mínimo de RAM
│   └── CMakeLists.txt    # ALLOY_MINIMAL_BUILD=ON
├── uart-echo-16kb/       # Para MCUs com 16KB
└── sensor-network-64kb/  # Para MCUs maiores
```

### Memory Budget Guidelines

Definir budgets recomendados para diferentes classes de MCU:

| MCU Class | RAM Total | Alloy Overhead | App Available | Example MCUs |
|-----------|-----------|----------------|---------------|--------------|
| Tiny      | 2-8 KB    | < 512 bytes    | 1.5-7.5 KB    | ATmega328P, RL78/G10 |
| Small     | 8-32 KB   | < 2 KB         | 6-30 KB       | RL78/G13, STM32F030 |
| Medium    | 32-128 KB | < 8 KB         | 24-120 KB     | STM32F103, RP2040 |
| Large     | 128+ KB   | < 16 KB        | 112+ KB       | STM32F4, ESP32 |

**Objetivo:** Alloy deve funcionar bem na categoria "Small" (8KB RAM) com aplicações reais.

### Compilation Flags para Otimização de Memória

```cmake
# CMake options para builds minimal
option(ALLOY_MINIMAL_BUILD "Optimize for smallest memory footprint" OFF)

if(ALLOY_MINIMAL_BUILD)
    add_compile_options(
        -Os                  # Optimize for size
        -ffunction-sections  # Each function in own section
        -fdata-sections      # Each data in own section
        -flto                # Link-time optimization
    )
    add_link_options(
        -Wl,--gc-sections    # Remove unused sections
        -Wl,--print-memory-usage  # Show memory usage
    )
endif()
```

### Validação

Criar testes específicos para validar uso de memória:

```cpp
// tests/memory/test_gpio_footprint.cpp
#include "hal/gpio.hpp"

// Verificar que GpioPin não aloca nada dinamicamente
static_assert(sizeof(alloy::hal::GpioPin<0>) <= 4);

// Verificar que é trivial (pode ser otimizado agressivamente)
static_assert(std::is_trivially_copyable_v<alloy::hal::GpioPin<0>>);
```

### Justificativa

- **Market reality**: Grande parte do mercado embedded é MCUs pequenos (8-32KB RAM)
- **Diferenciação**: Frameworks modernos (Zephyr, mbed) têm overhead alto
- **Renesas RL78**: Target explícito do projeto, tem 8-16KB típico
- **Learning**: Otimizar para low-memory força boas práticas que beneficiam todos os targets

### Consequências

- ✅ Alloy utilizável em ampla gama de MCUs (inclusive os mais baratos)
- ✅ Performance melhor mesmo em MCUs grandes (menos cache misses)
- ✅ Força boas práticas de design (zero-cost abstractions)
- ⚠️ Mais cuidado no design da API (não podemos ser "preguiçosos")
- ⚠️ Precisa de testes e validação constante de memory footprint
- 📊 Precisamos de tooling para analisar uso de memória

### Métricas de Sucesso

**Phase 0 (Host):**
- [ ] Implementar análise de footprint em CI
- [ ] Cada módulo documentar seu memory budget

**Phase 1 (Hardware):**
- [ ] Exemplo blinky rodando em MCU com 8KB RAM usando < 1KB
- [ ] Exemplo UART echo rodando em MCU com 16KB RAM usando < 2KB
- [ ] Documentação de memory usage para cada periférico

---

## ADR-014: Code Generation com SVD/Database para Suportar Múltiplos MCUs

**Data:** 2025-10-30
**Status:** Aceito
**Decisores:** Equipe Alloy

### Contexto

Para suportar centenas de MCUs de diferentes vendors (STM32, nRF, RL78, ESP32, etc.), precisamos decidir entre:
1. Escrever código manualmente para cada MCU
2. Usar sistema de geração de código baseado em databases

Frameworks de sucesso (modm, Zephyr, libopencm3) usam code generation extensivamente.

### Decisão

Usaremos **sistema de code generation** baseado em:
- **CMSIS-SVD files** (ARM-based MCUs): Parser automático
- **JSON databases** (MCUs sem SVD): Database manual estruturado
- **Python + Jinja2**: Generator + templates
- **Integração CMake**: Geração automática e transparente

### Arquitetura do Sistema

```
SVD/Headers → Parser Python → JSON Database → Generator → C++ Code
                                   ↓
                            (tools/codegen/database/families/)
                                   ↓
                            Jinja2 Templates
                                   ↓
                    (build/generated/STM32F446RE/)
```

**Arquivos gerados:**
- `startup.cpp` - Reset handler, inicialização .data/.bss
- `vectors.cpp` - Vector table específica do MCU
- `registers.hpp` - Structs para acessar periféricos
- `{mcu}.ld` - Linker script com layout de memória
- `system.cpp` - Configuração de clocks e PLLs

### Alternativas Consideradas

| Abordagem | Prós | Contras | Decisão |
|-----------|------|---------|---------|
| **Manual** | Controle total, simplicidade inicial | Não escala, muitos erros, manutenção impossível | ❌ Rejeitado |
| **SVD direto** | Dados oficiais dos vendors | SVD muito complexo, precisa parser | ⚠️ Parte da solução |
| **Code generation** | Escala massivamente, reduz erros | Complexidade inicial, precisa tooling | ✅ **Escolhido** |
| **Library externa** (HAL vendor) | Pronto para usar | Overhead alto, não portável, API ruim | ❌ Rejeitado |

### Componentes do Sistema

#### 1. SVD Parser (`tools/codegen/svd_parser.py`)
```python
def parse_svd(svd_path: Path) -> dict:
    """
    Converte SVD XML → JSON intermediário

    Extrai:
    - Peripherals (GPIO, UART, etc)
    - Memory layout (Flash, RAM)
    - Interrupt vectors
    - Clock configuration
    """
```

#### 2. Code Generator (`tools/codegen/generator.py`)
```python
class CodeGenerator:
    def generate_all(self, mcu: str):
        self.generate_startup()
        self.generate_vectors()
        self.generate_registers()
        self.generate_linker_script()
```

#### 3. Database Format (JSON)
```json
{
  "family": "STM32F4",
  "mcus": {
    "STM32F446RE": {
      "flash": {"size_kb": 512, "base": "0x08000000"},
      "ram": {"size_kb": 128, "base": "0x20000000"},
      "peripherals": {
        "GPIO": {
          "instances": [
            {"name": "GPIOA", "base": "0x40020000"}
          ]
        }
      }
    }
  }
}
```

#### 4. Templates (Jinja2)
```jinja2
{# templates/registers/peripheral_struct.hpp.j2 #}
struct {{ peripheral_name }}_TypeDef {
    {% for reg in registers %}
    volatile uint32_t {{ reg.name }};  // {{ reg.offset }}
    {% endfor %}
};
```

### Coverage de Vendors

| Vendor | Tecnologia | SVD Disponível? | Esforço | Status |
|--------|-----------|-----------------|---------|--------|
| ST (STM32) | ARM Cortex-M | ✅ Sim | Baixo | Planejado Phase 1 |
| Nordic (nRF) | ARM Cortex-M | ✅ Sim | Baixo | Planejado Phase 2 |
| NXP (LPC) | ARM Cortex-M | ✅ Sim | Baixo | Planejado Phase 2 |
| Renesas (RL78) | 16-bit | ❌ Não | Médio | Database manual |
| Espressif (ESP32) | Xtensa | ⚠️ Headers | Médio | Parser de headers |
| Raspberry Pi (RP2040) | ARM Cortex-M0+ | ✅ Sim | Baixo | Planejado Phase 1 |

### Fluxo de Uso (Transparente ao Usuário)

```cmake
# User project CMakeLists.txt
set(ALLOY_BOARD "bluepill")  # STM32F103C8

# CMake automaticamente:
# 1. Detecta que precisa gerar código
# 2. Executa generator.py
# 3. Compila código gerado
# 4. Linka tudo junto
```

**Usuário nunca precisa chamar o generator manualmente!**

### Adicionando Novo MCU

**Com SVD (ARM-based):**
```bash
# 1. Baixar SVD do vendor
wget https://example.com/STM32F446.svd

# 2. Parsear automaticamente
python tools/codegen/svd_parser.py \
    --input STM32F446.svd \
    --output database/families/stm32f4xx.json \
    --merge

# 3. Pronto! MCU suportado
```

**Sem SVD (RL78, ESP32):**
```bash
# 1. Criar database manual (1-2 dias de trabalho)
# 2. Validar com geração de teste
python tools/codegen/generator.py \
    --mcu RL78G13 \
    --database database/families/rl78g13.json \
    --output /tmp/test

# 3. Ajustar database se necessário
# 4. Pronto!
```

### Justificativa

**Escalabilidade:**
- **modm**: 3500+ MCUs suportados com code generation
- **Zephyr**: 1000+ boards
- **libopencm3**: 600+ MCUs
- **Alloy**: Meta de 500+ MCUs até 2026

**Redução de Erros:**
- Endereços de registradores sempre corretos
- Vetores de interrupção sempre na ordem certa
- Linker scripts sempre compatíveis com memória

**Manutenção:**
- 1 bugfix no template = fix em todos os MCUs
- Vendor atualiza SVD = re-parsear e pronto
- Adicionar periférico = update template (não código manual)

**Time to Market:**
- Novo MCU: **Horas** vs **Semanas** (manual)
- Nova família: **Dias** vs **Meses** (manual)

### Consequências

**Positivas:**
- ✅ Suporte massivo a MCUs (centenas)
- ✅ Código gerado é legível, navegável, debugável
- ✅ Zero overhead (tão eficiente quanto manual)
- ✅ Reduz drasticamente erros de digitação
- ✅ Facilita contribuições da comunidade

**Negativas:**
- ⚠️ Complexidade inicial (desenvolver o generator)
- ⚠️ Precisa Python 3.8+ e Jinja2
- ⚠️ MCUs sem SVD precisam database manual
- ⚠️ Templates precisam ser bem testados

**Riscos Mitigados:**
- 📝 Código gerado versionado no git (visibilidade total)
- 📝 Templates bem documentados
- 📝 Testes automatizados do generator
- 📝 Validação que código gerado compila

### Métricas de Sucesso

**Phase 0 (Atual):**
- [ ] Generator MVP funcional
- [ ] 1 template (startup) gerando código válido
- [ ] Integração CMake básica

**Phase 1:**
- [ ] SVD parser funcional (STM32F103)
- [ ] Todos os templates implementados
- [ ] 5+ MCUs STM32 suportados
- [ ] Blinky rodando em hardware gerado

**Phase 2:**
- [ ] 50+ MCUs suportados
- [ ] 3+ vendors (STM32, nRF, RL78)
- [ ] Documentação completa
- [ ] Contribuições da comunidade

### Implementação

**Próximos passos:**
1. Implementar `svd_parser.py` MVP
2. Criar template básico de startup
3. Testar com STM32F103 (Blue Pill)
4. Integrar com CMake
5. Expandir templates (vectors, registers, linker)

**Dependências:**
- Python 3.8+
- Jinja2
- lxml (para parsing SVD XML)
- CMake 3.25+

---

## Decisões Pendentes (Para Discutir)

Estas decisões ainda precisam ser tomadas nas próximas iterações:

### ⏳ ADR-015: Formato de Logging/Debugging

**Opções:**
- `printf`-like tradicional
- Custom logging framework
- Sem logging na HAL (deixar para usuário)

### ⏳ ADR-016: Política de Interrupts

**Opções:**
- Callbacks registrados em compile-time
- Virtual table (runtime)
- Template-based dispatch
- Direct ISR implementation pelo usuário

### ⏳ ADR-017: Clock Configuration

**Opções:**
- Gerado automaticamente (ferramenta tipo STM32CubeMX)
- Configuração manual via CMake
- Runtime configuration
- Templates com validação compile-time

---

**Última atualização:** 2025-10-29
**Próxima revisão:** Após completar Phase 0
