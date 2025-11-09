# SAME70 SysTick Timer Test

Este exemplo demonstra o uso do timer SysTick para delays precisos e monitoramento de tempo no SAME70 Xplained Ultra.

## Funcionalidades Implementadas

### 1. SysTick HAL (`src/hal/platform/same70/systick.hpp`)
- Configuração automática baseada na frequência do MCK
- Timer de 1MHz (resolução de 1us)
- Contador de 32 bits (overflow após ~71 minutos)
- Interrupt-driven para precisão
- Integração com Clock HAL

### 2. Funções de Delay (`src/hal/platform/same70/systick_delay.hpp`)
- `delay_us(us)` - Delay em microsegundos
- `delay_ms(ms)` - Delay em milissegundos
- `micros()` - Tempo atual em microsegundos
- `millis()` - Tempo atual em milissegundos
- `elapsed_us(start)` - Tempo decorrido desde start
- `is_timeout(start, timeout)` - Verifica se timeout expirou
- `get_uptime_seconds()` - Tempo de execução em segundos
- `get_uptime_minutes()` - Tempo de execução em minutos
- `get_uptime_hours()` - Tempo de execução em horas

### 3. Características do Teste
- Inicialização de Clock (RC 12MHz ou PLL 150MHz)
- Inicialização do SysTick com timing preciso
- Blink de LED com delays precisos de 100ms/400ms
- Monitoramento de uptime
- Marker visual a cada 10 segundos

## Como Compilar e Gravar

### Compilar
```bash
make same70-systick-build
```

### Gravar na Placa
```bash
make same70-systick-flash
```

## Comportamento do LED

1. **3 blinks rápidos** (50ms on/off): Sistema iniciado
2. **5 blinks médios** (100ms on/off): Clock e SysTick inicializados com sucesso
3. **Blink contínuo 1Hz**: Operação normal
   - LED on por 100ms
   - LED off por 400ms
   - Total: 500ms por ciclo (2Hz)
4. **Blink extra longo a cada 10 segundos**: Marcador de tempo

## Configurações de Clock Disponíveis

### Opção 1: RC 12MHz (Padrão)
```cpp
ClockConfig config = {
    .main_source = MainClockSource::InternalRC_12MHz,
    .mck_source = MasterClockSource::MainClock,
    .mck_prescaler = MasterClockPrescaler::DIV_1  // 12MHz MCK
};
```

### Opção 2: Crystal + PLL 150MHz (Alta Performance)
```cpp
ClockConfig config = {
    .main_source = MainClockSource::ExternalCrystal,
    .crystal_freq_hz = 12000000,
    .plla = {24, 1},  // 12MHz * 25 / 1 = 300MHz
    .mck_source = MasterClockSource::PLLAClock,
    .mck_prescaler = MasterClockPrescaler::DIV_2  // 300/2 = 150MHz MCK
};
```

## Arquitetura do SysTick

### Configuração Automática
O SysTick é configurado automaticamente com base no MCK:

```
Reload Value = (MCK_Hz / 1,000,000) - 1

Exemplos:
- MCK 12MHz  → Reload = 11   → 1us por tick
- MCK 150MHz → Reload = 149  → 1us por tick
```

### Interrupt Handler
A cada 1us, o interrupt incrementa o contador:

```cpp
extern "C" void SysTick_Handler() {
    alloy::hal::same70::SystemTick::irq_handler();
}
```

### Overflow
O contador de 32 bits overflow após:
- 2^32 microsegundos = 4,294,967,296 us
- ~71.58 minutos
- Arithmetic overflow-safe usando subtração de unsigned

## Uso no RTOS

Este SysTick será usado pelo RTOS para:
1. **Task Switching** - Preempção baseada em tempo
2. **Sleep/Delay** - Suspensão de tasks por tempo determinado
3. **Timeouts** - Operações com timeout configurável
4. **Profiling** - Medição de tempo de execução de tasks

## Tamanho do Binário

```
Memory region         Used Size  Region Size  %age Used
           FLASH:        1588 B         2 MB      0.08%
             RAM:       16440 B       384 KB      4.18%
```

- **Flash**: 1588 bytes (~380 bytes a mais que exemplo básico)
- **RAM**: 16440 bytes (inclui stack e contador do SysTick)

## Precisão do Timing

### Fontes de Erro
1. **Quantização**: ±1us devido à resolução do timer
2. **Interrupt Latency**: ~10-20 ciclos de clock (~67-133ns @ 150MHz)
3. **Jitter**: Mínimo devido ao hardware dedicado

### Accuracy
- **Curto prazo** (< 1s): ±0.01% (10 ppm)
- **Longo prazo**: Limitado pela precisão do crystal (±50ppm típico)

## Debugging

### LED não pisca
1. Verifique conexão USB do programador
2. Verifique se Clock::initialize() retornou Ok
3. Verifique se SystemTick::init() retornou Ok

### LED pisca muito rápido/devagar
1. Verifique configuração do Clock
2. Verifique se MCK está correto: `Clock::getMCKFrequency()`
3. Verifique reload value do SysTick

### LED para após alguns minutos
- Overflow do contador (esperado após ~71 minutos)
- Use `get_uptime_*()` functions que são overflow-safe

## Próximos Passos

1. ✅ Clock HAL implementado
2. ✅ SysTick HAL implementado
3. ✅ Delay functions implementadas
4. ⏳ UART HAL (para debug output)
5. ⏳ RTOS integration
6. ⏳ Task scheduler com SysTick

## Referências

- [SAME70 Datasheet](https://www.microchip.com/wwwproducts/en/ATSAME70Q21)
- [ARM Cortex-M7 TRM](https://developer.arm.com/documentation/ddi0489/latest/)
- [SysTick Timer Reference](https://developer.arm.com/documentation/dui0553/a/cortex-m4-peripherals/system-timer--systick)
