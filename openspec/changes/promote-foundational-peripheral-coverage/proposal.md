## Why

`alloy` já validou bem:

- boundary
- clocks/startup
- gpio
- uart
- dma/time/event base

Mas a support matrix ainda deixa muita HAL principal como `representative` ou `experimental`.

Para competir de verdade, o repo precisa promover o que já existe em API para cobertura
fundacional real:

- `i2c`
- `spi`
- `timer`
- `pwm`
- `rtc`
- `watchdog`
- `adc`
- `dac`
- `low-power`
- `can`

## What Changes

- fechar validation ladders e examples canônicos para os periféricos principais restantes
- promover de `representative` para `foundational` só quando os gates estiverem completos
- endurecer `can` até sair de `experimental`

## Outcome

Depois dessa change, a support matrix deixa de mostrar uma HAL “larga no papel, curta na prova”.

## Impact

- Affected specs:
  - `public-hal-api`
  - `runtime-validation`
  - `runtime-release-discipline`
- Affected code and docs:
  - `examples/**`
  - `tests/host_mmio/**`
  - `tests/emulation/**`
  - `tests/hardware/**`
  - `docs/SUPPORT_MATRIX.md`
  - `docs/RELEASE_MANIFEST.json`
