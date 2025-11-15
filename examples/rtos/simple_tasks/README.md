# Simple RTOS Multi-Task Example

Exemplo básico demonstrando o RTOS Alloy com múltiplas tarefas em diferentes prioridades.

## Visão Geral

Este exemplo mostra:
- **Múltiplas tarefas** - 4 tarefas com diferentes prioridades
- **Escalonamento preemptivo** - Tarefas de alta prioridade interrompem tarefas de baixa prioridade
- **RTOS::delay()** - Delays precisos usando o tick do RTOS
- **Integração SysTick** - O SysTick fornece o tick de 1ms para o scheduler
- **Footprint de memória** - Cálculo em tempo de compilação (~1.7 KB RAM)

## Hardware Necessário

- Qualquer placa suportada:
  - Nucleo-F401RE (STM32F401RE @ 84 MHz)
  - Nucleo-F722ZE (STM32F722ZE @ 180 MHz)
  - Nucleo-G071RB (STM32G071RB @ 64 MHz)
  - Nucleo-G0B1RE (STM32G0B1RE @ 64 MHz)
  - SAME70 Xplained (ATSAME70Q21B @ 12 MHz)
- Cabo USB para programação

## Tarefas Implementadas

### 1. High Priority Task (Prioridade Alta)
```cpp
Task<512, Priority::High> high_task(high_priority_task_func, "HighTask");
```
- **Stack**: 512 bytes
- **Período**: 100ms
- **Ação**: Toggle LED rápido
- **Comportamento**: Preempta todas as outras tarefas

### 2. Normal Priority Task (Prioridade Normal)
```cpp
Task<512, Priority::Normal> normal_task(normal_priority_task_func, "NormalTask");
```
- **Stack**: 512 bytes
- **Período**: 500ms
- **Ação**: Toggle LED médio
- **Comportamento**: Roda quando high_task está bloqueada

### 3. Low Priority Task (Prioridade Baixa)
```cpp
Task<256, Priority::Low> low_task(low_priority_task_func, "LowTask");
```
- **Stack**: 256 bytes (mínimo)
- **Período**: 1000ms (1 segundo)
- **Ação**: Toggle LED lento
- **Comportamento**: Roda quando tarefas superiores estão bloqueadas

### 4. Idle Task (Prioridade Idle)
```cpp
Task<256, Priority::Idle> idle_task(idle_task_func, "IdleTask");
```
- **Stack**: 256 bytes
- **Período**: Contínuo
- **Ação**: Coloca CPU em sleep (WFI)
- **Comportamento**: Roda apenas quando todas as outras tarefas estão bloqueadas

## Build e Flash

### Nucleo-F401RE
```bash
make nucleo-f401re-rtos-simple-tasks-build
make nucleo-f401re-rtos-simple-tasks-flash
```

### Nucleo-F722ZE
```bash
make nucleo-f722ze-rtos-simple-tasks-build
make nucleo-f722ze-rtos-simple-tasks-flash
```

### Nucleo-G071RB
```bash
make nucleo-g071rb-rtos-simple-tasks-build
make nucleo-g071rb-rtos-simple-tasks-flash
```

### Nucleo-G0B1RE
```bash
make nucleo-g0b1re-rtos-simple-tasks-build
make nucleo-g0b1re-rtos-simple-tasks-flash
```

### SAME70 Xplained
```bash
make same70-xplained-rtos-simple-tasks-build
make same70-xplained-rtos-simple-tasks-flash
```

## Comportamento Esperado

O LED da placa irá piscar mostrando a execução das tarefas:
- A tarefa de **maior prioridade ready** sempre executa primeiro
- Quando uma tarefa chama `RTOS::delay()`, ela é bloqueada e o scheduler executa a próxima tarefa de maior prioridade
- O padrão de pisca do LED reflete qual tarefa está executando no momento

**Exemplo de Timeline:**
```
Tempo (ms) | Tarefa Executando | LED
-----------|-------------------|-----
0-100      | high_task         | ON
100-200    | high_task         | OFF
200-300    | high_task         | ON
...        | ...               | ...
```

## Uso de Memória

### RAM Total: ~1724 bytes

**Task Control Blocks (TCBs):**
- 4 tarefas × 32 bytes = 128 bytes

**Stacks das Tarefas:**
- high_task: 512 bytes
- normal_task: 512 bytes
- low_task: 256 bytes
- idle_task: 256 bytes
- **Total**: 1536 bytes

**RTOS Core:**
- Ready queue: ~36 bytes
- Scheduler state: ~24 bytes
- **Total**: ~60 bytes

## Integração SysTick

O exemplo demonstra a integração SysTick implementada na Fase 1:

### No board.cpp
```cpp
extern "C" void SysTick_Handler() {
    // Atualiza tick do HAL
    board::BoardSysTick::increment_tick();

    // Encaminha para scheduler RTOS (habilitado via -DALLOY_RTOS_ENABLED)
    #ifdef ALLOY_RTOS_ENABLED
        alloy::rtos::RTOS::tick();
    #endif
}
```

### No main.cpp
```cpp
int main() {
    board::init();      // Inicializa SysTick com 1ms tick
    RTOS::start();      // Inicia scheduler (nunca retorna)
    return 0;
}
```

## Conceitos Fundamentais

### Escalonamento Preemptivo por Prioridade

O scheduler sempre executa a tarefa **ready** de maior prioridade:

```
Prioridades (0-7):
7 = Critical  (mais alta)
6 = Highest
5 = Higher
4 = High
3 = Normal    ← Padrão
2 = Low
1 = Lowest
0 = Idle      (mais baixa)
```

### Estados das Tarefas

Cada tarefa pode estar em um dos seguintes estados:

- **Ready**: Pronta para executar
- **Running**: Executando atualmente
- **Blocked**: Aguardando em IPC (Queue, Mutex, Semaphore)
- **Delayed**: Aguardando delay expirar
- **Suspended**: Suspensa manualmente

### RTOS::delay()

```cpp
RTOS::delay(100);  // Delay de 100ms
```

O que acontece:
1. Tarefa atual é marcada como **Delayed**
2. Tarefa é removida da ready queue
3. Scheduler escolhe próxima tarefa de maior prioridade
4. Context switch para nova tarefa
5. Após 100ms, SysTick ISR acorda a tarefa
6. Tarefa retorna para ready queue

## Próximos Passos

Após dominar este exemplo básico, explore:

1. **IPC entre tarefas**:
   - Queues - Passar mensagens entre tarefas
   - Mutexes - Proteger recursos compartilhados
   - Semaphores - Sincronização de tarefas
   - Event Flags - Sinalização de eventos

2. **Padrões avançados**:
   - Producer/Consumer com queues
   - Priority inheritance com mutexes
   - Task notifications (IPC leve)
   - Software timers

3. **Otimização**:
   - Medição de uso de stack
   - Detecção de stack overflow
   - Profiling de tempo de execução
   - Análise de latência de interrupção

## Troubleshooting

### LED não pisca
- ✅ Certifique-se que `board::init()` é chamado antes de `RTOS::start()`
- ✅ Verifique que o SysTick está configurado (feito em `board::init()`)
- ✅ Confirme que `-DALLOY_RTOS_ENABLED` está definido no build

### Comportamento errático
- ⚠️ Stack overflow? Aumente o tamanho do stack das tarefas
- ⚠️ Prioridades conflitantes? Revise as prioridades das tarefas
- ⚠️ SysTick muito rápido/lento? Deve ser exatamente 1ms

### Build falhando
- ❌ Faltando arquivos RTOS? Verifique que `src/rtos/scheduler.cpp` existe
- ❌ Erros de linkagem? Certifique-se que platform context está incluído
- ❌ Stack muito pequeno? Mínimo é 256 bytes, 8-byte aligned

## Objetivos de Aprendizado

Após trabalhar com este exemplo, você deve entender:

1. ✅ Como criar e registrar tarefas RTOS
2. ✅ Como funciona o escalonamento por prioridade
3. ✅ Como usar RTOS::delay() para sincronização
4. ✅ Como o SysTick integra com o RTOS
5. ✅ Como calcular footprint de memória em tempo de compilação

## Referências

- **Alloy RTOS Documentation**: `src/rtos/rtos.hpp`
- **SysTick Integration**: OpenSpec `integrate-systick-rtos-improvements`
- **Board Abstraction**: `boards/<board>/board.hpp`
- **ARM Cortex-M RTOS**: ARM CMSIS-RTOS specification

---

**Nota**: Este exemplo usa o RTOS Alloy nativo. Para outros RTOS (FreeRTOS, Zephyr, etc.), consulte os exemplos de integração específicos.
