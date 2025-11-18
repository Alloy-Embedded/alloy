# Peripheral Type Aliases - Usage Guide

## Overview

Este guia explica como usar os **type aliases** de periféricos no board SAME70 Xplained Ultra, seguindo o padrão **OpenSpec Template Parameters** (REQ-TP-008).

## Status da Implementação

✅ **Completo** - Todos os periféricos seguem o padrão OpenSpec

### Periféricos Implementados (SAME70)

| Periférico | Template Signature | Type Aliases | OpenSpec Req |
|------------|-------------------|--------------|--------------|
| **GPIO** | `GpioPin<PORT_BASE, PIN_NUM>` | Via `pins::` namespace | REQ-TP-001 |
| **I2C** | `I2c<BASE_ADDR, IRQ_ID>` | I2c0, I2c1, I2c2 | REQ-TP-002 |
| **SPI** | `Spi<BASE_ADDR, IRQ_ID>` | Spi0, Spi1 | REQ-TP-003 |
| **Timer** | `Timer<BASE_ADDR, IRQ_ID>` | Timer0, Timer1, Timer2, Timer3 | REQ-TP-004 |
| **PWM** | `Pwm<BASE_ADDR, IRQ_ID>` | Pwm0, Pwm1 | REQ-TP-005 |
| **ADC** | `Adc<BASE_ADDR, IRQ_ID>` | Adc0, Adc1 | REQ-TP-006 |
| **DMA** | `Dma<BASE_ADDR, IRQ_ID>` | Dma | REQ-TP-007 |
| **UART** | `Uart<BASE_ADDR, IRQ_ID>` | Uart0, Uart1, Uart2 | *(Added)* |

## Como Usar

### Opção 1: Board Header (RECOMENDADO)

A forma mais simples e portável:

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init();  // Initialize board

    // Usar periféricos diretamente pelos aliases
    Uart0 uart;
    Spi0 spi;
    I2c0 i2c;
    Timer0 timer;
    Adc0 adc;
}
```

### Opção 2: Platform Header Direto

Para mais controle ou uso fora de boards:

```cpp
#include "hal/platform/same70/uart.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

using namespace alloy::hal::same70;
using namespace alloy::generated::atsame70q21b;

int main() {
    // Template direto
    Uart<peripherals::USART0, id::USART0> uart;

    // Ou usar alias da plataforma
    Uart0 uart0;
}
```

## Exemplos Completos

### Exemplo 1: UART Hello World

```cpp
#include "boards/same70_xplained/board.hpp"
#include "hal/uart_expert.hpp"

using namespace alloy::boards::same70_xplained;
using namespace alloy::hal;

int main() {
    board::init();

    // Configurar UART0
    UartExpertConfig uart_config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false
    };

    Uart0 uart;
    uart.initialize(uart_config);

    // Enviar mensagem
    const char* msg = "Hello World\r\n";
    uart.write_buffer(reinterpret_cast<const u8*>(msg), 13);

    while (true) {
        board::delay_ms(1000);
    }
}
```

### Exemplo 2: SPI Flash

```cpp
#include "boards/same70_xplained/board.hpp"
#include "hal/platform/same70/spi.hpp"

using namespace alloy::boards::same70_xplained;
using namespace alloy::hal::same70;

int main() {
    board::init();

    // Inicializar SPI0
    Spi0 spi;
    spi.open();
    spi.configureChipSelect(SpiChipSelect::CS0, 10, SpiMode::Mode0);

    // Ler ID do flash (comando 0x9F)
    u8 tx[4] = {0x9F, 0x00, 0x00, 0x00};
    u8 rx[4] = {0};

    spi.transfer(tx, rx, 4, SpiChipSelect::CS0);

    // rx[1], rx[2], rx[3] contém o ID do flash

    spi.close();
}
```

### Exemplo 3: I2C EEPROM

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init();

    // Inicializar I2C0
    I2c0 i2c;
    i2c.open();
    i2c.setSpeed(I2cSpeed::Standard100kHz);

    // Escrever para EEPROM (endereço 0x50)
    u8 write_data[] = {0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF};  // addr + data
    i2c.write(0x50, write_data, 6);

    board::delay_ms(5);  // EEPROM write cycle time

    // Ler de volta
    u8 addr_buf[] = {0x00, 0x00};
    i2c.write(0x50, addr_buf, 2);  // Set address

    u8 read_data[4];
    i2c.read(0x50, read_data, 4);

    i2c.close();
}
```

### Exemplo 4: Timer Periódico

```cpp
#include "boards/same70_xplained/board.hpp"
#include "hal/timer_expert.hpp"

using namespace alloy::boards::same70_xplained;
using namespace alloy::hal;

int main() {
    board::init();

    // Configurar Timer0 para 1ms
    auto timer_config = TimerExpertConfig::periodic_interrupt(
        PeripheralId::TIMER0,
        1000  // 1000 us = 1 ms
    );

    Timer0 timer;
    timer.initialize(timer_config);
    timer.start();

    while (true) {
        // Timer rodando em background
        board::delay_ms(100);
    }
}
```

### Exemplo 5: ADC Leitura Simples

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init();

    // Inicializar ADC0
    Adc0 adc;
    adc.open();

    while (true) {
        // Ler canal 0
        auto result = adc.read(0);
        if (result.is_ok()) {
            u16 value = result.unwrap();
            // value está em 0-4095 (12-bit)
        }

        board::delay_ms(100);
    }
}
```

### Exemplo 6: GPIO Pins

```cpp
#include "boards/same70_xplained/board.hpp"

using namespace alloy::boards::same70_xplained;

int main() {
    board::init();

    // Usar pins convenientes do board
    pins::Led0 led;
    led.setMode(GpioMode::Output);
    led.clear();  // LED on (active-low)

    pins::Button0 button;
    button.setMode(GpioMode::Input);
    button.enablePullUp();

    while (true) {
        if (!button.read()) {  // Button pressed (active-low)
            led.toggle();
            board::delay_ms(20);  // Debounce
        }
    }
}
```

## Periféricos Disponíveis por Alias

### UART (3 instâncias)
- `Uart0` → USART0 (0x40024000, IRQ 13)
- `Uart1` → USART1 (0x40028000, IRQ 14)
- `Uart2` → USART2 (0x4002C000, IRQ 15)

### SPI (2 instâncias)
- `Spi0` → SPI0 (0x40008000, IRQ 21)
- `Spi1` → SPI1 (0x40058000, IRQ 42)

### I2C (3 instâncias)
- `I2c0` → TWIHS0 (0x40018000, IRQ 19)
- `I2c1` → TWIHS1 (0x4001C000, IRQ 20)
- `I2c2` → TWIHS2 (0x40060000, IRQ 41)

### Timer (4 instâncias)
- `Timer0` → TC0 (0x4000C000, IRQ 23)
- `Timer1` → TC1 (0x40010000, IRQ 26)
- `Timer2` → TC2 (0x40014000, IRQ 47)
- `Timer3` → TC3 (0x40054000, IRQ 50)

### PWM (2 instâncias)
- `Pwm0` → PWM0 (0x40020000, IRQ 31)
- `Pwm1` → PWM1 (0x4005C000, IRQ 60)

### ADC (2 instâncias)
- `Adc0` → AFEC0 (0x4003C000, IRQ 29)
- `Adc1` → AFEC1 (0x40064000, IRQ 40)

### DMA (1 instância)
- `Dma` → XDMAC (0x40078000, IRQ 58)

### GPIO Ports
- `GpioA` → PIOA (0x400E0E00, IRQ 10)
- `GpioB` → PIOB (0x400E1000, IRQ 11)
- `GpioC` → PIOC (0x400E1200, IRQ 12)
- `GpioD` → PIOD (0x400E1400, IRQ 16)
- `GpioE` → PIOE (0x400E1600, IRQ 17)

## GPIO Pin Aliases Convenientes

Todos no namespace `pins::`:

### LEDs
- `pins::Led0` → PC8 (LED0, active-low)
- `pins::Led1` → PC9 (LED1, active-low)

### Buttons
- `pins::Button0` → PA11 (SW0, active-low)
- `pins::Button1` → PC2 (SW1, active-low)

### UART0 Pins
- `pins::Uart0_Tx` → PA10 (URXD0)
- `pins::Uart0_Rx` → PA9 (UTXD0)

### SPI0 Pins
- `pins::Spi0_Miso` → PD20
- `pins::Spi0_Mosi` → PD21
- `pins::Spi0_Sck` → PD22
- `pins::Spi0_Cs0` → PB2

### I2C0 Pins
- `pins::I2c0_Sda` → PA3 (TWD0)
- `pins::I2c0_Scl` → PA4 (TWCK0)

## Vantagens do Padrão OpenSpec

1. **Zero Overhead**: Todos os endereços resolvidos em compile-time
2. **Type-Safe**: Compilador valida tudo
3. **Múltiplas Instâncias**: Fácil usar Uart0, Uart1, Uart2 simultaneamente
4. **Portável**: Trocar de board = trocar apenas o include
5. **Documentado**: Cada alias tem comentários com base address e IRQ

## Comparação: Antes vs Depois

### ❌ Antes (Preprocessor Dispatch)
```cpp
#include "hal/uart.hpp"

// Não conseguia ter múltiplas instâncias facilmente
Uart<PeripheralId::USART0>::write('H');
```

### ✅ Depois (Template Parameters + Aliases)
```cpp
#include "boards/same70_xplained/board.hpp"
using namespace alloy::boards::same70_xplained;

// Limpo, múltiplas instâncias, zero overhead
Uart0::write('H');
Uart1::write('H');
Uart2::write('H');
```

## Arquivos Relacionados

- **Board Config**: `boards/same70_xplained/board_config.hpp`
- **Platform Implementations**: `src/hal/platform/same70/*.hpp`
- **Peripheral Addresses**: `hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp`
- **OpenSpec**: `openspec/specs/hal-template-peripherals/spec.md`

## Próximos Passos

1. ✅ Type aliases criados
2. ⏭️ Criar exemplos completos funcionando em hardware
3. ⏭️ Adicionar testes de zero-overhead
4. ⏭️ Criar outros boards (STM32F4, ESP32)
