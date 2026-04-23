## Why

O `runtime-async-model` já existe como surface, mas ainda está raso no uso real.

Hoje o repo já tem:

- `time`
- `event`
- `interrupt_event`
- `dma_event`
- adapters iniciais

Mas ainda falta virar vantagem clara frente ao Embassy:

- examples reais
- drivers mostrando completion/evento útil
- coordenação melhor com `low_power`
- documentação de uso prático

## What Changes

- transformar o modelo async/event/time em fluxo real de app
- adicionar examples e probes que usem completions tipados
- endurecer a integração com DMA/UART e wake/time

## Outcome

Depois dessa change, `async/time/event` deixa de ser só infraestrutura e vira parte usável do
produto.

## Impact

- Affected specs:
  - `runtime-async-model`
  - `public-hal-api`
  - `runtime-validation`
- Affected code and docs:
  - `src/runtime/**`
  - `examples/**`
  - `tests/host_mmio/**`
  - `docs/RUNTIME_ASYNC_MODEL.md`
