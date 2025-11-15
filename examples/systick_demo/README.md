# SysTick Demo - Tick Modes and RTOS Integration

Demonstra diferentes taxas de tick do SysTick e padrões de integração com RTOS.

## Visão Geral

Este exemplo mostra:
- **Modo 1ms tick** - Padrão RTOS, baixo overhead
- **Modo 100us tick** - Alta resolução, maior overhead
- **Integração RTOS** - Hooks para scheduler (FreeRTOS, Zephyr, etc.)
- **Análise de trade-offs** - Resolução vs overhead da CPU

## Hardware Necessário

- Qualquer placa suportada (Nucleo-F401RE, F722ZE, G071RB, G0B1RE, SAME70)
- Cabo USB para programação
- **Opcional**: Analisador lógico para medir tempo de interrupção

## Conceitos Fundamentais

### Taxa de Tick (Tick Rate)

A taxa de tick determina com que frequência a interrupção SysTick é disparada:

| Taxa | Período | Overhead CPU | Resolução | Uso Típico |
|------|---------|--------------|-----------|------------|
| 100 Hz | 10ms | <0.01% | ±10ms | Sensores lentos, UI |
| 1 kHz | 1ms | ~0.1% | ±1ms | **RTOS padrão** ✅ |
| 10 kHz | 100us | ~1% | ±100us | Controle de motores |
| 100 kHz | 10us | ~10% | ±10us | DSP real-time |

### Overhead da Interrupção

O overhead é calculado como:
```
Overhead = (Tempo_ISR × Frequência_Tick) / 1_000_000
```

Exemplos @ 100 MHz:
- **1ms tick**, ISR de 10us: `(10us × 1000 Hz) = 0.01%` ✅
- **100us tick**, ISR de 10us: `(10us × 10000 Hz) = 0.1%` ✅
- **10us tick**, ISR de 10us: `(10us × 100000 Hz) = 10%` ⚠️

## Integração com RTOS

### Padrão de Integração

O SysTick é usado pelo RTOS para:
1. **Tick do scheduler** - Incrementar contador de tempo
2. **Context switch** - Trocar entre tasks (via PendSV)
3. **Timers de software** - Timeouts, delays de tasks
4. **Sleep/wake** - Acordar tasks em sleep

### Exemplo: FreeRTOS

```cpp
// In board.cpp
extern "C" void SysTick_Handler() {
    // 1. Atualizar tick do HAL
    board::BoardSysTick::increment_tick();

    // 2. Atualizar tick do RTOS
    #if (configUSE_TICKLESS_IDLE == 0)
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            xPortSysTickHandler();
        }
    #endif
}
```

### Exemplo: Zephyr RTOS

```cpp
// In board.cpp
extern "C" void SysTick_Handler() {
    // 1. Atualizar tick do HAL
    board::BoardSysTick::increment_tick();

    // 2. Anunciar tick para Zephyr
    sys_clock_announce(1);
}
```

### Exemplo: CMSIS-RTOS2

```cpp
// In board.cpp
extern "C" void SysTick_Handler() {
    // 1. Atualizar tick do HAL
    board::BoardSysTick::increment_tick();

    // 2. Tick handler do CMSIS-RTOS
    osRtxTick_Handler();
}
```

## Modos Demonstrados

### Modo 1: Tick Padrão 1ms (RTOS-Compatible)

**Características**:
- ✅ Baixo overhead (~0.1% CPU)
- ✅ Padrão para FreeRTOS, Zephyr
- ✅ Adequado para maioria das aplicações
- ⚠️ Resolução de ±1ms

**LED**: Pisca a 2 Hz por 5 segundos

**Código**:
```cpp
// Já configurado em board::init()
SysTickTimer::init_ms<board::BoardSysTick>(1);  // 1ms tick

// Usar delays normalmente
SysTickTimer::delay_ms<board::BoardSysTick>(500);
```

### Modo 2: Alta Resolução 100us

**Características**:
- ✅ Resolução 10x melhor (±100us)
- ✅ Bom para controle de motores, áudio
- ⚠️ Overhead maior (~1% CPU)
- ❌ Não recomendado para RTOS padrão

**LED**: Toggle a 1 kHz (brilho reduzido) por 3 segundos

**Código**:
```cpp
// Reconfigurar para 100us
SysTickTimer::init_us<board::BoardSysTick>(100);

// Loop de controle preciso
while (true) {
    u64 start = SysTickTimer::micros<board::BoardSysTick>();

    // Trabalho do loop (PID, etc)
    update_control_loop();

    // Esperar próximo período (exatamente 1ms)
    while (SysTickTimer::micros<board::BoardSysTick>() - start < 1000);
}
```

### Modo 3: Integração RTOS

**Características**:
- ✅ Tick compartilhado HAL + RTOS
- ✅ Context switch eficiente
- ✅ APIs de timing disponíveis
- ✅ Scheduler funcionando

**LED**: 10 toggles a cada 100ms

**Demonstra**:
- Hook do RTOS no SysTick_Handler
- Sincronização HAL/RTOS ticks
- Uso simultâneo de ambas APIs

### Modo 4: Análise de Trade-offs

**LED**: Taxa de pisca varia com frequência de interrupção
- Lento = 10ms tick (baixa frequência)
- Médio = 1ms tick (padrão)
- Rápido = 100us tick (alta frequência)
- Muito rápido = 10us tick (muito alta)

## Build e Flash

```bash
# Build
make nucleo-f401re-systick-demo-build
make nucleo-f722ze-systick-demo-build
make nucleo-g071rb-systick-demo-build

# Flash
make nucleo-f401re-systick-demo-flash
make nucleo-f722ze-systick-demo-flash
```

## Comportamento Esperado

O LED executa uma sequência contínua:

1. **Demo 1: Tick 1ms** (5s)
   - LED pisca a 2 Hz (500ms ON/OFF)
   - Simula RTOS rodando

2. **Pausa** (2s)

3. **Demo 2: Alta Resolução** (3s)
   - LED toggle a 1 kHz (brilho reduzido)
   - Simula loop de controle preciso

4. **Pausa** (2s)

5. **Demo 3: RTOS Integration** (1s)
   - 10 toggles a 100ms cada
   - Mostra tick sincronizado

6. **Pausa** (2s)

7. **Demo 4: Trade-off Analysis** (20s)
   - Blink lento → médio → rápido → muito rápido
   - Cada padrão por ~4s

**Ciclo total**: ~35 segundos, depois repete

## Medindo Overhead com Logic Analyzer

### Setup
1. Conectar probe ao pino do LED
2. Trigger na borda de subida
3. Medir largura do pulso alto da ISR

### Cálculos

**Overhead %** = `(Tempo_ISR_us / Período_Tick_us) × 100`

Exemplos:
```
Tick 1ms, ISR 10us:   (10 / 1000) × 100 = 1%    ✅ Bom
Tick 100us, ISR 10us: (10 / 100) × 100 = 10%    ⚠️ Alto
Tick 10us, ISR 10us:  (10 / 10) × 100 = 100%    ❌ Impossível!
```

### Regra Geral
**ISR deve consumir <10% do período do tick**

## Escolhendo Taxa de Tick

### Para Aplicações RTOS Genéricas
✅ **Use 1ms tick (1 kHz)**
- Padrão do FreeRTOS, Zephyr, CMSIS-RTOS
- Overhead desprezível
- Resolução adequada para UI, comunicação, sensores

### Para Controle em Tempo Real
✅ **Use 100us tick (10 kHz)**
- Bom para: Motores, IMU, áudio
- Overhead aceitável (~1%)
- Combine com hardware timers para PWM

### Para Baixo Consumo
✅ **Use 10ms tick (100 Hz)**
- Minimiza acordar CPU
- Bom para: Sensores lentos, datalogging
- Economiza bateria

### Para DSP/Controle Muito Rápido
❌ **NÃO use SysTick!**
- Use hardware timers (TIM1-TIM8)
- Use DMA para amostragem
- SysTick tem overhead muito alto >10 kHz

## Código RTOS-Ready

### board.cpp com RTOS

```cpp
// Habilitado condicionalmente
extern "C" void SysTick_Handler() {
    // HAL tick (sempre)
    board::BoardSysTick::increment_tick();

    // RTOS tick (se habilitado)
    #ifdef ALLOY_RTOS_ENABLED
        #if defined(USE_FREERTOS)
            xPortSysTickHandler();
        #elif defined(USE_ZEPHYR)
            sys_clock_announce(1);
        #elif defined(USE_CMSIS_RTOS)
            osRtxTick_Handler();
        #endif
    #endif

    // Application hook (opcional)
    #ifdef ALLOY_SYSTICK_HOOK
        app_systick_hook();
    #endif
}
```

### CMakeLists.txt com RTOS

```cmake
# Opção para habilitar RTOS
option(ALLOY_RTOS_ENABLED "Enable RTOS integration" OFF)

if(ALLOY_RTOS_ENABLED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ALLOY_RTOS_ENABLED=1
    )

    # Link RTOS library
    target_link_libraries(${PROJECT_NAME} PRIVATE freertos)
endif()
```

## Troubleshooting

### LED não pisca
- ✅ Board inicializado? `board::init()` chamado?
- ✅ SysTick configurado? (feito em `board::init()`)

### Timing impreciso
- ⚠️ Outras interrupções bloqueando SysTick?
- ⚠️ ISR muito longa (>10% do período)?
- ⚠️ Clock source instável (HSI vs HSE)?

### Overhead muito alto
- ❌ ISR fazendo muito trabalho?
- ❌ Taxa de tick muito alta para aplicação?
- ✅ Mova processamento pesado para fora da ISR

### RTOS não agenda tasks
- ⚠️ Tick do RTOS sendo chamado?
- ⚠️ PendSV habilitado?
- ⚠️ Prioridades corretas (SysTick > PendSV)?

## Objetivos de Aprendizado

Após trabalhar com este exemplo, você deve entender:

1. ✅ Trade-off entre resolução de timing e overhead da CPU
2. ✅ Como integrar SysTick com diferentes RTOS
3. ✅ Quando usar 1ms vs 100us vs hardware timers
4. ✅ Como medir e minimizar overhead de interrupção
5. ✅ Padrões de código RTOS-ready e portável

## Próximos Passos

- **FreeRTOS Integration**: Projeto completo com tasks e queues
- **Hardware Timers**: Quando usar TIM ao invés de SysTick
- **Low Power**: Tickless idle mode para economizar bateria
- **Performance**: Profiling de ISRs com DWT cycle counter

## Referências

- **ARM Cortex-M SysTick**: ARM Architecture Reference Manual
- **FreeRTOS**: https://www.freertos.org/RTOS-SysTick-tick-interrupt-ARM-Cortex-M.html
- **Zephyr**: https://docs.zephyrproject.org/latest/kernel/services/timing/clocks.html
- **CMSIS-RTOS**: https://arm-software.github.io/CMSIS_5/RTOS2/html/group__CMSIS__RTOS__KernelCtrl.html

---

**Nota**: Este exemplo demonstra padrões de integração mas não inclui um RTOS real. Para usar com RTOS, adicione a biblioteca apropriada (FreeRTOS, Zephyr, etc.) ao seu projeto.
