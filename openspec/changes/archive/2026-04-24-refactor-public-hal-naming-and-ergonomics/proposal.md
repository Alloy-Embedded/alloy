## Why

`alloy` já tem uma história boa de board helpers e uma arquitetura interna mais escalável que a
média, mas a sintaxe crua da HAL ainda cobra contexto demais do usuário.

Hoje a comparação com `modm` mostra um padrão claro:

- a camada `board` do `alloy` já está competitiva
- a camada HAL direta continua mais verbosa que o necessário
- a verbosidade vem mais de repetição estrutural do que de precisão útil

Exemplos do problema atual:

- `alloy::device::PeripheralId::USART2`
- `alloy::device::PinId::PA2`
- `alloy::device::SignalId::signal_tx`
- `alloy::hal::connection::connector<...>`
- `alloy::hal::gpio::pin_handle<alloy::device::pin<...>>`

Esses nomes são corretos como contrato canônico interno, mas não são a melhor superfície pública
para uso manual, documentação, exemplos e tooling.

## What Changes

- introduzir uma façade pública curta e explícita sobre o mesmo HAL
- manter os ids canônicos gerados como contrato interno estável
- padronizar aliases públicos para device ids, pins, rotas e papéis de sinal
- atualizar docs, exemplos e diagnósticos para ensinar a superfície ergonômica
- definir migração compatível e faseada sem quebrar o runtime/device boundary

## Outcome

Depois dessa change:

- a sintaxe pública do `alloy` fica mais curta e mais ensinável
- a API continua específica o suficiente para uso expert e multi-vendor
- o contrato gerado continua estável e não vira refém da ergonomia pública
- docs, exemplos e tooling passam a falar a mesma linguagem user-facing

## Impact

- Affected specs:
  - `public-hal-api`
  - `migration-cleanup`
  - `runtime-tooling`
- Affected code and docs:
  - `src/device/**`
  - `src/hal/**`
  - `boards/**`
  - `examples/**`
  - `docs/COOKBOOK.md`
  - `docs/MIGRATION_GUIDE.md`
  - `docs/ARCHITECTURE.md`
  - `scripts/alloyctl.py`
