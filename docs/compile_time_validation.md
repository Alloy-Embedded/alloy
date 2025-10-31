# Compile-Time Resource Validation

Alloy HAL usa validação em tempo de compilação para garantir que você não tente usar recursos que não existem no seu MCU alvo.

## Como Funciona

### 1. Metadados Gerados Automaticamente

Quando o código é gerado para um MCU específico, o `peripherals.hpp` inclui metadados sobre os recursos disponíveis:

```cpp
namespace alloy::generated::stm32f103c8::traits {
    // Flash e RAM
    constexpr uint32_t flash_size_kb = 64;
    constexpr uint32_t ram_size_kb = 20;

    // Disponibilidade de periféricos
    constexpr bool has_gpio = true;
    constexpr uint32_t num_gpio_instances = 7;

    constexpr bool has_usart = true;
    constexpr uint32_t num_usart_instances = 5;

    // GPIO específico
    constexpr uint32_t num_gpio_ports = 7;     // GPIOA-GPIOG
    constexpr uint32_t max_gpio_pins = 112;    // 7 portas × 16 pinos

    // USART específico
    constexpr bool has_usart1 = true;
    constexpr bool has_usart2 = true;
    constexpr bool has_usart3 = true;
    constexpr bool has_uart4 = true;
    constexpr bool has_uart5 = true;
}
```

### 2. Validação no HAL

O código HAL usa `static_assert` para validar em tempo de compilação:

#### Exemplo: GPIO Pin

```cpp
template<uint8_t PIN>
class GpioPin {
    // Valida se o pino existe no MCU
    static_assert(PIN < ALLOY_GENERATED_NAMESPACE::traits::max_gpio_pins,
                  "GPIO pin not available on this MCU - check your pin number");

    // Valida se a porta existe no MCU
    static_assert(static_cast<uint8_t>(port) < ALLOY_GENERATED_NAMESPACE::traits::num_gpio_ports,
                  "GPIO port not available on this MCU - this port doesn't exist");
};
```

#### Exemplo: UART Device

```cpp
template<UsartId ID>
class UartDevice {
    // Valida se o periférico USART existe
    static_assert(ALLOY_GENERATED_NAMESPACE::traits::has_usart,
                  "USART peripheral not available on this MCU");

    // Valida se a instância específica existe
    static_assert(
        (ID == UsartId::USART1 && ALLOY_GENERATED_NAMESPACE::traits::has_usart1) ||
        (ID == UsartId::USART2 && ALLOY_GENERATED_NAMESPACE::traits::has_usart2) ||
        (ID == UsartId::USART3 && ALLOY_GENERATED_NAMESPACE::traits::has_usart3),
        "This USART instance is not available on the target MCU");
};
```

## Benefícios

### 1. Erros Claros em Tempo de Compilação

Em vez de bugs sutis em tempo de execução, você recebe erros claros durante a compilação:

```cpp
// Tentando usar um pino que não existe
GpioPin<150> invalid_pin;  // ❌ ERRO DE COMPILAÇÃO:
// "GPIO pin not available on this MCU - check your pin number"
```

### 2. Código Seguro por Design

Impossível usar recursos inexistentes - o código simplesmente não compila:

```cpp
// STM32F103C8 tem 7 portas GPIO (A-G)
GpioPin<112> pg0;  // ❌ ERRO: Port G não existe neste chip
```

### 3. Zero Overhead em Runtime

Toda validação acontece em compile-time, sem custo em runtime:
- Sem verificações if/else
- Sem código extra no binário final
- Performance máxima

## Exemplos de Uso

### Válido ✅

```cpp
// STM32F103C8 - 48 pinos, tem GPIOA, GPIOB, GPIOC

// GPIO válido
GpioPin<13> pa13;  // PA13 - OK
GpioPin<45> pc13;  // PC13 (LED BluePill) - OK

// USART válido
UartDevice<UsartId::USART1> serial1;  // OK
UartDevice<UsartId::USART2> serial2;  // OK
```

### Inválido ❌

```cpp
// Pino fora do range
GpioPin<200> invalid;
// ❌ Erro: "GPIO pin not available on this MCU"

// Porta que não existe
GpioPin<112> pg0;  // Port G
// ❌ Erro: "GPIO port not available on this MCU"
```

## Para Desenvolvedores de HAL

### Adicionando Validação em Novos Periféricos

1. **Adicione traits no template** (`templates/peripherals/stm32_peripherals.hpp.j2`):

```jinja2
// No namespace traits:
{% if mcu.peripherals.SPI is defined %}
constexpr bool has_spi = true;
constexpr uint32_t num_spi_instances = {{ mcu.peripherals.SPI.instances | length }};
{% for instance in mcu.peripherals.SPI.instances %}
constexpr bool has_{{ instance.name | lower }} = true;
{% endfor %}
{% endif %}
```

2. **Adicione validação no HAL**:

```cpp
template<SpiId ID>
class SpiDevice {
    static_assert(ALLOY_GENERATED_NAMESPACE::traits::has_spi,
                  "SPI peripheral not available on this MCU");

    static_assert(
        (ID == SpiId::SPI1 && ALLOY_GENERATED_NAMESPACE::traits::has_spi1) ||
        (ID == SpiId::SPI2 && ALLOY_GENERATED_NAMESPACE::traits::has_spi2),
        "This SPI instance is not available on the target MCU");
};
```

## Diferenças Entre MCUs da Mesma Família

Mesmo dentro da mesma família (ex: STM32F1), diferentes MCUs têm recursos diferentes:

| MCU | Pacote | Flash | RAM | GPIO Ports | USARTs |
|-----|--------|-------|-----|------------|--------|
| STM32F103C4 | 48-pin | 16KB | 6KB | A,B,C | 1,2 |
| STM32F103C8 | 48-pin | 64KB | 20KB | A,B,C | 1,2,3 |
| STM32F103R8 | 64-pin | 64KB | 20KB | A,B,C,D | 1,2,3 |
| STM32F103V8 | 100-pin | 64KB | 20KB | A,B,C,D,E | 1,2,3,4,5 |

O sistema de validação garante que seu código funcione corretamente em todos esses casos.

## Testing

Veja os exemplos em `tests/compile_tests/`:
- `test_gpio_validation.cpp` - Testes de validação GPIO
- `test_uart_validation.cpp` - Testes de validação UART

Para testar que a validação funciona:
1. Descomente um teste "SHOULD_FAIL"
2. Compile - deve falhar com mensagem clara
3. Re-comente o teste

## Perguntas Frequentes

### P: O que acontece se eu tentar usar um pino que não existe?

R: O código não compila. Você recebe uma mensagem de erro clara indicando o problema.

### P: Isso afeta a performance?

R: Não. Toda validação é feita em tempo de compilação. Zero overhead em runtime.

### P: Como sei quais recursos meu MCU tem?

R: Verifique o arquivo gerado `peripherals.hpp` na seção `traits`. Ou consulte o datasheet do MCU.

### P: Posso desabilitar a validação?

R: Não é recomendado, mas tecnicamente você poderia comentar os `static_assert`. Porém, isso remove a segurança em tempo de compilação.

### P: A validação funciona para todos os vendors?

R: Sim, desde que o código gerado inclua os metadados de traits. Atualmente implementado para STM32, mas o sistema é genérico.
